#include "stdafx.h"

#pragma once

#include "../../../scene/assets/AssetStructures.h"
#include "GpuUploadPlannerStructures.h"
#include "../StreamingStructures.h"
#include "./UploadExecutor.h"

namespace Engine::System::Streaming {
	class GpuUploadPlanner {
	public:
		static DSTORAGE_REQUEST CreateDStorageRequest(
			IDStorageFile* storageFile,
			uint64_t offset,
			uint64_t size,
			ID3D12Resource* destinationResource,
			uint64_t destinationOffset,
			uint64_t destinationSize)
		{
			DSTORAGE_REQUEST request = {};
			request.Options.CompressionFormat = DSTORAGE_COMPRESSION_FORMAT_NONE;
			request.Options.SourceType = DSTORAGE_REQUEST_SOURCE_FILE;
			request.Options.DestinationType = DSTORAGE_REQUEST_DESTINATION_BUFFER;

			request.Source.File.Source = storageFile;
			request.Source.File.Offset = offset;
			request.Source.File.Size = static_cast<uint32_t>(size);

			request.Destination.Buffer.Resource = destinationResource;
			request.Destination.Buffer.Offset = destinationOffset;
			request.Destination.Buffer.Size = static_cast<uint32_t>(destinationSize);

			return request;
		}
		static void PopulateMeshUpload(
			std::optional<Render::Memory::HeapPool::AllocateResult>& alloc,
			std::optional<DSTORAGE_REQUEST>& requestSlot,
			MeshUploadResource& resourceSlot,
			const uint64_t offset, const uint64_t size,
			IDStorageFile* storageFile, Render::Manager::ResourceManager& rm
		) {
			if (!alloc) return;
			auto* res = rm.get(alloc->resourceHandle);
			if (!res) return;

			requestSlot.emplace(CreateDStorageRequest(storageFile, offset, size, res->getResource(), 0, res->getSize()));
			resourceSlot.resourceHandle = alloc->resourceHandle;
			resourceSlot.allocateResult = *alloc;
			resourceSlot.isHeap = true;
		}
		static void CreatePlanForMesh(ftl::TaskScheduler* ts, void* arg) {
			auto args = reinterpret_cast<MeshArgs*>(arg);
			args->step = StreamingStep::GpuUploadPlanner;
			auto event = args->event;
			auto scene = args->streamingSystemArgs->getScene();

			auto& asset = scene->assetManager.getMeshAsset(event.id);
			if (asset.source == Scene::Asset::SourceMesh::File) {
				auto& sourceData = std::get<Scene::Asset::FileSourceMesh>(asset.sourceData);
				auto& additionalData = std::get<Scene::Asset::FileMeshAdditionalData>(asset.additionalData);

				WPtr<IDStorageFile> storageFile;
				ThrowIfFailed(args->streamingSystemArgs->getDfactory()->OpenFile(sourceData.path.c_str(), IID_PPV_ARGS(&storageFile)));

				std::optional<Render::Memory::HeapPool::AllocateResult> ski, att, ind;

				if (asset.usage == Scene::Asset::UsageMesh::Static) {
					if (additionalData.file.header.skinnedSizeInBytes) {
						D3D12_RESOURCE_DESC descSki = CD3DX12_RESOURCE_DESC::Buffer(additionalData.file.header.skinnedSizeInBytes);
						ski = scene->skiDefaultHeapPool.allocate(descSki, D3D12_RESOURCE_STATE_COPY_DEST);
					}
					D3D12_RESOURCE_DESC descAtt = CD3DX12_RESOURCE_DESC::Buffer(additionalData.file.header.attributeSizeInBytes);
					att = scene->attDefaultHeapPool.allocate(descAtt, D3D12_RESOURCE_STATE_COPY_DEST);
					D3D12_RESOURCE_DESC descInd = CD3DX12_RESOURCE_DESC::Buffer(additionalData.file.header.indexSizeInBytes);
					ind = scene->indDefaultHeapPool.allocate(descInd, D3D12_RESOURCE_STATE_COPY_DEST);
				}

				MeshGpuUploadPlan meshGpuUploadPlan{};
				meshGpuUploadPlan.assetId = event.id;
				meshGpuUploadPlan.uploadType = GpuUploadType::DirectStorage;

				DSMeshUploadTypeData dsMeshUploadTypeData{};
				dsMeshUploadTypeData.storageFile = storageFile;
				if (att)
					PopulateMeshUpload(
						att,
						dsMeshUploadTypeData.attReq,
						meshGpuUploadPlan.resourceAtt.emplace(),
						additionalData.file.header.attributeDataOffset,
						additionalData.file.header.attributeSizeInBytes,
						dsMeshUploadTypeData.storageFile.Get(),
						scene->resourceManager
					);
				if (ind)
					PopulateMeshUpload(
						ind,
						dsMeshUploadTypeData.indReq,
						meshGpuUploadPlan.resourceInd.emplace(),
						additionalData.file.header.indexDataOffset,
						additionalData.file.header.indexSizeInBytes,
						dsMeshUploadTypeData.storageFile.Get(),
						scene->resourceManager
					);
				if (ski)
					PopulateMeshUpload(
						ski,
						dsMeshUploadTypeData.skiReq,
						meshGpuUploadPlan.resourceSki.emplace(),
						additionalData.file.header.skinnedDataOffset,
						additionalData.file.header.skinnedSizeInBytes,
						dsMeshUploadTypeData.storageFile.Get(),
						scene->resourceManager
					);
				meshGpuUploadPlan.uploadTypeData = std::move(dsMeshUploadTypeData);
				args->uploadPlan = std::move(meshGpuUploadPlan);
				ts->AddTask({ UploadExecutor::ExecuteMesh, arg }, ftl::TaskPriority::Normal);
				return;
			}
		}
	};
}
