#include "stdafx.h"

#pragma once

#include "../../../systems/render/memory/Resource.h"
#include "../../../systems/render/RenderStructures.h"
#include "../../../structures.h"

namespace Engine::Scene::Asset {
	struct CPUAttributeData {
		Render::VertexAttribute attribute;
		std::vector<std::byte> data;
	};
	struct CPUDataSubMesh {
		std::vector<CPUAttributeData> attributes;
		std::vector<std::byte> indicesData;
		DXGI_FORMAT indicesFormat;
		uint64_t indicesSizeInBytes;
		uint64_t totalCPUSizeInBytes;
	};

	struct GPUAttributeData {
		Render::VertexAttribute attribute;
		Render::Memory::Resource::PackedHandle resourceId;
		D3D12_GPU_VIRTUAL_ADDRESS gpuVirtualAddress;
	};
	struct GPUDataSubMesh {
		Render::Memory::Resource::PackedHandle indexResourceId;
		D3D12_GPU_VIRTUAL_ADDRESS indexGpuVirtualAddress;
		DXGI_FORMAT indicesFormat;
		std::vector<GPUAttributeData> attributes;
		uint64_t indicesSizeInBytes;
		uint64_t totalGPUSizeInBytes;
	};

	struct SubMesh {
		std::string name;
		CPUDataSubMesh cpuData;
		GPUDataSubMesh gpuData;
		D3D_PRIMITIVE_TOPOLOGY topology;
		Structures::AABB aabb;
	};

	struct Mesh {
		std::string name;
		std::vector<SubMesh> subMeshes;
		uint64_t totalCPUIndicesSizeInBytes;
		uint64_t totalGPUIndicesSizeInBytes;
		uint64_t totalCPUAttributesSizeInBytes;
		uint64_t totalGPUAttributesSizeInBytes;
		uint64_t totalCPUSkinnedSizeInBytes;
		uint64_t totalGPUSkinnedSizeInBytes;
	};
}
