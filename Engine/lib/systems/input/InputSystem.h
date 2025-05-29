#include "stdafx.h"

#pragma once

#include "../../scene/Scene.h"
#include "../ISystem.h"
#include "Keyboard.h"
#include "Mouse.h"

namespace Engine::System {
	class InputSystem : public ISystem {
	public:
		InputSystem() = default;
		virtual ~InputSystem() = default;
		void initialize(Scene::Scene& scene) override {};
		void update(float dt) override {};
		void shutdown() override {};
		void onMouseUpdate(DX::Mouse::State state) {};
		void onKeyboardUpdate(DX::Keyboard::State state) {};
	};
}

