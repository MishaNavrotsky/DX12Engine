#include "stdafx.h"
#include "GLTFStreamReader.h"

BS::thread_pool<> GLTFLocal::m_threadPool;

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
	auto streamPath = m_pathBase / fs::u8path(filename);
	auto stream = std::make_shared<std::ifstream>(streamPath, std::ios_base::binary);

	if (!stream || !(*stream))
	{
		throw std::runtime_error("Unable to create a valid input stream for uri: " + filename);
	}

	return stream;
}

std::vector<std::unique_ptr<Engine::MeshData>> GLTFLocal::GetMeshesInfo(const fs::path& path) {
	using namespace std;

	std::vector<std::unique_ptr<Engine::MeshData>> meshDataList;

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
		auto gltfStream = strReader->GetInputStream(pathFile.u8string());
		auto gltfResourceReader = make_unique<GLTFResourceReader>(move(streamReader));

		stringstream manifestStream;

		manifestStream << gltfStream->rdbuf();
		manifest = manifestStream.str();

		resourceReader = move(gltfResourceReader);
	}
	else if (pathFileExt == MakePathExt(GLB_EXTENSION))
	{
		auto glbStream = strReader->GetInputStream(pathFile.u8string());
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

	for (const auto& mesh : document.meshes.Elements()) {

		for (const auto& primitive : mesh.primitives) {
			auto meshData = std::make_unique<Engine::MeshData>();

			auto& indicesAccessor = document.accessors.Get(primitive.indicesAccessorId);
			meshData->setIndices(std::make_shared<std::vector<uint32_t>>(resourceReader.get()->ReadBinaryData<uint32_t>(document, indicesAccessor)));
			auto& material = document.materials.Get(primitive.materialId);
			meshData->alphaCutoff = material.alphaCutoff;
			auto alphaMode = material.alphaMode;
			if (alphaMode == ALPHA_UNKNOWN || alphaMode == ALPHA_OPAQUE) {
				meshData->alphaMode = Engine::AlphaMode::Opaque;
			}
			else if (alphaMode == ALPHA_MASK) {
				meshData->alphaMode = Engine::AlphaMode::Mask;
			}
			else if (alphaMode == ALPHA_BLEND) {
				meshData->alphaMode = Engine::AlphaMode::Blend;
			}
			for (const auto& texture : material.GetTextures()) {
				if (texture.first.empty()) continue;
				auto& tex = document.textures.Get(texture.first);
				auto& image = document.images.Get(tex.imageId);
				auto& sampler = document.samplers.Get(tex.samplerId);
				auto buffer = resourceReader.get()->ReadBinaryData(document, image);


				int desiredChannels = 4;
				auto engineTexture = Engine::Texture();

				DirectX::ScratchImage scratchImage;
				ThrowIfFailed(DirectX::LoadFromWICMemory(buffer.data(), buffer.size(), DirectX::WIC_FLAGS_NONE, nullptr, scratchImage));
				auto images = scratchImage.GetImages();

				auto mipMapedScratchImage = std::make_shared<DirectX::ScratchImage>();

				ThrowIfFailed(DirectX::GenerateMipMaps(images, 1, scratchImage.GetMetadata(), DirectX::TEX_FILTER_DEFAULT, 0, *mipMapedScratchImage));

				engineTexture.setScratchImage(mipMapedScratchImage);

				auto engineSampler = std::make_shared<Engine::Sampler>();
				auto magFilter = sampler.magFilter.Get();
				auto minFilter = sampler.minFilter.Get();
				auto wrapS = sampler.wrapS;
				auto wrapT = sampler.wrapT;


				if (magFilter == MagFilter_NEAREST && minFilter == MinFilter_NEAREST) {
					engineSampler->samplerDesc.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT;
				}
				else if (magFilter == MagFilter_LINEAR && minFilter == MinFilter_LINEAR) {
					engineSampler->samplerDesc.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
				}
				else if (magFilter == MagFilter_NEAREST && minFilter == MinFilter_LINEAR) {
					engineSampler->samplerDesc.Filter = D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT;
				}
				else if (magFilter == MagFilter_LINEAR && minFilter == MinFilter_NEAREST) {
					engineSampler->samplerDesc.Filter = D3D12_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT;
				}


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

				if (texture.second == TextureType::BaseColor) {
					auto diffuseTexture = std::make_shared<Engine::DiffuseTexture>(engineTexture);
					diffuseTexture->baseColorFactor.x = material.metallicRoughness.baseColorFactor.r;
					diffuseTexture->baseColorFactor.y = material.metallicRoughness.baseColorFactor.g;
					diffuseTexture->baseColorFactor.z = material.metallicRoughness.baseColorFactor.b;
					diffuseTexture->baseColorFactor.w = material.metallicRoughness.baseColorFactor.a;

					meshData->textures.setDiffuseTexture(diffuseTexture);
					meshData->textures.setDiffuseSampler(engineSampler);
				}
				else if (texture.second == TextureType::Normal) {
					auto normalTexture = std::make_shared<Engine::NormalTexture>(engineTexture);
					normalTexture->scale = material.normalTexture.scale;

					meshData->textures.setNormalTexture(normalTexture);
					meshData->textures.setNormalSampler(engineSampler);
				}
				else if (texture.second == TextureType::Occlusion) {
					auto occlusionTexture = std::make_shared<Engine::OcclusionTexture>(engineTexture);
					occlusionTexture->strength = material.occlusionTexture.strength;

					meshData->textures.setOcclusionTexture(occlusionTexture);
					meshData->textures.setOcclusionSampler(engineSampler);
				}
				else if (texture.second == TextureType::Emissive) {
					auto emissiveTexture = std::make_shared<Engine::EmissiveTexture>(engineTexture);
					emissiveTexture->emissiveFactor.x = material.emissiveFactor.r;
					emissiveTexture->emissiveFactor.y = material.emissiveFactor.g;
					emissiveTexture->emissiveFactor.z = material.emissiveFactor.b;

					meshData->textures.setEmissiveTexture(emissiveTexture);
					meshData->textures.setEmissiveSampler(engineSampler);
				}
				else if (texture.second == TextureType::MetallicRoughness) {
					auto metallicRoughnessTexture = std::make_shared<Engine::MetallicRoughnessTexture>(engineTexture);
					metallicRoughnessTexture->metallicFactor = material.metallicRoughness.metallicFactor;
					metallicRoughnessTexture->roughnessFactor = material.metallicRoughness.roughnessFactor;

					meshData->textures.setMetallicRoughnessTexture(metallicRoughnessTexture);
					meshData->textures.setMetallicRoughnessSampler(engineSampler);
				}
			}

			for (const auto& attribute : primitive.attributes) {
				auto& accessor = document.accessors.Get(attribute.second);
				if (attribute.first == ACCESSOR_POSITION) {
					auto data = std::make_shared<std::vector<float>>(resourceReader.get()->ReadBinaryData<float>(document, accessor));
					meshData->setVertices(data);
				}
				if (attribute.first == ACCESSOR_NORMAL) {
					auto data = std::make_shared<std::vector<float>>(resourceReader.get()->ReadBinaryData<float>(document, accessor));
					meshData->setNormals(data);
				}
				if (attribute.first == ACCESSOR_TEXCOORD_0) {
					auto data = std::make_shared<std::vector<float>>(resourceReader.get()->ReadBinaryData<float>(document, accessor));
					meshData->setTexCoords(data);
				}
				if (attribute.first == ACCESSOR_TANGENT) {
					auto data = std::make_shared<std::vector<float>>(resourceReader.get()->ReadBinaryData<float>(document, accessor));
					meshData->setTangents(data);
				}
			}

			meshDataList.push_back(std::move(meshData));
		}

	}

	return meshDataList;
}
