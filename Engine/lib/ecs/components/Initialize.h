#include "stdafx.h"

#pragma once

#include "ComponentRegistry.h"
#include "ComponentCamera.h"
#include "ComponentTransform.h"
#include "ComponentMesh.h"

namespace Engine::ECS::Component {
	inline static void Initialize() {
		ComponentRegistry::RegisterComponent<WComponentCamera>();
		ComponentRegistry::RegisterComponent<WComponentTransform>();
		ComponentRegistry::RegisterComponent<WComponentMesh>();
	}
}
