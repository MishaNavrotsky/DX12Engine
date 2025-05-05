#pragma once

#include "../scene/ISceneObject.h"
#include "../scene/ISceneRenderable.h"
#include <filesystem>
#include "GLTFStreamReader.h"
#include "../mesh/helpers.h"
#include "../loaders/ModelLoader.h"
#include "../managers/ModelManager.h"
#include "../managers/GPUMeshManager.h"
#include "../renderable/MeshRenderable.h"

namespace Engine {
	class GLTFSceneObject : public ISceneObject {
	public:
		GLTFSceneObject(const std::filesystem::path& path);

		bool isLoadComplete() const override;
	private:
		bool m_isLoadComplete = false;
		std::filesystem::path m_path;
		ModelManager& m_modelManager = ModelManager::getInstance();
		GPUMeshManager& m_gpuMeshManager = GPUMeshManager::getInstance();

		GUID m_modelId = GUID_NULL;
		Model* m_model = nullptr;

		// Inherited via 
		std::vector<std::unique_ptr<ISceneRenderable>>& getRenderables() override;

		std::vector<std::unique_ptr<ISceneRenderable>> m_renderables;

		void onLoadComplete();
	};
}
