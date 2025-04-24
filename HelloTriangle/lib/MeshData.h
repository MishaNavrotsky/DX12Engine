#include "stdafx.h"

#pragma once

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
		std::shared_ptr<std::vector<float>> getVertices() {
			return m_vertices;
		}
		std::shared_ptr<std::vector<uint32_t>> getIndices() {
			return m_indices;
		}

		void createBuffers(ID3D12Device* device, ID3D12GraphicsCommandList* commandList) {
			if (m_vertices && m_indices) {
				UINT vertexBufferSize = static_cast<UINT>(m_vertices->size() * sizeof(float));
				UINT indexBufferSize = static_cast<UINT>(m_indices->size() * sizeof(uint32_t));
				uploadBufferToGpu(device, commandList, gpuVerticesBuffer, *m_vertices, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
				uploadBufferToGpu(device, commandList, gpuIndicesBuffer, *m_indices, D3D12_RESOURCE_STATE_INDEX_BUFFER);
				vertexBufferView.BufferLocation = gpuVerticesBuffer->GetGPUVirtualAddress();
				vertexBufferView.StrideInBytes = sizeof(float) * 3;
				vertexBufferView.SizeInBytes = vertexBufferSize;
				indexBufferView.BufferLocation = gpuIndicesBuffer->GetGPUVirtualAddress();
				indexBufferView.Format = DXGI_FORMAT_R32_UINT;
				indexBufferView.SizeInBytes = indexBufferSize;
			}
		}

		D3D12_VERTEX_BUFFER_VIEW* getVertexBufferView() {
			return &vertexBufferView;
		}
		D3D12_INDEX_BUFFER_VIEW* getIndexBufferView() {
			return &indexBufferView;
		}

	private:
		template<typename T>
		void uploadBufferToGpu(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, ComPtr<ID3D12Resource>& gpuBuffer, const std::vector<T>& data, D3D12_RESOURCE_STATES s) {
			const UINT bufferSize = static_cast<UINT>(data.size() * sizeof(T));

			CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
			CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

			ThrowIfFailed(device->CreateCommittedResource(
				&heapProps,
				D3D12_HEAP_FLAG_NONE,
				&bufferDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&gpuBuffer)));

			void* mappedData = nullptr;
			CD3DX12_RANGE readRange(0, 0); // We don't intend to read back.
			ThrowIfFailed(gpuBuffer->Map(0, &readRange, reinterpret_cast<void**>(&mappedData)));
			memcpy(mappedData, data.data(), bufferSize);
			gpuBuffer->Unmap(0, nullptr);
		}

		std::shared_ptr<std::vector<float>> m_vertices = nullptr;
		std::shared_ptr<std::vector<uint32_t>> m_indices = nullptr;
		ComPtr<ID3D12Resource> gpuVerticesBuffer;
		D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
		ComPtr<ID3D12Resource> gpuIndicesBuffer;
		D3D12_INDEX_BUFFER_VIEW indexBufferView;

	};
}
