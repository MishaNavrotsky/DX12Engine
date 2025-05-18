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
#include "../managers/CPUGPUManager.h"


#include "../mesh/Model.h"
#include "../mesh/ModelHeaps.h"

#include "../descriptors/BindlessHeapDescriptor.h"

#include "../managers/helpers.h"
#include "../helpers.h"

namespace Engine {
	using Microsoft::WRL::ComPtr;
	using namespace DirectX;

	class ModelGPULoader {
	public:
		ModelGPULoader(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, GUID modelId) : m_device(device), m_commandList(commandList), m_modelId(modelId) {
			auto& model = m_modelManager.get(m_modelId);
			auto meshes = m_cpuMeshManager.getMany(model.getCPUMeshIds());
			auto modelHeaps = std::make_unique<ModelHeaps>();

			std::unordered_set<GUID, GUIDHash, GUIDEqual> materialIds;
			for (auto& mesh : meshes) {
				materialIds.insert(mesh.get().getCPUMaterialId());
			}
			auto vMaterialIds = std::vector<GUID>(materialIds.begin(), materialIds.end());


			std::vector<GUID> newMaterialIds;
			auto gpuMaterials = m_gpuMaterialManager.try_getMany(vMaterialIds);
			for (auto i = 0; i < vMaterialIds.size(); i++) {
				if (!gpuMaterials[i].has_value()) {
					newMaterialIds.push_back(vMaterialIds[i]);
				}
			}
			auto cpuMaterials = m_cpuMaterialManager.getMany(newMaterialIds);

			uploadGeometry(model, meshes, modelHeaps.get());
			if (cpuMaterials.size()) {
				uploadTexturesAndSamplers(cpuMaterials, modelHeaps.get());
				uploadMaterials(cpuMaterials, modelHeaps.get());
			}

			m_uploadHeapManager.add(m_modelId, std::move(modelHeaps));
		};
	private:
		void uploadGeometry(Model& model, std::vector<std::reference_wrapper<CPUMesh>>& meshes, ModelHeaps* modelHeaps) {
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


			auto& geometryUploadHeap = modelHeaps->getGeometryUploadHeap();
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


			auto& geometryDefaultHeap = modelHeaps->getGeometryDefaultHeap();
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


			auto& indicesUploadHeap = modelHeaps->getIndexUploadHeap();
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

			auto& indicesDefaultHeap = modelHeaps->getIndexDefaultHeap();
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
			for (auto i = 0; i < gpuMeshIds.size(); i++) {
				auto cpugpu = std::make_unique<CPUGPU>();
				cpugpu.get()->gpuId = gpuMeshIds[i];
				m_cpuGPUManager.add(meshes[i].get().getID(), std::move(cpugpu));
			}
			model.setGPUMeshIds(std::move(gpuMeshIds));
		}

