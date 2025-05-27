#include "stdafx.h"

#pragma once

#include "../entity/Entity.h"

namespace Engine::ECS::Component {
	class ComponentTestA {
	public:
		float x, y, z;
	};
	class ComponentTestB {
	public:
		float x, y;
	};
	class ComponentTestC {
	public:
		float x;
	};
}
