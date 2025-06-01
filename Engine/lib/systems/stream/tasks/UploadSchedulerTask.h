#include "stdafx.h"

#pragma once

#include "../../../scene/assets/AssetStructures.h"
#include "../../render/memory/Resource.h"
#include "../controllers/BarrierController.h"

namespace Engine::System::Streaming {
	struct UploadSchedulerTask {
		static void ApplyBarrier(Render::Memory::Resource* resource, BarrierController<D3D12_BUFFER_BARRIER>& barrierController) {
			if (resource) {
				D3D12_BUFFER_BARRIER bufferBarrier = {};
				bufferBarrier.SyncBefore = D3D12_BARRIER_SYNC_COPY;
				bufferBarrier.SyncAfter = D3D12_BARRIER_SYNC_COPY;
				bufferBarrier.AccessBefore = D3D12_BARRIER_ACCESS_COMMON;
				bufferBarrier.AccessAfter = D3D12_BARRIER_ACCESS_COPY_DEST;
				bufferBarrier.pResource = resource->getResource();

				barrierController.addBarrier(std::move(bufferBarrier));
			}
		}
		static void ScheduleMesh(ftl::TaskScheduler* ts, void* arg) {
			auto args = reinterpret_cast<MeshArgs*>(arg);
			auto event = args->event;
			auto scene = args->scene;

			auto& asset = scene->assetManager.getMeshAsset(event.id);
			if (asset.source == Scene::Asset::SourceMesh::File) {
				auto& sourceData = std::get<Scene::Asset::FileSourceMesh>(asset.sourceData);
				auto& additionalData = std::get<Scene::Asset::FileMeshAdditionalData>(asset.additionalData);

				WPtr<IDStorageFile> storageFile;
				ThrowIfFailed(args->dfactory->OpenFile(sourceData.path.c_str(), IID_PPV_ARGS(&storageFile)));

				auto resourceAtt = Render::Memory::Resource::Create(D3D12_HEAP_TYPE_DEFAULT, static_cast<uint32_t>(additionalData.file.header.attributeSizeInBytes), D3D12_RESOURCE_STATE_COMMON);
				auto resourceInd = Render::Memory::Resource::Create(D3D12_HEAP_TYPE_DEFAULT, static_cast<uint32_t>(additionalData.file.header.indexSizeInBytes), D3D12_RESOURCE_STATE_COMMON);
				std::unique_ptr<Render::Memory::Resource> resourceSki;
				if (additionalData.file.header.skinnedSizeInBytes) {
					resourceSki = Render::Memory::Resource::Create(D3D12_HEAP_TYPE_DEFAULT, static_cast<uint32_t>(additionalData.file.header.skinnedSizeInBytes), D3D12_RESOURCE_STATE_COMMON);
				}

				ApplyBarrier(resourceAtt.get(), args->barrierController);
				ApplyBarrier(resourceInd.get(), args->barrierController);
				ApplyBarrier(resourceSki.get(), args->barrierController);
				
				DSTORAGE_REQUEST request = {};
				request.Options.CompressionFormat = DSTORAGE_COMPRESSION_FORMAT_NONE;  // If no GPU decompression
				request.Source.File.Source = storageFile.Get();       
				request.Source.File.Offset = additionalData.file.header.attributeDataOffset;           
				request.Destination.Buffer.Resource = resourceAtt->getResource();
				request.Destination.Buffer.Offset = 0; 
				request.UncompressedSize = static_cast<uint32_t>(additionalData.file.header.attributeSizeInBytes);

				


			}
		}
	};

}