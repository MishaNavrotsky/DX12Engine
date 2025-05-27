#include "stdafx.h"

#pragma once

#include "ComponentRegistry.h"
#include "ComponentTest.h"

namespace Engine::ECS::Component {
	inline static void Initialize() {
		ComponentRegistry::RegisterComponent<ComponentTestA>();
		ComponentRegistry::RegisterComponent<ComponentTestB>();
		ComponentRegistry::RegisterComponent<ComponentTestC>();
	}
}
