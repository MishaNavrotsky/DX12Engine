#include "stdafx.h"

#pragma once

#include "DirectXTex.h"

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

	enum AlphaMode {
		Opaque,
		Mask,
		Blend
	};
}
