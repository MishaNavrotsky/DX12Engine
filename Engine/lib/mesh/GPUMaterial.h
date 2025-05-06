#include "stdafx.h"

#pragma once

#include "helpers.h"

namespace Engine {
	using namespace Microsoft::WRL;

	class GPUMaterial : public IID {
	public:
		GPUMaterial() = default;

		std::vector<GUID>& getTextureIds() {
			return m_textureIds;
		}
		std::vector<GUID>& getSamplerIds() {
			return m_samplerIds;
		}
		void setTextureIds(std::vector<GUID>&& v) {
			m_textureIds = std::move(v);
		}
		void setSamplerIds(std::vector<GUID>&& v) {
			m_samplerIds = std::move(v);
		}

		ID3D12Resource* getCBVResource() const {
			return m_cbv.Get();
		}
	private:
		std::vector<GUID> m_textureIds;
		std::vector<GUID> m_samplerIds;
		ComPtr<ID3D12Resource> m_cbv;
	};
}