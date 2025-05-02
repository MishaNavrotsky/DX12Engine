#include "stdafx.h"

#pragma once

#include "helpers.h"

namespace Engine {
	using namespace Microsoft::WRL;

	class CPUMaterial: public IID {
	public:
		CPUMaterial() {};

		std::vector<GUID>& getTextureIds() {
			return m_textureIds;
		}
		void setTextureIds(std::vector<GUID>&& texturesIds) noexcept {
			m_textureIds = std::move(texturesIds);
		}

		std::vector<GUID>& getSamplerIds() {
			return m_samplerIds;
		}
		void setSamplerIds(std::vector<GUID>&& samplerIds) noexcept {
			m_samplerIds = std::move(samplerIds);
		}

	private:
		std::vector<GUID> m_textureIds;
		std::vector<GUID> m_samplerIds;
	};
}