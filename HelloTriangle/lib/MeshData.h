#include "stdafx.h"

#pragma once

#include "DXSampleHelper.h"

namespace Engine {
	using namespace Microsoft::WRL;
	class MeshData {
	public:
		MeshData() = default;
		MeshData(const MeshData&) = delete;
		MeshData& operator=(const MeshData&) = delete;

		void setVertices(std::shared_ptr<std::vector<float>> v) {
			m_vertices = v;
		}
		void setIndices(std::shared_ptr<std::vector<uint32_t>> v) {
			m_indices = v;
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
		std::shared_ptr<std::vector<float>> getVertices() {
			return m_vertices;
		}
		std::shared_ptr<std::vector<uint32_t>> getIndices() {
			return m_indices;
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

		void createBuffers(ID3D12Device* device, ID3D12GraphicsCommandList* commandList) {
			uploadDataToUploadBufferAndInitializeBufferViews(device);
		}

		D3D12_VERTEX_BUFFER_VIEW* getVertexBufferView() {
			return &vertexBufferView;
		}
		D3D12_INDEX_BUFFER_VIEW* getIndicesBufferView() {
			return &indexBufferView;
		}
		D3D12_VERTEX_BUFFER_VIEW* getNormalsBufferView() {
			return &normalsBufferView;
		}
		D3D12_VERTEX_BUFFER_VIEW* getTexCoordsBufferView() {
			return &texCoordsBufferView;
		}
		D3D12_VERTEX_BUFFER_VIEW* getTangentsBufferView() {
			return &tangentsBufferView;
		}

	private:
		void uploadDataToUploadBufferAndInitializeBufferViews(ID3D12Device* device) {
			auto ver = m_vertices.get();
			auto ind = m_indices.get();
			auto nor = m_normals.get();
			auto tex = m_texCoords.get();
			auto tan = m_tangents.get();

			auto verSize = ver->size() * sizeof(ver->front());
			auto indSize = ind->size() * sizeof(ind->front());
			auto norSize = nor->size() * sizeof(nor->front());
			auto texSize = tex->size() * sizeof(tex->front());
			auto tanSize = tan->size() * sizeof(tan->front());


			size_t bufferSize = verSize + indSize + norSize + texSize + tanSize;
			device->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Buffer(bufferSize),
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&bufferUploadHeap));

			uint8_t* mappedData;
			ThrowIfFailed(bufferUploadHeap->Map(0, nullptr, reinterpret_cast<void(**)>(&mappedData)));
			memcpy(mappedData, ver->data(), verSize);
			memcpy(mappedData + verSize, ind->data(), indSize);
			memcpy(mappedData + verSize + indSize, nor->data(), norSize);
			memcpy(mappedData + verSize + indSize + norSize, tex->data(), texSize);
			memcpy(mappedData + verSize + indSize + norSize + texSize, tan->data(), tanSize);
			bufferUploadHeap->Unmap(0, nullptr);

			vertexBufferView.BufferLocation = bufferUploadHeap->GetGPUVirtualAddress();
			vertexBufferView.StrideInBytes = sizeof(float) * 3;
			vertexBufferView.SizeInBytes = static_cast<UINT>(verSize);

			indexBufferView.BufferLocation = bufferUploadHeap->GetGPUVirtualAddress() + verSize;
			indexBufferView.Format = DXGI_FORMAT_R32_UINT;
			indexBufferView.SizeInBytes = static_cast<UINT>(indSize);

			normalsBufferView.BufferLocation = bufferUploadHeap->GetGPUVirtualAddress() + verSize + indSize;
			normalsBufferView.StrideInBytes = sizeof(float) * 3;
			normalsBufferView.SizeInBytes = static_cast<UINT>(norSize);

			texCoordsBufferView.BufferLocation = bufferUploadHeap->GetGPUVirtualAddress() + verSize + indSize + norSize;
			texCoordsBufferView.StrideInBytes = sizeof(float) * 2;
			texCoordsBufferView.SizeInBytes = static_cast<UINT>(texSize);

			tangentsBufferView.BufferLocation = bufferUploadHeap->GetGPUVirtualAddress() + verSize + indSize + norSize + texSize;
			tangentsBufferView.StrideInBytes = sizeof(float) * 4;
			tangentsBufferView.SizeInBytes = static_cast<UINT>(tanSize);
		}

		std::shared_ptr<std::vector<float>> m_vertices = nullptr;
		std::shared_ptr<std::vector<uint32_t>> m_indices = nullptr;
		std::shared_ptr<std::vector<float>> m_normals = std::make_shared<std::vector<float>>(2, 0);
		std::shared_ptr<std::vector<float>> m_texCoords = std::make_shared<std::vector<float>>(1, 0);
		std::shared_ptr<std::vector<float>> m_tangents = std::make_shared<std::vector<float>>(1, 0);

		D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
		D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
		D3D12_VERTEX_BUFFER_VIEW normalsBufferView = {};
		D3D12_VERTEX_BUFFER_VIEW texCoordsBufferView = {};
		D3D12_VERTEX_BUFFER_VIEW tangentsBufferView = {};

		ComPtr<ID3D12Resource> bufferUploadHeap = nullptr;
	};
}
