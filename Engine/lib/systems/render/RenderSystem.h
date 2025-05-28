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
		void initialize(ECS::EntityManager& em, bool useWarpDevice, HWND hwnd, uint32_t width, uint32_t height) {
			m_width = width;
			m_height = height;
			m_device = Render::Device::Initialize(useWarpDevice);
			Render::Descriptor::BindlessHeapDescriptor::GetInstance().initialize();

			createDirectQueue();
			createComputeQueue();
			createPasses(hwnd);
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
		uint32_t m_rtvDescriptorSize, m_width, m_height;

		HANDLE m_fenceEvent;
		WPtr<ID3D12Fence> m_fence;
		uint64_t m_fenceValue = 0;


		std::unique_ptr<Render::Pipeline::GBufferPass> m_gbufferPass;
		std::unique_ptr<Render::Pipeline::LightingPass> m_lightingPass;
		std::unique_ptr<Render::Pipeline::GizmosPass> m_gizmosPass;
		std::unique_ptr<Render::Pipeline::CompositionPass> m_compositionPass;
		std::unique_ptr<Render::Pipeline::UIPass> m_uiPass;

		WPtr<ID3D12CommandQueue> m_directCommandQueue, m_computeCommandQueue;

		float yaw = 0, pich = 0;
		bool isCursorCaptured = false;
		DX::Keyboard::KeyboardStateTracker trackerKeyboard;
		DX::Mouse::ButtonStateTracker trackerMouse;


		void LoadPipeline() {}
		void LoadAssets() {}
		CommandLists PopulateCommandLists() {}
		void WaitForCommandQueueExecute() {}

		void createDirectQueue() {
			D3D12_COMMAND_QUEUE_DESC directQueueDesc = {};
			directQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
			directQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
			ThrowIfFailed(m_device->CreateCommandQueue(&directQueueDesc, IID_PPV_ARGS(&m_directCommandQueue)));
		}
		void createComputeQueue() {
			D3D12_COMMAND_QUEUE_DESC computeQueueDesc = {};
			computeQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
			computeQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
			ThrowIfFailed(m_device->CreateCommandQueue(&computeQueueDesc, IID_PPV_ARGS(&m_computeCommandQueue)));
		}
		void createPasses(HWND hwnd) {
			using namespace Render::Pipeline;

			m_gbufferPass = std::unique_ptr<GBufferPass>(new GBufferPass(m_device, m_width, m_height));
			m_lightingPass = std::unique_ptr<LightingPass>(new LightingPass(m_device, m_width, m_height));
			m_gizmosPass = std::unique_ptr<GizmosPass>(new GizmosPass(m_device, m_width, m_height));
			m_compositionPass = std::unique_ptr<CompositionPass>(new CompositionPass(m_device, m_width, m_height));
			m_uiPass = std::unique_ptr<UIPass>(new UIPass(hwnd, m_device, m_directCommandQueue, FrameCount, m_width, m_height));
		}
	};
}
