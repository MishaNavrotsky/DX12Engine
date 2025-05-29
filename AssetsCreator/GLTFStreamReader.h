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

#include <future>
#include <atomic>
#include <cstdint>
#include <cstring>

#include "Structures.h"



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

	std::vector<std::unique_ptr<AssetsCreator::Asset::Mesh>> GetMeshesInfo(const fs::path& path, bool compressMesh);
}