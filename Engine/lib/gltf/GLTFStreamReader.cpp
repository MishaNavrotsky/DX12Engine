#include "stdafx.h"
#include "GLTFStreamReader.h"

#define DEBUG_MESHES_LOAD

namespace GLTFLocal {
	struct D3DTopologyMapping {
		D3D12_PRIMITIVE_TOPOLOGY_TYPE type;
		D3D_PRIMITIVE_TOPOLOGY topology;
		bool supported;
	};

	static D3D12_FILTER ConvertToD3D12Filter(MagFilterMode magFilter, MinFilterMode minFilter)
	{
		switch (minFilter)
		{
		case MinFilter_NEAREST:
			return (magFilter == MagFilter_NEAREST) ? D3D12_FILTER_MIN_MAG_MIP_POINT : D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;

		case MinFilter_LINEAR:
			return (magFilter == MagFilter_NEAREST) ? D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT : D3D12_FILTER_MIN_MAG_MIP_LINEAR;

		case MinFilter_NEAREST_MIPMAP_NEAREST:
			return (magFilter == MagFilter_NEAREST) ? D3D12_FILTER_MIN_MAG_MIP_POINT : D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;

		case MinFilter_LINEAR_MIPMAP_NEAREST:
			return (magFilter == MagFilter_NEAREST) ? D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT : D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;

		case MinFilter_NEAREST_MIPMAP_LINEAR:
			return (magFilter == MagFilter_NEAREST) ? D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR : D3D12_FILTER_MIN_POINT_MAG_MIP_LINEAR;

		case MinFilter_LINEAR_MIPMAP_LINEAR:
			return (magFilter == MagFilter_NEAREST) ? D3D12_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR : D3D12_FILTER_MIN_MAG_MIP_LINEAR;

		default:
			return D3D12_FILTER_MIN_MAG_MIP_LINEAR; // Safe default
		}
	}

