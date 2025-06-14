#include "stdafx.h"

#pragma once

#include "../../../scene/assets/AssetStructures.h"
#include "../../../structures.h"

namespace Engine::Render::Manager {
	//D3D12_INPUT_ELEMENT_DESC m_inputElementDescs[4] =
	//{
	//	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	//	{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	//	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	//	{ "TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 3, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	//};
	struct RenderableSubMesh {
		D3D12_VERTEX_BUFFER_VIEW position;
		D3D12_VERTEX_BUFFER_VIEW normal;
		D3D12_VERTEX_BUFFER_VIEW texcoord;
		D3D12_VERTEX_BUFFER_VIEW tangent;
		D3D12_INDEX_BUFFER_VIEW index;
		uint64_t indexCount;
		Structures::AABB aabb;
	};
	struct RenderableMesh {
		Scene::Asset::MeshId meshId;
		std::vector<RenderableSubMesh> subMeshes;
	};
}