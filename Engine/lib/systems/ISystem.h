#include "stdafx.h"

#pragma once

#include "../ecs/EntityManager.h"

namespace Engine::System {
	class ISystem {
	public:
		virtual ~ISystem() = default;
		virtual void initialize(ECS::EntityManager& em) {};
		virtual void update(float dt) = 0;
		virtual void shutdown() = 0;
	};
}
