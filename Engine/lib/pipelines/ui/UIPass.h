#include "stdafx.h"

#pragma once

#include "../../../external/imgui.h"
#include "../../../external/imgui_impl_win32.h"
#include "../../../external/imgui_impl_dx12.h"
#include "../../memory/Resource.h"
#include "../../DXSampleHelper.h"


namespace Engine {
	using namespace Microsoft::WRL;
	using namespace DirectX;
	class UIPass {
	public:
		UIPass(HWND hwnd, ComPtr<ID3D12Device> device, ComPtr<ID3D12CommandQueue> directCommandQueue, uint32_t frameCount, uint32_t width, uint32_t height)
			: m_frameCount(frameCount), m_width(width), m_height(height), m_device(device) {
			m_frameCount = frameCount;
			m_width = width;
			m_height = height;
			m_device = device;

			ImGui::CreateContext();
			ImGuiIO& io = ImGui::GetIO();
			io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
			io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

			ImGui::StyleColorsDark();
			ImGui_ImplWin32_Init(hwnd);

			D3D12_DESCRIPTOR_HEAP_DESC desc = {};
			desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			desc.NumDescriptors = frameCount;
			desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			ThrowIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_guidDescriptorHeap)));

			createRtvs();
			createRtvDescriptorHeap();

			ImGui_ImplDX12_InitInfo info = {};
			info.CommandQueue = directCommandQueue.Get();
			info.Device = device.Get();
			info.NumFramesInFlight = frameCount;
			info.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
			info.DSVFormat = DXGI_FORMAT_D32_FLOAT;
			info.SrvDescriptorHeap = m_guidDescriptorHeap.Get();
			info.LegacySingleSrvCpuDescriptor = m_guidDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
			info.LegacySingleSrvGpuDescriptor = m_guidDescriptorHeap->GetGPUDescriptorHandleForHeapStart();

			ImGui_ImplDX12_Init(&info);

			ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)));
			ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocatorBarrier)));

			ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_commandList)));
			ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocatorBarrier.Get(), nullptr, IID_PPV_ARGS(&m_commandAllocatorList)));

			ThrowIfFailed(m_commandList->Close());
			ThrowIfFailed(m_commandAllocatorList->Close());
		}
		std::array<ID3D12CommandList*, 2> renderUI(uint32_t frame) {
			ThrowIfFailed(m_commandAllocator->Reset());
			ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), nullptr));

			{
				CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[frame]->getResource(),
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
					D3D12_RESOURCE_STATE_RENDER_TARGET);
				m_commandList->ResourceBarrier(1, &barrier);
			}

			ImGui_ImplDX12_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();
			{
				ImGui::Begin("Hello, ImGui!");
				ImGui::Text("DirectX 12 + ImGui setup complete!");
				ImGui::Text("DirectX 12 + ImGui setup complete!");
				ImGui::Text("DirectX 12 + ImGui setup complete!");
				ImGui::Text("DirectX 12 + ImGui setup complete!");
				ImGui::Text("DirectX 12 + ImGui setup complete!");
				ImGui::End();
			}
			ImGui::ShowDemoWindow();
			ImGui::Render();

			ID3D12DescriptorHeap* heaps[] = { m_guidDescriptorHeap.Get() };
			m_commandList->SetDescriptorHeaps(_countof(heaps), heaps);


			auto rtvHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), frame, m_rtvDescriptorSize);
			auto clearValue = m_renderTargets[frame]->getClearValue();
			m_commandList->ClearRenderTargetView(rtvHandle, clearValue->Color, 0, nullptr);
			m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

			ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_commandList.Get());

			ThrowIfFailed(m_commandList->Close());

			ThrowIfFailed(m_commandAllocatorBarrier->Reset());
			ThrowIfFailed(m_commandAllocatorList->Reset(m_commandAllocatorBarrier.Get(), nullptr));

			{
				CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[frame]->getResource(),
					D3D12_RESOURCE_STATE_RENDER_TARGET,
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
					);
				m_commandAllocatorList->ResourceBarrier(1, &barrier);
			}

			ThrowIfFailed(m_commandAllocatorList->Close());

			return std::array<ID3D12CommandList*, 2>({ m_commandList.Get(), m_commandAllocatorList.Get() });
		}

		~UIPass()
		{
			ImGui_ImplDX12_Shutdown();
			ImGui_ImplWin32_Shutdown();
			ImGui::DestroyContext();
		}

		Memory::Resource* getRtvResource(uint32_t frame) const {
			return m_renderTargets[frame].get();
		}
	private:
		void createRtvs() {
			D3D12_RESOURCE_DESC desc = {};
			desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			desc.Width = m_width;
			desc.Height = m_height;
			desc.DepthOrArraySize = 1;
			desc.MipLevels = 1;
			desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			desc.SampleDesc.Count = 1;
			desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
			desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

			CD3DX12_HEAP_PROPERTIES props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

			D3D12_CLEAR_VALUE clearValue = {};
			clearValue.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			clearValue.Color[0] = 0.0f;
			clearValue.Color[1] = 0.0f;
			clearValue.Color[2] = 0.0f;
			clearValue.Color[3] = 0.0f;

			for (uint32_t i = 0; i < m_frameCount; ++i) {
				auto resource = Memory::Resource::Create(
					props,
					desc,
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
					&clearValue);
				m_renderTargets.push_back(std::move(resource));
			}
		}

		void createRtvDescriptorHeap() {
			D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
			rtvHeapDesc.NumDescriptors = m_frameCount;
			rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

			ThrowIfFailed(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvDescriptorHeap)));

			m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

			CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

			for (uint32_t i = 0; i < m_frameCount; ++i)
			{
				m_device->CreateRenderTargetView(m_renderTargets[i]->getResource(), nullptr, rtvHandle);
				rtvHandle.Offset(1, m_rtvDescriptorSize);
			}
		}

		ComPtr<ID3D12Device> m_device;
		uint32_t m_width;
		uint32_t m_height;
		uint32_t m_frameCount;
		ComPtr<ID3D12GraphicsCommandList> m_commandList, m_commandAllocatorList;
		ComPtr<ID3D12CommandAllocator> m_commandAllocator, m_commandAllocatorBarrier;
		ComPtr<ID3D12DescriptorHeap> m_guidDescriptorHeap;
		ComPtr<ID3D12DescriptorHeap> m_rtvDescriptorHeap;
		uint32_t m_rtvDescriptorSize = 0;
		std::vector<std::unique_ptr<Memory::Resource>> m_renderTargets;
	};
}