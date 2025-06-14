#include "stdafx.h"

#pragma once

#include "systems/render/RenderSystem.h"
#include "systems/input/InputSystem.h"
#include "systems/stream/StreamingSystem.h"

#include "scene/Scene.h"

#include "Keyboard.h"
#include "Mouse.h"


namespace Engine {
	class Engine
	{
	public:
		Engine(UINT width, UINT height, std::wstring name) : m_width(width), m_height(height), m_title(name) {
			m_taskScheduler.Init();
			m_taskScheduler.SetEmptyQueueBehavior(ftl::EmptyQueueBehavior::Sleep);
		};

		void initialize(HWND hwnd) {
			m_inputSystem.initialize(m_scene);
			m_renderSystem.initialize(m_scene, m_useWarpDevice, hwnd, m_width, m_height);
			m_streamSystem.initialize(m_scene, m_renderSystem.getDirectQueue(), &m_taskScheduler);
			m_scene.initialize(m_renderSystem.getDirectQueue(), m_renderSystem.getComputeQueue());
			//test
			auto camera = m_scene.entityManager.createEntity();
			auto componentCamera = ECS::Component::ComponentCamera{};
			componentCamera.aspectRatio = static_cast<float>(m_width) / m_height;
			componentCamera.farPlane = 1000000.f;
			componentCamera.nearPlane = 0.1f;
			componentCamera.fov = 90.f;

			componentCamera.isMain = true;

			auto componentTransform = ECS::Component::ComponentTransform{};
			componentTransform.position = { 0, 2000, 0, 1 };
			componentTransform.scale = { 1, 1, 1, 1 };
			DX::XMStoreFloat4(&componentTransform.rotation, DX::XMQuaternionRotationRollPitchYaw(0, 0, 0));

			m_scene.entityManager.addComponent(camera, componentCamera, componentTransform, ECS::Component::ComponentTransformDirty{});

			m_taskScheduler.AddTask({ .Function = Engine::Test, .ArgData = this, }, ftl::TaskPriority::Normal);
		}
		static void Test(ftl::TaskScheduler* ts, void* args) {
			auto* self = reinterpret_cast<Engine*>(args);

			float i = 0;
			while (true) {

				i += 1000;
				auto meshId = self->m_scene.assetManager.registerMesh(Scene::Asset::UsageMesh::Static, Scene::Asset::SourceMesh::File, Scene::Asset::FileSourceMesh{ std::filesystem::path("D:\\DX12En\\AssetsCreator\\assets\\alicev2rigged_0.mesh.asset") });

				auto mesh = self->m_scene.entityManager.createEntity();
				auto meshTransform = ECS::Component::ComponentTransform{};
				meshTransform.position = { -6000 + i, 0, 2000, 1 };
				meshTransform.scale = { 0.1f, 0.1f, 0.1f, 0.1f };
				meshTransform.rotation = { 0, 0, 0, 1 };
				auto meshMesh = ECS::Component::ComponentMesh{};
				meshMesh.assetId = meshId;

				self->m_scene.entityManager.addComponent(mesh, meshTransform, meshMesh, ECS::Component::ComponentTransformDirty{});
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
			}
		}
		void update(float dt) {
			m_scene.entityManager.processCommands();
			m_inputSystem.update(dt);
			m_streamSystem.update(dt);
			m_renderSystem.update(dt);
		}

		void destroy() {
			m_streamSystem.shutdown();
			m_inputSystem.shutdown();
			m_renderSystem.shutdown();
		}
		void onMouseUpdate(DX::Mouse::State state) {
			m_inputSystem.onMouseUpdate(state);

		}
		void onKeyboardUpdate(DX::Keyboard::State state) {
			m_inputSystem.onKeyboardUpdate(state);
		}
		void parseCommandLineArgs(WCHAR* argv[], int argc)
		{
			for (int i = 1; i < argc; ++i)
			{
				if (_wcsnicmp(argv[i], L"-warp", wcslen(argv[i])) == 0 ||
					_wcsnicmp(argv[i], L"/warp", wcslen(argv[i])) == 0)
				{
					m_useWarpDevice = true;
					m_title = m_title + L" (WARP)";
				}
			}
		}

		inline UINT getWidth() const { return m_width; }
		inline UINT getHeight() const { return m_height; }
		inline const std::wstring& getTitle() const { return m_title; }
	private:
		UINT m_width, m_height;
		std::wstring m_title;
		bool m_useWarpDevice = false;

		Scene::Scene m_scene;

		System::InputSystem m_inputSystem;
		System::RenderSystem m_renderSystem;
		System::StreamingSystem m_streamSystem;
		ftl::TaskScheduler m_taskScheduler;
	};
}