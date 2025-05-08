#include "stdafx.h"

#pragma once

#include "../managers/ModelManager.h"
#include "../managers/CPUMaterialManager.h"
#include "../managers/CPUMeshManager.h"
#include "../managers/CPUTextureManager.h"
#include "../managers/SamplerManager.h"
#include "../managers/ModelHeapsManager.h"

#include "../managers/GPUMaterialManager.h"
#include "../managers/GPUMeshManager.h"
#include "../managers/GPUTextureManager.h"

#include "../mesh/Model.h"
#include "../mesh/ModelHeaps.h"

#include "../descriptors/BindlessHeapDescriptor.h"
#include "../managers/helpers.h"

namespace Engine {
	using Microsoft::WRL::ComPtr;
	using namespace DirectX;

	inline uint64_t Align(uint64_t size, uint64_t alignment) {
		return (size + alignment - 1) & ~(alignment - 1);
	}

	struct GUIDComparator {
		bool operator()(const GUID& lhs, const GUID& rhs) const {
			return memcmp(&lhs, &rhs, sizeof(GUID)) < 0; // Binary ordering
		}
	};

	class ModelGPULoader {
	public:
		ModelGPULoader(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, GUID modelId) : m_device(device), m_commandList(commandList), m_modelId(modelId) {
			auto& model = m_modelManager.get(m_modelId);
			auto meshes = m_cpuMeshManager.getMany(model.getCPUMeshIds());
			auto modelHeaps = std::make_unique<ModelHeaps>();

			uint64_t sizeOfAllGeometry = 0;
			for (auto& mesh : meshes) {
				auto& v = mesh.get().getVertices();
				auto& n = mesh.get().getNormals();
				auto& c = mesh.get().getTexCoords();
				auto& t = mesh.get().getTangents();

				auto sV = v.size() * sizeof(v.front());
				auto sN = n.size() * sizeof(n.front());
				auto sT = t.size() * sizeof(t.front());
				auto sC = c.size() * sizeof(c.front());

				sizeOfAllGeometry += sV + sN + sT + sC;
			}

			D3D12_HEAP_PROPERTIES geometryUploadHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_GPU_UPLOAD);
			D3D12_RESOURCE_DESC geometryUploadHeapDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeOfAllGeometry);


			auto& geometryUploadHeap = modelHeaps.get()->getGeometryUploadHeap();
			ThrowIfFailed(m_device->CreateCommittedResource(
				&geometryUploadHeapProps,
				D3D12_HEAP_FLAG_NONE,
				&geometryUploadHeapDesc,
				D3D12_RESOURCE_STATE_COMMON,
				nullptr,
				IID_PPV_ARGS(&geometryUploadHeap)
			));


			uint8_t* mappedGeometryData;
			ThrowIfFailed(geometryUploadHeap->Map(0, nullptr, reinterpret_cast<void**>(&mappedGeometryData)));
			uint64_t geometryOffset = 0;
			for (auto& mesh : meshes) {
				auto& v = mesh.get().getVertices();
				auto& n = mesh.get().getNormals();
				auto& c = mesh.get().getTexCoords();
				auto& t = mesh.get().getTangents();


				auto sV = v.size() * sizeof(v.front());
				auto sN = n.size() * sizeof(n.front());
				auto sC = c.size() * sizeof(c.front());
				auto sT = t.size() * sizeof(t.front());



				memcpy(mappedGeometryData + geometryOffset, v.data(), sV);
				geometryOffset += sV;
				memcpy(mappedGeometryData + geometryOffset, n.data(), sN);
				geometryOffset += sN;
				memcpy(mappedGeometryData + geometryOffset, c.data(), sC);
				geometryOffset += sC;
				memcpy(mappedGeometryData + geometryOffset, t.data(), sT);
				geometryOffset += sT;
			}
			geometryUploadHeap->Unmap(0, nullptr);

