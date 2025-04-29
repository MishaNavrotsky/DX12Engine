#include "stdafx.h"

#pragma once

#include "Material.h"
#include "helpers.h"

namespace Engine {
	using namespace Microsoft::WRL;

	class Mesh {
	public:
		Mesh() = default;
		Mesh(const Mesh&) = delete;
		Mesh& operator=(const Mesh&) = delete;

		std::unique_ptr<Material> material;

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
			material->textures.createBuffers(device, commandList);
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
