#include "stdafx.h"

#pragma once

#include "../../../scene/assets/AssetStructures.h"
#include "../../../scene/Scene.h"

#include "../controllers/BarrierController.h"

namespace Engine::System::Streaming {
	struct Args {
		IDStorageFactory* dfactory;
		IDStorageQueue2* dqueue;
	};
	struct MeshArgs : Args {
		Scene::Asset::MeshAssetEvent event;
		Scene::Scene* scene;
		std::atomic<bool> isDone{ false };
		BarrierController<D3D12_BUFFER_BARRIER> barrierController;
	};
}
