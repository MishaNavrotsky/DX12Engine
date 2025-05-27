#include "stdafx.h"

#pragma once

#include "ComponentRegistry.h"
#include "ComponentTest.h"

namespace Engine::Component {
	inline static void Initialize() {
		ComponentRegistry::RegisterComponent<Engine::Component::ComponentTestA>();
		ComponentRegistry::RegisterComponent<Engine::Component::ComponentTestB>();
		ComponentRegistry::RegisterComponent<Engine::Component::ComponentTestC>();
	}
}
