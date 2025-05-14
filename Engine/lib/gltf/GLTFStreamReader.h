#pragma once
#include <GLTFSDK/GLTF.h>
#include <GLTFSDK/GLTFResourceReader.h>
#include <GLTFSDK/GLBResourceReader.h>
#include <GLTFSDK/Deserialize.h>
#include <variant>

#include <filesystem>

#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>

#include "../DXSampleHelper.h"

#include "../mesh/CPUMesh.h"
#include "../mesh/CPUTexture.h"
#include "../mesh/CPUMaterial.h"
#include "../mesh/Sampler.h"


#include "DirectXTex.h"
#include <future>
#include <atomic>

#include "../managers/CPUMaterialManager.h"
#include "../managers/CPUMeshManager.h"
#include "../managers/CPUTextureManager.h"
#include "../managers/SamplerManager.h"


namespace GLTFLocal
{
	namespace fs = std::filesystem;

	using namespace Microsoft::glTF;

	class GLTFStreamReader : public IStreamReader
	{
	public:
		GLTFStreamReader(fs::path pathBase);

		std::shared_ptr<std::istream> GetInputStream(const std::string& filename) const override;

	private:
		fs::path m_pathBase;
	};

	std::vector<GUID> GetMeshesInfo(const fs::path& path);
	extern BS::thread_pool<> m_threadPool;
	extern BS::thread_pool<> m_texturesThreadPool;
}