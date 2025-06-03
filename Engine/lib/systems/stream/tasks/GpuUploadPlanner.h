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



				//if (asset.usage == Scene::Asset::UsageMesh::Static) {
				//	if (additionalData.file.header.skinnedSizeInBytes) {
				//		resourceSki = Render::Memory::Resource::Create(D3D12_HEAP_TYPE_DEFAULT, static_cast<uint32_t>(additionalData.file.header.skinnedSizeInBytes), D3D12_RESOURCE_STATE_COMMON);
				//	}
				//	resourceAtt = Render::Memory::Resource::Create(D3D12_HEAP_TYPE_DEFAULT, static_cast<uint32_t>(additionalData.file.header.attributeSizeInBytes), D3D12_RESOURCE_STATE_COMMON);
				//	resourceInd = Render::Memory::Resource::Create(D3D12_HEAP_TYPE_DEFAULT, static_cast<uint32_t>(additionalData.file.header.indexSizeInBytes), D3D12_RESOURCE_STATE_COMMON);
				//}

				//MeshGpuUploadPlan meshGpuUploadPlan{};
				//meshGpuUploadPlan.assetId = event.id;
				//meshGpuUploadPlan.uploadType = GpuUploadType::DirectStorage;

				//DSMeshUploadTypeData dsMeshUploadTypeData{};
				//dsMeshUploadTypeData.storageFile = storageFile;
				//dsMeshUploadTypeData.attReq.first = CreateDStorageRequest(
				//	dsMeshUploadTypeData.storageFile.Get(),
				//	additionalData.file.header.attributeDataOffset,
				//	additionalData.file.header.attributeDataOffset + additionalData.file.header.attributeSizeInBytes,
				//	resourceAtt->getResource(),
				//	0,
				//	resourceAtt->getSize()
				//	);
				//dsMeshUploadTypeData.attReq.second = true;
				//dsMeshUploadTypeData.indReq.first = CreateDStorageRequest(
				//	dsMeshUploadTypeData.storageFile.Get(),
				//	additionalData.file.header.indexDataOffset,
				//	additionalData.file.header.indexDataOffset + additionalData.file.header.indexSizeInBytes,
				//	resourceInd->getResource(),
				//	0,
				//	resourceInd->getSize()
				//);
				//dsMeshUploadTypeData.indReq.second = true;
				//if (resourceSki) {
				//	dsMeshUploadTypeData.skiReq.first = CreateDStorageRequest(
				//		dsMeshUploadTypeData.storageFile.Get(),
				//		additionalData.file.header.skinnedDataOffset,
				//		additionalData.file.header.skinnedDataOffset + additionalData.file.header.skinnedSizeInBytes,
				//		resourceSki->getResource(),
				//		0,
				//		resourceSki->getSize()
				//	);
				//	dsMeshUploadTypeData.skiReq.second = true;
				//}
				//meshGpuUploadPlan.uploadTypeData = std::move(dsMeshUploadTypeData);

				//meshGpuUploadPlan.resourceAtt = std::move(resourceAtt);
				//meshGpuUploadPlan.resourceInd = std::move(resourceInd);
				//meshGpuUploadPlan.resourceSki = resourceSki ? nullptr : std::move(resourceSki);
				//args->uploadPlan = std::move(meshGpuUploadPlan);
				//ts->AddTask({ UploadExecutor::ExecuteMesh, arg }, ftl::TaskPriority::Normal);
				return;
			}
		}
	};
}
