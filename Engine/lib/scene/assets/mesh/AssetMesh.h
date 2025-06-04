#include "stdafx.h"

#pragma once

#include "../../../systems/render/memory/Resource.h"
#include "../../../systems/render/memory/Heap.h"
#include "../../../systems/render/RenderStructures.h"
#include "../../../structures.h"

namespace Engine::Scene::Asset {
	struct CpuAttributeData {
		Render::VertexAttribute attribute;
		std::optional<std::vector<std::byte>> data;
	};
	struct CpuDataSubMesh {
		std::vector<CpuAttributeData> attributes;
		std::optional<std::vector<std::byte>> indicesData;
		DXGI_FORMAT indicesFormat;
		uint64_t indicesSizeInBytes;
		uint64_t totalCPUSizeInBytes;
	};

	struct GpuAttributeData {
		Render::VertexAttribute attribute;
		std::optional<Render::Memory::Heap::HeapId> heapId;
		std::optional<Render::Memory::Resource::PackedHandle> resourceHandle;
		std::optional<D3D12_GPU_VIRTUAL_ADDRESS> gpuVirtualAddress;
	};
	struct GpuDataSubMesh {
		std::optional<Render::Memory::Heap::HeapId> indexHeapId;
		std::optional<Render::Memory::Resource::PackedHandle> indexResourceHandle;
		std::optional<D3D12_GPU_VIRTUAL_ADDRESS> indexGpuVirtualAddress;
		DXGI_FORMAT indicesFormat;
		std::vector<GpuAttributeData> attributes;
		uint64_t indicesSizeInBytes;
		uint64_t totalGPUSizeInBytes;
	};

	struct SubMesh {
		std::string name;
		CpuDataSubMesh cpuData;
		GpuDataSubMesh gpuData;
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
