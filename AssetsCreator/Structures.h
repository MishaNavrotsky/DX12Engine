#pragma once

#include <vector>
#include <memory>
#include <d3d12.h>
#include <map>
#include <string>

namespace AssetsCreator::Asset {
	enum class AttributeType {
		POSITION,    // Vertex position
		NORMAL,      // Vertex normal
		TANGENT,     // Tangent vector
		BITANGENT,   // Bitangent vector
		TEXCOORD,    // Texture coordinate
		JOINT,
		WEIGHT,
		COLOR        // Vertex color
	};

	struct Attribute {
		AttributeType type;
		std::string semanticName;
		uint32_t semanticIndex;
		DXGI_FORMAT format;
		uint8_t strideInBytes;
		std::vector<uint8_t> data;
	};

	struct Indices {
		DXGI_FORMAT format;
		uint8_t strideInBytes;
		std::vector<uint8_t> data;
	};

	struct SubMesh
	{
		std::string id;
		std::multimap<AttributeType, std::unique_ptr<Attribute>> attributes;
		float aabbMin[3];
		float aabbMax[3];
		Indices indices;
		D3D_PRIMITIVE_TOPOLOGY topology;
	};

	struct Mesh {
		std::string id;
		std::vector<std::unique_ptr<SubMesh>> submeshes;
	};
}

namespace AssetsCreator::Asset::File {
	constexpr uint32_t ASSET_MAGIC = 0x4D404D4; // "MESH"
	constexpr uint32_t ASSET_MESH = 0x1; // "MESH"

#pragma pack(push, 1)
	struct MeshHeader {
		uint32_t magic = ASSET_MAGIC;
		uint32_t fileType = ASSET_MESH;
		uint32_t version = 1;
		char id[50];

		uint32_t attributeBufferCount;
		uint32_t indexBufferCount;
		uint32_t skinnedBufferCount;
		uint32_t submeshCount;

		uint32_t reserved[3] = { 0 };
		uint64_t attributeDataOffset;
		uint64_t indexDataOffset;
		uint64_t skinnedDataOffset;

		uint64_t attributeSizeInBytes; // includes 64kb alignment
		uint64_t indexSizeInBytes;  // includes 64kb alignment
		uint64_t skinnedSizeInBytes; // includes 64kb alignment
	};

	struct AttributeBufferEntry {
		DXGI_FORMAT format;
		AttributeType type;
		uint32_t typeIndex;
		uint32_t vertexCount;
		uint64_t fileOffset; 
		uint64_t sizeInBytes;
	};

	struct IndexBufferEntry {
		DXGI_FORMAT format;
		uint32_t indexCount;
		uint64_t fileOffset;
		uint64_t sizeInBytes;
	};

	struct SkinnedBufferEntry {
		DXGI_FORMAT format;
		AttributeType type;
		uint32_t typeIndex;
		uint32_t vertexCount;
		uint64_t fileOffset;
		uint64_t sizeInBytes;
	};

	struct SubmeshEntry {
		char id[50];
		uint32_t attributeBufferIndex;
		uint32_t indexBufferIndex;
		uint32_t skinnedBufferIndex; // optional; ~0 if not skinned

		uint32_t attributeBufferCount;
		uint32_t skinnedBufferCount;

		float aabbMin[3];
		float aabbMax[3];

		D3D_PRIMITIVE_TOPOLOGY topology;
		uint32_t materialID = 0;
		uint32_t reserved[3] = { 0 };
	};

	struct MeshAsset {
		MeshHeader header;
		std::vector<AttributeBufferEntry> attributeBuffers;
		std::vector<IndexBufferEntry> indexBuffers;
		std::vector<SkinnedBufferEntry> skinnedBuffers;
		std::vector<SubmeshEntry> submeshes;
		// Move constructor
		MeshAsset(MeshAsset&& other) noexcept
			: header(std::move(other.header)),
			attributeBuffers(std::move(other.attributeBuffers)),
			indexBuffers(std::move(other.indexBuffers)),
			skinnedBuffers(std::move(other.skinnedBuffers)),
			submeshes(std::move(other.submeshes)) {
		}

		// Move assignment operator
		MeshAsset& operator=(MeshAsset&& other) noexcept {
			if (this != &other) {
				header = std::move(other.header);
				attributeBuffers = std::move(other.attributeBuffers);
				indexBuffers = std::move(other.indexBuffers);
				skinnedBuffers = std::move(other.skinnedBuffers);
				submeshes = std::move(other.submeshes);
			}
			return *this;
		}

		// Optional: default constructor/assignment as needed
		MeshAsset() = default;
		MeshAsset(const MeshAsset&) = delete;
		MeshAsset& operator=(const MeshAsset&) = delete;
		// Raw data sections would follow here in the actual file
	};
#pragma pack(pop)
}