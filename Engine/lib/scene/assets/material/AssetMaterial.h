#include "stdafx.h"

#pragma once

#include "../../../systems/render/pipelines/PSOShader.h"


namespace Engine::Scene::Asset {
	enum class AssetTypeMaterial {
		PBR,
		Custom,
	};

	struct AssetTypePBRMaterialData {
		DX::XMFLOAT4 emissiveFactor = { 0.f, 0.f, 0.f, 1.f };
		DX::XMFLOAT4 baseColorFactor = { 1.f, 1.f, 1.f, 1.f };
		DX::XMFLOAT4 normalScaleOcclusionStrengthMRFactors = { 1.f, 1.f, 0.f, 1.f };
	};
	struct AssetTypePBRMaterialInstanceData {
		std::optional<DX::XMFLOAT4> emissiveFactor;
		std::optional<DX::XMFLOAT4> baseColorFactor;
		std::optional<DX::XMFLOAT4> normalScaleOcclusionStrengthMRFactors;
	};

	struct CustomMaterialData {
		// 
	};
	struct CustomMaterialInstanceData {
		// 
	};

	using AssetMaterialData = std::variant<AssetTypePBRMaterialData, CustomMaterialData>;
	using AssetMaterialInstanceData = std::variant<AssetTypePBRMaterialInstanceData, CustomMaterialInstanceData>;


	struct AssetMaterial {
		AssetTypeMaterial type;
		std::unique_ptr<Render::Pipeline::PSOShader> shader;

		AssetMaterialData data;
	};

	struct AssetMaterialInstance {
		AssetMaterialInstanceData data;
	};
}
