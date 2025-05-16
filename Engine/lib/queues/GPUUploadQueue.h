#include "stdafx.h"

#pragma once

#include "../loaders/ModelGPULoader.h"
#include "../managers/ModelHeapsManager.h"
#include "../managers/ModelManager.h"

#include <algorithm>


namespace Engine {
	using namespace Microsoft::WRL;
	struct GPUUploadQueueTask {

		std::vector<GUID> modelIds;
		std::mutex mutex;
		ComPtr<ID3D12CommandAllocator> commandAllocator;
		ComPtr<ID3D12GraphicsCommandList> commandList;
	};

	class GPUUploadQueue {
	public:
		static GPUUploadQueue& GetInstance() {
			static GPUUploadQueue instance;
			return instance;
		}

		GPUUploadQueue() {
			for (auto& ptr : m_tasks) {
				ptr = std::make_unique<GPUUploadQueueTask>();
			}
		}

		void registerDevice(ComPtr<ID3D12Device> device) {
			m_device = device;

			D3D12_COMMAND_QUEUE_DESC queueDesc = {};
			queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
			queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;

			ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_uploadCommandQueue)));
			for (auto& task : m_tasks) {
				ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&task->commandAllocator)));
				ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COPY, task->commandAllocator.Get(), nullptr, IID_PPV_ARGS(&task->commandList)));
			}
		}

		std::future<void> queueModel(GUID modelId) {
			auto lambda = [&, modelId] {
				std::osyncstream(std::cout) << "[GPUUploadQueue] Queue geometry for model: " << modelId.Data1 << std::endl;
				const int maxAttempts = 5;
				GPUUploadQueueTask* task = nullptr;

				for (int attempt = 0; attempt < maxAttempts && !task; ++attempt) {
					std::vector<GPUUploadQueueTask*> taskPtrs;

					{
						std::lock_guard lock(m_mutex);
						for (auto& t : m_tasks)
							taskPtrs.push_back(t.get());

						std::sort(taskPtrs.begin(), taskPtrs.end(),
							[](const GPUUploadQueueTask* a, const GPUUploadQueueTask* b) {
								return a->modelIds.size() < b->modelIds.size();
							});
					}

					for (auto& t : taskPtrs) {
						if (t->mutex.try_lock()) {
							task = t;
							break;
						}
					}

					if (!task) {
						std::this_thread::sleep_for(std::chrono::milliseconds(1 << attempt)); // exponential backoff
					}
				}

				if (!task) {
					std::osyncstream(std::cout) << "[GPUUploadQueue] Queue for model error: " << modelId.Data1 << std::endl;
					throw std::runtime_error("[GPUUploadQueue] All GPU upload tasks are busy — retry later.");
				}

				std::lock_guard<std::mutex> taskLock(task->mutex, std::adopt_lock);

				{
					std::lock_guard lock(m_mutex);
					task->modelIds.push_back(modelId);
				}
				try {
					ModelGPULoader modelGPULoader(m_device.Get(), task->commandList.Get(), modelId);
				}
				catch (const std::exception& e) {
					std::osyncstream(std::cout) << "[GPUUploadQueue] ModelGPULoader error " << modelId.Data1 << ": " <<  e.what() << std::endl;
					throw std::runtime_error("[GPUUploadQueue] ModelGPULoader error");
				}
				std::osyncstream(std::cout) << "[GPUUploadQueue] Queue for model success: " << modelId.Data1 << std::endl;
				};

			return m_threadPool.submit_task(lambda);
		}

		std::future<void> execute() {
			return std::async(std::launch::async, [&] {
				std::osyncstream(std::cout) << "[GPUUploadQueue] Queue execute" << std::endl;
				m_threadPool.pause();
				m_threadPool.wait();

				std::vector<ID3D12CommandList*> commandLists;
				std::vector<ID3D12CommandAllocator*> commandAllocators;

				for (auto& task : m_tasks) {
					if (task->modelIds.size()) {
						task->commandList->Close();
						commandLists.push_back(task->commandList.Get());
						commandAllocators.push_back(task->commandAllocator.Get());
					}
				}

				m_uploadCommandQueue->ExecuteCommandLists(static_cast<UINT>(commandLists.size()), commandLists.data());
				ComPtr<ID3D12Fence> fence;
				ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
				HANDLE fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
				if (!fenceEvent) throw std::runtime_error("Failed to create fence event.");
				m_uploadCommandQueue->Signal(fence.Get(), 1);
				fence->SetEventOnCompletion(1, fenceEvent);
				WaitForSingleObject(fenceEvent, INFINITE);
				CloseHandle(fenceEvent);

				for (uint32_t i = 0; i < commandLists.size(); i++) {
					ThrowIfFailed(commandAllocators[i]->Reset());
					ThrowIfFailed(static_cast<ID3D12GraphicsCommandList*>(commandLists[i])->Reset(commandAllocators[i], nullptr));
				}


				for (auto& task : m_tasks) {
					for (auto& modelId : task->modelIds) {
						m_modelManager.get(modelId).setLoaded();
						m_modelHeapsManager.get(modelId).releaseUploadHeaps();
					}
					task->modelIds.clear();
				}

				m_threadPool.unpause();
				std::osyncstream(std::cout) << "[GPUUploadQueue] Queue execute finish" << std::endl;
				});
		}
	private:
		ComPtr<ID3D12CommandQueue> m_uploadCommandQueue;
		ComPtr<ID3D12Device> m_device;
		ModelHeapsManager& m_modelHeapsManager = ModelHeapsManager::GetInstance();
		ModelManager& m_modelManager = ModelManager::GetInstance();


		std::unique_ptr<GPUUploadQueueTask> m_tasks[8];
		BS::pause_thread_pool m_threadPool = BS::pause_thread_pool(8);
		std::mutex m_mutex;
	};
}