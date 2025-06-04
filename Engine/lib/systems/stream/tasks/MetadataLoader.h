#include "stdafx.h"

#pragma once

#include <AssetReader.h>

#include "../StreamingStructures.h"
#include "GpuUploadPlanner.h"

namespace Engine::System::Streaming {
	class MetadataLoader {
	public:
		static void LoadMesh(ftl::TaskScheduler* ts, void* arg) {
			auto args = reinterpret_cast<MeshArgs*>(arg);
			args->step = StreamingStep::MetadataLoader;
			auto event = args->event;
			auto scene = args->streamingSystemArgs->getScene();

			auto* asset = event.asset;
			if (asset->source == Scene::Asset::SourceMesh::File) {
				auto& sourceData = std::get<Scene::Asset::FileSourceMesh>(asset->sourceData);
				auto header = AssetsCreator::Asset::AssetReader::ReadMeshHeaders(sourceData.path);
				Scene::Asset::Mesh mesh{};
				mesh.name = header->header.id;
				std::vector<Scene::Asset::SubMesh> submeshes(header->submeshes.size());
				for (uint32_t i = 0; i < header->submeshes.size(); i++) {
					auto& headerSubmesh = header->submeshes[i];
					auto& submesh = submeshes[i];
					submesh.cpuData.attributes.reserve(static_cast<uint64_t>(headerSubmesh.attributeBufferCount) + headerSubmesh.skinnedBufferCount);
					submesh.gpuData.attributes.reserve(static_cast<uint64_t>(headerSubmesh.attributeBufferCount) + headerSubmesh.skinnedBufferCount);

					submesh.name = headerSubmesh.id;
					submesh.aabb.max = DX::XMVectorSet(headerSubmesh.aabbMax[0], headerSubmesh.aabbMax[1], headerSubmesh.aabbMax[2], 0);
					submesh.aabb.min = DX::XMVectorSet(headerSubmesh.aabbMin[0], headerSubmesh.aabbMin[1], headerSubmesh.aabbMin[2], 0);
					submesh.topology = headerSubmesh.topology;

					uint64_t cpuAttrSizeInBytes = 0;
					uint64_t gpuAttSizeInBytes = 0;
					uint64_t cpuSkinSizeInBytes = 0;
					uint64_t gpuSkinSizeInBytes = 0;

					auto& headerIndices = header->indexBuffers[headerSubmesh.indexBufferIndex];
					submesh.cpuData.indicesFormat = headerIndices.format;
					submesh.gpuData.indicesFormat = headerIndices.format;
					submesh.cpuData.indicesSizeInBytes = headerIndices.sizeInBytes;
					submesh.gpuData.indicesSizeInBytes = headerIndices.sizeInBytes;

					mesh.totalCPUIndicesSizeInBytes += submesh.cpuData.indicesSizeInBytes;


					for (uint32_t j = headerSubmesh.attributeBufferIndex; j < headerSubmesh.attributeBufferIndex + headerSubmesh.attributeBufferCount; j++) {
						auto& headerAttribute = header->attributeBuffers[j];
						Render::VertexAttribute vertexAttribute{};
						vertexAttribute.format = headerAttribute.format;
						vertexAttribute.sizeInBytes = headerAttribute.sizeInBytes;
						vertexAttribute.type = headerAttribute.type;
						vertexAttribute.typeIndex = headerAttribute.typeIndex;

						cpuAttrSizeInBytes += headerAttribute.sizeInBytes;
						gpuAttSizeInBytes += headerAttribute.sizeInBytes;

						Scene::Asset::CPUAttributeData cpuAttributeData;
						Scene::Asset::GPUAttributeData gpuAttributeData;
						cpuAttributeData.attribute = vertexAttribute;
						gpuAttributeData.attribute = vertexAttribute;
						submesh.cpuData.attributes.push_back(std::move(cpuAttributeData));
						submesh.gpuData.attributes.push_back(std::move(gpuAttributeData));
					}
					mesh.totalCPUAttributesSizeInBytes += cpuAttrSizeInBytes;


					for (uint32_t j = headerSubmesh.skinnedBufferIndex; j < headerSubmesh.skinnedBufferIndex + headerSubmesh.skinnedBufferCount; j++) {
						auto& headerAttribute = header->skinnedBuffers[j];
						Render::VertexAttribute vertexAttribute{};
						vertexAttribute.format = headerAttribute.format;
						vertexAttribute.sizeInBytes = headerAttribute.sizeInBytes;
						vertexAttribute.type = headerAttribute.type;
						vertexAttribute.typeIndex = headerAttribute.typeIndex;

						cpuSkinSizeInBytes += headerAttribute.sizeInBytes;
						gpuSkinSizeInBytes += headerAttribute.sizeInBytes;

						Scene::Asset::CPUAttributeData cpuAttributeData;
						Scene::Asset::GPUAttributeData gpuAttributeData;
						cpuAttributeData.attribute = vertexAttribute;
						gpuAttributeData.attribute = vertexAttribute;
						submesh.cpuData.attributes.push_back(std::move(cpuAttributeData));
						submesh.gpuData.attributes.push_back(std::move(gpuAttributeData));
					}
					mesh.totalCPUSkinnedSizeInBytes += cpuSkinSizeInBytes;

					submesh.cpuData.totalCPUSizeInBytes = cpuAttrSizeInBytes + submesh.cpuData.indicesSizeInBytes + cpuSkinSizeInBytes;
					submesh.gpuData.totalGPUSizeInBytes = gpuAttSizeInBytes + submesh.gpuData.indicesSizeInBytes + gpuSkinSizeInBytes;
				}
				mesh.subMeshes = submeshes;
				mesh.totalGPUAttributesSizeInBytes = header->header.attributeSizeInBytes;
				mesh.totalGPUIndicesSizeInBytes = header->header.indexSizeInBytes;
				mesh.totalGPUSkinnedSizeInBytes = header->header.skinnedSizeInBytes;
				asset->asset = std::move(mesh);
				asset->additionalData = Scene::Asset::FileMeshAdditionalData{ .file = std::move(*header) };
			}
			asset->status.store(Scene::Asset::Status::MetadataLoaded, std::memory_order_release);
			ts->AddTask({ GpuUploadPlanner::CreatePlanForMesh, arg }, ftl::TaskPriority::Normal);
		}
		static void LoadMaterial(const Scene::Asset::MaterialAssetEvent event) {

		}
		static void LoadMaterialInstance(const Scene::Asset::MaterialInstanceAssetEvent event) {

		}
	};
}