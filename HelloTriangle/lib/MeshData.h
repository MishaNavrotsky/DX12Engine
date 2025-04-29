#include "stdafx.h"

#pragma once

#include "DXSampleHelper.h"
#include "DirectXMath.h"
#include "DirectXTex.h"

namespace Engine {
	inline size_t CalculateBufferSize(const DirectX::ScratchImage& scratchImage) {
		size_t totalSize = 0;
		const size_t imageCount = scratchImage.GetImageCount();
		const DirectX::Image* images = scratchImage.GetImages();

		for (size_t i = 0; i < imageCount; ++i) {
			const DirectX::Image& img = images[i];
			totalSize += img.rowPitch * img.height;
		}

		return totalSize;
	}

	enum AlphaMode {
		Opaque,
		Mask,
		Blend
	};

	using namespace Microsoft::WRL;
	class Sampler {
	public:
		Sampler() = default;
		Sampler(const Sampler&) = delete;
		Sampler& operator=(const Sampler&) = delete;

		D3D12_SAMPLER_DESC samplerDesc = {};
		ComPtr<ID3D12Resource> getSamplerResource() {
			return m_sampler;
		}
	private:
		ComPtr<ID3D12Resource> m_sampler = nullptr;
	};

	class Texture {
	public:
		Texture() {
			m_scratchImage->Initialize2D(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, 1, 1, 1, 0);
			const DirectX::Image* images = m_scratchImage->GetImages();
			for (size_t i = 0; i < m_scratchImage->GetImageCount(); ++i) {
				memset(images[i].pixels, 0, images[i].slicePitch);
			}
		};
		Texture(const Texture& other) {
			m_scratchImage = other.m_scratchImage;
			isDefaultScratchImage = other.isDefaultScratchImage;
		}
		Texture& operator=(const Texture&) = delete;

		void setScratchImage(std::shared_ptr<DirectX::ScratchImage> bitmapData) {
			isDefaultScratchImage = false;
			m_scratchImage = bitmapData;
			auto& metadata = bitmapData->GetMetadata();
		}
		std::shared_ptr<DirectX::ScratchImage> getScratchImage() {
			return m_scratchImage;
		}
		bool getIsDefaultScratchImage() const {
			return isDefaultScratchImage;
		}
		ComPtr<ID3D12Resource> getTextureResource() const {
			return m_texture;
		}
		void createBuffer(ID3D12Device* device) {
			ThrowIfFailed(DirectX::CreateTextureEx(
				device,
				m_scratchImage->GetMetadata(),
				D3D12_RESOURCE_FLAG_NONE,
				DirectX::CREATETEX_FLAGS::CREATETEX_DEFAULT,
				&m_texture
			));
		}

	private:
		std::shared_ptr<DirectX::ScratchImage> m_scratchImage = std::make_shared<DirectX::ScratchImage>();
		bool isDefaultScratchImage = true;
		ComPtr<ID3D12Resource> m_texture = nullptr;
	};

	class EmissiveTexture : public Texture {
	public:
		EmissiveTexture() = default;
		EmissiveTexture(const Texture& other) : Texture(other) {};
		DirectX::XMFLOAT3 emissiveFactor = { 0.f, 0.f, 0.f };
	};

	class DiffuseTexture : public Texture {
	public:
		DiffuseTexture() = default;
		DiffuseTexture(const Texture& other) : Texture(other) {};
		DirectX::XMFLOAT4 baseColorFactor = { 1.f, 1.f, 1.f, 1.f };
	};

	class NormalTexture : public Texture {
	public:
		NormalTexture() = default;
		NormalTexture(const Texture& other) : Texture(other) {};
		float scale = 1.f;
	};

	class OcclusionTexture : public Texture {
	public:
		OcclusionTexture() = default;
		OcclusionTexture(const Texture& other) : Texture(other) {};
		float strength = 1.f;
	};

