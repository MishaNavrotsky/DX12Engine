#include "stdafx.h"

#pragma once

#include "../scene/SceneNode.h"
#include "../managers/ModelManager.h"
#include "../loaders/ModelLoader.h"
#include "GLTFMeshSceneNode.h"
#include "../managers/CPUMeshManager.h"
#include "../managers/GPUMeshManager.h"
#include "../managers/CPUMaterialManager.h"
#include "../managers/GPUMaterialManager.h"

namespace Engine {
	class GLTFModelSceneNode : public SceneNode {
	public:
		static std::shared_ptr<GLTFModelSceneNode> ReadFromFile(const std::filesystem::path& path) {
			static auto& modelLoader = ModelLoader::GetInstance();
			static auto& modelManager = ModelManager::GetInstance();

			auto modelId = modelLoader.queueGLTF(path);
			auto ptr = std::shared_ptr<GLTFModelSceneNode>(new GLTFModelSceneNode(modelManager.get(modelId)));
			ptr->m_loadingThread = std::thread([node = ptr.get()] {
				node->m_model.waitForIsLoaded();
				node->m_isReady.store(true, std::memory_order_release);
				node->onLoadComplete();
				});

			return ptr;
		}

		void draw(ID3D12GraphicsCommandList* commandList, Camera* camera, const std::function<bool(CPUMesh&, GPUMesh&, SceneNode* node)>& callback) override {
			if (!m_cachedReady) {
				m_cachedReady = m_isReady.load(std::memory_order_acquire);
			}
			if (m_cachedReady)
				SceneNode::draw(commandList, camera, callback);
		}

		~GLTFModelSceneNode() override {
			if (m_loadingThread.joinable()) {
				m_loadingThread.join();
			}
		}
	private:
		void onLoadComplete() {
			static auto& cpuMeshManager = CPUMeshManager::GetInstance();
			static auto& gpuMeshManager = GPUMeshManager::GetInstance();
			static auto& cpuMaterialManager = CPUMaterialManager::GetInstance();
			static auto& gpuMaterialManager = GPUMaterialManager::GetInstance();

			auto cpuMeshes = cpuMeshManager.getMany(m_model.getCPUMeshIds());
			auto gpuMeshes = gpuMeshManager.getMany(m_model.getGPUMeshIds());

			for (uint32_t i = 0; i < cpuMeshes.size(); i++) {
				auto& cpuMesh = cpuMeshes[i].get();
				auto& gpuMesh = gpuMeshes[i].get();
				auto& cpuMaterial = cpuMaterialManager.get(cpuMesh.getMaterialId());
				auto& gpuMaterial = gpuMaterialManager.get(gpuMesh.getGPUMaterialId());

				m_children.push_back(GLTFMeshSceneNode::CreateFromMesh(cpuMesh, gpuMesh, cpuMaterial, gpuMaterial));
			}

		}
		std::atomic<bool> m_isReady = false;
		bool m_cachedReady = false;
		GLTFModelSceneNode(Model& model) : m_model(model) {};
		Model& m_model;
		std::thread m_loadingThread;
	};
}
