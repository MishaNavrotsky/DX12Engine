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

		D3D12_CPU_DESCRIPTOR_HANDLE& getSRVHandle() {
			return m_srvHandle;
		}

		D3D12_RESOURCE_DESC& getResourceDesc() {
			return m_resourceDesc;
		}

		D3D12_SHADER_RESOURCE_VIEW_DESC& getSRVDesc() {
			return m_srvDesc;
		}

	private:
		ComPtr<ID3D12Resource> m_texture = nullptr;
		D3D12_CPU_DESCRIPTOR_HANDLE m_srvHandle;
		D3D12_RESOURCE_DESC m_resourceDesc;
		D3D12_SHADER_RESOURCE_VIEW_DESC m_srvDesc;
	};
}
