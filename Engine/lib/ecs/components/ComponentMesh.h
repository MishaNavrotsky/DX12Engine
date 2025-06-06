#include "stdafx.h"

#pragma once

#include "../../scene/assets/AssetStructures.h"


namespace Engine::ECS::Component {
	struct ComponentMesh {
		Scene::Asset::MeshId assetId;
	};
}
