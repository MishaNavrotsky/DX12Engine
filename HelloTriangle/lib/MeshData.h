#include "stdafx.h"

#pragma once

#include "DXSampleHelper.h"

namespace Engine {
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
		Texture() = default;
		Texture(const Texture&) = delete;
		Texture& operator=(const Texture&) = delete;

		D3D12_RESOURCE_DESC textureDesc = {};
		ComPtr<ID3D12Resource> getTextureResource() {
			return m_texture;
		}
		void setBitmapData(std::shared_ptr<std::vector<uint8_t>> bitmapData) {
			m_bitmapData = bitmapData;
		}
	private:
		ComPtr<ID3D12Resource> m_texture = nullptr;
		std::shared_ptr<std::vector<uint8_t>> m_bitmapData = std::make_shared<std::vector<uint8_t>>(1, 0);
	};

	class MeshDataTextures {
	public:
		MeshDataTextures() = default;
		MeshDataTextures(const MeshDataTextures&) = delete;
		MeshDataTextures& operator=(const MeshDataTextures&) = delete;

		void setDiffuseTexture(std::shared_ptr<Texture> texture) {
			m_diffuseTexture = texture;
		}
		void setNormalTexture(std::shared_ptr<Texture> texture) {
			m_normalTexture = texture;
		}
		void setOcclusionTexture(std::shared_ptr<Texture> texture) {
			m_occlusionTexture = texture;
		}
		void setEmissiveTexture(std::shared_ptr<Texture> texture) {
			m_emissiveTexture = texture;
		}
		void setMetallicRoughnessTexture(std::shared_ptr<Texture> texture) {
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

		ComPtr<ID3D12DescriptorHeap> getSRVHeap() {
			return m_srvHeap;
		}
		ComPtr<ID3D12DescriptorHeap> getSamplerHeap() {
			return m_samplerHeap;
		}

	private:
		std::shared_ptr<Texture> m_diffuseTexture = std::make_shared<Texture>();
		std::shared_ptr<Texture> m_normalTexture = std::make_shared<Texture>();
		std::shared_ptr<Texture> m_occlusionTexture = std::make_shared<Texture>();
		std::shared_ptr<Texture> m_emissiveTexture = std::make_shared<Texture>();
		std::shared_ptr<Texture> m_metallicRoughnessTexture = std::make_shared<Texture>();

		std::shared_ptr<Sampler> m_diffuseSampler = std::make_shared<Sampler>();
		std::shared_ptr<Sampler> m_normalSampler = std::make_shared<Sampler>();
		std::shared_ptr<Sampler> m_occlusionSampler = std::make_shared<Sampler>();
		std::shared_ptr<Sampler> m_emissiveSampler = std::make_shared<Sampler>();
		std::shared_ptr<Sampler> m_metallicRoughnessSampler = std::make_shared<Sampler>();

		ComPtr<ID3D12DescriptorHeap> m_srvHeap = nullptr;
		ComPtr<ID3D12DescriptorHeap> m_samplerHeap = nullptr;

		ComPtr<ID3D12Resource> m_srvUploadHeap = nullptr;

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
			uploadDataToUploadBufferAndInitializeBufferViews(device);
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

	private:
		void uploadDataToUploadBufferAndInitializeBufferViews(ID3D12Device* device) {
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


			size_t bufferSize = verSize + indSize + norSize + texSize + tanSize;
			device->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Buffer(bufferSize),
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&m_bufferUploadHeap));

			uint8_t* mappedData;
			ThrowIfFailed(m_bufferUploadHeap->Map(0, nullptr, reinterpret_cast<void(**)>(&mappedData)));
			memcpy(mappedData, ver->data(), verSize);
			memcpy(mappedData + verSize, nor->data(), norSize);
			memcpy(mappedData + verSize + norSize, tex->data(), texSize);
			memcpy(mappedData + verSize + norSize + texSize, tan->data(), tanSize);
			memcpy(mappedData + verSize + norSize + texSize + tanSize, ind->data(), indSize);
			m_bufferUploadHeap->Unmap(0, nullptr);

			m_vertexBufferView.BufferLocation = m_bufferUploadHeap->GetGPUVirtualAddress();
			m_vertexBufferView.StrideInBytes = sizeof(float) * 3;
			m_vertexBufferView.SizeInBytes = static_cast<UINT>(verSize);

			m_normalsBufferView.BufferLocation = m_bufferUploadHeap->GetGPUVirtualAddress() + verSize;
			m_normalsBufferView.StrideInBytes = sizeof(float) * 3;
			m_normalsBufferView.SizeInBytes = static_cast<UINT>(norSize);

			m_texCoordsBufferView.BufferLocation = m_bufferUploadHeap->GetGPUVirtualAddress() + verSize + norSize;
			m_texCoordsBufferView.StrideInBytes = sizeof(float) * 2;
			m_texCoordsBufferView.SizeInBytes = static_cast<UINT>(texSize);

			m_tangentsBufferView.BufferLocation = m_bufferUploadHeap->GetGPUVirtualAddress() + verSize + norSize + texSize;
			m_tangentsBufferView.StrideInBytes = sizeof(float) * 4;
			m_tangentsBufferView.SizeInBytes = static_cast<UINT>(tanSize);

			m_indexBufferView.BufferLocation = m_bufferUploadHeap->GetGPUVirtualAddress() + verSize + norSize + texSize + tanSize;
			m_indexBufferView.Format = DXGI_FORMAT_R32_UINT;
			m_indexBufferView.SizeInBytes = static_cast<UINT>(indSize);

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

		ComPtr<ID3D12Resource> m_bufferUploadHeap = nullptr;
	};
}
