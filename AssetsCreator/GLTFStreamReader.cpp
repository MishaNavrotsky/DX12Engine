#include "GLTFStreamReader.h"
#include <Windows.h>

namespace GLTFLocal {
	struct Vec3 {
		float x, y, z;
	};

	inline static void computeMinMax(const std::vector<float>& data, Vec3& minVal, Vec3& maxVal) {
		if (data.empty() || data.size() % 3 != 0) {
			throw std::runtime_error("Invalid data format (must be multiple of 3)");
		}

		minVal = { data[0], data[1], data[2] };
		maxVal = minVal;

		for (size_t i = 3; i < data.size(); i += 3) {
			minVal.x = std::min<float>(minVal.x, data[i]);
			minVal.y = std::min<float>(minVal.y, data[i + 1]);
			minVal.z = std::min<float>(minVal.z, data[i + 2]);

			maxVal.x = std::max<float>(maxVal.x, data[i]);
			maxVal.y = std::max<float>(maxVal.y, data[i + 1]);
			maxVal.z = std::max<float>(maxVal.z, data[i + 2]);
		}
	}

	inline static std::vector<uint8_t> ConvertFloatVectorToByteVector(const std::vector<float>& floatVec) {
		std::vector<uint8_t> byteVec(floatVec.size() * sizeof(float));
		std::memcpy(byteVec.data(), floatVec.data(), byteVec.size());
		return byteVec;
	}

	inline static std::vector<uint8_t> ConvertUint32VectorToByteVector(const std::vector<uint32_t>& input) {
		std::vector<uint8_t> output;
		output.reserve(input.size() * 4);

		for (uint32_t value : input) {
			for (int i = 0; i < 4; ++i) {
				output.push_back(static_cast<uint8_t>((value >> (8 * i)) & 0xFF));
			}
		}

		return output;
	}

	struct AttributeFormat
	{
		int strideInBytes;
		DXGI_FORMAT dxgiFormat;
	};

	AttributeFormat GetAttributeFormat(AccessorType type, ComponentType componentType, bool normalized = false)
	{
		int numComponents = 0;
		switch (type)
		{
		case TYPE_SCALAR: numComponents = 1; break;
		case TYPE_VEC2:   numComponents = 2; break;
		case TYPE_VEC3:   numComponents = 3; break;
		case TYPE_VEC4:   numComponents = 4; break;
		default:
			throw std::invalid_argument("Unsupported AccessorType");
		}

		int componentSize = 0;
		DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;

		switch (componentType)
		{
		case COMPONENT_FLOAT:
			componentSize = 4;
			switch (type)
			{
			case TYPE_SCALAR: format = DXGI_FORMAT_R32_FLOAT; break;
			case TYPE_VEC2:   format = DXGI_FORMAT_R32G32_FLOAT; break;
			case TYPE_VEC3:   format = DXGI_FORMAT_R32G32B32_FLOAT; break;
			case TYPE_VEC4:   format = DXGI_FORMAT_R32G32B32A32_FLOAT; break;
			}
			break;

		case COMPONENT_UNSIGNED_BYTE:
			componentSize = 1;
			if (type == TYPE_VEC4)
				format = normalized ? DXGI_FORMAT_R8G8B8A8_UNORM : DXGI_FORMAT_R8G8B8A8_UINT;
			else if (type == TYPE_VEC2)
				format = normalized ? DXGI_FORMAT_R8G8_UNORM : DXGI_FORMAT_R8G8_UINT;
			else if (type == TYPE_SCALAR)
				format = DXGI_FORMAT_R8_UINT; // no UNORM scalar
			break;

		case COMPONENT_BYTE:
			componentSize = 1;
			if (type == TYPE_VEC4)
				format = normalized ? DXGI_FORMAT_R8G8B8A8_SNORM : DXGI_FORMAT_UNKNOWN;
			else if (type == TYPE_VEC2)
				format = normalized ? DXGI_FORMAT_R8G8_SNORM : DXGI_FORMAT_UNKNOWN;
			else if (type == TYPE_SCALAR)
				format = DXGI_FORMAT_R8_SINT;
			break;

		case COMPONENT_UNSIGNED_SHORT:
			componentSize = 2;
			if (type == TYPE_VEC4)
				format = normalized ? DXGI_FORMAT_R16G16B16A16_UNORM : DXGI_FORMAT_R16G16B16A16_UINT;
			else if (type == TYPE_VEC2)
				format = normalized ? DXGI_FORMAT_R16G16_UNORM : DXGI_FORMAT_R16G16_UINT;
			else if (type == TYPE_SCALAR)
				format = DXGI_FORMAT_R16_UINT;
			break;

		case COMPONENT_SHORT:
			componentSize = 2;
			if (type == TYPE_VEC4)
				format = normalized ? DXGI_FORMAT_R16G16B16A16_SNORM : DXGI_FORMAT_UNKNOWN;
			else if (type == TYPE_VEC2)
				format = normalized ? DXGI_FORMAT_R16G16_SNORM : DXGI_FORMAT_UNKNOWN;
			else if (type == TYPE_SCALAR)
				format = DXGI_FORMAT_R16_SINT;
			break;

		default:
			throw std::invalid_argument("Unsupported ComponentType");
		}

		int strideInBytes = numComponents * componentSize;
		return { strideInBytes, format };
	}
}