	class MetallicRoughnessTexture : public Texture {
	public:
		MetallicRoughnessTexture() = default;
		MetallicRoughnessTexture(const Texture& other) : Texture(other) {};
		float metallicFactor = 1.f;
		float roughnessFactor = 1.f;
	};


	class MeshDataTextures {
	public:
		MeshDataTextures() = default;
		MeshDataTextures(const MeshDataTextures&) = delete;
		MeshDataTextures& operator=(const MeshDataTextures&) = delete;

		void setDiffuseTexture(std::shared_ptr<DiffuseTexture> texture) {
			m_diffuseTexture = texture;
		}
		void setNormalTexture(std::shared_ptr<NormalTexture> texture) {
			m_normalTexture = texture;
		}
		void setOcclusionTexture(std::shared_ptr<OcclusionTexture> texture) {
			m_occlusionTexture = texture;
		}
		void setEmissiveTexture(std::shared_ptr<EmissiveTexture> texture) {
			m_emissiveTexture = texture;
		}
		void setMetallicRoughnessTexture(std::shared_ptr<MetallicRoughnessTexture> texture) {
			m_metallicRoughnessTexture = texture;
		}
		void setDiffuseSampler(std::shared_ptr<Sampler> sampler) {
			m_diffuseSampler = sampler;
		}
		void setNormalSampler(std::shared_ptr<Sampler> sampler) {
			m_normalSampler = sampler;
		}
		void setOcclusionSampler(std::shared_ptr<Sampler> sampler) {
			m_occlusionSampler = sampler;
		}
		void setEmissiveSampler(std::shared_ptr<Sampler> sampler) {
			m_emissiveSampler = sampler;
		}
		void setMetallicRoughnessSampler(std::shared_ptr<Sampler> sampler) {
			m_metallicRoughnessSampler = sampler;
		}

		void createBuffers(ID3D12Device* device, ID3D12GraphicsCommandList* commandList) {
			uploadDataToUploadBufferAndInitializeTextureResources(device, commandList);
		}

		void prepareBuffers(ID3D12GraphicsCommandList* commandList) {
			if (isBuffersPrepared) return;
			const size_t textureCount = 5;
			ID3D12Resource* textureResources[textureCount] = { m_diffuseTexture->getTextureResource().Get(), m_normalTexture->getTextureResource().Get(), m_occlusionTexture->getTextureResource().Get(), m_emissiveTexture->getTextureResource().Get(), m_metallicRoughnessTexture->getTextureResource().Get() };
			D3D12_RESOURCE_BARRIER barriers[textureCount];
			
			for (uint32_t i = 0; i < textureCount; i++) {
				barriers[i] = CD3DX12_RESOURCE_BARRIER::Transition(
					textureResources[i],
					D3D12_RESOURCE_STATE_COMMON,
					D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
				);
			}
			commandList->ResourceBarrier(textureCount, barriers);
			isBuffersPrepared = true;
		}
	private:
		void uploadDataToUploadBufferAndInitializeTextureResources(ID3D12Device* device, ID3D12GraphicsCommandList* commandList) {
			const size_t textureCount = 5;
			std::shared_ptr<Texture> textures[textureCount] = { m_diffuseTexture, m_normalTexture, m_occlusionTexture, m_emissiveTexture, m_metallicRoughnessTexture };
			auto diffuseTextureSize = CalculateBufferSize(*m_diffuseTexture->getScratchImage());
			auto normalTextureSize = CalculateBufferSize(*m_normalTexture->getScratchImage());
			auto occlusionTextureSize = CalculateBufferSize(*m_occlusionTexture->getScratchImage());
			auto emissiveTextureSize = CalculateBufferSize(*m_emissiveTexture->getScratchImage());
			auto metallicRoughnessTextureSize = CalculateBufferSize(*m_metallicRoughnessTexture->getScratchImage());
			uint64_t bufferSize = diffuseTextureSize + normalTextureSize + occlusionTextureSize + emissiveTextureSize + metallicRoughnessTextureSize;


			auto heapUploadProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
			auto heapUploadDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

			ThrowIfFailed(device->CreateCommittedResource(
				&heapUploadProps,
				D3D12_HEAP_FLAG_NONE,
				&heapUploadDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&m_srvUploadHeap)));


