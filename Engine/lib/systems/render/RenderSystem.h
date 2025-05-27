#include "stdafx.h"

#pragma once

#include "Keyboard.h"
#include "Mouse.h"

#include "Device.h"
#include "descriptors/BindlessHeapDescriptor.h"
#include "../../ecs/EntityManager.h"
#include "../ISystem.h"
#include "pipelines/CompositionPass.h"
#include "pipelines/gizmos/GizmosPass.h"
#include "pipelines/GBufferPass.h"
#include "pipelines/LightingPass.h"
#include "pipelines/ui/UIPass.h"

namespace Engine::System {
	class RenderSystem : public ISystem {
		struct CommandLists {
			std::array<ID3D12CommandList*, 2> d_ui;
			std::array<ID3D12CommandList*, 2> d_gbuffer;
			std::array<ID3D12CommandList*, 2> d_gizmos;
			std::array<ID3D12CommandList*, 1> c_lighting;
			std::array<ID3D12CommandList*, 2> d_composition;
		};
	public:
		void initialize(ECS::EntityManager& em, bool useWarpDevice) {
			Render::Device::Initialize(useWarpDevice);
			Render::Descriptor::BindlessHeapDescriptor::GetInstance().initialize();
		}
		void update(float dt) override {}
		void shutdown() override {}
	private:
		inline static const UINT FrameCount = 2;

		// Pipeline objects.
		WPtr<IDXGISwapChain3> m_swapChain;
		WPtr<ID3D12Device> m_device;
		WPtr<ID3D12Resource> m_renderTargets[FrameCount];
		WPtr<ID3D12CommandAllocator> m_commandAllocator;
		WPtr<ID3D12DescriptorHeap> m_rtvHeap;
		WPtr<ID3D12GraphicsCommandList> m_commandList;
		UINT m_rtvDescriptorSize;

		HANDLE m_fenceEvent;
		WPtr<ID3D12Fence> m_fence;
		uint64_t m_fenceValue = 0;


		std::unique_ptr<Render::Pipeline::GBufferPass> m_gbufferPass;
		std::unique_ptr<Render::Pipeline::LightingPass> m_lightingPass;
		std::unique_ptr<Render::Pipeline::GizmosPass> m_gizmosPass;
		std::unique_ptr<Render::Pipeline::CompositionPass> m_compositionPass;
		std::unique_ptr<Render::Pipeline::UIPass> m_uiPass;



		WPtr<ID3D12CommandQueue> m_directCommandQueue;
		WPtr<ID3D12CommandQueue> m_computeCommandQueue;


		float yaw = 0;
		float pitch = 0;
		bool isCursorCaptured = false;
		DX::Keyboard::KeyboardStateTracker trackerKeyboard;
		DX::Mouse::ButtonStateTracker trackerMouse;


		void LoadPipeline() {}
		void LoadAssets() {}
		CommandLists PopulateCommandLists() {}
		void WaitForCommandQueueExecute() {}
	};
}
