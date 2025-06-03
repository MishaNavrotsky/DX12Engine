#include "stdafx.h"

#pragma once

#include "../../../scene/assets/AssetStructures.h"
#include "../../render/memory/Resource.h"
#include "../../render/memory/Heap.h"
#include "../../render/memory/pools/HeapPool.h"

namespace Engine::System::Streaming {
	enum class GpuUploadType {
		DirectStorage,
		CpuStaging,
		Procedural,
	};
	struct DSMeshUploadTypeData {
		std::optional<DSTORAGE_REQUEST> attReq, indReq, skiReq;
		WPtr<IDStorageFile> storageFile;
	};
	struct CSMeshUploadTypeData {

	};
	struct PCMeshUploadTypeData {

	};
	using MeshUploadTypeData = std::variant<DSMeshUploadTypeData, CSMeshUploadTypeData, PCMeshUploadTypeData>;
	struct MeshUploadResource {
		bool isHeap = false;
		Render::Memory::HeapPool::AllocateResult allocateResult{};
		Render::Memory::Resource::PackedHandle resourceHandle = 0;
	};
	struct MeshGpuUploadPlan {
		Scene::Asset::MeshId assetId;
		GpuUploadType uploadType;

		
		std::optional<MeshUploadResource> resourceAtt, resourceInd, resourceSki;
		MeshUploadTypeData uploadTypeData;
	};
}
