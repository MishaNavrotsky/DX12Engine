#include "stdafx.h"

#pragma once

#include "../../scene/assets/AssetStructures.h"


namespace Engine::ECS::Component {
	struct ComponentMaterialInstance {
		Scene::Asset::MaterialInstanceId assetId;
	};
}
