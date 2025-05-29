#include "stdafx.h"

#pragma once

#include "../../../systems/render/memory/Resource.h"
#include "../../../systems/render/RenderStructures.h"

namespace Engine::Scene::Asset {
	struct CPUDataMesh {
		std::vector<std::pair<Render::VertexAttribute, std::vector<std::byte>>> attributes;
		uint64_t totalCPUSizeInBytes;
	};

	struct GPUDataMesh {
		std::vector<std::pair<Render::VertexAttribute, std::pair<Render::Memory::Resource::PackedHandle, D3D12_GPU_VIRTUAL_ADDRESS>>> attributes;
		uint64_t totalGPUSizeInBytes;
	};

	struct AssetMesh {
		std::shared_ptr<CPUDataMesh> cpuData;
		std::shared_ptr<GPUDataMesh> gpuData;
	};
}
