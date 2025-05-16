#include "stdafx.h"

#pragma once

#include "../scene/SceneNode.h"
#include "../managers/ModelManager.h"
#include "../loaders/ModelLoader.h"
#include "MeshSceneNode.h"
#include "../managers/CPUMeshManager.h"
#include "../managers/GPUMeshManager.h"
#include "../managers/CPUMaterialManager.h"
#include "../managers/GPUMaterialManager.h"
#include "../managers/CPUGPUManager.h"

namespace Engine {
	class ModelSceneNode : public SceneNode {
	public:
		static std::shared_ptr<ModelSceneNode> CreateFromGLTFFile(std::filesystem::path path) {
			static auto& modelLoader = ModelLoader::GetInstance();
			static auto& modelManager = ModelManager::GetInstance();

			auto modelId = modelLoader.queueGLTF(std::move(path));
			auto ptr = std::shared_ptr<ModelSceneNode>(new ModelSceneNode(modelManager.get(modelId)));
			ptr->m_loadingFuture = std::async(std::launch::async, [node = ptr.get()] {
				node->m_model.waitForIsLoaded();
				node->m_isReady.store(true, std::memory_order_release);
				node->onLoadComplete();
				});

			return ptr;
		}

		static std::shared_ptr<ModelSceneNode> CreateFromGeometry(std::vector<GUID> cpuMeshGUIDs) {
			static auto& modelLoader = ModelLoader::GetInstance();
			static auto& modelManager = ModelManager::GetInstance();

			auto modelId = modelLoader.queueGeometry(std::move(cpuMeshGUIDs));
			auto ptr = std::shared_ptr<ModelSceneNode>(new ModelSceneNode(modelManager.get(modelId)));
			ptr->m_loadingFuture = std::async(std::launch::async, [node = ptr.get()] {
				node->m_model.waitForIsLoaded();
				node->m_isReady.store(true, std::memory_order_release);
				node->onLoadComplete();
				});

			return ptr;
		}

		void draw(ID3D12GraphicsCommandList* commandList, Camera* camera, bool enableFrustumCulling, const std::function<bool(CPUMesh&, CPUMaterial&, SceneNode* node)>& callback) override {
			if (!m_cachedReady) {
				m_cachedReady = m_isReady.load(std::memory_order_acquire);
			}
			if (m_cachedReady)
				SceneNode::draw(commandList, camera, enableFrustumCulling, callback);
		}

		~ModelSceneNode() override {
			waitUntilLoadComplete();
		}

		void waitUntilLoadComplete() {
			if (m_loadingFuture.valid()) m_loadingFuture.get();
		}
	private:
		void onLoadComplete() {
			static auto& cpuMeshManager = CPUMeshManager::GetInstance();
			static auto& gpuMeshManager = GPUMeshManager::GetInstance();
			static auto& cpuMaterialManager = CPUMaterialManager::GetInstance();
			static auto& gpuMaterialManager = GPUMaterialManager::GetInstance();
			static auto& cpuGPUManager = CPUGPUManager::GetInstance();

			auto cpuMeshes = cpuMeshManager.getMany(m_model.getCPUMeshIds());
			auto gpuMeshes = gpuMeshManager.getMany(m_model.getGPUMeshIds());

			for (uint32_t i = 0; i < cpuMeshes.size(); i++) {
				auto& cpuMesh = cpuMeshes[i].get();
				auto& gpuMesh = gpuMeshes[i].get();
				auto& cpuMaterial = cpuMaterialManager.get(cpuMesh.getCPUMaterialId());

				auto& cpuMaterialId = cpuMesh.getCPUMaterialId();
				auto cpugpuOptional = cpuGPUManager.try_get(cpuMaterialId);
				if (!cpugpuOptional.has_value()) {
					std::osyncstream(std::cout) << "[ModelSceneNode (OnLoadComplete)]: " << cpuMesh.getID().Data1 << " Missing gpuMaterial proceeding with default \n";
				}
				auto& gpuMaterialId = (cpugpuOptional.has_value() ? cpugpuOptional.value().get() : cpuGPUManager.get(GUID_NULL)).gpuId;


				auto& gpuMaterial = gpuMaterialManager.get(gpuMaterialId);


				m_children.push_back(MeshSceneNode::CreateFromMesh(cpuMesh, gpuMesh, cpuMaterial, gpuMaterial));
			}

		}
		std::atomic<bool> m_isReady = false;
		bool m_cachedReady = false;
		ModelSceneNode(Model& model) : m_model(model) {};
		Model& m_model;
		std::future<void> m_loadingFuture;
	};
}
