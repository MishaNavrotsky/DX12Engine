#include "stdafx.h"

#pragma once

#include "../../ecs/EntityManager.h"
#include "../ISystem.h"
#include "Keyboard.h"
#include "Mouse.h"

namespace Engine::System {
	class InputSystem : public ISystem {
	public:
		InputSystem() = default;
		virtual ~InputSystem() = default;
		void initialize(ECS::EntityManager& em) override {};
		void update(float dt) override {};
		void shutdown() override {};
		void onMouseUpdate(DX::Mouse::State state) {};
		void onKeyboardUpdate(DX::Keyboard::State state) {};
	};
}

