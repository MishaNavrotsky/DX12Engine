#include "stdafx.h"

#pragma once

#include "helpers.h"

namespace Engine {
	using namespace Microsoft::WRL;

	class GPUMaterial : public IID {
	public:
		GPUMaterial() = default;

		ComPtr<ID3D12Resource>& getCBVResource() {
			return m_cbv;
		}
	private:

		ComPtr<ID3D12Resource> m_cbv;
	};
}