inline static void GetAssetsPath(_Out_writes_(pathSize) WCHAR* path, UINT pathSize)
{
	if (path == nullptr)
	{
		throw std::exception();
	}

	DWORD size = GetModuleFileName(nullptr, path, pathSize);
	if (size == 0 || size == pathSize)
	{
		throw std::exception();
	}

	WCHAR* lastSlash = wcsrchr(path, L'\\');
	if (lastSlash)
	{
		*(lastSlash + 1) = L'\0';
	}
}

inline GLTFLocal::GLTFStreamReader::GLTFStreamReader(fs::path pathBase) : m_pathBase(std::move(pathBase))
{
	WCHAR assetsPath[512];
	GetAssetsPath(assetsPath, _countof(assetsPath));
	fs::path assetsPathW(assetsPath);
	m_pathBase = assetsPathW / m_pathBase;
	assert(m_pathBase.has_root_path());
}

inline std::shared_ptr<std::istream> GLTFLocal::GLTFStreamReader::GetInputStream(const std::string& filename) const
{
	auto streamPath = m_pathBase / filename;
	auto stream = std::make_shared<std::ifstream>(streamPath, std::ios_base::binary);

	if (!stream || !(*stream))
	{
		throw std::runtime_error("Unable to create a valid input stream for uri: " + filename);
	}

	return stream;
}

