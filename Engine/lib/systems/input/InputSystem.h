#include "stdafx.h"

#pragma once

#include "../../scene/Scene.h"
#include "../ISystem.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "../../MainThreadQueue.h"
#include <queue>

namespace Engine::System {
	class InputSystem : public ISystem {
	public:
		InputSystem() = default;
		virtual ~InputSystem() = default;
		void initialize(Scene::Scene& scene) override {
			m_scene = &scene;
		};
		void update(float dt) override {
			m_kst.Update(m_lastKeyboardState);
			m_bst.Update(m_lastMouseState);

			//debug
			if (m_lockCursor) {
				//HWND hwnd = GetForegroundWindow();
				//RECT rect;
				//GetClientRect(hwnd, &rect);

				//POINT windowPos;
				//ClientToScreen(hwnd, &windowPos);

				//int centerX = windowPos.x + (rect.right - rect.left) / 2;
				//int centerY = windowPos.y + (rect.bottom - rect.top) / 2;

				//SetCursorPos(centerX, centerY);
			}

			if (m_kst.pressed.Escape) {
				MainThreadQueue.push([this] {
					auto& mouse = DX::Mouse::Get();
					if (mouse.IsVisible()) {
						mouse.SetMode(DX::Mouse::MODE_RELATIVE);
						mouse.SetVisible(false);
						m_lockCursor = true;
					}
					else {
						mouse.SetMode(DX::Mouse::MODE_ABSOLUTE);
						mouse.SetVisible(true);
						m_lockCursor = false;
					}
					});

			}

			auto& registry = m_scene->entityManager.getRegistry();
			auto group = registry.group_if_exists<ECS::Component::ComponentCamera>(entt::get<ECS::Component::ComponentTransform>);
			for (const auto& [entity, camera, transform] : group.each()) {
				if (camera.isMain) {
					ECS::Component::ComponentTransform t(transform);

					{
						if (m_lockCursor) {
							float sensitivityX = 0.2f;
							float sensitivityY = 0.2f;
							m_yaw += m_deltaX * sensitivityX;

							m_pitch += m_deltaY * sensitivityY;

							m_pitch = std::clamp(m_pitch, -89.0f, 89.0f);
							float yawRad = DX::XMConvertToRadians(m_yaw);
							float pitchRad = DX::XMConvertToRadians(m_pitch);

							DX::XMVECTOR yawQuat = DX::XMQuaternionRotationAxis(DX::XMVectorSet(0, 1, 0, 0), yawRad);
							DX::XMVECTOR pitchQuat = DX::XMQuaternionRotationAxis(DX::XMVectorSet(1, 0, 0, 0), pitchRad);

							auto m_rotationQuat = DX::XMQuaternionMultiply(pitchQuat, yawQuat);

							m_rotationQuat = DX::XMQuaternionNormalize(m_rotationQuat);

							DX::XMStoreFloat4(&t.rotation, m_rotationQuat);
						}
					}
					{
						DX::XMVECTOR forward = DX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
						DX::XMVECTOR lookVector = DX::XMVector3Rotate(forward, DX::XMLoadFloat4(&t.rotation));
						DX::XMVECTOR worldUp = DX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
						DX::XMVECTOR rightVector = DX::XMVector3Normalize(DX::XMVector3Cross(worldUp, lookVector));
						DX::XMVECTOR upVector = DX::XMVector3Normalize(DX::XMVector3Cross(lookVector, rightVector));

						DX::XMVECTOR pos = XMLoadFloat4(&t.position);

						float speed = 1.f * dt;
						if (m_kst.lastState.LeftShift) speed *= 4;
						if (m_kst.lastState.W) {
							pos = DX::XMVectorAdd(pos, DX::XMVectorScale(lookVector, speed));
						}

						if (m_kst.lastState.S) {
							pos = DX::XMVectorSubtract(pos, DX::XMVectorScale(lookVector, speed));
						}

						if (m_kst.lastState.A) {
							pos = DX::XMVectorSubtract(pos, DX::XMVectorScale(rightVector, speed));
						}

						if (m_kst.lastState.D) {
							pos = DX::XMVectorAdd(pos, DX::XMVectorScale(rightVector, speed));
						}

						if (m_kst.lastState.LeftControl) {
							pos = DX::XMVectorSubtract(pos, DX::XMVectorScale(upVector, speed));
						}

						if (m_kst.lastState.Space) {
							pos = DX::XMVectorAdd(pos, DX::XMVectorScale(upVector, speed));
						}

						XMStoreFloat4(&t.position, pos);
					}
					registry.emplace_or_replace<ECS::Component::ComponentTransform>(entity, t);
					break;
				}
			}
			m_deltaX = 0;
			m_deltaY = 0;
		};
		void shutdown() override {};
		void onMouseUpdate(DX::Mouse::State state) {
			m_lastMouseState = state;
			m_deltaX += state.x;
			m_deltaY += state.y;
		};
		void onKeyboardUpdate(DX::Keyboard::State state) {
			m_lastKeyboardState = state;
		};
	private:
		Scene::Scene* m_scene;

		DX::Keyboard::KeyboardStateTracker m_kst;
		DX::Mouse::ButtonStateTracker m_bst;
		DX::Keyboard::State m_lastKeyboardState;
		DX::Mouse::State m_lastMouseState;

		int m_deltaX = 0;
		int m_deltaY = 0;

		//debug
		bool m_lockCursor = false;
		float m_yaw = 0.0f;    // Rotation around the Y-axis (left/right)
		float m_pitch = 0.0f;  // Rotation around the X-axis (up/down)
	};

}

