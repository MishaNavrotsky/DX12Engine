#include "stdafx.h"

#pragma once

#include "Texture.h"
#include "Sampler.h"
#include "helpers.h"

namespace Engine {
	using namespace Microsoft::WRL;

	class MeshTextures {
	public:
		MeshTextures() = default;
		MeshTextures(const MeshTextures&) = delete;
		MeshTextures& operator=(const MeshTextures&) = delete;

		void setDiffuseTexture(std::shared_ptr<DiffuseTexture> texture) {
			m_diffuseTexture = texture;
		}
		void setNormalTexture(std::shared_ptr<NormalTexture> texture) {
			m_normalTexture = texture;
		}
		void setOcclusionTexture(std::shared_ptr<OcclusionTexture> texture) {
			m_occlusionTexture = texture;
		}
		void setEmissiveTexture(std::shared_ptr<EmissiveTexture> texture) {
			m_emissiveTexture = texture;
		}
		void setMetallicRoughnessTexture(std::shared_ptr<MetallicRoughnessTexture> texture) {
			m_metallicRoughnessTexture = texture;
		}
		void setDiffuseSampler(std::shared_ptr<Sampler> sampler) {
			m_diffuseSampler = sampler;
		}
		void setNormalSampler(std::shared_ptr<Sampler> sampler) {
			m_normalSampler = sampler;
		}
		void setOcclusionSampler(std::shared_ptr<Sampler> sampler) {
			m_occlusionSampler = sampler;
		}
		void setEmissiveSampler(std::shared_ptr<Sampler> sampler) {
			m_emissiveSampler = sampler;
		}
		void setMetallicRoughnessSampler(std::shared_ptr<Sampler> sampler) {
			m_metallicRoughnessSampler = sampler;
		}

		void createBuffers(ID3D12Device* device, ID3D12GraphicsCommandList* commandList) {
			uploadDataToUploadBufferAndInitializeTextureResources(device, commandList);
		}

		void prepareBuffers(ID3D12GraphicsCommandList* commandList) {
			if (isBuffersPrepared) return;
			const size_t textureCount = 5;
			ID3D12Resource* textureResources[textureCount] = { m_diffuseTexture->getTextureResource().Get(), m_normalTexture->getTextureResource().Get(), m_occlusionTexture->getTextureResource().Get(), m_emissiveTexture->getTextureResource().Get(), m_metallicRoughnessTexture->getTextureResource().Get() };
			D3D12_RESOURCE_BARRIER barriers[textureCount];

			for (uint32_t i = 0; i < textureCount; i++) {
				barriers[i] = CD3DX12_RESOURCE_BARRIER::Transition(
					textureResources[i],
					D3D12_RESOURCE_STATE_COMMON,
					D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
				);
			}
			commandList->ResourceBarrier(textureCount, barriers);
			isBuffersPrepared = true;
		}
	private:
		void uploadDataToUploadBufferAndInitializeTextureResources(ID3D12Device* device, ID3D12GraphicsCommandList* commandList) {
			const size_t textureCount = 5;
			std::shared_ptr<Texture> textures[textureCount] = { m_diffuseTexture, m_normalTexture, m_occlusionTexture, m_emissiveTexture, m_metallicRoughnessTexture };
			auto diffuseTextureSize = CalculateBufferSize(*m_diffuseTexture->getScratchImage());
			auto normalTextureSize = CalculateBufferSize(*m_normalTexture->getScratchImage());
			auto occlusionTextureSize = CalculateBufferSize(*m_occlusionTexture->getScratchImage());
			auto emissiveTextureSize = CalculateBufferSize(*m_emissiveTexture->getScratchImage());
			auto metallicRoughnessTextureSize = CalculateBufferSize(*m_metallicRoughnessTexture->getScratchImage());
			uint64_t bufferSize = diffuseTextureSize + normalTextureSize + occlusionTextureSize + emissiveTextureSize + metallicRoughnessTextureSize;


			auto heapUploadProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
			auto heapUploadDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

			ThrowIfFailed(device->CreateCommittedResource(
				&heapUploadProps,
				D3D12_HEAP_FLAG_NONE,
				&heapUploadDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&m_srvUploadHeap)));


			uint64_t offset = 0;
			for (auto& tex : textures) {
				tex->createBuffer(device);

				auto scratchImage = tex->getScratchImage();
				auto textureResource = tex->getTextureResource();
				std::vector<D3D12_SUBRESOURCE_DATA> subresources;

				ThrowIfFailed(DirectX::PrepareUpload(device, scratchImage->GetImages(), scratchImage->GetImageCount(), scratchImage->GetMetadata(), subresources));

				const UINT64 uploadBufferSize = GetRequiredIntermediateSize(textureResource.Get(),
					0, static_cast<UINT>(subresources.size()));


				UpdateSubresources(commandList,
					textureResource.Get(), m_srvUploadHeap.Get(),
					offset, 0, static_cast<UINT>(subresources.size()),
					subresources.data());
				offset += uploadBufferSize;
			}
		}
		std::shared_ptr<DiffuseTexture> m_diffuseTexture = std::make_shared<DiffuseTexture>();
		std::shared_ptr<NormalTexture> m_normalTexture = std::make_shared<NormalTexture>();
		std::shared_ptr<OcclusionTexture> m_occlusionTexture = std::make_shared<OcclusionTexture>();
		std::shared_ptr<EmissiveTexture> m_emissiveTexture = std::make_shared<EmissiveTexture>();
		std::shared_ptr<MetallicRoughnessTexture> m_metallicRoughnessTexture = std::make_shared<MetallicRoughnessTexture>();

		std::shared_ptr<Sampler> m_diffuseSampler = std::make_shared<Sampler>();
		std::shared_ptr<Sampler> m_normalSampler = std::make_shared<Sampler>();
		std::shared_ptr<Sampler> m_occlusionSampler = std::make_shared<Sampler>();
		std::shared_ptr<Sampler> m_emissiveSampler = std::make_shared<Sampler>();
		std::shared_ptr<Sampler> m_metallicRoughnessSampler = std::make_shared<Sampler>();

		ComPtr<ID3D12Resource> m_srvUploadHeap = nullptr;

		bool isBuffersPrepared = false;
	};
}