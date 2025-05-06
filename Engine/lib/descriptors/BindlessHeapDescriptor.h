#include "stdafx.h"

#pragma once

#include "../DXSampleHelper.h"
#include "../mesh/helpers.h"
#include <numeric>

const uint32_t N_SRV_DESCRIPTORS = 10000;
const uint32_t N_CBV_DESCRIPTORS = 10000;
const uint32_t N_SAMPLERS = 2048;

inline D3D12_SAMPLER_DESC GetSamplerDescForTexture(Engine::TextureType textureType)
{
	D3D12_SAMPLER_DESC samplerDesc = {};

	// Common properties
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;  // Wrap mode for texture tiling
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	samplerDesc.MinLOD = 0.0f;
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
	samplerDesc.MipLODBias = 0.0f;

	// Customize sampler properties based on texture type
	switch (textureType)
	{
	case Engine::TextureType::BASE_COLOR:
		// Linear filtering for base color, no anisotropy
		samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.MaxAnisotropy = 1;  // No anisotropic filtering for base color
		break;

	case Engine::TextureType::NORMAL:
		// Anisotropic filtering for normal maps (improves quality at glancing angles)
		samplerDesc.Filter = D3D12_FILTER_ANISOTROPIC;
		samplerDesc.MaxAnisotropy = 16; // 16x anisotropic filtering for normal maps
		break;

	case Engine::TextureType::METALLIC_ROUGHNESS:
		// Linear filtering for metallic-roughness map, no anisotropy
		samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.MaxAnisotropy = 1;
		break;

	case Engine::TextureType::EMISSIVE:
		// Linear filtering for emissive map, no anisotropy
		samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.MaxAnisotropy = 1;
		break;

	case Engine::TextureType::OCCLUSION:
		// Linear filtering for occlusion map, no anisotropy
		samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.MaxAnisotropy = 1;
		break;

	default:
		// Default to linear filtering and no anisotropy
		samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.MaxAnisotropy = 1;
		break;
	}

	return samplerDesc;
}


namespace Engine {
	using Microsoft::WRL::ComPtr;
	using namespace DirectX;

	struct SamplerHash {
		std::size_t operator()(const D3D12_SAMPLER_DESC& desc) const {
			return std::hash<std::string_view>()(
				std::string_view(reinterpret_cast<const char*>(&desc), sizeof(D3D12_SAMPLER_DESC))
				);
		}
	};

	struct SamplerEqual {
		bool operator()(const D3D12_SAMPLER_DESC& lhs, const D3D12_SAMPLER_DESC& rhs) const {
			return std::memcmp(&lhs, &rhs, sizeof(D3D12_SAMPLER_DESC)) == 0;
		}
	};

	class BindlessHeapDescriptor {
	public:
		BindlessHeapDescriptor() {
			std::iota(m_freeSrvSlots.rbegin(), m_freeSrvSlots.rend(), 0);
			std::iota(m_freeCbvSlots.rbegin(), m_freeCbvSlots.rend(), N_SRV_DESCRIPTORS);

			std::iota(m_freeSamplerSlots.rbegin(), m_freeSamplerSlots.rend(), 0);
		}

		static BindlessHeapDescriptor& getInstance() {
			static BindlessHeapDescriptor instance;
			return instance;
		}

		void registerDevice(ComPtr<ID3D12Device> device) {
			m_device = device;

			D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
			srvHeapDesc.NumDescriptors = N_SRV_DESCRIPTORS + N_CBV_DESCRIPTORS;
			srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			ThrowIfFailed(m_device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_srvDescriptorHeap)));

			D3D12_DESCRIPTOR_HEAP_DESC samplerHeapDesc = {};
			samplerHeapDesc.NumDescriptors = N_SAMPLERS;
			samplerHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
			samplerHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			ThrowIfFailed(m_device->CreateDescriptorHeap(&samplerHeapDesc, IID_PPV_ARGS(&m_samplerDescriptorHeap)));

			auto defaultPBRTextures = CreateDefaultPBRTextures(device.Get());
			this->addTexture(defaultPBRTextures.baseColor);
			this->addTexture(defaultPBRTextures.emissive);
			this->addTexture(defaultPBRTextures.metallicRoughness);
			this->addTexture(defaultPBRTextures.normal);
			this->addTexture(defaultPBRTextures.occlusion);