			uint64_t offset = 0;
			for (auto& tex : textures) {
				tex->createBuffer(device);

				auto scratchImage = tex->getScratchImage();
				auto textureResource = tex->getTextureResource();
				std::vector<D3D12_SUBRESOURCE_DATA> subresources;

				ThrowIfFailed(DirectX::PrepareUpload(device, scratchImage->GetImages(), scratchImage->GetImageCount(), scratchImage->GetMetadata(), subresources));

				const UINT64 uploadBufferSize = GetRequiredIntermediateSize(textureResource.Get(),
					0, static_cast<UINT>(subresources.size()));


				UpdateSubresources(commandList,
					textureResource.Get(), m_srvUploadHeap.Get(),
					offset, 0, static_cast<UINT>(subresources.size()),
					subresources.data());
				offset += uploadBufferSize;
			}
		}
		std::shared_ptr<DiffuseTexture> m_diffuseTexture = std::make_shared<DiffuseTexture>();
		std::shared_ptr<NormalTexture> m_normalTexture = std::make_shared<NormalTexture>();
		std::shared_ptr<OcclusionTexture> m_occlusionTexture = std::make_shared<OcclusionTexture>();
		std::shared_ptr<EmissiveTexture> m_emissiveTexture = std::make_shared<EmissiveTexture>();
		std::shared_ptr<MetallicRoughnessTexture> m_metallicRoughnessTexture = std::make_shared<MetallicRoughnessTexture>();

		std::shared_ptr<Sampler> m_diffuseSampler = std::make_shared<Sampler>();
		std::shared_ptr<Sampler> m_normalSampler = std::make_shared<Sampler>();
		std::shared_ptr<Sampler> m_occlusionSampler = std::make_shared<Sampler>();
		std::shared_ptr<Sampler> m_emissiveSampler = std::make_shared<Sampler>();
		std::shared_ptr<Sampler> m_metallicRoughnessSampler = std::make_shared<Sampler>();

		ComPtr<ID3D12Resource> m_srvUploadHeap = nullptr;

