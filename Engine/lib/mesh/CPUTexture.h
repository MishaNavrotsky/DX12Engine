#include "stdafx.h"

#pragma once

#include "DirectXTex.h"
#include "../DXSampleHelper.h"
#include "helpers.h"
#include "../managers/helpers.h"

namespace Engine {
	class CPUTexture: public IID {
	public:
		virtual ~CPUTexture() = default;

		CPUTexture() {
			m_scratchImage->Initialize2D(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, 1, 1, 1, 0);
			const DX::Image* images = m_scratchImage->GetImages();
			for (size_t i = 0; i < m_scratchImage->GetImageCount(); ++i) {
				memset(images[i].pixels, 0, images[i].slicePitch);
			}
		};
		CPUTexture(CPUTexture&& other) noexcept {
			m_scratchImage = std::move(other.m_scratchImage);
			m_isDefaultScratchImage = other.m_isDefaultScratchImage;
			m_id = other.m_id;
			m_cpuMaterialsIds = std::move(other.m_cpuMaterialsIds);
			m_samplerId = other.m_samplerId;
		}
		CPUTexture(const CPUTexture& other) = delete;
		CPUTexture& operator=(const CPUTexture&) = delete;

		void setScratchImage(std::unique_ptr<DX::ScratchImage>&& bitmapData) {
			m_isDefaultScratchImage = false;
			m_scratchImage = std::move(bitmapData);
		}
		DX::ScratchImage& getScratchImage() {
			return *m_scratchImage;
		}
		bool getIsDefaultScratchImage() const {
			return m_isDefaultScratchImage;
		}
		virtual TextureType getType() const {
			return TextureType::DEFAULT;
		}

		std::unordered_set<GUID, GUIDHash, GUIDEqual>& getCpuMaterialsIds() {
			return m_cpuMaterialsIds;
		}
		void addCpuMaterialId(const GUID& id) {
			m_cpuMaterialsIds.insert(id);
		}
		void removeCpuMaterialId(const GUID& id) {
			m_cpuMaterialsIds.erase(id);
		}
		void clearCpuMaterialIds() {
			m_cpuMaterialsIds.clear();
		}
		void setCpuMaterialIds(std::unordered_set<GUID, GUIDHash, GUIDEqual>&& ids) {
			m_cpuMaterialsIds = std::move(ids);
		}

		GUID& getSamplerId() {
			return m_samplerId;
		}
		void setSamplerId(const GUID& id) {
			m_samplerId = id;
		}

		uint32_t bindlessHeapSlot = 0;
	protected:
		std::unique_ptr<DX::ScratchImage> m_scratchImage = std::make_unique<DX::ScratchImage>();
		bool m_isDefaultScratchImage = true;
		std::unordered_set<GUID, GUIDHash, GUIDEqual> m_cpuMaterialsIds;
		GUID m_samplerId = GUID_NULL;
	};

	class CPUEmissiveTexture : public CPUTexture {
	public:
		CPUEmissiveTexture() = default;
		CPUEmissiveTexture(CPUTexture&& other) noexcept : CPUTexture(std::move(other)) {};

		TextureType getType() const override { return TextureType::EMISSIVE; }
	};

	class CPUDiffuseTexture : public CPUTexture {
	public:
		CPUDiffuseTexture() = default;
		CPUDiffuseTexture(CPUTexture&& other) noexcept : CPUTexture(std::move(other)) {};

		TextureType getType() const override { return TextureType::BASE_COLOR; }
	};

	class CPUNormalTexture : public CPUTexture {
	public:
		CPUNormalTexture() = default;
		CPUNormalTexture(CPUTexture&& other) noexcept : CPUTexture(std::move(other)) {};

		TextureType getType() const override { return TextureType::NORMAL; }
	};

	class CPUOcclusionTexture : public CPUTexture {
	public:
		CPUOcclusionTexture() = default;
		CPUOcclusionTexture(CPUTexture&& other) noexcept : CPUTexture(std::move(other)) {};

		TextureType getType() const override { return TextureType::OCCLUSION; }
	};

	class CPUMetallicRoughnessTexture : public CPUTexture {
	public:
		CPUMetallicRoughnessTexture() = default;
		CPUMetallicRoughnessTexture(CPUTexture&& other) noexcept : CPUTexture(std::move(other)) {};

		TextureType getType() const override { return TextureType::METALLIC_ROUGHNESS; }
	};
}
