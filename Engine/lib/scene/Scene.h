#include "stdafx.h"
#pragma once

#include "../ecs/EntityManager.h"
#include "graph/SceneGraph.h"
#include "assets/AssetManager.h"
#include "../systems/render/managers/ResourceManager.h"
#include "../systems/render/memory/pools/HeapPool.h"

namespace Engine::Scene {
	constexpr uint64_t MB512 = 512 * 1024 * 1024;
	struct Scene {
		Scene() {
			defaultHeapPool.initialize(D3D12_HEAP_TYPE_DEFAULT, MB512);
			uploadHeapPool.initialize(D3D12_HEAP_TYPE_UPLOAD, MB512);
		}
		ECS::EntityManager entityManager;
		SceneGraph sceneGraph;
		AssetManager assetManager;
		Render::Manager::ResourceManager resourceManager;
		Render::Memory::HeapPool defaultHeapPool;
		Render::Memory::HeapPool uploadHeapPool;
	};
}
