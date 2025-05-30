#include "stdafx.h"

#pragma once

#include "ComponentWrapper.h"
#include "../../scene/assets/AssetStructures.h"


namespace Engine::ECS::Component {
	struct ComponentMaterialInstance {
		Scene::Asset::MaterialInstanceId assetId;
	};

	using WComponentMaterialInstance = ComponentWrapper<ComponentMaterialInstance>;
}
