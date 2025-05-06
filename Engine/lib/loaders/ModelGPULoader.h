#include "stdafx.h"

#pragma once

#include "../managers/ModelManager.h"
#include "../managers/CPUMaterialManager.h"
#include "../managers/CPUMeshManager.h"
#include "../managers/CPUTextureManager.h"
#include "../managers/SamplerManager.h"
#include "../managers/ModelHeapsManager.h"
#include "../managers/CPUGPUBimap.h"

#include "../managers/GPUMaterialManager.h"
#include "../managers/GPUMeshManager.h"
#include "../managers/GPUTextureManager.h"

#include "../mesh/Model.h"
#include "../mesh/ModelHeaps.h"

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
			for (auto& mesh : meshes) {
				auto& materialId = mesh.get().getMaterialId();
				if (!IsEqualGUID(materialId, GUID_NULL)) {
					materialIds.insert(materialId);
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

				gpuMaterialIds.push_back(m_gpuMaterialManager.add(std::move(gpuMaterial)));
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


			m_uploadHeapManager.add(m_modelId, std::move(modelHeaps));
		};
	private:
		ID3D12Device* m_device;
		ID3D12GraphicsCommandList* m_commandList;
		GUID m_modelId;

		ModelManager& m_modelManager = ModelManager::getInstance();
		CPUMaterialManager& m_cpuMaterialManager = CPUMaterialManager::getInstance();
		CPUMeshManager& m_cpuMeshManager = CPUMeshManager::getInstance();
		CPUTextureManager& m_cpuTextureManager = CPUTextureManager::getInstance();
		SamplerManager& m_samplerManager = SamplerManager::getInstance();
		ModelHeapsManager& m_uploadHeapManager = ModelHeapsManager::getInstance();

		GPUMeshManager& m_gpuMeshManager = GPUMeshManager::getInstance();
		GPUTextureManager& m_gpuTextureManager = GPUTextureManager::getInstance();
		GPUMaterialManager& m_gpuMaterialManager = GPUMaterialManager::getInstance();



		CPUGPUBimap& m_cpugpuBimap = CPUGPUBimap::getInstance();
	};
}
