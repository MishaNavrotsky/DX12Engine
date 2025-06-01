#include "stdafx.h"

#pragma once

#include "ecs/components/Initialize.h"

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
		Engine(UINT width, UINT height, std::wstring name): m_width(width), m_height(height), m_title(name) {};


		void initialize(HWND hwnd) {
			ECS::Component::Initialize();

			m_inputSystem.initialize(m_scene);
			m_renderSystem.initialize(m_scene, m_useWarpDevice, hwnd, m_width, m_height);
			m_streamSystem.initialize(m_scene);
			//debug
			m_scene.assetManager.registerMesh(Scene::Asset::UsageMesh::Static, Scene::Asset::SourceMesh::File, Scene::Asset::FileSourceMesh{ std::filesystem::path("D:\\DX12En\\AssetsCreator\\assets\\alicev2rigged_0.mesh.asset") });
		}
		void update(float dt) {
			m_inputSystem.update(dt);
			m_renderSystem.update(dt);
			m_streamSystem.update(dt);

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
	};
}