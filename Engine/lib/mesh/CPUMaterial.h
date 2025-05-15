#include "stdafx.h"

#pragma once

#include "helpers.h"
#include "../managers/helpers.h"

namespace Engine {
	using namespace Microsoft::WRL;

	struct CPUMaterialCBVData {
		XMFLOAT4 emissiveFactor = { 0.f, 0.f, 0.f, 1.f };
		XMFLOAT4 baseColorFactor = { 1.f, 1.f, 1.f, 1.f };
		XMFLOAT4 normalScaleOcclusionStrengthMRFactors = { 1.f, 1.f, 1.f, 1.f };
		XMUINT4 diffuseEmissiveNormalOcclusionTexSlots = { 
			static_cast<uint32_t>(DefaultTexturesSlot::BASE_COLOR),
			static_cast<uint32_t>(DefaultTexturesSlot::EMISSIVE),
			static_cast<uint32_t>(DefaultTexturesSlot::NORMAL),
			static_cast<uint32_t>(DefaultTexturesSlot::OCCLUSION)
		};
		XMUINT4 MrTexSlots = { static_cast<uint32_t>(DefaultTexturesSlot::METALLIC_ROUGHNESS), 0, 0, 0 };

		XMUINT4 diffuseEmissiveNormalOcclusionSamSlots = {
			static_cast<uint32_t>(DefaultSamplersSlot::BASE_COLOR),
			static_cast<uint32_t>(DefaultSamplersSlot::EMISSIVE),
			static_cast<uint32_t>(DefaultSamplersSlot::NORMAL),
			static_cast<uint32_t>(DefaultSamplersSlot::OCCLUSION)
		};
		XMUINT4 MrSamSlots = { static_cast<uint32_t>(DefaultSamplersSlot::METALLIC_ROUGHNESS), 0, 0, 0 };
	};

	class CPUMaterial : public IID {
	public:
		CPUMaterial() {};

		std::vector<GUID>& getTextureIds() {
			return m_textureIds;
		}
		void setTextureIds(std::vector<GUID>&& texturesIds) noexcept {
			m_textureIds = std::move(texturesIds);
		}

		std::vector<GUID>& getSamplerIds() {
			return m_samplerIds;
		}
		void setSamplerIds(std::vector<GUID>&& samplerIds) noexcept {
			m_samplerIds = std::move(samplerIds);
		}

		CPUMaterialCBVData& getCBVData() {
			return m_cbvData;
		}
		
		void setCBVDataBindlessHeapSlot(int32_t v) {
			m_cbvDataBindlessHeapSlot = v;
		}

		uint32_t getCBVDataBindlessHeapSlot() const {
			return m_cbvDataBindlessHeapSlot;
		}

		AlphaMode alphaMode = AlphaMode::Opaque;
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
		std::vector<GUID> m_textureIds;
		std::vector<GUID> m_samplerIds;
		CPUMaterialCBVData m_cbvData;

		uint32_t m_cbvDataBindlessHeapSlot;
	};
}