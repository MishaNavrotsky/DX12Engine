#include "stdafx.h"

#pragma once

namespace Engine::ECS::Component {
	template<typename T>
	struct ComponentWrapper {
		T component;
		bool isDirty;
	};
}
