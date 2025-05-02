#include "stdafx.h"

#pragma once

#include "helpers.h"

namespace Engine {
	using namespace Microsoft::WRL;

	class GPUMaterial : public IID {
	public:
		GPUMaterial(std::vector<GUID> textureResourceIds, std::vector<GUID> samplerIds)
			: m_textureResourceIds(std::move(textureResourceIds)),
			m_samplerIds(std::move(samplerIds)) {
		}

		std::vector<GUID>& getTextureResourceIds() {
			return m_textureResourceIds;
		}
		std::vector<GUID>& getSamplerDescIds() {
			return m_samplerIds;
		}

	private:
		std::vector<GUID> m_textureResourceIds;
		std::vector<GUID> m_samplerIds;
	};
}