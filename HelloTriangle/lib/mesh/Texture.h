#include "stdafx.h"

#pragma once

#include "DirectXTex.h"
#include "../DXSampleHelper.h"

namespace Engine {
	using namespace Microsoft::WRL;

	class Texture {
	public:
		Texture() {
			m_scratchImage->Initialize2D(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, 1, 1, 1, 0);
			const DirectX::Image* images = m_scratchImage->GetImages();
			for (size_t i = 0; i < m_scratchImage->GetImageCount(); ++i) {
				memset(images[i].pixels, 0, images[i].slicePitch);
			}
		};
		Texture(const Texture& other) {
			m_scratchImage = other.m_scratchImage;
			isDefaultScratchImage = other.isDefaultScratchImage;
		}
		Texture& operator=(const Texture&) = delete;

		void setScratchImage(std::shared_ptr<DirectX::ScratchImage> bitmapData) {
			isDefaultScratchImage = false;
			m_scratchImage = bitmapData;
			auto& metadata = bitmapData->GetMetadata();
		}
		std::shared_ptr<DirectX::ScratchImage> getScratchImage() {
			return m_scratchImage;
		}
		bool getIsDefaultScratchImage() const {
			return isDefaultScratchImage;
		}
		ComPtr<ID3D12Resource> getTextureResource() const {
			return m_texture;
		}
		void createBuffer(ID3D12Device* device) {
			ThrowIfFailed(DirectX::CreateTextureEx(
				device,
				m_scratchImage->GetMetadata(),
				D3D12_RESOURCE_FLAG_NONE,
				DirectX::CREATETEX_FLAGS::CREATETEX_DEFAULT,
				&m_texture
			));
		}

	private:
		std::shared_ptr<DirectX::ScratchImage> m_scratchImage = std::make_shared<DirectX::ScratchImage>();
		bool isDefaultScratchImage = true;
		ComPtr<ID3D12Resource> m_texture = nullptr;
	};

	class EmissiveTexture : public Texture {
	public:
		EmissiveTexture() = default;
		EmissiveTexture(const Texture& other) : Texture(other) {};
		DirectX::XMFLOAT3 emissiveFactor = { 0.f, 0.f, 0.f };
	};

	class DiffuseTexture : public Texture {
	public:
		DiffuseTexture() = default;
		DiffuseTexture(const Texture& other) : Texture(other) {};
		DirectX::XMFLOAT4 baseColorFactor = { 1.f, 1.f, 1.f, 1.f };
	};

	class NormalTexture : public Texture {
	public:
		NormalTexture() = default;
		NormalTexture(const Texture& other) : Texture(other) {};
		float scale = 1.f;
	};

	class OcclusionTexture : public Texture {
	public:
		OcclusionTexture() = default;
		OcclusionTexture(const Texture& other) : Texture(other) {};
		float strength = 1.f;
	};

	class MetallicRoughnessTexture : public Texture {
	public:
		MetallicRoughnessTexture() = default;
		MetallicRoughnessTexture(const Texture& other) : Texture(other) {};
		float metallicFactor = 1.f;
		float roughnessFactor = 1.f;
	};
}
