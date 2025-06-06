#include "stdafx.h"

#pragma once

namespace Engine::ECS::Component {
	struct ComponentCamera {
		float fov, nearPlane, farPlane, aspectRatio;
		bool isMain;
	};
}