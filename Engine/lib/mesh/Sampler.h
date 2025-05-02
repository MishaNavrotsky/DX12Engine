#include "stdafx.h"

#pragma once

#include "helpers.h"

namespace Engine {
	using namespace Microsoft::WRL;

	class Sampler: public IID {
	public:
		Sampler() = default;
		Sampler(const Sampler&) = delete;
		Sampler& operator=(const Sampler&) = delete;

		D3D12_SAMPLER_DESC samplerDesc = {};
	};
}
