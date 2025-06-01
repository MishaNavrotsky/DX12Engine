#include "stdafx.h"

#pragma once

#include "../ISystem.h"
#include "../render/Device.h"
#include "tasks/Structures.h"
#include "tasks/GpuUploadTask.h"
#include "tasks/MetadataLoaderTask.h"
#include "tasks/UploadSchedulerTask.h"
#include "controllers/BarrierController.h"

namespace Engine::System {
	class StreamingSystem : public ISystem {
	public:
		StreamingSystem() = default;
		~StreamingSystem() = default;
		void initialize(Scene::Scene& scene) override {
			auto device = Render::Device::GetDevice();
			createCopyQueue(device.Get());
			createDirectStorageQueue(device.Get());
			m_taskScheduler.Init(ftl::TaskSchedulerInitOptions{.ThreadPoolSize = 4});
			m_taskScheduler.SetEmptyQueueBehavior(ftl::EmptyQueueBehavior::Sleep);

			m_scene = &scene;

			m_scene->assetManager.subscribeMesh([this](const Scene::Asset::MeshAssetEvent& event) {
				subscribeMesh(event);
				});
		};
		void update(float dt) override {

		};
		void shutdown() override {};
	private:
		void createCopyQueue(ID3D12Device* device) {
			D3D12_COMMAND_QUEUE_DESC directQueueDesc = {};
			directQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
			directQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
			ThrowIfFailed(device->CreateCommandQueue(&directQueueDesc, IID_PPV_ARGS(&m_directCommandQueue)));
		}
		void createDirectStorageQueue(ID3D12Device* device) {
			ThrowIfFailed(DStorageGetFactory(IID_PPV_ARGS(&m_dstorageFactory)));

			DSTORAGE_QUEUE_DESC queueDesc = {};
			queueDesc.Capacity = 512;
			queueDesc.Priority = DSTORAGE_PRIORITY_NORMAL;
			queueDesc.SourceType = DSTORAGE_REQUEST_SOURCE_FILE;
			queueDesc.Device = device;

			WPtr<IDStorageQueue> tempQeue = nullptr;

			ThrowIfFailed(m_dstorageFactory->CreateQueue(&queueDesc, IID_PPV_ARGS(&tempQeue)));
			ThrowIfFailed(tempQeue->QueryInterface(IID_PPV_ARGS(&m_dstorageQueue)));

		}
		void subscribeMesh(const Scene::Asset::MeshAssetEvent& event) {
			if (event.type == Scene::Asset::IAssetEvent::Type::Registered) {
				auto args = std::make_unique<Streaming::MeshArgs>();
				args->event = event;
				args->scene = m_scene;
				args->dqueue = m_dstorageQueue.Get();
				args->dfactory = m_dstorageFactory.Get();

				ftl::Task task{
					.Function = [](ftl::TaskScheduler* s, void* p) {
						std::unique_ptr<Streaming::MeshArgs> args(static_cast<Streaming::MeshArgs*>(p));
						StreamingSystem::StreamMesh(s, args.get());
					},
					.ArgData = args.release(),
				};
				
				m_taskScheduler.AddTask(task, ftl::TaskPriority::Normal);
			}
		}


		static bool AddTaskAndYieldWithTimeout(ftl::TaskScheduler* ts, ftl::Task& task, Streaming::MeshArgs* args, std::chrono::milliseconds ms) {
			auto start = std::chrono::steady_clock::now();
			ts->AddTask(task, ftl::TaskPriority::Normal);
			while (!args->isDone.load(std::memory_order_acquire)) {
				if (std::chrono::steady_clock::now() - start > ms) {
					args->isDone.store(false, std::memory_order_release);
					return false;
				}
				ftl::YieldThread();
			}
			args->isDone.store(false, std::memory_order_release);
			return true;
		}
		static void StreamMesh(ftl::TaskScheduler* ts, void* arg) {
			auto args = reinterpret_cast<Streaming::MeshArgs*>(arg);

			ftl::Task task{
				.Function = Streaming::MetadataLoaderTasks::LoadMesh,
				.ArgData = args,
			};

			{
				auto res = AddTaskAndYieldWithTimeout(ts, task, args, std::chrono::milliseconds(100));
				// TODO: check res
			}

			task.Function = Streaming::UploadSchedulerTask::ScheduleMesh;
			{
				auto res = AddTaskAndYieldWithTimeout(ts, task, args, std::chrono::milliseconds(100));
				// TODO: check res
			}

			task.Function = Streaming::GpuUploadTask::UploadMesh;
			{
				auto res = AddTaskAndYieldWithTimeout(ts, task, args, std::chrono::milliseconds(100));
				// TODO: check res
			}

		}

		Scene::Scene* m_scene;
		ftl::TaskScheduler m_taskScheduler;
		WPtr<ID3D12CommandQueue> m_directCommandQueue;
		WPtr<IDStorageFactory> m_dstorageFactory;
		WPtr<IDStorageQueue2> m_dstorageQueue;
	};
}