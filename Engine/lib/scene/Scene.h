#include "stdafx.h"
#pragma once

#include "../ecs/EntityManager.h"

namespace Engine::Scene {
	struct Scene {
		ECS::EntityManager entityManager;
	};
}
