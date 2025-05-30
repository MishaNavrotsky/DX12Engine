#include "stdafx.h"

#pragma once

#include "ComponentWrapper.h"
#include "../../scene/assets/AssetStructures.h"


namespace Engine::ECS::Component {
	struct ComponentMesh {
		Scene::Asset::MeshId assetId;
	};

	using WComponentMesh = ComponentWrapper<ComponentMesh>;
}
