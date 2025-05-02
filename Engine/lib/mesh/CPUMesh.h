#include "stdafx.h"

#pragma once

#include "helpers.h"
#include "../geometry/ModelMatrix.h"

namespace Engine {
	using namespace Microsoft::WRL;

	class CPUMesh: public IID {
	public:
		void setVertices(std::vector<float>&& v) noexcept {
			m_vertices = std::move(v);
		}

		void setNormals(std::vector<float>&& v) noexcept {
			m_normals = std::move(v);
		}
		void setTexCoords(std::vector<float>&& v) noexcept {
			m_texCoords = std::move(v);
		}
		void setTangents(std::vector<float>&& v) noexcept {
			m_tangents = std::move(v);
		}

		void setIndices(std::vector<uint32_t>&& v) noexcept {
			m_indices = std::move(v);
		}

		std::vector<float>& getVertices() {
			return m_vertices;
		}

		std::vector<float>& getNormals() {
			return m_normals;
		}
		std::vector<float>& getTexCoords() {
			return m_texCoords;
		}
		std::vector<float>& getTangents() {
			return m_tangents;
		}
		std::vector<uint32_t>& getIndices() {
			return m_indices;
		}

		AlphaMode alphaMode = AlphaMode::Opaque;
		D3D12_CULL_MODE cullMode = D3D12_CULL_MODE_BACK;
		float alphaCutoff = 0.5f;

		GUID& getMaterialId() {
			return m_materialId;
		}
		void setMaterialId(GUID materialId) {
			m_materialId = materialId;
		}
	private:
		std::vector<float> m_vertices;
		std::vector<float> m_normals = std::vector<float>(3, 0.f);
		std::vector<float> m_texCoords = std::vector<float>(2, 0.f);
		std::vector<float> m_tangents = std::vector<float>(4, 0.f);
		std::vector<uint32_t> m_indices;
		GUID m_materialId = GUID_NULL;
	};
}
