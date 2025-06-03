#include "stdafx.h"

#pragma once

#include "../ISystem.h"
#include "../render/Device.h"
#include "StreamingStructures.h"
#include "tasks/MetadataLoader.h"
#include "StreamingSystemArgs.h"
namespace Engine::System {
	class StreamingSystem : public ISystem {
	public:
		StreamingSystem() = default;
		~StreamingSystem() = default;
		void initialize(Scene::Scene& scene) override {
			m_device = Render::Device::GetDevice().Get();
			m_streamingSystemArgs.initialize(m_device, &scene);
			m_taskScheduler.Init(ftl::TaskSchedulerInitOptions{.ThreadPoolSize = 24});
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

		void subscribeMesh(const Scene::Asset::MeshAssetEvent& event) {
			if (event.type == Scene::Asset::IAssetEvent::Type::Registered) {
				auto streamingRequestId = m_nextStreamingRequestId.fetch_add(1, std::memory_order_relaxed);
				auto args = std::make_unique<Streaming::MeshArgs>();
				args->event = event;
				args->streamingSystemArgs = &m_streamingSystemArgs;
				args->streamingRequestId = streamingRequestId;
				args->fenceValue = 0;
				m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&args->fence));
				args->finalize = [this, streamingRequestId](){
					this->m_streamingRequestsMap.erase(streamingRequestId);
					};

				m_streamingRequestsMap[streamingRequestId] = std::move(args);

				ftl::Task task{
					.Function = Streaming::MetadataLoader::LoadMesh,
					.ArgData = m_streamingRequestsMap[streamingRequestId].get(),
				};
				
				m_taskScheduler.AddTask(task, ftl::TaskPriority::Normal);
			}
		}

		std::unordered_map<Streaming::StreamingRequestId, std::unique_ptr<Streaming::Args>> m_streamingRequestsMap;
		std::atomic<uint64_t> m_nextStreamingRequestId{ 0 };
		Scene::Scene* m_scene;
		ftl::TaskScheduler m_taskScheduler;
		ID3D12Device* m_device;
		StreamingSystemArgs m_streamingSystemArgs;
	};
}