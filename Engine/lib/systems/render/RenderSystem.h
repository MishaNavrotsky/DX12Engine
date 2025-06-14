#include "stdafx.h"

#pragma once

#include "Keyboard.h"
#include "Mouse.h"

#include "../../global.h"
#include "Device.h"
#include "descriptors/BindlessHeapDescriptor.h"
#include "memory/Resource.h"
#include "../../scene/Scene.h"
#include "../ISystem.h"
#include "pipelines/CompositionPass.h"
#include "pipelines/gizmos/GizmosPass.h"
#include "pipelines/GBufferPass.h"
#include "pipelines/LightingPass.h"
#include "pipelines/ui/UIPass.h"
#include "queus/ComputeQueue.h"
#include "queus/DirectQueue.h"
#include "../../ecs/classes/ClassCamera.h"
#include "managers/CameraManager.h"
#include "managers/TransformMatrixManager.h"
#include "managers/GlobalsManager.h"



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
		void initialize(Scene::Scene& scene, bool useWarpDevice, HWND hwnd, uint32_t width, uint32_t height) {
			m_width = width;
			m_height = height;
			m_scene = &scene;
			auto factory = createFactory();
			m_device = Render::Device::Initialize(factory.Get(), useWarpDevice);
			m_cameraManager.initialize(m_scene);
			m_directCommandQueue.initialize(m_device);
			m_computeCommandQueue.initialize(m_device);
			m_bindlessHeap.initialize(m_device);
			m_transfromMatrixManager.initialize(m_scene, &m_bindlessHeap);
			m_globalsManager.initialize(m_scene);

			createCommandLists();
			createFence();
			createPasses(hwnd);
			createSwapChain(factory.Get(), hwnd);
		}
		void update(float dt) {
 			auto* dCommandQueue = m_directCommandQueue.getQueue();
			auto* cCommandQueue = m_computeCommandQueue.getQueue();
			Render::Manager::GlobalsManager::Globals globals{};
			globals.screenX = m_width;
			globals.screenY = m_height;
			globals.transformsIndex = m_transfromMatrixManager.getBindlessSlot();

			auto* cameraResource = m_cameraManager.update();
			auto* transformResource = m_transfromMatrixManager.update();
			auto* globalsResource = m_globalsManager.update(globals);

			auto commandList = populateCommandLists(cameraResource, transformResource, globalsResource);

			dCommandQueue->ExecuteCommandLists(std::size(commandList.d_gbuffer), commandList.d_gbuffer.data());
			ThrowIfFailed(dCommandQueue->Signal(m_fence.Get(), ++m_fenceValue));

			ThrowIfFailed(cCommandQueue->Wait(m_fence.Get(), m_fenceValue));
			cCommandQueue->ExecuteCommandLists(std::size(commandList.c_lighting), commandList.c_lighting.data());
			ThrowIfFailed(cCommandQueue->Signal(m_fence.Get(), ++m_fenceValue));

			ThrowIfFailed(dCommandQueue->Wait(m_fence.Get(), m_fenceValue));
			dCommandQueue->ExecuteCommandLists(std::size(commandList.d_composition), commandList.d_composition.data());


			ThrowIfFailed(m_commandAllocator->Reset());
			ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), nullptr));

			auto compositionRtv = m_compositionPass->getRtvResource();
			auto swapChainBuffer = m_renderTargets[m_swapChain->GetCurrentBackBufferIndex()].Get();

			{
				CD3DX12_RESOURCE_BARRIER uavBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
					compositionRtv->getResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);
				CD3DX12_RESOURCE_BARRIER swapChainBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
					swapChainBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_DEST);
				D3D12_RESOURCE_BARRIER barriers[] = { uavBarrier,  swapChainBarrier };
				m_commandList->ResourceBarrier(2, barriers);
			}
			m_commandList->CopyResource(swapChainBuffer, compositionRtv->getResource());
			{
				CD3DX12_RESOURCE_BARRIER uavBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
					compositionRtv->getResource(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
				CD3DX12_RESOURCE_BARRIER swapChainBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
					swapChainBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT);
				D3D12_RESOURCE_BARRIER barriers[] = { uavBarrier,  swapChainBarrier };
				m_commandList->ResourceBarrier(2, barriers);
			}

			ThrowIfFailed(m_commandList->Close());
			ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
			dCommandQueue->ExecuteCommandLists(1, ppCommandLists);

			waitForCommandQueueExecute();
			ThrowIfFailed(m_swapChain->Present(0, 0));

			//Global::CurrentFrame += 1;
		}
		void shutdown() override {
			waitForCommandQueueExecute();
		}

		Render::Queue::ComputeQueue& getComputeQueue() {
			return m_computeCommandQueue;
		}
		Render::Queue::DirectQueue& getDirectQueue() {
			return m_directCommandQueue;
		}
	private:
		inline static const UINT FrameCount = 2;
		Scene::Scene* m_scene;
		Render::Manager::CameraManager m_cameraManager;
		Render::Manager::TransformMatrixManager m_transfromMatrixManager;
		Render::Manager::GlobalsManager m_globalsManager;
		// Pipeline objects.
		WPtr<IDXGISwapChain3> m_swapChain;
		ID3D12Device* m_device;
		WPtr<ID3D12Resource> m_renderTargets[FrameCount];
		WPtr<ID3D12CommandAllocator> m_commandAllocator;
		WPtr<ID3D12DescriptorHeap> m_rtvHeap;
		WPtr<ID3D12GraphicsCommandList> m_commandList;
		uint32_t m_rtvDescriptorSize, m_width, m_height;

		HANDLE m_fenceEvent;
		WPtr<ID3D12Fence> m_fence;
		uint64_t m_fenceValue = 0;

		Render::Descriptor::BindlessHeapDescriptor m_bindlessHeap;

		std::unique_ptr<Render::Pipeline::GBufferPass> m_gbufferPass;
		std::unique_ptr<Render::Pipeline::LightingPass> m_lightingPass;
		std::unique_ptr<Render::Pipeline::GizmosPass> m_gizmosPass;
		std::unique_ptr<Render::Pipeline::CompositionPass> m_compositionPass;
		std::unique_ptr<Render::Pipeline::UIPass> m_uiPass;

		Render::Queue::ComputeQueue m_computeCommandQueue;
		Render::Queue::DirectQueue m_directCommandQueue;


		CommandLists populateCommandLists(Render::Memory::Resource* cameraResource, Render::Memory::Resource* transformResource, Render::Memory::Resource* globalsResource) {
			auto frame = m_swapChain->GetCurrentBackBufferIndex();

			CommandLists cmd{};
			cmd.d_ui = m_uiPass->renderUI(frame);
			cmd.d_gbuffer = m_gbufferPass->renderGBuffers(cameraResource, globalsResource);
			cmd.c_lighting = m_lightingPass->computeLighting(m_gbufferPass.get(), cameraResource, globalsResource);
			cmd.d_gizmos = m_gizmosPass->renderGizmos(m_gbufferPass->getDepthStencilResource(), cameraResource);
			cmd.d_composition = m_compositionPass->renderComposition(m_lightingPass->getOutputTexture(), m_gizmosPass->getRtvResource(), m_uiPass->getRtvResource(frame), globalsResource);

			return cmd;
		}
		void waitForCommandQueueExecute() {
			ThrowIfFailed(m_directCommandQueue.getQueue()->Signal(m_fence.Get(), ++m_fenceValue));

			if (m_fence->GetCompletedValue() < m_fenceValue)
			{
				ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValue, m_fenceEvent));
				WaitForSingleObject(m_fenceEvent, INFINITE);
			}
		}

		WPtr<IDXGIFactory4> createFactory() {
			UINT dxgiFactoryFlags = 0;

#if defined(_DEBUG)
			{
				WPtr<ID3D12Debug> debugController;
				if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
				{
					debugController->EnableDebugLayer();

					dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
				}
			}
#endif

			WPtr<IDXGIFactory4> factory;
			ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));

			return factory;
		}

		void createPasses(HWND hwnd) {
			using namespace Render::Pipeline;

			m_gbufferPass = std::unique_ptr<GBufferPass>(new GBufferPass(m_device, m_width, m_height, &m_bindlessHeap, m_scene, &m_cameraManager, &m_transfromMatrixManager));
			m_lightingPass = std::unique_ptr<LightingPass>(new LightingPass(m_device, m_width, m_height));
			m_gizmosPass = std::unique_ptr<GizmosPass>(new GizmosPass(m_device, m_width, m_height, &m_bindlessHeap));
			m_compositionPass = std::unique_ptr<CompositionPass>(new CompositionPass(m_device, m_directCommandQueue.getQueue(), m_width, m_height));
			m_uiPass = std::unique_ptr<UIPass>(new UIPass(hwnd, m_device, m_directCommandQueue.getQueue(), FrameCount, m_width, m_height));
		}
		void createSwapChain(IDXGIFactory4* factory, HWND hwnd) {
			DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
			swapChainDesc.BufferCount = FrameCount;
			swapChainDesc.Width = m_width;
			swapChainDesc.Height = m_height;
			swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
			swapChainDesc.SampleDesc.Count = 1;

			WPtr<IDXGISwapChain1> swapChain;
			ThrowIfFailed(factory->CreateSwapChainForHwnd(
				m_directCommandQueue.getQueue(), hwnd,
				&swapChainDesc,
				nullptr,
				nullptr,
				&swapChain
			));

			ThrowIfFailed(factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER));

			ThrowIfFailed(swapChain.As(&m_swapChain));
			{
				D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
				rtvHeapDesc.NumDescriptors = FrameCount;
				rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
				rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
				ThrowIfFailed(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));

				m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
			}

			{
				CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());
				for (UINT n = 0; n < FrameCount; n++)
				{
					ThrowIfFailed(m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n])));


					m_device->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, rtvHandle);
					rtvHandle.Offset(1, m_rtvDescriptorSize);
				}
			}
		}
		void createCommandLists() {
			ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)));
			ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_commandList)));
			ThrowIfFailed(m_commandList->Close());
		}
		void createFence() {
			ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
			m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
			if (m_fenceEvent == nullptr)
			{
				ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
			}
		}
	};
}