		void uploadMaterials(std::vector<std::reference_wrapper<CPUMaterial>>& cpuMaterials, ModelHeaps* modelHeaps) {
			const uint64_t cbSize = Align(sizeof(CPUMaterialCBVData), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

			auto cbvsUploadBufferSize = cpuMaterials.size() * cbSize;
			D3D12_HEAP_PROPERTIES cbvsUploadBufferProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
			D3D12_RESOURCE_DESC cbvsUploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(cbvsUploadBufferSize);

			auto& cbvsUploadBuffer = modelHeaps->getCBVsUploadHeap();
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

			for (auto& cpuMaterial : cpuMaterials) {
				auto& mat = cpuMaterial.get();

				cbvsOffset = (cbvsOffset + alignment - 1) & ~(alignment - 1);
				memcpy(cbvsMappedData + cbvsOffset, &mat.getCBVData(), sizeof(CPUMaterialCBVData));
				cbvsOffset += sizeof(CPUMaterialCBVData);
			}
			cbvsUploadBuffer->Unmap(0, nullptr);

			uint64_t copyCBVsOffset = 0;
			for (auto& cpuMaterial : cpuMaterials) {
				auto& mat = cpuMaterial.get();
				auto gmat = std::make_unique<GPUMaterial>();

				auto& cbvRes = gmat->getCBVResource();
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
				//{

				//	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
				//		cbvRes.Get(),
				//		D3D12_RESOURCE_STATE_COMMON,
				//		D3D12_RESOURCE_STATE_COPY_DEST
				//	);
				//	commandList->ResourceBarrier(1, &barrier);
				//}
				m_commandList->CopyBufferRegion(
					cbvRes.Get(),
					0,
					cbvsUploadBuffer.Get(),
					copyCBVsOffset,
					cbSize
				);
				//{

				//	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
				//		cbvRes.Get(),
				//		D3D12_RESOURCE_STATE_COPY_DEST,
				//		D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
				//	);
				//	m_barrierCommandList->ResourceBarrier(1, &barrier);
				//}

				copyCBVsOffset += cbSize;
				auto slot = m_bindlessHeapDescriptor.addCBV(cbvRes, cbSize);
				mat.setCBVDataBindlessHeapSlot(slot);

				GUID gmatId;
				ThrowIfFailed(CoCreateGuid(&gmatId));
				gmat->setID(gmatId);

				auto cpugpu = std::make_unique<CPUGPU>();
				cpugpu.get()->gpuId = gmatId;

				m_cpuGPUManager.add(mat.getID(), std::move(cpugpu));
				m_gpuMaterialManager.add(gmatId, std::move(gmat));
			}
		}

		void uploadTexturesAndSamplers(std::vector<std::reference_wrapper<CPUMaterial>>& cpuMaterials, ModelHeaps* modelHeaps) {
			std::unordered_set<GUID, GUIDHash, GUIDEqual> uploadedCpuTextureIds;
			std::unordered_set<GUID, GUIDHash, GUIDEqual> nonUploadedCpuTextureIds;


			std::unordered_set<GUID, GUIDHash, GUIDEqual> allCpuTextureIds;
			for (auto& cpuMaterial : cpuMaterials) {
				auto& cpuTexIds = cpuMaterial.get().getCPUTextureIds();
				for (auto& cpuTexId : cpuTexIds) {
					allCpuTextureIds.insert(cpuTexId);
				}
			}

			std::vector<GUID> vAllCpuTextureIds(allCpuTextureIds.begin(), allCpuTextureIds.end());
			auto options = m_cpuGPUManager.try_getMany(std::vector<GUID>(vAllCpuTextureIds));
			for (auto i = 0; i < options.size(); i++) {
				options[i].has_value() ? uploadedCpuTextureIds.insert(vAllCpuTextureIds[i]) : nonUploadedCpuTextureIds.insert(vAllCpuTextureIds[i]);
			}


			auto nonUploadedCpuTextures = m_cpuTextureManager.getMany(std::vector<GUID>(nonUploadedCpuTextureIds.begin(), nonUploadedCpuTextureIds.end()));
			std::vector<std::unique_ptr<GPUTexture>> nonUploadedGpuTextures;

			std::vector<std::vector<D3D12_SUBRESOURCE_DATA>> textureSubResources;
			uint64_t totalTextureUploadBufferSize = 0;


			for (auto& cpuTexture : nonUploadedCpuTextures) {
				auto& tex = cpuTexture.get();
				auto& image = tex.getScratchImage();

				auto gpuTexture = std::make_unique<GPUTexture>();
				ThrowIfFailed(CreateTexture(m_device, image.GetMetadata(), gpuTexture->getResource().GetAddressOf()));
				//{

				//	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
				//		gpuTexture->getResource().Get(),
				//		D3D12_RESOURCE_STATE_COMMON,
				//		D3D12_RESOURCE_STATE_COPY_DEST
				//	);
				//	commandList->ResourceBarrier(1, &barrier);
				//}

				std::vector<D3D12_SUBRESOURCE_DATA> subresources;
				ThrowIfFailed(PrepareUpload(m_device, image.GetImages(), image.GetImageCount(), image.GetMetadata(), subresources));

				totalTextureUploadBufferSize += Align(
					GetRequiredIntermediateSize(gpuTexture->getResource().Get(), 0, static_cast<UINT>(subresources.size())),
					D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);

				GUID gtexId;
				ThrowIfFailed(CoCreateGuid(&gtexId));
				gpuTexture->setID(gtexId);

				auto cpugpu = std::make_unique<CPUGPU>();
				cpugpu.get()->gpuId = gtexId;
				m_cpuGPUManager.add(tex.getID(), std::move(cpugpu));

				tex.bindlessHeapSlot = m_bindlessHeapDescriptor.addTexture(gpuTexture->getResource());

				nonUploadedGpuTextures.push_back(std::move(gpuTexture));
				textureSubResources.push_back(std::move(subresources));
			}

			D3D12_HEAP_PROPERTIES texUploadHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
			D3D12_RESOURCE_DESC texUploadHeapDesc = CD3DX12_RESOURCE_DESC::Buffer(totalTextureUploadBufferSize);
			auto& texUploadHeap = modelHeaps->getTexturesUploadHeap();
			if (totalTextureUploadBufferSize > 0)
				ThrowIfFailed(m_device->CreateCommittedResource(
					&texUploadHeapProps,
					D3D12_HEAP_FLAG_NONE,
					&texUploadHeapDesc,
					D3D12_RESOURCE_STATE_GENERIC_READ,
					nullptr,
					IID_PPV_ARGS(&texUploadHeap)));

			uint64_t currentOffset = 0;
			for (size_t i = 0; i < nonUploadedGpuTextures.size(); ++i) {
				auto& texture = *nonUploadedGpuTextures[i];
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
				//CD3DX12_RESOURCE_BARRIER textureFinalBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
				//	texture.getResource().Get(),                  // Texture resource
				//	D3D12_RESOURCE_STATE_COPY_DEST,               // State after UpdateSubresources
				//	D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE     // Final state for using in shaders
				//);
				//commandList->ResourceBarrier(1, &textureFinalBarrier);

				uint64_t requiredSize = Align(
					GetRequiredIntermediateSize(texture.getResource().Get(), 0, static_cast<UINT>(subresources.size())),
					D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);

				currentOffset += requiredSize;

				m_gpuTextureManager.add(texture.getID(), std::move(nonUploadedGpuTextures[i]));
			}

			auto allCPUTextures = m_cpuTextureManager.getMany(std::vector<GUID>(vAllCpuTextureIds));
			for (auto& cpuMaterial : cpuMaterials) {
				auto& matCBV = cpuMaterial.get().getCBVData();
				auto& cpuTexIds = cpuMaterial.get().getCPUTextureIds();
				for (auto& cpuTexId : cpuTexIds) {
					auto& cpuTexture = std::find_if(allCPUTextures.begin(), allCPUTextures.end(), [&cpuTexId](auto& texture) {
						return IsEqualGUID(texture.get().getID(), cpuTexId);
						})->get();

					auto slotTex = cpuTexture.bindlessHeapSlot;
					auto slotSam = m_bindlessHeapDescriptor.addSampler(m_samplerManager.get(cpuTexture.getSamplerId()).samplerDesc);

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
			}

		}
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
		CPUGPUManager& m_cpuGPUManager = CPUGPUManager::GetInstance();


		BindlessHeapDescriptor& m_bindlessHeapDescriptor = BindlessHeapDescriptor::GetInstance();
	};
}
