#include "stdafx.h"
#pragma once

#include "../ecs/EntityManager.h"
#include "graph/SceneGraph.h"
#include "assets/AssetManager.h"
#include "../systems/render/managers/ResourceManager.h"
#include "../systems/render/managers/RenderableManager.h"
#include "../systems/render/memory/pools/HeapPool.h"
#include "../systems/render/queus/DirectQueue.h"
#include "../systems/render/queus/ComputeQueue.h"


namespace Engine::Scene {
	constexpr uint64_t MB512 = 512 * 1024 * 1024;
	constexpr uint64_t MB256 = 256 * 1024 * 1024;
	constexpr uint64_t MB128 = 128 * 1024 * 1024;
	constexpr uint64_t MB64 = 64 * 1024 * 1024;

	struct Scene {
		void initialize(Render::Queue::DirectQueue& directQueue, Render::Queue::ComputeQueue& computeQueue) {
			attDefaultHeapPool.initialize(D3D12_HEAP_TYPE_DEFAULT, MB64, D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS, &resourceManager);
			indDefaultHeapPool.initialize(D3D12_HEAP_TYPE_DEFAULT, MB64, D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS, &resourceManager);
			skiDefaultHeapPool.initialize(D3D12_HEAP_TYPE_DEFAULT, MB64, D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS, &resourceManager);

			uploadHeapPool.initialize(D3D12_HEAP_TYPE_UPLOAD, MB64, D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES, &resourceManager);
			renderableManager.initialize(directQueue);
		}
		ECS::EntityManager entityManager;
		SceneGraph sceneGraph;
		AssetManager assetManager;
		Render::Manager::ResourceManager resourceManager;
		Render::Manager::RenderableManager renderableManager;
		Render::Memory::HeapPool attDefaultHeapPool;
		Render::Memory::HeapPool indDefaultHeapPool;
		Render::Memory::HeapPool skiDefaultHeapPool;

		Render::Memory::HeapPool uploadHeapPool;
	};
}
