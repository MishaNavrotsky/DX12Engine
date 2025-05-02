#include "stdafx.h"
#pragma once

#include "helpers.h"

namespace Engine {
	using namespace Microsoft::WRL;

	class GPUMesh : public IID {
	public:
		GPUMesh(
			D3D12_VERTEX_BUFFER_VIEW vertexBufferView,
			D3D12_VERTEX_BUFFER_VIEW normalsBufferView,
			D3D12_VERTEX_BUFFER_VIEW texCoordsBufferView,
			D3D12_VERTEX_BUFFER_VIEW tangentsBufferView,
			D3D12_INDEX_BUFFER_VIEW indexBufferView
		)
			: m_vertexBufferView(vertexBufferView),
			m_normalsBufferView(normalsBufferView),
			m_texCoordsBufferView(texCoordsBufferView),
			m_tangentsBufferView(tangentsBufferView),
			m_indexBufferView(indexBufferView)
		{
			m_vertexBufferView.StrideInBytes = sizeof(float) * 3;
			m_normalsBufferView.StrideInBytes = sizeof(float) * 3;
			m_texCoordsBufferView.StrideInBytes = sizeof(float) * 2;
			m_tangentsBufferView.StrideInBytes = sizeof(float) * 4;
			m_indexBufferView.Format = DXGI_FORMAT_R32_UINT;
		}
		D3D12_VERTEX_BUFFER_VIEW* GetVertexBufferView() { return &m_vertexBufferView; }
		D3D12_VERTEX_BUFFER_VIEW* GetNormalsBufferView() { return &m_normalsBufferView; }
		D3D12_VERTEX_BUFFER_VIEW* GetTexCoordsBufferView() { return &m_texCoordsBufferView; }
		D3D12_VERTEX_BUFFER_VIEW* GetTangentsBufferView() { return &m_tangentsBufferView; }
		D3D12_INDEX_BUFFER_VIEW* GetIndexBufferView() { return &m_indexBufferView; }

		void setGPUMaterialId(GUID id) {
			m_gpuMaterialId = id;
		}
		GUID& getGPUMaterialId() {
			return m_gpuMaterialId;
		}
	private:
		D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView = {};
		D3D12_VERTEX_BUFFER_VIEW m_normalsBufferView = {};
		D3D12_VERTEX_BUFFER_VIEW m_texCoordsBufferView = {};
		D3D12_VERTEX_BUFFER_VIEW m_tangentsBufferView = {};
		D3D12_INDEX_BUFFER_VIEW m_indexBufferView = {};

		GUID m_gpuMaterialId = GUID_NULL;
	};
}