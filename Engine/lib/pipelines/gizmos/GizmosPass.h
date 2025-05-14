#include "stdafx.h"

#pragma once

#include "../../Device.h"
#include "../../DXSampleHelper.h"
#include "../PSOShader.h"
#include "../../camera/Camera.h"
#include "../../mesh/CPUMesh.h"
#include "../../mesh/GPUMesh.h"
#include "../../scene/SceneNode.h"
#include "../../scene/Scene.h"

namespace Engine {
	using namespace Microsoft::WRL;
	using namespace DirectX;
	struct EnumKey {
		D3D12_CULL_MODE cullMode;
		D3D12_PRIMITIVE_TOPOLOGY_TYPE topology;

		bool operator==(const EnumKey& other) const {
			return cullMode == other.cullMode && topology == other.topology;
		}
	};

	struct EnumKeyHash {
		std::size_t operator()(const EnumKey& key) const {
			return std::hash<int>()(static_cast<int>(key.cullMode)) ^
				(std::hash<int>()(static_cast<int>(key.topology)) << 1);
		}
	};
	class GizmosPass {
	public:
		GizmosPass(ComPtr<ID3D12Device> device, UINT width, UINT height) : m_device(device), m_width(width), m_height(height) {
			Engine::PSOShaderCreate psoSC;
			psoSC.PS = L"assets\\shaders\\gizmos.hlsl";
			psoSC.VS = L"assets\\shaders\\gizmos.hlsl";
			psoSC.PSEntry = L"PSMain";
			psoSC.VSEntry = L"VSMain";
			m_shaders = PSOShader::Create(psoSC);

			setRtvClearValue();
			createRtvCommitedResources();
			createRtvsDescriptorHeap();
			createDsv();
			createRootSignature();

			D3D12_COMMAND_QUEUE_DESC queueDesc = {};
			queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
			queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
			ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));
			ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)));
			ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocatorBarrier)));

			ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_commandList)));
			ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocatorBarrier.Get(), nullptr, IID_PPV_ARGS(&m_commandListBarrier)));
			ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
			m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
			if (m_fenceEvent == nullptr)
			{
				ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
			}
			ThrowIfFailed(m_commandList->Close());
			ThrowIfFailed(m_commandListBarrier->Close());


			m_viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));
			m_scissorRect = CD3DX12_RECT(0, 0, static_cast<LONG>(width), static_cast<LONG>(height));
		}
		void renderGizmos(Scene* scene, Camera* camera, ID3D12Resource* srcDepthBuffer) {
			ThrowIfFailed(m_commandAllocator->Reset());
			ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), nullptr));
			{
				CD3DX12_RESOURCE_BARRIER barrierBack = CD3DX12_RESOURCE_BARRIER::Transition(m_rtvResource.Get(),
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
					D3D12_RESOURCE_STATE_RENDER_TARGET);

				m_commandList->ResourceBarrier(1, &barrierBack);
			}
			cloneDepthBuffer(srcDepthBuffer);

			ThrowIfFailed(m_commandList->Close());


			ThrowIfFailed(m_commandAllocatorBarrier->Reset());
			ThrowIfFailed(m_commandListBarrier->Reset(m_commandAllocatorBarrier.Get(), nullptr));
			{
				CD3DX12_RESOURCE_BARRIER barrierBack = CD3DX12_RESOURCE_BARRIER::Transition(m_rtvResource.Get(),
					D3D12_RESOURCE_STATE_RENDER_TARGET,
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

				m_commandListBarrier->ResourceBarrier(1, &barrierBack);
			}

			ThrowIfFailed(m_commandListBarrier->Close());


			ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
			m_commandQueue->ExecuteCommandLists(1, ppCommandLists);
			ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), ++m_fenceValue));
			ThrowIfFailed(m_commandQueue->Wait(m_fence.Get(), m_fenceValue));

			ID3D12CommandList* ppCommandListsBarrier[] = { m_commandListBarrier.Get() };
			m_commandQueue->ExecuteCommandLists(1, ppCommandListsBarrier);
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

		ID3D12Resource* getRtvResource() const {
			return m_rtvResource.Get();
		}

		ID3D12Resource* getDepthStencilResource() const {
			return m_depthStencilBuffer.Get();
		}
	private:
		void cloneDepthBuffer(ID3D12Resource* srcDepthBuffer) {
			auto b = CD3DX12_RESOURCE_BARRIER::Transition(
				srcDepthBuffer, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_COPY_SOURCE);
			m_commandList->ResourceBarrier(1, &b);

			auto b1 = CD3DX12_RESOURCE_BARRIER::Transition(
				m_depthStencilBuffer.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_COPY_DEST);
			m_commandList->ResourceBarrier(1, &b1);

			// Copy entire depth buffer
			m_commandList->CopyResource(m_depthStencilBuffer.Get(), srcDepthBuffer);

			// Restore original states
			auto b2 = CD3DX12_RESOURCE_BARRIER::Transition(
				srcDepthBuffer, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);
			m_commandList->ResourceBarrier(1, &b2);

			auto b3 = CD3DX12_RESOURCE_BARRIER::Transition(
				m_depthStencilBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_DEPTH_WRITE);
			m_commandList->ResourceBarrier(1, &b3);
		}

		void setRtvClearValue() {
			m_rtvClearValue.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			m_rtvClearValue.Color[0] = 0.0f;
			m_rtvClearValue.Color[1] = 0.0f;
			m_rtvClearValue.Color[2] = 0.0f;
			m_rtvClearValue.Color[3] = 1.0f;
		}
		void createRtvCommitedResources() {
			D3D12_HEAP_PROPERTIES rtvProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
			D3D12_RESOURCE_DESC  rtvDesc = {};
			rtvDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			rtvDesc.Width = m_width;
			rtvDesc.Height = m_height;
			rtvDesc.DepthOrArraySize = 1;
			rtvDesc.MipLevels = 1;
			rtvDesc.SampleDesc.Count = 1;
			rtvDesc.SampleDesc.Quality = 0;
			rtvDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
			rtvDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
			rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

			ThrowIfFailed(m_device->CreateCommittedResource(
				&rtvProp,
				D3D12_HEAP_FLAG_NONE,
				&rtvDesc,
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
				&m_rtvClearValue,
				IID_PPV_ARGS(&m_rtvResource)));
		}
		void createRtvsDescriptorHeap() {
			D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};

			rtvHeapDesc.NumDescriptors = 1;
			rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			ThrowIfFailed(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));

			m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
			D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
			m_device->CreateRenderTargetView(m_rtvResource.Get(), nullptr, rtvHandle);
		}
		void createDsv() {
			D3D12_CLEAR_VALUE clearValue = {};
			clearValue.Format = DXGI_FORMAT_D32_FLOAT; // Match the depth buffer format
			clearValue.DepthStencil.Depth = 0.0f; // Default depth clear value
			clearValue.DepthStencil.Stencil = 0; // Default stencil clear value


			D3D12_RESOURCE_DESC depthBufferDesc = {};
			depthBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			depthBufferDesc.Width = m_width;
			depthBufferDesc.Height = m_height;
			depthBufferDesc.DepthOrArraySize = 1;
			depthBufferDesc.MipLevels = 1;
			depthBufferDesc.Format = DXGI_FORMAT_D32_FLOAT;
			depthBufferDesc.SampleDesc.Count = 1;
			depthBufferDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

			D3D12_HEAP_PROPERTIES heapProps = {};
			heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

			ThrowIfFailed(m_device->CreateCommittedResource(
				&heapProps,
				D3D12_HEAP_FLAG_NONE,
				&depthBufferDesc,
				D3D12_RESOURCE_STATE_DEPTH_WRITE,
				&clearValue,
				IID_PPV_ARGS(&m_depthStencilBuffer)
			));

			D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
			dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
			dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
			dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

			// Create a depth stencil view
			D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
			dsvHeapDesc.NumDescriptors = 1;
			dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
			dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			dsvHeapDesc.NodeMask = 0;

			ThrowIfFailed(m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap)));

			m_device->CreateDepthStencilView(
				m_depthStencilBuffer.Get(),
				&dsvDesc,
				m_dsvHeap->GetCPUDescriptorHandleForHeapStart()
			);
		}

		ComPtr<ID3D12PipelineState> createPso(const EnumKey key) {
			CD3DX12_DEPTH_STENCIL_DESC depthStencilDesc = {};
			depthStencilDesc.DepthEnable = TRUE;
			depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
			depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_GREATER;
			depthStencilDesc.StencilEnable = FALSE;

			D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
			psoDesc.InputLayout = { m_inputElementDescs, _countof(m_inputElementDescs) };


			psoDesc.DepthStencilState = depthStencilDesc;
			psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
			psoDesc.VS = CD3DX12_SHADER_BYTECODE(m_shaders->getVS()->GetBufferPointer(), m_shaders->getVS()->GetBufferSize());
			psoDesc.PS = CD3DX12_SHADER_BYTECODE(m_shaders->getPS()->GetBufferPointer(), m_shaders->getPS()->GetBufferSize());
			psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
			psoDesc.RasterizerState.CullMode = key.cullMode;
			psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			psoDesc.pRootSignature = m_rootSignature.Get();

			psoDesc.SampleMask = UINT_MAX;
			psoDesc.PrimitiveTopologyType = key.topology;
			psoDesc.NumRenderTargets = 1;
			psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
			psoDesc.SampleDesc.Count = 1;

			ComPtr<ID3D12PipelineState> pso;
			ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso)));
			m_psos.insert({ key, pso });

			return pso;
		}
		void createRootSignature() {
			D3D12_DESCRIPTOR_RANGE descriptorRanges[3] = {};

			descriptorRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
			descriptorRanges[0].NumDescriptors = N_SRV_DESCRIPTORS;
			descriptorRanges[0].BaseShaderRegister = 0; // t0
			descriptorRanges[0].RegisterSpace = 0;
			descriptorRanges[0].OffsetInDescriptorsFromTableStart = 0;

			descriptorRanges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
			descriptorRanges[1].NumDescriptors = N_CBV_DESCRIPTORS;
			descriptorRanges[1].BaseShaderRegister = 2; // b2 (b0 and b1 are non-bindless)
			descriptorRanges[1].RegisterSpace = 0;
			descriptorRanges[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

			descriptorRanges[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
			descriptorRanges[2].NumDescriptors = N_SAMPLERS;
			descriptorRanges[2].BaseShaderRegister = 0; // s0
			descriptorRanges[2].RegisterSpace = 0;
			descriptorRanges[2].OffsetInDescriptorsFromTableStart = 0;




			D3D12_ROOT_PARAMETER rootParameters[4] = {};

			// Non-bindless CBV (b0)
			rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
			rootParameters[0].Descriptor.ShaderRegister = 0; // b0
			rootParameters[0].Descriptor.RegisterSpace = 0;
			rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

			// Non-bindless CBV (b1)
			rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
			rootParameters[1].Descriptor.ShaderRegister = 1; // b1
			rootParameters[1].Descriptor.RegisterSpace = 0;
			rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

			// Bindless Descriptor Table (SRVs + CBVs)
			rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRanges) - 1; // Excluding sampler range
			rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRanges;
			rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

			// Separate Descriptor Table for Dynamic Samplers
			rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			rootParameters[3].DescriptorTable.NumDescriptorRanges = 1;
			rootParameters[3].DescriptorTable.pDescriptorRanges = &descriptorRanges[2]; // Sampler range
			rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;


			D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
			rootSignatureDesc.NumParameters = _countof(rootParameters);
			rootSignatureDesc.pParameters = rootParameters;
			rootSignatureDesc.NumStaticSamplers = 0;
			rootSignatureDesc.pStaticSamplers = nullptr;
			rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
				| D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED
				| D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED;

			ComPtr<ID3DBlob> signatureBlob, errorBlob;
			ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob));

			ThrowIfFailed(m_device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
		}

		ComPtr<ID3D12Device> m_device;
		UINT m_width;
		UINT m_height;

		D3D12_CLEAR_VALUE m_rtvClearValue;

		std::unordered_map<EnumKey, ComPtr<ID3D12PipelineState>, EnumKeyHash> m_psos;
		ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
		ComPtr<ID3D12RootSignature> m_rootSignature;
		D3D12_INPUT_ELEMENT_DESC m_inputElementDescs[1] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};

		ComPtr<ID3D12Resource> m_depthStencilBuffer;
		ComPtr<ID3D12Resource> m_rtvResource;

		ComPtr<ID3D12DescriptorHeap> m_dsvHeap;
		std::unique_ptr<PSOShader> m_shaders;

		ComPtr<ID3D12CommandQueue> m_commandQueue;
		ComPtr<ID3D12CommandAllocator> m_commandAllocator;
		ComPtr<ID3D12CommandAllocator> m_commandAllocatorBarrier;

		ComPtr<ID3D12GraphicsCommandList> m_commandList;
		ComPtr<ID3D12GraphicsCommandList> m_commandListBarrier;
		Engine::BindlessHeapDescriptor& m_bindlessHeapDescriptor = Engine::BindlessHeapDescriptor::GetInstance();

		CD3DX12_VIEWPORT m_viewport;
		CD3DX12_RECT m_scissorRect;

		HANDLE m_fenceEvent;
		ComPtr<ID3D12Fence> m_fence;
		UINT64 m_fenceValue = 0;

		UINT m_rtvDescriptorSize = 0;
	};
}
