#include "stdafx.h"

#pragma once

#include "helpers.h"

namespace Engine {
    using namespace Microsoft::WRL;

    class GPUTexture : public IID {
    public:
        GPUTexture(ID3D12Resource* texture, D3D12_CPU_DESCRIPTOR_HANDLE srvHandle,
            D3D12_RESOURCE_DESC resourceDesc, D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc)
            : m_texture(texture), m_srvHandle(srvHandle),
            m_resourceDesc(resourceDesc), m_srvDesc(srvDesc) {
        }

        ID3D12Resource* getResource() const {
            return m_texture.Get();
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
        ComPtr<ID3D12Resource> m_texture;
        D3D12_CPU_DESCRIPTOR_HANDLE m_srvHandle;
        D3D12_RESOURCE_DESC m_resourceDesc;
        D3D12_SHADER_RESOURCE_VIEW_DESC m_srvDesc;
    };
}
