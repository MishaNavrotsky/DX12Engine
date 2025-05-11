#pragma once

#include "../scene/ISceneObject.h"
#include <filesystem>
#include "GLTFStreamReader.h"
#include "../mesh/helpers.h"
#include "../loaders/ModelLoader.h"
#include "../managers/ModelManager.h"
#include "../managers/GPUMeshManager.h"
#include "../managers/CPUMeshManager.h"
#include "../renderable/MeshRenderable.h"

namespace Engine {
	struct CBVMeshData {
		XMFLOAT4X4 modelMatrix;
		XMFLOAT4X4 prevModelMatrix;

		XMUINT4 cbvDataBindlessHeapSlot;
	};

	class GLTFSceneObject : public ISceneObject {
	public:
		GLTFSceneObject(const std::filesystem::path& path);

		bool isLoadComplete() const override;
	private:
		bool m_isLoadComplete = false;
		std::filesystem::path m_path;
		ModelManager& m_modelManager = ModelManager::GetInstance();
		GPUMeshManager& m_gpuMeshManager = GPUMeshManager::GetInstance();
		CPUMeshManager& m_cpuMeshManager = CPUMeshManager::GetInstance();
		GPUMaterialManager& m_gpuMaterialManager = GPUMaterialManager::GetInstance();
		CPUMaterialManager& m_cpuMaterialManager = CPUMaterialManager::GetInstance();

		GUID m_modelId = GUID_NULL;
		Model* m_model = nullptr;

		std::vector<std::unique_ptr<MeshRenderable>> m_renderables;
		std::vector<ComPtr<ID3D12Resource>> m_cbvs;


		void onLoadComplete();

		// Inherited via ISceneObject
		void render(ID3D12GraphicsCommandList* commandList) override;
	};
}
