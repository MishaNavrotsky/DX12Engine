#include "stdafx.h"

#pragma once

#include "../mesh/Model.h"
#include <filesystem>
#include "../gltf/GLTFStreamReader.h"
#include "../managers/ModelManager.h"
#include "../queues/GPUUploadQueue.h"
#include "../managers/CPUMaterialManager.h"
#include "../managers/CPUMeshManager.h"
#include "../managers/CPUTextureManager.h"
#include "../managers/SamplerManager.h"

namespace Engine {
	class ModelLoader {
	public:
		static ModelLoader& getInstance() {
			static ModelLoader instance;
			return instance;
		}

		GUID queueGLTF(std::filesystem::path& path) {
			auto model = std::make_unique<Model>();
			auto guid = m_modelManager.add(std::move(model));
			m_threadPool.detach_task(std::bind(&ModelLoader::processGLTF, this, guid, path));
			return guid;
		}

		void waitForQueueEmpty() {
			m_threadPool.wait();
		}
	private:
		ModelManager& m_modelManager = ModelManager::getInstance();
		CPUMaterialManager& m_cpuMaterialManager = CPUMaterialManager::getInstance();
		CPUMeshManager& m_cpuMeshManager = CPUMeshManager::getInstance();
		CPUTextureManager& m_cpuTextureManager = CPUTextureManager::getInstance();
		SamplerManager& m_samplerManager = SamplerManager::getInstance();
		GPUUploadQueue& m_gpuUploadQueue = GPUUploadQueue::getInstance();


		BS::thread_pool<> m_threadPool;
		void processGLTF(GUID guid, std::filesystem::path& path) {
			auto& model = m_modelManager.get(guid);
			model.setCPUMeshIds(std::move(GLTFLocal::GetMeshesInfo(path)));
			m_gpuUploadQueue.queueModel(guid).get();

		}
	};
}
