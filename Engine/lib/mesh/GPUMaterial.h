#include "stdafx.h"

#pragma once

#include "../helpers.h"
#include "../structures.h"

namespace Engine {
	class GPUMaterial : public Structures::IID {
	public:
		GPUMaterial() = default;

		WPtr<ID3D12Resource>& getCBVResource() {
			return m_cbv;
		}
	private:

		WPtr<ID3D12Resource> m_cbv;
	};
}