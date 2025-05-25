#pragma once


#include "Structures.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <algorithm>


namespace Engine::Asset {
	namespace fs = std::filesystem;

	class AssetReader {
	public:
		static std::unique_ptr<File::MeshAsset> ReadMeshHeaders(const fs::path& path) {
			if (!fs::exists(path) || !fs::is_regular_file(path)) {
				throw std::runtime_error("[AssetWriter] Non existing file " + path.string());
			}

			auto meshAsset = std::make_unique<File::MeshAsset>();
			std::ifstream file(path, std::ios::binary);

			file.read(reinterpret_cast<char*>(&meshAsset->header), sizeof(meshAsset->header));

			if (meshAsset->header.magic != File::ASSET_MAGIC || meshAsset->header.fileType != File::ASSET_MESH) {
				throw std::runtime_error("[AssetWriter] Not a mesh " + path.string());
			}

			meshAsset->attributeBuffers.resize(meshAsset->header.attributeBufferCount);
			meshAsset->indexBuffers.resize(meshAsset->header.indexBufferCount);
			meshAsset->skinnedBuffers.resize(meshAsset->header.skinnedBufferCount);
			meshAsset->submeshes.resize(meshAsset->header.submeshCount);

			file.read(reinterpret_cast<char*>(meshAsset->attributeBuffers.data()), meshAsset->attributeBuffers.size() * sizeof(File::AttributeBufferEntry));
			file.read(reinterpret_cast<char*>(meshAsset->indexBuffers.data()), meshAsset->indexBuffers.size() * sizeof(File::IndexBufferEntry));
			file.read(reinterpret_cast<char*>(meshAsset->skinnedBuffers.data()), meshAsset->skinnedBuffers.size() * sizeof(File::SkinnedBufferEntry));
			file.read(reinterpret_cast<char*>(meshAsset->submeshes.data()), meshAsset->submeshes.size() * sizeof(File::SubmeshEntry));


			return meshAsset;
		}
	};
}