			this->addSampler(GetSamplerDescForTexture(TextureType::BASE_COLOR));
			this->addSampler(GetSamplerDescForTexture(TextureType::EMISSIVE));
			this->addSampler(GetSamplerDescForTexture(TextureType::METALLIC_ROUGHNESS));
			this->addSampler(GetSamplerDescForTexture(TextureType::NORMAL));
			this->addSampler(GetSamplerDescForTexture(TextureType::OCCLUSION));
		}

		uint32_t addTexture(ComPtr<ID3D12Resource> texture) {
			std::lock_guard lock(m_texture);

			if (m_freeSrvSlots.empty()) {
				throw std::runtime_error("No available SRV slots in descriptor heap.");
			}

			uint32_t slot = m_freeSrvSlots.back();
			m_freeSrvSlots.pop_back();

			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Format = texture->GetDesc().Format;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MipLevels = texture->GetDesc().MipLevels;
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;


			D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = m_srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
			srvHandle.ptr += static_cast<uint64_t>(slot) * m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

			m_device->CreateShaderResourceView(texture.Get(), &srvDesc, srvHandle);
			return slot;
		}

		uint32_t addSampler(const D3D12_SAMPLER_DESC& samplerDesc) {
			std::lock_guard lock(m_sampler);

			auto it = m_samplerMap.find(samplerDesc);
			if (it != m_samplerMap.end()) {
				return it->second; // Return existing slot
			}

			if (m_freeSamplerSlots.empty()) {
				throw std::runtime_error("No available sampler slots in descriptor heap.");
			}

			uint32_t slot = m_freeSamplerSlots.back();
			m_freeSamplerSlots.pop_back();

			D3D12_CPU_DESCRIPTOR_HANDLE samplerHandle = m_samplerDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
			samplerHandle.ptr += static_cast<uint64_t>(slot) * m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

			m_device->CreateSampler(&samplerDesc, samplerHandle);

			m_samplerMap[samplerDesc] = slot;

			return slot;
		}

		uint32_t addCBV(ComPtr<ID3D12Resource> constantBuffer, UINT size) {
			std::lock_guard lock(m_cbv);

			if (m_freeCbvSlots.empty()) {
				throw std::runtime_error("No available CBV slots in descriptor heap.");
			}

			uint32_t slot = m_freeCbvSlots.back();
			m_freeCbvSlots.pop_back();

			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
			cbvDesc.BufferLocation = constantBuffer->GetGPUVirtualAddress();
			cbvDesc.SizeInBytes = (size + 255) & ~255;  // Align to 256 bytes

			D3D12_CPU_DESCRIPTOR_HANDLE cbvHandle = m_srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
			cbvHandle.ptr += static_cast<uint64_t>(slot) * m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

			m_device->CreateConstantBufferView(&cbvDesc, cbvHandle);

			return slot;
		}

		void removeTexture(uint32_t slot) {
			std::lock_guard lock(m_texture);

			if (slot < N_SRV_DESCRIPTORS) {
				D3D12_SHADER_RESOURCE_VIEW_DESC nullDesc = {};
				nullDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
				nullDesc.ViewDimension = D3D12_SRV_DIMENSION_UNKNOWN; // Set to an invalid type

				D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = m_srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
				srvHandle.ptr += static_cast<uint64_t>(slot) * m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

				m_device->CreateShaderResourceView(nullptr, &nullDesc, srvHandle); // Nullify descriptor

				m_freeSrvSlots.push_back(slot); // Mark slot as free
			}
		}

		void removeCBV(uint32_t slot) {
			std::lock_guard lock(m_cbv);

			if (slot >= N_SRV_DESCRIPTORS && slot < (N_SRV_DESCRIPTORS + N_CBV_DESCRIPTORS)) {
				D3D12_CONSTANT_BUFFER_VIEW_DESC nullDesc = {};
				nullDesc.BufferLocation = 0;
				nullDesc.SizeInBytes = 0;

				D3D12_CPU_DESCRIPTOR_HANDLE cbvHandle = m_srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
				cbvHandle.ptr += static_cast<uint64_t>(slot) * m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

				m_device->CreateConstantBufferView(&nullDesc, cbvHandle); // Nullify descriptor

				m_freeCbvSlots.push_back(slot); // Mark slot as free
			}
		}

		ID3D12DescriptorHeap* getSrvDescriptorHeap() const {
			return m_srvDescriptorHeap.Get();
		}

		ID3D12DescriptorHeap* getSamplerDescriptorHeap() const {
			return m_samplerDescriptorHeap.Get();
		}

	private:
		ComPtr<ID3D12Device> m_device;
		ComPtr<ID3D12DescriptorHeap> m_srvDescriptorHeap;
		ComPtr<ID3D12DescriptorHeap> m_samplerDescriptorHeap;

		std::vector<uint32_t> m_freeCbvSlots = std::vector<uint32_t>(N_CBV_DESCRIPTORS);
		std::vector<uint32_t> m_freeSrvSlots = std::vector<uint32_t>(N_SRV_DESCRIPTORS);
		std::vector<uint32_t> m_freeSamplerSlots = std::vector<uint32_t>(N_SAMPLERS);

		std::unordered_map<D3D12_SAMPLER_DESC, uint32_t, SamplerHash, SamplerEqual> m_samplerMap;

		std::mutex m_cbv;
		std::mutex m_sampler;
		std::mutex m_texture;
	};
}