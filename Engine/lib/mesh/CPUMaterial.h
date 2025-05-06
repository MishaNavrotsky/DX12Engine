#include "stdafx.h"

#pragma once

#include "helpers.h"

namespace Engine {
	using namespace Microsoft::WRL;

	struct CPUMaterialCBVData {
		XMFLOAT3 emissiveFactor = { 0.f, 0.f, 0.f };
		XMFLOAT4 baseColorFactor = { 1.f, 1.f, 1.f, 1.f };
		XMFLOAT4 normalScaleOcclusionStrengthMRFactors = { 1.f, 1.f, 1.f, 1.f };
		XMUINT4 diffuseEmissiveNormalOcclusionTexSlots = { 
			static_cast<uint32_t>(DefaultTexturesSlot::BASE_COLOR),
			static_cast<uint32_t>(DefaultTexturesSlot::EMISSIVE),
			static_cast<uint32_t>(DefaultTexturesSlot::NORMAL),
			static_cast<uint32_t>(DefaultTexturesSlot::OCCLUSION)
		};
		XMUINT4 MrTexSlots = { static_cast<uint32_t>(DefaultTexturesSlot::METALLIC_ROUGHNESS), 0, 0, 0 };

		XMUINT4 diffuseEmissiveNormalOcclusionSamSlots = {
			static_cast<uint32_t>(DefaultSamplersSlot::BASE_COLOR),
			static_cast<uint32_t>(DefaultSamplersSlot::EMISSIVE),
			static_cast<uint32_t>(DefaultSamplersSlot::NORMAL),
			static_cast<uint32_t>(DefaultSamplersSlot::OCCLUSION)
		};
		XMUINT4 MrSamSlots = { static_cast<uint32_t>(DefaultSamplersSlot::METALLIC_ROUGHNESS), 0, 0, 0 };
	};

	class CPUMaterial : public IID {
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

		CPUMaterialCBVData& getCBVData() {
			return m_cbvData;
		}

	private:
		std::vector<GUID> m_textureIds;
		std::vector<GUID> m_samplerIds;
		CPUMaterialCBVData m_cbvData;
	};
}