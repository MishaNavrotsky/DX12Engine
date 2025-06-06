#include "stdafx.h"

#pragma once

#include "ComponentRegistry.h"
#include "ComponentCamera.h"
#include "ComponentTransform.h"
#include "ComponentMesh.h"
#include "ComponentMaterialInstance.h"

namespace Engine::ECS::Component {
	inline static void Initialize() {
		ComponentRegistry::RegisterComponent<ComponentCamera>();
		ComponentRegistry::RegisterComponent<ComponentTransform>();
		ComponentRegistry::RegisterComponent<ComponentMesh>();
		ComponentRegistry::RegisterComponent<ComponentMaterialInstance>();
	}
}
