#include "stdafx.h"

#pragma once

#include "../scene/Scene.h"

namespace Engine::System {
	class ISystem {
	public:
		virtual ~ISystem() = default;
		virtual void initialize(Scene::Scene& scene) {};
		virtual void update(float dt) = 0;
		virtual void shutdown() = 0;
	};
}