std::vector<std::unique_ptr<Engine::Asset::Mesh>> GLTFLocal::GetMeshesInfo(const fs::path& path, bool compressIntoOneMesh) {
	using namespace std;

	auto streamReader = make_unique<GLTFStreamReader>(path.parent_path());

	fs::path pathFile = path.filename();
	fs::path pathFileExt = pathFile.extension();
	fs::path pathFileName = pathFile.stem();

	string manifest;

	auto MakePathExt = [](const string& ext)
		{
			return "." + ext;
		};

	unique_ptr<GLTFResourceReader> resourceReader;
	auto strReader = streamReader.get();

	if (pathFileExt == MakePathExt(GLTF_EXTENSION))
	{
		auto gltfStream = strReader->GetInputStream(pathFile.string());
		auto gltfResourceReader = make_unique<GLTFResourceReader>(move(streamReader));

		stringstream manifestStream;

		manifestStream << gltfStream->rdbuf();
		manifest = manifestStream.str();

		resourceReader = move(gltfResourceReader);
	}
	else if (pathFileExt == MakePathExt(GLB_EXTENSION))
	{
		auto glbStream = strReader->GetInputStream(pathFile.string());
		auto glbResourceReader = make_unique<GLBResourceReader>(move(streamReader), move(glbStream));

		manifest = glbResourceReader->GetJson();

		resourceReader = move(glbResourceReader);
	}

	if (!resourceReader)
	{
		throw runtime_error("Command line argument path filename extension must be .gltf or .glb");
	}

	Document document;

	try
	{
		document = Deserialize(manifest);
	}
	catch (const GLTFException& ex)
	{
		stringstream ss;

		ss << "Deserialize failed: ";
		ss << ex.what();

		throw runtime_error(ss.str());
	}


	std::vector<std::unique_ptr<Engine::Asset::Mesh>> vMeshes;
	auto& meshes = document.meshes.Elements();
	uint32_t i = 0;
	for (auto& mesh : meshes) {
		auto vMesh = std::make_unique<Engine::Asset::Mesh>();
		vMesh->id = pathFileName.generic_string() + "_" + std::to_string(i++);

		auto& primitives = mesh.primitives;
		uint32_t j = 0;
		for (auto& primitive : primitives) {
			auto vSubMesh = std::make_unique<Engine::Asset::SubMesh>();
			vSubMesh->id = vMesh->id + "_" + std::to_string(j++);

			auto& indicesAccessor = document.accessors.Get(primitive.indicesAccessorId);
			auto indices = resourceReader->ReadBinaryData<uint32_t>(document, indicesAccessor);

			vSubMesh->indices.data = ConvertUint32VectorToByteVector(indices);
			vSubMesh->indices.format = DXGI_FORMAT_R32_UINT;
			vSubMesh->indices.strideInBytes = 4;


			auto& attributes = primitive.attributes;
			for (auto& attribute : attributes) {
				auto vAttribute = std::make_unique<Engine::Asset::Attribute>();

				auto& accessor = document.accessors.Get(attribute.second);
				auto attributeFormat = GetAttributeFormat(accessor.type, accessor.componentType, accessor.normalized);
				vAttribute->format = attributeFormat.dxgiFormat;
				vAttribute->strideInBytes = attributeFormat.strideInBytes;

				if (attribute.first == ACCESSOR_POSITION) {
					auto data = resourceReader->ReadFloatData(document, accessor);
					Vec3 min, max;
					computeMinMax(data, min, max);

					vAttribute->data = ConvertFloatVectorToByteVector(data);
					vAttribute->semanticIndex = 0;
					vAttribute->semanticName = "POSITION";
					vAttribute->type = Engine::Asset::AttributeType::Position;


					vSubMesh->aabbMin[0] = min.x;
					vSubMesh->aabbMin[1] = min.y;
					vSubMesh->aabbMin[2] = min.z;

					vSubMesh->aabbMax[0] = max.x;
					vSubMesh->aabbMax[1] = max.y;
					vSubMesh->aabbMax[2] = max.z;

					vSubMesh->attributes.emplace(vAttribute->type, std::move(vAttribute));
				}
				else if (attribute.first == ACCESSOR_NORMAL) {
					auto data = resourceReader->ReadFloatData(document, accessor);
					vAttribute->data = ConvertFloatVectorToByteVector(data);
					vAttribute->semanticIndex = 0;
					vAttribute->semanticName = "NORMAL";
					vAttribute->type = Engine::Asset::AttributeType::Normal;
					vSubMesh->attributes.emplace(vAttribute->type, std::move(vAttribute));
				}
				else if (attribute.first == ACCESSOR_TEXCOORD_0) {
					auto data = resourceReader->ReadFloatData(document, accessor);
					vAttribute->data = ConvertFloatVectorToByteVector(data);
					vAttribute->semanticIndex = 0;
					vAttribute->semanticName = "TEXCOORD";
					vAttribute->type = Engine::Asset::AttributeType::TEXCOORD;
					vSubMesh->attributes.emplace(vAttribute->type, std::move(vAttribute));
				}
				else if (attribute.first == ACCESSOR_TEXCOORD_1) {
					auto data = resourceReader->ReadFloatData(document, accessor);
					vAttribute->data = ConvertFloatVectorToByteVector(data);
					vAttribute->semanticIndex = 1;
					vAttribute->semanticName = "TEXCOORD";
					vAttribute->type = Engine::Asset::AttributeType::TEXCOORD;
					vSubMesh->attributes.emplace(vAttribute->type, std::move(vAttribute));
				}
				else if (attribute.first == ACCESSOR_TANGENT) {
					auto data = resourceReader->ReadFloatData(document, accessor);
					vAttribute->data = ConvertFloatVectorToByteVector(data);
					vAttribute->semanticIndex = 0;
					vAttribute->semanticName = "TANGENT";
					vAttribute->type = Engine::Asset::AttributeType::Tangent;
					vSubMesh->attributes.emplace(vAttribute->type, std::move(vAttribute));
				}
				else if (attribute.first == ACCESSOR_JOINTS_0) {
					auto data = resourceReader->ReadFloatData(document, accessor);
					vAttribute->data = ConvertFloatVectorToByteVector(data);
					vAttribute->semanticIndex = 0;
					vAttribute->semanticName = "JOINT";
					vAttribute->type = Engine::Asset::AttributeType::JOINT;
					vSubMesh->attributes.emplace(vAttribute->type, std::move(vAttribute));
				}
				else if (attribute.first == ACCESSOR_WEIGHTS_0) {
					auto data = resourceReader->ReadFloatData(document, accessor);
					vAttribute->data = ConvertFloatVectorToByteVector(data);
					vAttribute->semanticIndex = 0;
					vAttribute->semanticName = "WEIGHT";
					vAttribute->type = Engine::Asset::AttributeType::WEIGHT;
					vSubMesh->attributes.emplace(vAttribute->type, std::move(vAttribute));
				}
				else if (attribute.first == ACCESSOR_COLOR_0) {
					auto data = resourceReader->ReadFloatData(document, accessor);
					vAttribute->data = ConvertFloatVectorToByteVector(data);
					vAttribute->semanticIndex = 0;
					vAttribute->semanticName = "COLOR";
					vAttribute->type = Engine::Asset::AttributeType::Color;
					vSubMesh->attributes.emplace(vAttribute->type, std::move(vAttribute));
				}

			}
			vMesh->submeshes.push_back(std::move(vSubMesh));
		}

		vMeshes.push_back(std::move(vMesh));
	}

	if (compressIntoOneMesh) {
		auto oneMesh = std::make_unique<Engine::Asset::Mesh>();
		oneMesh->id = vMeshes[0]->id;

		for (auto& mesh : vMeshes) {
			for (auto& submesh : mesh->submeshes) {
				oneMesh->submeshes.push_back(std::move(submesh));
			}
		}

		std::vector<std::unique_ptr<Engine::Asset::Mesh>> v;
		v.push_back(std::move(oneMesh));
		return v;
	}

	return vMeshes;
}
