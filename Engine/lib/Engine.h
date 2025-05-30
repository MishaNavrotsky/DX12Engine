#include "stdafx.h"

#pragma once

#include "ecs/components/Initialize.h"

#include "systems/render/RenderSystem.h"
#include "systems/input/InputSystem.h"
#include "systems/stream/MetadataLoaderSystem.h"
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

			m_streamSystem.initialize(m_scene);
			m_inputSystem.initialize(m_scene);
			m_renderSystem.initialize(m_scene, m_useWarpDevice, hwnd, m_width, m_height);

			//debug
			m_scene.assetManager.registerMesh(Scene::Asset::UsageMesh::Static, Scene::Asset::SourceMesh::File, Scene::Asset::FileSourceMesh{ std::filesystem::path("D:\\DX12En\\AssetsCreator\\assets\\alicev2rigged_0.mesh.asset") });
		}
		void update(float dt) {
			m_streamSystem.update(dt);
			m_inputSystem.update(dt);
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
		System::MetadataLoaderSystem m_streamSystem;
		//static const UINT FrameCount = 2;

		//struct Vertex
		//{
		//    XMFLOAT3 position;
		//};

		//// Pipeline objects.
		//ComPtr<IDXGISwapChain3> m_swapChain;
		//ComPtr<ID3D12Device> m_device;
		//ComPtr<ID3D12Resource> m_renderTargets[FrameCount];
		//ComPtr<ID3D12CommandAllocator> m_commandAllocator;
		//ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
		//ComPtr<ID3D12GraphicsCommandList> m_commandList;
		//UINT m_rtvDescriptorSize;

		//HANDLE m_fenceEvent;
		//ComPtr<ID3D12Fence> m_fence;
		//uint64_t m_fenceValue = 0;

		//std::unique_ptr <Main::Camera> m_camera;
		//Main::Scene m_scene;
		//Main::BindlessHeapDescriptor& m_bindlessHeapDescriptor = Main::BindlessHeapDescriptor::GetInstance();



		//std::unique_ptr<Main::GBufferPass> m_gbufferPass;
		//std::unique_ptr<Main::LightingPass> m_lightingPass;
		//std::unique_ptr<Main::GizmosPass> m_gizmosPass;
		//std::unique_ptr<Main::CompositionPass> m_compositionPass;
		//std::unique_ptr<Main::UIPass> m_uiPass;



		//ComPtr<ID3D12CommandQueue> m_directCommandQueue;
		//ComPtr<ID3D12CommandQueue> m_computeCommandQueue;


		//float yaw = 0;
		//float pitch = 0;
		//bool isCursorCaptured = false;
		//DX::Keyboard::KeyboardStateTracker trackerKeyboard;
		//DX::Mouse::ButtonStateTracker trackerMouse;


		//void LoadPipeline();
		//void LoadAssets();
		//CommandLists PopulateCommandLists();
		//void WaitForCommandQueueExecute();
	};
}