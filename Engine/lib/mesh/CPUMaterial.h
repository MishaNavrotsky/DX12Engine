#include "stdafx.h"

#pragma once

#include "../helpers.h"
#include "../structures.h"

namespace Engine {
	struct CPUMaterialCBVData {
		DX::XMFLOAT4 emissiveFactor = { 0.f, 0.f, 0.f, 1.f };
		DX::XMFLOAT4 baseColorFactor = { 1.f, 1.f, 1.f, 1.f };
		DX::XMFLOAT4 normalScaleOcclusionStrengthMRFactors = { 1.f, 1.f, 1.f, 1.f };
		DX::XMUINT4 diffuseEmissiveNormalOcclusionTexSlots = {
			static_cast<uint32_t>(Structures::DefaultTexturesSlot::BASE_COLOR),
			static_cast<uint32_t>(Structures::DefaultTexturesSlot::EMISSIVE),
			static_cast<uint32_t>(Structures::DefaultTexturesSlot::NORMAL),
			static_cast<uint32_t>(Structures::DefaultTexturesSlot::OCCLUSION)
		};
		DX::XMUINT4 MrTexSlots = { static_cast<uint32_t>(Structures::DefaultTexturesSlot::METALLIC_ROUGHNESS), 0, 0, 0 };

		DX::XMUINT4 diffuseEmissiveNormalOcclusionSamSlots = {
			static_cast<uint32_t>(Structures::DefaultSamplersSlot::BASE_COLOR),
			static_cast<uint32_t>(Structures::DefaultSamplersSlot::EMISSIVE),
			static_cast<uint32_t>(Structures::DefaultSamplersSlot::NORMAL),
			static_cast<uint32_t>(Structures::DefaultSamplersSlot::OCCLUSION)
		};
		DX::XMUINT4 MrSamSlots = { static_cast<uint32_t>(Structures::DefaultSamplersSlot::METALLIC_ROUGHNESS), 0, 0, 0 };
	};

	class CPUMaterial : public IID {
	public:
		CPUMaterial() {};

		CPUMaterialCBVData& getCBVData() {
			return m_cbvData;
		}
		
		void setCBVDataBindlessHeapSlot(int32_t v) {
			m_cbvDataBindlessHeapSlot = v;
		}

		uint32_t getCBVDataBindlessHeapSlot() const {
			return m_cbvDataBindlessHeapSlot;
		}

		Structures::AlphaMode alphaMode = Structures::AlphaMode::Opaque;
		D3D12_CULL_MODE cullMode = D3D12_CULL_MODE_BACK;
		float alphaCutoff = 0.5f;

		static void PrintCBVData(Engine::CPUMaterial& data)
		{
			auto& cbvData = data.getCBVData();
			std::cout << "emissiveFactor: ("
				<< cbvData.emissiveFactor.x << ", "
				<< cbvData.emissiveFactor.y << ", "
				<< cbvData.emissiveFactor.z << ")\n";

			std::cout << "baseColorFactor: ("
				<< cbvData.baseColorFactor.x << ", "
				<< cbvData.baseColorFactor.y << ", "
				<< cbvData.baseColorFactor.z << ", "
				<< cbvData.baseColorFactor.w << ")\n";

			std::cout << "normalScaleOcclusionStrengthMRFactors: ("
				<< cbvData.normalScaleOcclusionStrengthMRFactors.x << ", "
				<< cbvData.normalScaleOcclusionStrengthMRFactors.y << ", "
				<< cbvData.normalScaleOcclusionStrengthMRFactors.z << ", "
				<< cbvData.normalScaleOcclusionStrengthMRFactors.w << ")\n";

			std::cout << "diffuseEmissiveNormalOcclusionTexSlots: ("
				<< cbvData.diffuseEmissiveNormalOcclusionTexSlots.x << ", "
				<< cbvData.diffuseEmissiveNormalOcclusionTexSlots.y << ", "
				<< cbvData.diffuseEmissiveNormalOcclusionTexSlots.z << ", "
				<< cbvData.diffuseEmissiveNormalOcclusionTexSlots.w << ")\n";

			std::cout << "MrTexSlots: ("
				<< cbvData.MrTexSlots.x << ", "
				<< cbvData.MrTexSlots.y << ", "
				<< cbvData.MrTexSlots.z << ", "
				<< cbvData.MrTexSlots.w << ")\n";

			std::cout << "diffuseEmissiveNormalOcclusionSamSlots: ("
				<< cbvData.diffuseEmissiveNormalOcclusionSamSlots.x << ", "
				<< cbvData.diffuseEmissiveNormalOcclusionSamSlots.y << ", "
				<< cbvData.diffuseEmissiveNormalOcclusionSamSlots.z << ", "
				<< cbvData.diffuseEmissiveNormalOcclusionSamSlots.w << ")\n";

			std::cout << "MrSamSlots: ("
				<< cbvData.MrSamSlots.x << ", "
				<< cbvData.MrSamSlots.y << ", "
				<< cbvData.MrSamSlots.z << ", "
				<< cbvData.MrSamSlots.w << ")\n";

			std::cout << "m_cbvDataBindlessHeapSlot: " << data.getCBVDataBindlessHeapSlot() << '\n';
		}

		const std::unordered_set<GUID, GUIDHash, GUIDEqual>& getCPUTextureIds() const {
			return m_cpuTextureIds;
		}
		void setCPUTextureIds(std::unordered_set<GUID, GUIDHash, GUIDEqual>&& textureIds) noexcept {
			m_cpuTextureIds = std::move(textureIds);
		}
		void addCPUTextureId(GUID textureId) noexcept {
			m_cpuTextureIds.insert(textureId);
		}
		void removeCPUTextureId(GUID textureId) noexcept {
			m_cpuTextureIds.erase(textureId);
		}
		void clearCPUTextureIds() noexcept {
			m_cpuTextureIds.clear();
		}

		std::unordered_set<GUID, GUIDHash, GUIDEqual>& getCPUMeshIds() {
			return m_cpuMeshesIds;
		}
		void setCPUMeshIds(std::unordered_set<GUID, GUIDHash, GUIDEqual>&& cpuMeshIds) noexcept {
			m_cpuMeshesIds = std::move(cpuMeshIds);
		}
		void addCPUMeshId(GUID cpuMeshId) noexcept {
			m_cpuMeshesIds.insert(cpuMeshId);
		}
		void removeCPUMeshId(GUID cpuMeshId) noexcept {
			m_cpuMeshesIds.erase(cpuMeshId);
		}
		void clearCPUMeshIds() noexcept {
			m_cpuMeshesIds.clear();
		}
	private:
		std::unordered_set<GUID, GUIDHash, GUIDEqual> m_cpuMeshesIds;
		std::unordered_set<GUID, GUIDHash, GUIDEqual> m_cpuTextureIds;
		CPUMaterialCBVData m_cbvData;

		uint32_t m_cbvDataBindlessHeapSlot;
	};
}