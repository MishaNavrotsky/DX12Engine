#pragma once

#include "../scene/ISceneObject.h"
#include "../scene/ISceneRenderable.h"
#include <filesystem>
#include "GLTFStreamReader.h"
#include "../managers/CPUMaterialManager.h"
#include "../managers/CPUMeshManager.h"
#include "../managers/CPUTextureManager.h"
#include "../managers/SamplerManager.h"

namespace Engine {
	class GLTFSceneObject : public ISceneObject {
	public:
		GLTFSceneObject(const std::filesystem::path& path);

		void prepareCPUData() override;
		void prepareGPUData() override;

		bool isCPULoadComplete() const override;
		bool isGPULoadComplete() const override;
	private:
		bool m_isCPULoaded = false;
		bool m_isGPULoaded = false;
		std::filesystem::path m_path;
	};
}
