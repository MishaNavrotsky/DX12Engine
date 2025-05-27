#include "stdafx.h"

#pragma once

#include "../scene/SceneNode.h"
#include "../managers/ModelManager.h"
#include "MeshSceneNode.h"
#include "../managers/CPUMeshManager.h"
#include "../managers/GPUMeshManager.h"
#include "../managers/CPUMaterialManager.h"
#include "../managers/GPUMaterialManager.h"
#include "../managers/CPUGPUManager.h"
#include "AssetReader.h"

namespace Engine {
	class ModelSceneNode : public SceneNode {
	public:
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
