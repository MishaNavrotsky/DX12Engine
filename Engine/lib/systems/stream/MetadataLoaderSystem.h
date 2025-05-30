#include "stdafx.h"

#pragma once

#include <AssetReader.h>

#include "../ISystem.h"
#include "../../scene/assets/AssetStructures.h"

namespace Engine::System {
	class MetadataLoaderSystem : public ISystem {
	public:
		void initialize(Scene::Scene& scene) {
			m_scene = &scene;
			m_workerThread = std::thread(&MetadataLoaderSystem::workerLoop, this);
			m_running.store(true);

			scene.assetManager.subscribeMesh([this](const Scene::Asset::MeshAssetEvent& event) {
				if (event.type == Scene::Asset::IAssetEvent::Type::Registered) {
					this->loadMesh(event);
				}
				});
			scene.assetManager.subscribeMaterial([this](const Scene::Asset::MaterialAssetEvent& event) {
				Scene::Asset::MaterialAssetEvent ev = event;
				if (event.type == Scene::Asset::IAssetEvent::Type::Registered) {
					this->loadMaterial(event);
				}
				});
			scene.assetManager.subscribeMaterialInstance([this](const Scene::Asset::MaterialInstanceAssetEvent& event) {
				Scene::Asset::MaterialInstanceAssetEvent ev = event;
				if (event.type == Scene::Asset::IAssetEvent::Type::Registered) {
					this->loadMaterialInstance(event);
				}
				});
		};
		void update(float dt) override {

		};
		void shutdown() override {
			stopWorkerLoop();
		};

		~MetadataLoaderSystem() {
			shutdown();
		}

	private:
		using Task = std::function<void()>;
		void loadMesh(const Scene::Asset::MeshAssetEvent event) {
			post([this, event] {
				auto& asset = this->m_scene->assetManager.getMeshAsset(event.id);
				if (asset.source == Scene::Asset::SourceMesh::File) {
					auto& sourceData = std::get<Scene::Asset::FileSourceMesh>(asset.sourceData);
					auto header = AssetsCreator::Asset::AssetReader::ReadMeshHeaders(sourceData.path);

					Scene::Asset::Mesh mesh{};
					mesh.name = header->header.id;
					std::vector<Scene::Asset::SubMesh> submeshes(header->submeshes.size());
					for (uint32_t i = 0; i < header->submeshes.size(); i++) {
						auto& headerSubmesh = header->submeshes[i];
						auto& submesh = submeshes[i];
						submesh.cpuData.attributes.reserve(headerSubmesh.attributeBufferCount + headerSubmesh.skinnedBufferCount);
						submesh.gpuData.attributes.reserve(headerSubmesh.attributeBufferCount + headerSubmesh.skinnedBufferCount);

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
					asset.asset = std::move(mesh);
				}
				this->m_scene->assetManager.setMeshStatus(event.id, Scene::Asset::Status::MetadataLoaded);
				Scene::Asset::MeshAssetEvent newEvent{};
				newEvent.id = event.id;
				newEvent.newStatus = Scene::Asset::Status::MetadataLoaded;
				newEvent.oldStatus = event.oldStatus;
				newEvent.type = Scene::Asset::IAssetEvent::Type::MetadataLoaded;

				this->m_scene->assetManager.notifyMesh(newEvent);
				});
		}
		void loadMaterial(const Scene::Asset::MaterialAssetEvent event) {
			post([this, event] {
				std::cout << "material \n";
				});
		}
		void loadMaterialInstance(const Scene::Asset::MaterialInstanceAssetEvent event) {
			post([this, event] {
				std::cout << "material instance \n";
				});
		}

		void post(Task task) {
			{
				std::lock_guard<std::mutex> lock(m_mutex);
				m_queue.push(std::move(task));
			}
			m_condition.notify_one();
		}
		void stopWorkerLoop() {
			{
				std::lock_guard<std::mutex> lock(m_mutex);
				m_running = false;
			}
			m_condition.notify_one(); // Wake it up to allow exit
			if (m_workerThread.joinable()) {
				m_workerThread.join();
			}
		}

		void workerLoop() {
			while (true) {
				Task task;

				{
					std::unique_lock<std::mutex> lock(m_mutex);
					m_condition.wait(lock, [this] {
						return !m_queue.empty() || !m_running;
						});

					if (!m_running && m_queue.empty())
						break;

					task = std::move(m_queue.front());
					m_queue.pop();
				}

				// Process the task outside the lock
				task();
			}
		}

		Scene::Scene* m_scene;
		std::thread m_workerThread;
		std::mutex m_mutex;
		std::condition_variable m_condition;
		std::queue<Task> m_queue;
		std::atomic<bool> m_running;
	};
}