	static D3DTopologyMapping ConvertMeshMode(MeshMode mode) {
		switch (mode) {
		case MESH_POINTS:
			return { D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT, D3D_PRIMITIVE_TOPOLOGY_POINTLIST, true };
		case MESH_LINES:
			return { D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE, D3D_PRIMITIVE_TOPOLOGY_LINELIST, true };
		case MESH_LINE_STRIP:
			return { D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE, D3D_PRIMITIVE_TOPOLOGY_LINESTRIP, true };
		case MESH_TRIANGLES:
			return { D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, true };
		case MESH_TRIANGLE_STRIP:
			return { D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE, D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP, true };
		case MESH_LINE_LOOP:
		case MESH_TRIANGLE_FAN:
			return { D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED, D3D_PRIMITIVE_TOPOLOGY_UNDEFINED, false };
		default:
			return { D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED, D3D_PRIMITIVE_TOPOLOGY_UNDEFINED, false };
		}
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

std::vector<GUID> GLTFLocal::GetMeshesInfo(const fs::path& path) {
	using namespace std;
	//start loading time
#ifdef DEBUG_MESHES_LOAD
	auto startTime = std::chrono::high_resolution_clock::now();
#endif // DEBUG_MESHES_LOAD

	std::vector<GUID> meshDataList = std::vector<GUID>();

	auto streamReader = make_unique<GLTFStreamReader>(path.parent_path());

	fs::path pathFile = path.filename();
	fs::path pathFileExt = pathFile.extension();

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


	auto& materials = document.materials.Elements();
	std::unordered_map<std::string, std::unique_ptr<Engine::CPUMaterial>> materialsMap;
	for (auto& material : materials) {
		auto cpuMaterial = std::make_unique<Engine::CPUMaterial>();
		GUID materialId;
		ThrowIfFailed(CoCreateGuid(&materialId));
		cpuMaterial->setID(materialId);

		cpuMaterial->alphaCutoff = material.alphaCutoff;
		auto alphaMode = material.alphaMode;
		if (alphaMode == ALPHA_UNKNOWN || alphaMode == ALPHA_OPAQUE) {
			cpuMaterial->alphaMode = Engine::AlphaMode::Opaque;
		}
		else if (alphaMode == ALPHA_MASK) {
			cpuMaterial->alphaMode = Engine::AlphaMode::Mask;
		}
		else if (alphaMode == ALPHA_BLEND) {
			cpuMaterial->alphaMode = Engine::AlphaMode::Blend;
		}
		cpuMaterial->cullMode = material.doubleSided ? D3D12_CULL_MODE_NONE : D3D12_CULL_MODE_BACK;

		auto& matData = cpuMaterial.get()->getCBVData();


		matData.baseColorFactor.x = material.metallicRoughness.baseColorFactor.r;
		matData.baseColorFactor.y = material.metallicRoughness.baseColorFactor.g;
		matData.baseColorFactor.z = material.metallicRoughness.baseColorFactor.b;
		matData.baseColorFactor.w = material.metallicRoughness.baseColorFactor.a;

		matData.normalScaleOcclusionStrengthMRFactors.x = material.normalTexture.scale;

		matData.normalScaleOcclusionStrengthMRFactors.y = material.occlusionTexture.strength;

		matData.normalScaleOcclusionStrengthMRFactors.z = material.metallicRoughness.metallicFactor;
		matData.normalScaleOcclusionStrengthMRFactors.w = material.metallicRoughness.roughnessFactor;

		matData.emissiveFactor.x = material.emissiveFactor.r;
		matData.emissiveFactor.y = material.emissiveFactor.g;
		matData.emissiveFactor.z = material.emissiveFactor.b;



		materialsMap.emplace(material.id, std::move(cpuMaterial));
	}

	auto& samplers = document.samplers.Elements();
	std::unordered_map<std::string, std::unique_ptr<Engine::Sampler>> samplersMap;
	for (auto& sampler : samplers) {
		auto engineSampler = std::make_unique<Engine::Sampler>();
		GUID engineSamplerId;
		ThrowIfFailed(CoCreateGuid(&engineSamplerId));
		engineSampler->setID(engineSamplerId);

		auto magFilter = sampler.magFilter.HasValue() ? sampler.magFilter.Get() : MagFilter_NEAREST;
		auto minFilter = sampler.minFilter.HasValue() ? sampler.minFilter.Get() : MinFilter_NEAREST;
		auto wrapS = sampler.wrapS;
		auto wrapT = sampler.wrapT;

		engineSampler->samplerDesc.Filter = ConvertToD3D12Filter(magFilter, minFilter);

		if (wrapS == Wrap_CLAMP_TO_EDGE) {
			engineSampler->samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		}
		else if (wrapS == Wrap_REPEAT) {
			engineSampler->samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		}
		else if (wrapS == Wrap_MIRRORED_REPEAT) {
			engineSampler->samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
		}

		if (wrapT == Wrap_CLAMP_TO_EDGE) {
			engineSampler->samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		}
		else if (wrapT == Wrap_REPEAT) {
			engineSampler->samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		}
		else if (wrapT == Wrap_MIRRORED_REPEAT) {
			engineSampler->samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
		}
		engineSampler->samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;

		samplersMap.emplace(sampler.id, std::move(engineSampler));
	}

	std::unordered_map<std::string, std::unique_ptr<Engine::CPUTexture>> texturesMap;
	for (auto& material : materials) {
		auto& cpuMaterial = materialsMap.at(material.id);
		auto mTextures = material.GetTextures();
		for (auto& mTexture : mTextures) {
			if (mTexture.first.empty()) continue;

			auto cpuTexture = Engine::CPUTexture();
			GUID cpuTextureId;
			ThrowIfFailed(CoCreateGuid(&cpuTextureId));
			cpuTexture.setID(cpuTextureId);
			cpuTexture.addCpuMaterialId(cpuMaterial->getID());

			cpuMaterial->addCPUTextureId(cpuTextureId);

			auto& texture = document.textures.Get(mTexture.first);
			auto& sampler = samplersMap.at(texture.samplerId);
			sampler->addCpuTextureId(cpuTextureId);
			cpuTexture.setSamplerId(sampler->getID());

			auto& image = document.images.Get(texture.imageId);
			auto buffer = resourceReader.get()->ReadBinaryData(document, image);
			auto scratchImage = std::make_unique<DirectX::ScratchImage>();
			ThrowIfFailed(DirectX::LoadFromWICMemory(buffer.data(), buffer.size(), DirectX::WIC_FLAGS_NONE, nullptr, *scratchImage));
			cpuTexture.setScratchImage(std::move(scratchImage));

			//auto images = scratchImage.GetImages();
			//auto mipMapedScratchImage = std::make_unique<DirectX::ScratchImage>();
			//ThrowIfFailed(DirectX::GenerateMipMaps(images, 1, scratchImage.GetMetadata(), DirectX::TEX_FILTER_DEFAULT, 0, *mipMapedScratchImage));



			if (mTexture.second == TextureType::BaseColor) {
				auto diffuseTexture = std::make_unique<Engine::CPUDiffuseTexture>(std::move(cpuTexture));

				texturesMap.emplace(mTexture.first, std::move(diffuseTexture));
			}
			else if (mTexture.second == TextureType::Normal) {
				auto normalTexture = std::make_unique<Engine::CPUNormalTexture>(std::move(cpuTexture));

				texturesMap.emplace(mTexture.first, std::move(normalTexture));
			}
			else if (mTexture.second == TextureType::Occlusion) {
				auto occlusionTexture = std::make_unique<Engine::CPUOcclusionTexture>(std::move(cpuTexture));

				texturesMap.emplace(mTexture.first, std::move(occlusionTexture));
			}
			else if (mTexture.second == TextureType::Emissive) {
				auto emissiveTexture = std::make_unique<Engine::CPUEmissiveTexture>(std::move(cpuTexture));

				texturesMap.emplace(mTexture.first, std::move(emissiveTexture));
			}
			else if (mTexture.second == TextureType::MetallicRoughness) {
				auto metallicRoughnessTexture = std::make_unique<Engine::CPUMetallicRoughnessTexture>(std::move(cpuTexture));

				texturesMap.emplace(mTexture.first, std::move(metallicRoughnessTexture));
			}
		}
	}

	std::unordered_map<std::string, std::unique_ptr<Engine::CPUMesh>> meshesMap;
	auto& documentMeshes = document.meshes.Elements();
	for (auto& mesh : documentMeshes) {
		for (uint32_t i = 0; i < mesh.primitives.size(); i++) {
			auto& primitive = mesh.primitives[i];

			auto cpuMesh = std::make_unique<Engine::CPUMesh>();
			GUID cpuMeshId;
			ThrowIfFailed(CoCreateGuid(&cpuMeshId));
			cpuMesh->setID(cpuMeshId);

			auto mode = ConvertMeshMode(primitive.mode);
			if (!mode.supported) {
				std::cerr << "Mesh mode not supported" << '\n';
				continue;
			}
			cpuMesh->topologyType = mode.type;
			cpuMesh->topology = mode.topology;


			auto& cpuMaterial = materialsMap.at(primitive.materialId);
			cpuMesh->setCPUMaterialId(cpuMaterial->getID());
			cpuMaterial->addCPUMeshId(cpuMeshId);

			auto& indicesAccessor = document.accessors.Get(primitive.indicesAccessorId);
			cpuMesh->setIndices(std::move(resourceReader.get()->ReadBinaryData<uint32_t>(document, indicesAccessor)));
			for (const auto& attribute : primitive.attributes) {
				auto& accessor = document.accessors.Get(attribute.second);
				if (attribute.first == ACCESSOR_POSITION) {
					auto data = resourceReader.get()->ReadBinaryData<float>(document, accessor);
					cpuMesh->setVertices(std::move(data));
				}
				if (attribute.first == ACCESSOR_NORMAL) {
					auto data = resourceReader.get()->ReadBinaryData<float>(document, accessor);
					cpuMesh->setNormals(std::move(data));
				}
				if (attribute.first == ACCESSOR_TEXCOORD_0) {
					auto data = resourceReader.get()->ReadBinaryData<float>(document, accessor);
					cpuMesh->setTexCoords(std::move(data));
				}
				if (attribute.first == ACCESSOR_TANGENT) {
					auto data = resourceReader.get()->ReadBinaryData<float>(document, accessor);
					cpuMesh->setTangents(std::move(data));
				}
			}

			meshesMap.emplace(mesh.id + "_" + std::to_string(i), std::move(cpuMesh));
		}
	}
	

	static auto& cpuMeshManager = Engine::CPUMeshManager::GetInstance();
	static auto& cpuMaterialManager = Engine::CPUMaterialManager::GetInstance();
	static auto& cpuTextureManager = Engine::CPUTextureManager::GetInstance();
	static auto& samplerManager = Engine::SamplerManager::GetInstance();

	for (auto& [key, mesh] : meshesMap) {		
		meshDataList.push_back(mesh->getID());
		cpuMeshManager.add(mesh->getID(), std::move(mesh));
	}

	for (auto& [key, material] : materialsMap) {
		cpuMaterialManager.add(material->getID(), std::move(material));
	}
	for (auto& [key, sampler] : samplersMap) {
		samplerManager.add(sampler->getID(), std::move(sampler));
	}
	for (auto& [key, texture] : texturesMap) {
		cpuTextureManager.add(texture->getID(), std::move(texture));
	}

#ifdef DEBUG_MESHES_LOAD
	auto endTime = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
	std::osyncstream(std::cout) << pathFile << " loading time: " << duration << "ms" << std::endl;
#endif // DEBUG_MESHES_LOAD

	return meshDataList;
}
