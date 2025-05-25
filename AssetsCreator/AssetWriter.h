#pragma once


#include "Structures.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <algorithm>


inline uint64_t Align(uint64_t size, uint64_t alignment) {
	return (size + alignment - 1) & ~(alignment - 1);
}

void CopyStringToChar50(const std::string& input, char(&output)[50]) {
	std::fill(std::begin(output), std::end(output), 0); // Ensure null-termination
	std::copy_n(input.c_str(), std::min<std::size_t>(input.size(), sizeof(output) - 1), output);
}

namespace Engine::Asset {
	namespace fs = std::filesystem;

	class AssetWriter {
	public:
		static void Write(const Engine::Asset::Mesh& mesh) {
			static fs::path dir = fs::current_path() / "assets";
			if (!fs::exists(dir)) {
				fs::create_directories(dir);
			}

			fs::path filename = dir / (mesh.id + ".mesh.asset");
			std::ofstream file(filename, std::ios::binary);

			File::MeshHeader header = {};
			CopyStringToChar50(mesh.id, header.id);
			header.submeshCount = static_cast<uint32_t>(mesh.submeshes.size());

			std::vector<File::AttributeBufferEntry> vAttributeBufferEntry;
			std::vector<std::vector<uint8_t>> vAttributeBufferEntryData;

			std::vector<File::IndexBufferEntry> vIndexBufferEntry;
			std::vector<std::vector<uint8_t>> vIndexBufferEntryData;

			std::vector<File::SkinnedBufferEntry> vSkinnedBufferEntry;
			std::vector<std::vector<uint8_t>> vSkinnedBufferEntryData;

			std::vector<File::SubmeshEntry> vSubmeshEntry;

			for (auto& submesh : mesh.submeshes) {
				File::SubmeshEntry submeshEntry = {};
				CopyStringToChar50(submesh->id, submeshEntry.id);
				submeshEntry.attributeBufferIndex = header.attributeBufferCount;
				submeshEntry.indexBufferIndex = header.indexBufferCount;
				submeshEntry.skinnedBufferIndex = header.skinnedBufferCount;
				submeshEntry.aabbMin[0] = submesh->aabbMin[0];
				submeshEntry.aabbMin[1] = submesh->aabbMin[1];
				submeshEntry.aabbMin[2] = submesh->aabbMin[2];

				submeshEntry.aabbMax[0] = submesh->aabbMax[0];
				submeshEntry.aabbMax[1] = submesh->aabbMax[1];
				submeshEntry.aabbMax[2] = submesh->aabbMax[2];


				for (auto& [attributeType, attribute] : submesh->attributes) {
					if (attributeType == AttributeType::JOINT || attributeType == AttributeType::WEIGHT) {
						submeshEntry.skinnedBufferCount++;

						File::SkinnedBufferEntry skinnedBufferEntry = {};
						skinnedBufferEntry.format = attribute->format;
						skinnedBufferEntry.size = attribute->data.size();
						skinnedBufferEntry.type = attribute->type;
						skinnedBufferEntry.typeIndex = attribute->semanticIndex;
						skinnedBufferEntry.vertexCount = static_cast<uint32_t>(attribute->data.size()) / attribute->strideInBytes;
						vSkinnedBufferEntry.push_back(std::move(skinnedBufferEntry));
						vSkinnedBufferEntryData.push_back(std::move(attribute->data));

						header.skinnedBufferCount++;
					}
					else {
						submeshEntry.attributeBufferCount++;

						File::AttributeBufferEntry attributeBufferEntry = {};
						attributeBufferEntry.format = attribute->format;
						attributeBufferEntry.size = attribute->data.size();
						attributeBufferEntry.type = attribute->type;
						attributeBufferEntry.typeIndex = attribute->semanticIndex;
						attributeBufferEntry.vertexCount = static_cast<uint32_t>(attribute->data.size()) / attribute->strideInBytes;
						vAttributeBufferEntry.push_back(std::move(attributeBufferEntry));
						vAttributeBufferEntryData.push_back(std::move(attribute->data));

						header.attributeBufferCount++;
					}
				}

				File::IndexBufferEntry indexBufferEntry = {};
				indexBufferEntry.format = submesh->indices.format;
				indexBufferEntry.indexCount = static_cast<uint32_t>(submesh->indices.data.size()) / submesh->indices.strideInBytes;
				indexBufferEntry.size = submesh->indices.data.size();
				vIndexBufferEntryData.push_back(std::move(submesh->indices.data));
				vIndexBufferEntry.push_back(std::move(indexBufferEntry));
				header.indexBufferCount++;

				vSubmeshEntry.push_back(std::move(submeshEntry));
			}

			for (auto& v : vAttributeBufferEntryData) {
				header.attributeSize += v.size();
			}
			for (auto& v : vIndexBufferEntryData) {
				header.indexSize += v.size();
			}
			for (auto& v : vSkinnedBufferEntryData) {
				header.skinnedSize += v.size();
			}
			uint64_t nonAlignedAttributeSize = header.attributeSize;
			uint64_t nonAlignedIndexSize = header.indexSize;
			uint64_t nonAlignedSkinnedSize = header.skinnedSize;
			header.attributeSize = Align(header.attributeSize, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
			header.indexSize = Align(header.indexSize, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
			header.skinnedSize = Align(header.skinnedSize, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

			header.attributeDataOffset = sizeof(File::MeshHeader)
				+ sizeof(File::AttributeBufferEntry) * vAttributeBufferEntry.size()
				+ sizeof(File::IndexBufferEntry) * vIndexBufferEntry.size()
				+ sizeof(File::SkinnedBufferEntry) * vSkinnedBufferEntry.size()
				+ sizeof(File::SubmeshEntry) * vSubmeshEntry.size();

			header.indexDataOffset = header.attributeDataOffset + header.attributeSize;
			header.skinnedDataOffset = header.indexDataOffset + header.indexSize;

			uint64_t attributeBufferFileOffset = header.attributeDataOffset;
			for (uint32_t i = 0; i < vAttributeBufferEntry.size(); i++) {
				auto& entry = vAttributeBufferEntry[i];
				auto& data = vAttributeBufferEntryData[i];
				entry.fileOffset = attributeBufferFileOffset;
				attributeBufferFileOffset += data.size();
			}

			uint64_t indexBufferFileOffset = header.indexDataOffset;
			for (uint32_t i = 0; i < vIndexBufferEntry.size(); i++) {
				auto& entry = vIndexBufferEntry[i];
				auto& data = vIndexBufferEntryData[i];
				entry.fileOffset = indexBufferFileOffset;
				indexBufferFileOffset += data.size();
			}

			uint64_t skinnedBufferFileOffset = header.skinnedDataOffset;
			for (uint32_t i = 0; i < vSkinnedBufferEntry.size(); i++) {
				auto& entry = vSkinnedBufferEntry[i];
				auto& data = vSkinnedBufferEntryData[i];
				entry.fileOffset = skinnedBufferFileOffset;
				skinnedBufferFileOffset += data.size();
			}

			file.write(reinterpret_cast<const char*>(&header), sizeof(header));
			file.write(reinterpret_cast<const char*>(vAttributeBufferEntry.data()), vAttributeBufferEntry.size() * sizeof(File::AttributeBufferEntry));
			file.write(reinterpret_cast<const char*>(vIndexBufferEntry.data()), vIndexBufferEntry.size() * sizeof(File::IndexBufferEntry));
			file.write(reinterpret_cast<const char*>(vSkinnedBufferEntry.data()), vSkinnedBufferEntry.size() * sizeof(File::SkinnedBufferEntry));
			file.write(reinterpret_cast<const char*>(vSubmeshEntry.data()), vSubmeshEntry.size() * sizeof(File::SubmeshEntry));

			{
				for (auto& v : vAttributeBufferEntryData) {
					file.write(reinterpret_cast<const char*>(v.data()), v.size());
				}
				auto zeroes = std::vector<char>(header.attributeSize - nonAlignedAttributeSize, 0);
				file.write(zeroes.data(), zeroes.size());
			}
			{
				for (auto& v : vIndexBufferEntryData) {
					file.write(reinterpret_cast<const char*>(v.data()), v.size());
				}
				auto zeroes = std::vector<char>(header.indexSize - nonAlignedIndexSize, 0);
				file.write(zeroes.data(), zeroes.size());
			}
			{
				for (auto& v : vSkinnedBufferEntryData) {
					file.write(reinterpret_cast<const char*>(v.data()), v.size());
				}
				auto zeroes = std::vector<char>(header.skinnedSize - nonAlignedSkinnedSize, 0);
				file.write(zeroes.data(), zeroes.size());
			}
		}
	};
}