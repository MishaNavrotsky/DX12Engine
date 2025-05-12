#include "stdafx.h"

#pragma once

#include "../Device.h"
#include "../DXSampleHelper.h"
#include "GBufferPipeline.h"
#include "PSOShader.h"

namespace Engine {
	using namespace Microsoft::WRL;
	class LightingPipeline {
	public:
		LightingPipeline(ComPtr<ID3D12Device> device, UINT width, UINT height) : m_device(device), m_width(width), m_height(height) {
			Engine::PSOShaderCreate psoSC;
			psoSC.CS = L"assets\\shaders\\lighting.hlsl";;
			psoSC.CSEntry = L"main";
			m_shaders = PSOShader::Create(psoSC);
			ThrowIfFailed(m_device->CreateRootSignature(0, m_shaders->getCOM()->GetBufferPointer(), m_shaders->getCOM()->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));

			D3D12_COMMAND_QUEUE_DESC queueDesc = {};
			queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
			queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
			ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));
			ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, IID_PPV_ARGS(&m_commandAllocator)));
			ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COMPUTE, m_commandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_commandList)));
			ThrowIfFailed(m_commandList->Close());
			ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
			m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
			if (m_fenceEvent == nullptr)
			{
				ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
			}

			createPso();
			createRWTex();
		}

		void computeLighting(GBufferPipeline* gbufferPipeline, ID3D12Resource* cameraBuffer) {
			createDescriptorHeap(gbufferPipeline->getRtvResources());

			ThrowIfFailed(m_commandAllocator->Reset());
			ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), m_pso.Get()));
			m_commandList->SetPipelineState(m_pso.Get());
			m_commandList->SetComputeRootSignature(m_rootSignature.Get());
			m_commandList->SetComputeRootConstantBufferView(0, cameraBuffer->GetGPUVirtualAddress());

			ID3D12DescriptorHeap* descriptorHeaps[] = { m_descriptorHeap.Get()};
			m_commandList->SetDescriptorHeaps(1, descriptorHeaps);
			m_commandList->SetComputeRootDescriptorTable(1, m_descriptorHeap.Get()->GetGPUDescriptorHandleForHeapStart());

			UINT dispatchX = (m_width + 15) / 16;
			UINT dispatchY = (m_height + 15) / 16;
			m_commandList->Dispatch(dispatchX, dispatchY, 1);
			ThrowIfFailed(m_commandList->Close());

			m_commandQueue->Wait(gbufferPipeline->getFence(), gbufferPipeline->getFenceValue());
			ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
			m_commandQueue->ExecuteCommandLists(1, ppCommandLists);
			ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), ++m_fenceValue));
		}

		ID3D12Fence* getFence() const {
			return m_fence.Get();
		}

		UINT64 getFenceValue() const {
			return m_fenceValue;
		}

		void waitForGPU() {
			if (m_fence->GetCompletedValue() < m_fenceValue) {
				ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValue, m_fenceEvent));
				WaitForSingleObject(m_fenceEvent, INFINITE);
			}
		}

		ID3D12Resource* getOutputTexture() {
			return m_rwTexture.Get();
		}
	private:
		void createPso() {
			D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
			psoDesc.pRootSignature = m_rootSignature.Get();
			psoDesc.CS.BytecodeLength = m_shaders->getCOM()->GetBufferSize();
			psoDesc.CS.pShaderBytecode = m_shaders->getCOM()->GetBufferPointer();
			ThrowIfFailed(m_device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&m_pso)));
		}

		void createRWTex() {
			D3D12_RESOURCE_DESC texDesc = {};
			texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			texDesc.Width = m_width;
			texDesc.Height = m_height;
			texDesc.DepthOrArraySize = 1;
			texDesc.MipLevels = 1;
			texDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
			texDesc.SampleDesc.Count = 1;
			texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

			auto texProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

			ThrowIfFailed(m_device->CreateCommittedResource(
				&texProps,
				D3D12_HEAP_FLAG_NONE,
				&texDesc,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				nullptr,
				IID_PPV_ARGS(&m_rwTexture)));
		}

		void createDescriptorHeap(const std::array<ID3D12Resource*, 7>& rtvs) {
			if (m_descriptorHeap.Get() != nullptr) return;

			D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
			heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			heapDesc.NumDescriptors = 8; // 7 SRVs + 1 UAV
			heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			heapDesc.NodeMask = 0;

			m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_descriptorHeap));

			UINT descriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			for (int i = 0; i < rtvs.size(); ++i)
			{
				D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
				srvDesc.Format = rtvs[i]->GetDesc().Format;
				srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
				srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
				srvDesc.Texture2D.MipLevels = rtvs[i]->GetDesc().MipLevels;

				CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(m_descriptorHeap->GetCPUDescriptorHandleForHeapStart(), i, descriptorSize);
				m_device->CreateShaderResourceView(rtvs[i], &srvDesc, srvHandle);
			}

			D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
			uavDesc.Format = m_rwTexture->GetDesc().Format;
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
			uavDesc.Texture2D.MipSlice = 0;

			CD3DX12_CPU_DESCRIPTOR_HANDLE uavHandle(m_descriptorHeap->GetCPUDescriptorHandleForHeapStart(), 7, descriptorSize);
			m_device->CreateUnorderedAccessView(m_rwTexture.Get(), nullptr, &uavDesc, uavHandle);
		}
		ComPtr<ID3D12Device> m_device;
		ComPtr<ID3D12PipelineState> m_pso;
		ComPtr<ID3D12RootSignature> m_rootSignature;
		ComPtr<ID3D12GraphicsCommandList> m_commandList;
		ComPtr<ID3D12CommandQueue> m_commandQueue;
		ComPtr<ID3D12CommandAllocator> m_commandAllocator;
		std::unique_ptr<PSOShader> m_shaders;

		ComPtr<ID3D12Resource> m_rwTexture;
		ComPtr<ID3D12DescriptorHeap> m_descriptorHeap = nullptr;


		HANDLE m_fenceEvent;
		ComPtr<ID3D12Fence> m_fence;
		UINT64 m_fenceValue = 0;

		UINT m_width;
		UINT m_height;
	};
}
