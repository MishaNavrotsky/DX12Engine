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

#include "../mesh/Mesh.h"

#include "stb_image.h"
#include "DirectXTex.h"
#include <future>
#include <atomic>
#include "../../external/BS_thread_pool.hpp"

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

	std::vector<std::unique_ptr<Engine::Mesh>> GetMeshesInfo(const fs::path& path);
	BS::thread_pool<> m_threadPool;
	BS::thread_pool<> m_texturesThreadPool;
}