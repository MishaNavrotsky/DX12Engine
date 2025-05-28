#include "stdafx.h"

#pragma once

#include "ComponentWrapper.h"


namespace Engine::ECS::Component {
	struct ComponentMesh {
		GUID assetId;
	};

	using WComponentMesh = ComponentWrapper<ComponentMesh>;
}
