#include "stdafx.h"

#pragma once

#include "DirectXTex.h"
#include <guiddef.h>

namespace Engine {
	using namespace Microsoft::WRL;

	inline size_t CalculateBufferSize(const DirectX::ScratchImage& scratchImage) {
		size_t totalSize = 0;
		const size_t imageCount = scratchImage.GetImageCount();
		const DirectX::Image* images = scratchImage.GetImages();

		for (size_t i = 0; i < imageCount; ++i) {
			const DirectX::Image& img = images[i];
			totalSize += img.rowPitch * img.height;
		}

		return totalSize;
	}

	enum class AlphaMode {
		Opaque,
		Mask,
		Blend
	};

	enum class TextureType {
		DEFAULT,
		BASE_COLOR,
		NORMAL,
		OCCLUSION,
		EMISSIVE,
		METALLIC_ROUGHNESS,
	};

	enum class DefaultTexturesSlot {
		BASE_COLOR = 0,
		EMISSIVE = 1,
		METALLIC_ROUGHNESS = 2,
		NORMAL = 3,
		OCCLUSION = 4,
	};

	enum class DefaultSamplersSlot {
		BASE_COLOR = 0,
		EMISSIVE = 1,
		METALLIC_ROUGHNESS = 2,
		NORMAL = 3,
		OCCLUSION = 4,
	};

	class IID {
	protected:
		GUID m_id = GUID_NULL;

	public:
		IID() = default;

		void setID(const GUID& id) {
			m_id = id;
		}

		GUID getID() const {
			return m_id;
		}

		virtual ~IID() = default;
	};
}
