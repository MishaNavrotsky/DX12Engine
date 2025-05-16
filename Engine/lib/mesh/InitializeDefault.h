#include "stdafx.h"

#pragma once

#include "CPUMaterial.h"
#include "GPUMaterial.h"
#include "../managers/CPUGPUManager.h"
#include "../managers/GPUMaterialManager.h"
#include "../managers/CPUMaterialManager.h"
#include "../descriptors/BindlessHeapDescriptor.h"


namespace Engine {
	using namespace Microsoft::WRL;

	class InitializeDefault {
	public:
		static void Initialize(ID3D12Device* device, BindlessHeapDescriptor* heapDesc) {
			auto cpuMaterial = std::make_unique<CPUMaterial>();
			auto cpuMaterialId = GUID_NULL;
			cpuMaterial->setID(cpuMaterialId);
			auto gpuMaterial = std::make_unique<GPUMaterial>();
			auto gpuMaterialId = GUID_NULL;
			gpuMaterial->setID(gpuMaterialId);

			const uint64_t cbSize = Helpers::Align(sizeof(CPUMaterialCBVData), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

			auto& cbvRes = gpuMaterial->getCBVResource();
			D3D12_HEAP_PROPERTIES cbvResProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
			D3D12_RESOURCE_DESC cbvResrDesc = CD3DX12_RESOURCE_DESC::Buffer(cbSize);
			ThrowIfFailed(device->CreateCommittedResource(
				&cbvResProps,
				D3D12_HEAP_FLAG_NONE,
				&cbvResrDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&cbvRes)
			));

			auto& cbvData = cpuMaterial->getCBVData();
			void* mappedData = nullptr;
			ThrowIfFailed(cbvRes->Map(0, nullptr, &mappedData));
			memcpy(mappedData, &cbvData, sizeof(CPUMaterialCBVData));
			cbvRes->Unmap(0, nullptr);

			auto slot = heapDesc->addCBV(cbvRes, cbSize);
			cpuMaterial->setCBVDataBindlessHeapSlot(slot);

			
			static auto& cpugpuManager = CPUGPUManager::GetInstance();
			static auto& cpuMaterialManager = CPUMaterialManager::GetInstance();
			static auto& gpuMaterialManager = GPUMaterialManager::GetInstance();

			cpuMaterialManager.add(cpuMaterialId, std::move(cpuMaterial));
			gpuMaterialManager.add(gpuMaterialId, std::move(gpuMaterial));
			auto cpugpu = std::make_unique<CPUGPU>();
			cpugpu->setID(cpuMaterialId);
			cpugpu->gpuId = gpuMaterialId;

			cpugpuManager.add(cpuMaterialId, std::move(cpugpu));

		}
	};
}