			D3D12_HEAP_PROPERTIES geometryDefaultHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
			D3D12_RESOURCE_DESC geometryDefaultHeapDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeOfAllGeometry);


			auto& geometryDefaultHeap = modelHeaps.get()->getGeometryDefaultHeap();
			ThrowIfFailed(m_device->CreateCommittedResource(
				&geometryUploadHeapProps,
				D3D12_HEAP_FLAG_NONE,
				&geometryUploadHeapDesc,
				D3D12_RESOURCE_STATE_COMMON,
				nullptr,
				IID_PPV_ARGS(&geometryDefaultHeap)
			));

			m_commandList->CopyBufferRegion(
				geometryDefaultHeap.Get(),
				0,
				geometryUploadHeap.Get(),
				0,
				sizeOfAllGeometry
			);


			uint64_t sizeIndices = 0;
			for (auto& mesh : meshes) {
				auto& i = mesh.get().getIndices();
				auto sI = i.size() * sizeof(i.front());

				sizeIndices += sI;
			}

			D3D12_HEAP_PROPERTIES indicesUploadHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_GPU_UPLOAD);
			D3D12_RESOURCE_DESC indicesUploadHeapDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeIndices);


			auto& indicesUploadHeap = modelHeaps.get()->getIndexUploadHeap();
			ThrowIfFailed(m_device->CreateCommittedResource(
				&indicesUploadHeapProps,
				D3D12_HEAP_FLAG_NONE,
				&indicesUploadHeapDesc,
				D3D12_RESOURCE_STATE_COMMON,
				nullptr,
				IID_PPV_ARGS(&indicesUploadHeap)
			));


			uint8_t* mappedIndicesData;
			ThrowIfFailed(indicesUploadHeap->Map(0, nullptr, reinterpret_cast<void**>(&mappedIndicesData)));
			uint64_t indicesOffset = 0;
			for (auto& mesh : meshes) {
				auto& i = mesh.get().getIndices();
				auto sI = i.size() * sizeof(i.front());


				memcpy(mappedIndicesData + indicesOffset, i.data(), sI);
				indicesOffset += sI;
			}
			indicesUploadHeap->Unmap(0, nullptr);


			D3D12_HEAP_PROPERTIES indicesDefaultHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
			D3D12_RESOURCE_DESC indicesDefaultHeapDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeIndices);

			auto& indicesDefaultHeap = modelHeaps.get()->getIndexDefaultHeap();
			ThrowIfFailed(m_device->CreateCommittedResource(
				&indicesDefaultHeapProps,
				D3D12_HEAP_FLAG_NONE,
				&indicesDefaultHeapDesc,
				D3D12_RESOURCE_STATE_COMMON,
				nullptr,
				IID_PPV_ARGS(&indicesDefaultHeap)
			));

			m_commandList->CopyBufferRegion(
				indicesDefaultHeap.Get(),
				0,
				indicesUploadHeap.Get(),
				0,
				sizeIndices
			);


			uint64_t gO = 0;
			uint64_t iO = 0;
			std::vector<GUID> gpuMeshIds;
			for (auto& mesh : meshes) {
				auto& v = mesh.get().getVertices();
				auto& n = mesh.get().getNormals();
				auto& c = mesh.get().getTexCoords();
				auto& t = mesh.get().getTangents();


				auto sV = v.size() * sizeof(v.front());
				auto sN = n.size() * sizeof(n.front());
				auto sC = c.size() * sizeof(c.front());
				auto sT = t.size() * sizeof(t.front());

				auto& i = mesh.get().getIndices();
				auto sI = i.size() * sizeof(i.front());

				D3D12_VERTEX_BUFFER_VIEW vertexBufferView{
					.BufferLocation = geometryDefaultHeap->GetGPUVirtualAddress() + gO,
					.SizeInBytes = static_cast<UINT>(sV),
					.StrideInBytes = sizeof(float) * 3,
				};
				gO += sV;
				D3D12_VERTEX_BUFFER_VIEW normalsBufferView{
					.BufferLocation = geometryDefaultHeap->GetGPUVirtualAddress() + gO,
					.SizeInBytes = static_cast<UINT>(sN),
					.StrideInBytes = sizeof(float) * 3,
				};
				gO += sN;
				D3D12_VERTEX_BUFFER_VIEW texCoordsBufferView{
					.BufferLocation = geometryDefaultHeap->GetGPUVirtualAddress() + gO,
					.SizeInBytes = static_cast<UINT>(sC),
					.StrideInBytes = sizeof(float) * 2,
				};
				gO += sC;
				D3D12_VERTEX_BUFFER_VIEW tangentsBufferView{
					.BufferLocation = geometryDefaultHeap->GetGPUVirtualAddress() + gO,
					.SizeInBytes = static_cast<UINT>(sT),
					.StrideInBytes = sizeof(float) * 4,
				};
				gO += sT;
				D3D12_INDEX_BUFFER_VIEW indexBufferView{
					.BufferLocation = indicesDefaultHeap->GetGPUVirtualAddress() + iO,
					.SizeInBytes = static_cast<UINT>(sI),
				};
				iO += sI;

				auto gpuMesh = std::make_unique<GPUMesh>(vertexBufferView, normalsBufferView, texCoordsBufferView, tangentsBufferView, indexBufferView);
				gpuMeshIds.push_back(m_gpuMeshManager.add(std::move(gpuMesh)));
			}
			model.setGPUMeshIds(std::move(gpuMeshIds));

			std::set<GUID, GUIDComparator> materialIds;
			std::unordered_map<GUID, GUID, GUIDHash, GUIDEqual> cpuMaterialToGpuMeshId;
			for (uint32_t i = 0; i < meshes.size(); i++) {
				auto& materialId = meshes[i].get().getMaterialId();
				if (!IsEqualGUID(materialId, GUID_NULL)) {
					materialIds.insert(materialId);
					cpuMaterialToGpuMeshId.insert(std::pair<GUID, GUID>(materialId, model.getGPUMeshIds()[i]));
				}
			}

			auto materials = m_cpuMaterialManager.getMany(std::vector(materialIds.begin(), materialIds.end()));
			std::vector<GUID> gpuTextureIds;
			std::vector<GUID> gpuMaterialIds;
			std::vector<std::vector<D3D12_SUBRESOURCE_DATA>> textureSubResources;
			uint64_t totalTextureUploadBufferSize = 0;

			for (auto& material : materials) {
				auto textures = m_cpuTextureManager.getMany(material.get().getTextureIds());
				auto gpuMaterial = std::make_unique<GPUMaterial>();
				auto& samplerIds = material.get().getSamplerIds();
				gpuMaterial->setSamplerIds(std::vector<GUID>(samplerIds));

				auto& gpuMesh = m_gpuMeshManager.get(cpuMaterialToGpuMeshId.at(material.get().getID()));

				for (auto& texture : textures) {
					auto& tex = texture.get();
					auto& image = tex.getScratchImage();

					auto gpuTexture = std::make_unique<GPUTexture>();
					ThrowIfFailed(CreateTexture(m_device, image.GetMetadata(), gpuTexture->getResource().GetAddressOf()));

					std::vector<D3D12_SUBRESOURCE_DATA> subresources;
					ThrowIfFailed(PrepareUpload(m_device, image.GetImages(), image.GetImageCount(), image.GetMetadata(), subresources));

					totalTextureUploadBufferSize += Align(
						GetRequiredIntermediateSize(gpuTexture->getResource().Get(), 0, static_cast<UINT>(subresources.size())),
						D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);

					auto gpuTextureId = m_gpuTextureManager.add(std::move(gpuTexture));
					gpuTextureIds.push_back(gpuTextureId);
					gpuMaterial.get()->getTextureIds().push_back(gpuTextureId);
					textureSubResources.push_back(std::move(subresources));
				}

				auto gpuMaterialId = m_gpuMaterialManager.add(std::move(gpuMaterial));
				gpuMesh.setGPUMaterialId(gpuMaterialId);
				gpuMaterialIds.push_back(gpuMaterialId);
			}

			D3D12_HEAP_PROPERTIES texUploadHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
			D3D12_RESOURCE_DESC texUploadHeapDesc = CD3DX12_RESOURCE_DESC::Buffer(totalTextureUploadBufferSize);
			auto& texUploadHeap = modelHeaps.get()->getTexturesUploadHeap();
			ThrowIfFailed(m_device->CreateCommittedResource(
				&texUploadHeapProps,
				D3D12_HEAP_FLAG_NONE,
				&texUploadHeapDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&texUploadHeap)));

			uint64_t currentOffset = 0;
			auto gpuTextures = m_gpuTextureManager.getMany(gpuTextureIds);
			for (size_t i = 0; i < gpuTextures.size(); ++i) {
				auto& texture = gpuTextures[i].get();
				auto& subresources = textureSubResources[i];

				UpdateSubresources(
					m_commandList,
					texture.getResource().Get(),
					texUploadHeap.Get(),
					currentOffset,
					0,
					static_cast<UINT>(subresources.size()),
					subresources.data()
				);

				uint64_t requiredSize = Align(
					GetRequiredIntermediateSize(texture.getResource().Get(), 0, static_cast<UINT>(subresources.size())),
					D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);

				currentOffset += requiredSize;
			}

			auto gpuMeshes = m_gpuMeshManager.getMany(model.getGPUMeshIds());
			std::vector<std::pair<std::pair<GUID, GUID>, CPUMaterialCBVData>> matCBVsData;
			uint64_t cbvsUploadBufferSize = 0;

			for (uint32_t i = 0; i < meshes.size(); i++) {
				auto& m = meshes[i].get();
				auto& gm = gpuMeshes[i].get();

				auto& mat = m_cpuMaterialManager.get(m.getMaterialId());
				auto& gmat = m_gpuMaterialManager.get(gm.getGPUMaterialId());

				auto& matCBV = mat.getCBVData();

				auto& cpuTextureIds = mat.getTextureIds();
				auto& gpuTextureIds = gmat.getTextureIds();
				auto& samplerIds = gmat.getSamplerIds();

				for (uint32_t j = 0; j < gpuTextureIds.size(); j++) {
					auto& gpuTexture = m_gpuTextureManager.get(gpuTextureIds[j]);
					auto& cpuTexture = m_cpuTextureManager.get(cpuTextureIds[j]);

					auto& sampler = m_samplerManager.get(samplerIds[j]);

					auto slotTex = m_bindlessHeapDescriptor.addTexture(gpuTexture.getResource());
					auto slotSam = m_bindlessHeapDescriptor.addSampler(sampler.samplerDesc);

					if (cpuTexture.getType() == TextureType::BASE_COLOR) {
						matCBV.diffuseEmissiveNormalOcclusionTexSlots.x = slotTex;
						matCBV.diffuseEmissiveNormalOcclusionSamSlots.x = slotSam;
					}
					else if (cpuTexture.getType() == TextureType::EMISSIVE) {
						matCBV.diffuseEmissiveNormalOcclusionTexSlots.y = slotTex;
						matCBV.diffuseEmissiveNormalOcclusionSamSlots.y = slotSam;
					}
					else if (cpuTexture.getType() == TextureType::METALLIC_ROUGHNESS) {
						matCBV.MrTexSlots.x = slotTex;
						matCBV.MrSamSlots.x = slotSam;
					}
					else if (cpuTexture.getType() == TextureType::NORMAL) {
						matCBV.diffuseEmissiveNormalOcclusionTexSlots.z = slotTex;
						matCBV.diffuseEmissiveNormalOcclusionSamSlots.z = slotSam;
					}
					else if (cpuTexture.getType() == TextureType::OCCLUSION) {
						matCBV.diffuseEmissiveNormalOcclusionTexSlots.w = slotTex;
						matCBV.diffuseEmissiveNormalOcclusionSamSlots.w = slotSam;
					}
				}

				cbvsUploadBufferSize += Align(sizeof(CPUMaterialCBVData), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
				matCBVsData.push_back(std::pair<std::pair<GUID, GUID>, CPUMaterialCBVData>(std::pair<GUID, GUID>(mat.getID(), gmat.getID()), matCBV));
			}

			D3D12_HEAP_PROPERTIES cbvsUploadBufferProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
			D3D12_RESOURCE_DESC cbvsUploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(cbvsUploadBufferSize);

			auto& cbvsUploadBuffer = modelHeaps.get()->getCBVsUploadHeap();
			ThrowIfFailed(m_device->CreateCommittedResource(
				&cbvsUploadBufferProps,
				D3D12_HEAP_FLAG_NONE,
				&cbvsUploadBufferDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&cbvsUploadBuffer)));

			UINT8* cbvsMappedData = nullptr;
			ThrowIfFailed(cbvsUploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&cbvsMappedData)));

			size_t cbvsOffset = 0;
			const size_t alignment = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;

			for (const auto& [_, cbvData] : matCBVsData)
			{
				cbvsOffset = (cbvsOffset + alignment - 1) & ~(alignment - 1);
				memcpy(cbvsMappedData + cbvsOffset, &cbvData, sizeof(CPUMaterialCBVData));
				cbvsOffset += sizeof(CPUMaterialCBVData);
			}
			cbvsUploadBuffer->Unmap(0, nullptr);


			const uint64_t cbSize = Align(sizeof(CPUMaterialCBVData), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
			uint64_t copyCBVsOffset = 0;

			for (const auto& [guids, _] : matCBVsData) {
				auto& mat = m_cpuMaterialManager.get(guids.first);
				auto& gmat = m_gpuMaterialManager.get(guids.second);

				auto& cbvRes = gmat.getCBVResource();
				D3D12_HEAP_PROPERTIES cbvResProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
				D3D12_RESOURCE_DESC cbvResrDesc = CD3DX12_RESOURCE_DESC::Buffer(cbSize);
				ThrowIfFailed(m_device->CreateCommittedResource(
					&cbvResProps,
					D3D12_HEAP_FLAG_NONE,
					&cbvResrDesc,
					D3D12_RESOURCE_STATE_COMMON,
					nullptr,
					IID_PPV_ARGS(&cbvRes)
				));

				commandList->CopyBufferRegion(
					cbvRes.Get(),         // Destination
					0,                    // Destination offset
					cbvsUploadBuffer.Get(), // Source
					copyCBVsOffset,           // Source offset in upload buffer
					cbSize                // Aligned CBV size
				);

				copyCBVsOffset += cbSize;
				mat.setCBVDataBindlessHeapSlot(m_bindlessHeapDescriptor.addCBV(cbvRes, cbSize));
			}

			m_uploadHeapManager.add(m_modelId, std::move(modelHeaps));
		};
	private:
		ID3D12Device* m_device;
		ID3D12GraphicsCommandList* m_commandList;
		GUID m_modelId;

		ModelManager& m_modelManager = ModelManager::GetInstance();
		CPUMaterialManager& m_cpuMaterialManager = CPUMaterialManager::GetInstance();
		CPUMeshManager& m_cpuMeshManager = CPUMeshManager::GetInstance();
		CPUTextureManager& m_cpuTextureManager = CPUTextureManager::GetInstance();
		SamplerManager& m_samplerManager = SamplerManager::GetInstance();
		ModelHeapsManager& m_uploadHeapManager = ModelHeapsManager::GetInstance();

		GPUMeshManager& m_gpuMeshManager = GPUMeshManager::GetInstance();
		GPUTextureManager& m_gpuTextureManager = GPUTextureManager::GetInstance();
		GPUMaterialManager& m_gpuMaterialManager = GPUMaterialManager::GetInstance();

		BindlessHeapDescriptor& m_bindlessHeapDescriptor = BindlessHeapDescriptor::GetInstance();
	};
}
