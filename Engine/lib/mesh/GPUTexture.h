#include "stdafx.h"

#pragma once

#include "helpers.h"

namespace Engine {
	using namespace Microsoft::WRL;

	class GPUTexture : public IID {
	public:
		GPUTexture() = default;

		ComPtr<ID3D12Resource>& getResource() {
			return m_texture;
		}

	private:
		ComPtr<ID3D12Resource> m_texture = nullptr;
	};
}
