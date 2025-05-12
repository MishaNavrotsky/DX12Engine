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
		std::atomic<bool> isRunning = false;
		bool isDirty = false;
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
			queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

			ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_uploadCommandQueue)));
			for (auto& task : m_tasks) {
				ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&task->commandAllocator)));
				ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, task->commandAllocator.Get(), nullptr, IID_PPV_ARGS(&task->commandList)));
			}
		}

		std::future<void> queueModel(GUID& modelId) {
			auto lambda = [&, modelId] {
				GPUUploadQueueTask* task = nullptr;
				std::vector<GPUUploadQueueTask*> taskPtrs;
				{
					std::lock_guard lock(m_mutex);
					for (auto& task : m_tasks) {
						taskPtrs.push_back(task.get()); // Extract raw pointer for sorting
					}
					std::sort(taskPtrs.begin(), taskPtrs.end(), [](const GPUUploadQueueTask* a, const GPUUploadQueueTask* b) {
						return a->modelIds.size() < b->modelIds.size();
						});
				}
				for (auto& t : taskPtrs) {
					bool expected = false;
					if (t->isRunning.compare_exchange_strong(expected, true)) {
						{
							std::lock_guard lock(m_mutex);
							t->modelIds.push_back(modelId);
						}
						task = t;
						break;
					}
				}

				if (!task) {
					std::cerr << "No task is free" << '\n';
					throw std::runtime_error("No task is free");
				}

				ModelGPULoader modelGPULoader(m_device.Get(), task->commandList.Get(), modelId);
				task->isDirty = true;
				task->isRunning = false;
				};
			return m_threadPool.submit_task(lambda);
		}

		std::future<void> execute() {
			return std::async(std::launch::async, [&] {
				m_threadPool.pause();
				m_threadPool.wait();

				std::vector<ID3D12CommandList*> commandLists;
				std::vector<ID3D12CommandAllocator*> commandAllocators;

				for (auto& task : m_tasks) {
					if (task->isDirty == true) {
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
					if (task->isDirty == true) {
						for (auto& modelId : task->modelIds) {
							m_modelManager.get(modelId).setIsLoaded();
							m_modelHeapsManager.get(modelId).releaseUploadHeaps();
						}
						task->isDirty = false;
						task->modelIds.clear();
					}
				}

				m_threadPool.unpause();
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