#include "stdafx.h"

#pragma once

namespace Engine {
	using namespace Microsoft::WRL;

	class Sampler {
	public:
		Sampler() = default;
		Sampler(const Sampler&) = delete;
		Sampler& operator=(const Sampler&) = delete;

		D3D12_SAMPLER_DESC samplerDesc = {};
		ComPtr<ID3D12Resource> getSamplerResource() {
			return m_sampler;
		}
	private:
		ComPtr<ID3D12Resource> m_sampler = nullptr;
	};
}