		bool isBuffersPrepared = false;
	};

	class MeshData {
	public:
		MeshData() = default;
		MeshData(const MeshData&) = delete;
		MeshData& operator=(const MeshData&) = delete;

		MeshDataTextures textures;

		void setVertices(std::shared_ptr<std::vector<float>> v) {
			m_vertices = v;
		}

		void setNormals(std::shared_ptr<std::vector<float>> v) {
			m_normals = v;
		}
		void setTexCoords(std::shared_ptr<std::vector<float>> v) {
			m_texCoords = v;
		}
		void setTangents(std::shared_ptr<std::vector<float>> v) {
			m_tangents = v;
		}
		void setIndices(std::shared_ptr<std::vector<uint32_t>> v) {
			m_indices = v;
		}
		std::shared_ptr<std::vector<float>> getVertices() {
			return m_vertices;
		}

		std::shared_ptr<std::vector<float>> getNormals() {
			return m_normals;
		}
		std::shared_ptr<std::vector<float>> getTexCoords() {
			return m_texCoords;
		}
		std::shared_ptr<std::vector<float>> getTangents() {
			return m_tangents;
		}
		std::shared_ptr<std::vector<uint32_t>> getIndices() {
			return m_indices;
		}

		void createBuffers(ID3D12Device* device, ID3D12GraphicsCommandList* commandList) {
			uploadDataToGPUUploadHeapAndInitializeBufferViews(device, commandList);
			textures.createBuffers(device, commandList);
		}

		D3D12_VERTEX_BUFFER_VIEW* getVertexBufferView() {
			return &m_vertexBufferView;
		}
		D3D12_INDEX_BUFFER_VIEW* getIndicesBufferView() {
			return &m_indexBufferView;
		}
		D3D12_VERTEX_BUFFER_VIEW* getNormalsBufferView() {
			return &m_normalsBufferView;
		}
		D3D12_VERTEX_BUFFER_VIEW* getTexCoordsBufferView() {
			return &m_texCoordsBufferView;
		}
		D3D12_VERTEX_BUFFER_VIEW* getTangentsBufferView() {
			return &m_tangentsBufferView;
		}

		void transitionVertexBufferAndIndexBufferToTheirStates(ID3D12GraphicsCommandList* commandList) {
			if (m_isVertexAndIndexBufferTransitioned) return;
			auto barrierVertex = CD3DX12_RESOURCE_BARRIER::Transition(m_vertexBuffersDefaultHeap.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
			auto barrierIndex = CD3DX12_RESOURCE_BARRIER::Transition(m_indexBuffersDefaultHeap.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);

			commandList->ResourceBarrier(1, &barrierVertex);
			commandList->ResourceBarrier(1, &barrierIndex);
			m_isVertexAndIndexBufferTransitioned = true;
		}

		AlphaMode alphaMode = AlphaMode::Opaque;
		D3D12_CULL_MODE cullMode = D3D12_CULL_MODE_BACK;
		float alphaCutoff = 0.5f;
	private:
		void uploadDataToGPUUploadHeapAndInitializeBufferViews(ID3D12Device* device, ID3D12GraphicsCommandList* commandList) {
			auto ver = m_vertices.get();
			auto nor = m_normals.get();
			auto tex = m_texCoords.get();
			auto tan = m_tangents.get();
			auto ind = m_indices.get();

			auto verSize = ver->size() * sizeof(ver->front());
			auto norSize = nor->size() * sizeof(nor->front());
			auto texSize = tex->size() * sizeof(tex->front());
			auto tanSize = tan->size() * sizeof(tan->front());
			auto indSize = ind->size() * sizeof(ind->front());


			{
				size_t bufferSize = verSize + norSize + texSize + tanSize;


				auto heapGpuProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_GPU_UPLOAD);
				auto heapGpuDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
				device->CreateCommittedResource(
					&heapGpuProps,
					D3D12_HEAP_FLAG_NONE,
					&heapGpuDesc,
					D3D12_RESOURCE_STATE_COMMON,
					nullptr,
					IID_PPV_ARGS(&m_vertexBuffersGPUUploadHeap));

				uint8_t* mappedData;
				ThrowIfFailed(m_vertexBuffersGPUUploadHeap->Map(0, nullptr, reinterpret_cast<void**>(&mappedData)));
				memcpy(mappedData, ver->data(), verSize);
				memcpy(mappedData + verSize, nor->data(), norSize);
				memcpy(mappedData + verSize + norSize, tex->data(), texSize);
				memcpy(mappedData + verSize + norSize + texSize, tan->data(), tanSize);
				m_vertexBuffersGPUUploadHeap->Unmap(0, nullptr);


				auto defaultHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
				auto defaultHeapDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

				ThrowIfFailed(device->CreateCommittedResource(
					&defaultHeapProps,
					D3D12_HEAP_FLAG_NONE,
					&defaultHeapDesc,
					D3D12_RESOURCE_STATE_COMMON,
					nullptr,
					IID_PPV_ARGS(&m_vertexBuffersDefaultHeap)));

				// copy the bufferUploadHeap to the bufferDefaultHeap
				commandList->CopyBufferRegion(
					m_vertexBuffersDefaultHeap.Get(),
					0,
					m_vertexBuffersGPUUploadHeap.Get(),
					0,
					bufferSize
				);
				m_vertexBufferView.BufferLocation = m_vertexBuffersDefaultHeap->GetGPUVirtualAddress();
				m_vertexBufferView.StrideInBytes = sizeof(float) * 3;
				m_vertexBufferView.SizeInBytes = static_cast<UINT>(verSize);

				m_normalsBufferView.BufferLocation = m_vertexBuffersDefaultHeap->GetGPUVirtualAddress() + verSize;
				m_normalsBufferView.StrideInBytes = sizeof(float) * 3;
				m_normalsBufferView.SizeInBytes = static_cast<UINT>(norSize);

				m_texCoordsBufferView.BufferLocation = m_vertexBuffersDefaultHeap->GetGPUVirtualAddress() + verSize + norSize;
				m_texCoordsBufferView.StrideInBytes = sizeof(float) * 2;
				m_texCoordsBufferView.SizeInBytes = static_cast<UINT>(texSize);

				m_tangentsBufferView.BufferLocation = m_vertexBuffersDefaultHeap->GetGPUVirtualAddress() + verSize + norSize + texSize;
				m_tangentsBufferView.StrideInBytes = sizeof(float) * 4;
				m_tangentsBufferView.SizeInBytes = static_cast<UINT>(tanSize);
			}

			{
				auto heapUploadProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
				auto heapUploadDesc = CD3DX12_RESOURCE_DESC::Buffer(indSize);
				ThrowIfFailed(device->CreateCommittedResource(
					&heapUploadProps,
					D3D12_HEAP_FLAG_NONE,
					&heapUploadDesc,
					D3D12_RESOURCE_STATE_GENERIC_READ,
					nullptr,
					IID_PPV_ARGS(&m_indexBufferUploadHeap)));

				uint8_t* mappedData;
				ThrowIfFailed(m_indexBufferUploadHeap->Map(0, nullptr, reinterpret_cast<void**>(&mappedData)));
				memcpy(mappedData, ind->data(), indSize);
				m_indexBufferUploadHeap->Unmap(0, nullptr);

				auto heapDefaultProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
				auto heapDefaultDesc = CD3DX12_RESOURCE_DESC::Buffer(indSize);
				ThrowIfFailed(device->CreateCommittedResource(
					&heapDefaultProps,
					D3D12_HEAP_FLAG_NONE,
					&heapDefaultDesc,
					D3D12_RESOURCE_STATE_COMMON,
					nullptr,
					IID_PPV_ARGS(&m_indexBuffersDefaultHeap)));

				// copy the bufferUploadHeap to the bufferDefaultHeap
				commandList->CopyBufferRegion(
					m_indexBuffersDefaultHeap.Get(),
					0,
					m_indexBufferUploadHeap.Get(),
					0,
					indSize
				);

				m_indexBufferView.BufferLocation = m_indexBuffersDefaultHeap->GetGPUVirtualAddress();
				m_indexBufferView.Format = DXGI_FORMAT_R32_UINT;
				m_indexBufferView.SizeInBytes = static_cast<UINT>(indSize);
			}
		}

		std::shared_ptr<std::vector<float>> m_vertices = nullptr;
		std::shared_ptr<std::vector<float>> m_normals = std::make_shared<std::vector<float>>(3, 0);
		std::shared_ptr<std::vector<float>> m_texCoords = std::make_shared<std::vector<float>>(2, 0);
		std::shared_ptr<std::vector<float>> m_tangents = std::make_shared<std::vector<float>>(4, 0);
		std::shared_ptr<std::vector<uint32_t>> m_indices = nullptr;

		D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView = {};
		D3D12_VERTEX_BUFFER_VIEW m_normalsBufferView = {};
		D3D12_VERTEX_BUFFER_VIEW m_texCoordsBufferView = {};
		D3D12_VERTEX_BUFFER_VIEW m_tangentsBufferView = {};
		D3D12_INDEX_BUFFER_VIEW m_indexBufferView = {};

		ComPtr<ID3D12Resource> m_vertexBuffersGPUUploadHeap = nullptr;
		ComPtr<ID3D12Resource> m_vertexBuffersDefaultHeap = nullptr;
		ComPtr<ID3D12Resource> m_indexBufferUploadHeap = nullptr;
		ComPtr<ID3D12Resource> m_indexBuffersDefaultHeap = nullptr;

		bool m_isVertexAndIndexBufferTransitioned = false;
	};
}
