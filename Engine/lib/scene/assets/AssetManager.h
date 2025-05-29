#include "stdafx.h"

#pragma once

#include "../../ecs/components/ComponentMesh.h"
#include "AssetStructures.h"



namespace Engine::Scene {
	class AssetManager {
	public:
		AssetManager() {
			m_meshAssetMap.reserve(2ULL << 10);
			m_materialAssetMap.reserve(2ULL << 4);
			m_materialInstanceAssetMap.reserve(2ULL << 10);
		}
	private:
		ankerl::unordered_dense::map<Asset::MeshAssetId, Asset::MeshAssetMapValue> m_meshAssetMap;
		ankerl::unordered_dense::map<Asset::MaterialAssetId, Asset::MaterialAssetMapValue> m_materialAssetMap;
		ankerl::unordered_dense::map<Asset::MaterialInstanceAssetId, Asset::MaterialInstanceAssetMapValue> m_materialInstanceAssetMap;
	};
}
