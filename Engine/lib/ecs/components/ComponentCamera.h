#include "stdafx.h"

#pragma once

#include "ComponentWrapper.h"

namespace Engine::ECS::Component {
	struct ComponentCamera {
		float fov, nearPlane, farPlane, aspectRatio;
	};

	using WComponentCamera = ComponentWrapper<ComponentCamera>;
}