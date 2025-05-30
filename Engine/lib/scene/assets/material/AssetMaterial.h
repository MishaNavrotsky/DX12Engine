#include "stdafx.h"

#pragma once

#include "../../../systems/render/pipelines/PSOShader.h"


namespace Engine::Scene::Asset {
	enum class TypeMaterial {
		PBR,
		Custom,
	};

	struct TypePBRMaterialData {
		DX::XMFLOAT4 emissiveFactor = { 0.f, 0.f, 0.f, 1.f };
		DX::XMFLOAT4 baseColorFactor = { 1.f, 1.f, 1.f, 1.f };
		DX::XMFLOAT4 normalScaleOcclusionStrengthMRFactors = { 1.f, 1.f, 0.f, 1.f };
	};
	struct TypePBRMaterialInstanceData {
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

	using MaterialData = std::variant<TypePBRMaterialData, CustomMaterialData>;
	using MaterialInstanceData = std::variant<TypePBRMaterialInstanceData, CustomMaterialInstanceData>;


	struct Material {
		TypeMaterial type;
		std::unique_ptr<Render::Pipeline::PSOShader> shader;

		MaterialData data;
	};

	struct MaterialInstance {
		MaterialInstanceData data;
	};
}
