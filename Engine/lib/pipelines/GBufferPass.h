#include "stdafx.h"

#pragma once

#include "../Device.h"
#include "../DXSampleHelper.h"
#include "PSOShader.h"
#include "../camera/Camera.h"
#include "../mesh/CPUMesh.h"
#include "../mesh/CPUMaterial.h"
#include "../scene/SceneNode.h"


const uint32_t N_OF_RTVS = 7;

namespace Engine {
	using namespace Microsoft::WRL;
	class GBufferPass {
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
	public:
		GBufferPass(ComPtr<ID3D12Device> device, UINT width, UINT height) : m_device(device), m_width(width), m_height(height) {
			Engine::PSOShaderCreate psoSC;
			psoSC.PS = L"assets\\shaders\\gbuffers.hlsl";
			psoSC.VS = L"assets\\shaders\\gbuffers.hlsl";
			psoSC.PSEntry = L"PSMain";
			psoSC.VSEntry = L"VSMain";
			m_shaders = PSOShader::Create(psoSC);

			populateRtvClearValues();
			createRtvCommitedResources();
			createRtvsDescriptorHeap();
			createDsv();
			createRootSignature();

			ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)));
			ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocatorBarrier)));

			ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_commandList)));
			ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocatorBarrier.Get(), nullptr, IID_PPV_ARGS(&m_commandListBarrier)));
			ThrowIfFailed(m_commandListBarrier->Close());
			ThrowIfFailed(m_commandList->Close());


			m_viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));
			m_scissorRect = CD3DX12_RECT(0, 0, static_cast<LONG>(width), static_cast<LONG>(height));
		}

		std::array<ID3D12CommandList*, 2> renderGBuffers(Scene* scene, Camera* camera) {
			ThrowIfFailed(m_commandAllocator->Reset());
			ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), nullptr));
			{
				CD3DX12_RESOURCE_BARRIER barrierBack[N_OF_RTVS];
				for (uint32_t i = 0; i < N_OF_RTVS; i++) {
					barrierBack[i] = CD3DX12_RESOURCE_BARRIER::Transition(m_rtvResources[i].Get(),
						D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
						D3D12_RESOURCE_STATE_RENDER_TARGET);
				}
				m_commandList->ResourceBarrier(N_OF_RTVS, barrierBack);
			}

			m_commandList->SetGraphicsRootSignature(getRootSignature());
			m_commandList->SetGraphicsRootConstantBufferView(0, camera->getResource()->GetGPUVirtualAddress());

			ID3D12DescriptorHeap* heaps[] = { m_bindlessHeapDescriptor.getSrvDescriptorHeap(), m_bindlessHeapDescriptor.getSamplerDescriptorHeap() };
			m_commandList->SetDescriptorHeaps(_countof(heaps), heaps);
			m_commandList->SetGraphicsRootDescriptorTable(2, m_bindlessHeapDescriptor.getSrvDescriptorHeap()->GetGPUDescriptorHandleForHeapStart());
			m_commandList->SetGraphicsRootDescriptorTable(3, m_bindlessHeapDescriptor.getSamplerDescriptorHeap()->GetGPUDescriptorHandleForHeapStart());

			m_commandList->RSSetViewports(1, &m_viewport);
			m_commandList->RSSetScissorRects(1, &m_scissorRect);

			auto rtvHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(getRtvDescHeap()->GetCPUDescriptorHandleForHeapStart());
			auto dsvHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(getDsvDescHeap()->GetCPUDescriptorHandleForHeapStart());
			m_commandList->OMSetRenderTargets(getRenderTargetsSize(), &rtvHandle, TRUE, &dsvHandle);

			const auto& rtvsClearColors = getRtvsClearValues();
			UINT rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
			for (auto& clearColor : rtvsClearColors) {
				m_commandList->ClearRenderTargetView(rtvHandle, clearColor.Color, 0, nullptr);
				rtvHandle.Offset(1, rtvDescriptorSize);
			}
			m_commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 0.0f, 0, 0, nullptr);

			m_commandList->SetGraphicsRootConstantBufferView(0, camera->getResource()->GetGPUVirtualAddress());
			auto lambda = std::function([&](CPUMesh& mesh, CPUMaterial& material, SceneNode* node) {
				m_commandList->IASetPrimitiveTopology(mesh.topology);
				m_commandList->SetPipelineState(getPso({ material.cullMode, mesh.topologyType }));
				m_commandList->SetGraphicsRootConstantBufferView(1, node->getResource()->GetGPUVirtualAddress());
				return false;
			});

			scene->draw(m_commandList.Get(), camera, true, lambda);

			ThrowIfFailed(m_commandList->Close());


			ThrowIfFailed(m_commandAllocatorBarrier->Reset());
			ThrowIfFailed(m_commandListBarrier->Reset(m_commandAllocatorBarrier.Get(), nullptr));
			{
				CD3DX12_RESOURCE_BARRIER barrierBack[N_OF_RTVS];
				for (uint32_t i = 0; i < N_OF_RTVS; i++) {
					barrierBack[i] = CD3DX12_RESOURCE_BARRIER::Transition(m_rtvResources[i].Get(),
						D3D12_RESOURCE_STATE_RENDER_TARGET,
						D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
				}
				m_commandListBarrier->ResourceBarrier(N_OF_RTVS, barrierBack);
			}
			ThrowIfFailed(m_commandListBarrier->Close());


			return std::array<ID3D12CommandList*, 2>({ m_commandList.Get(), m_commandListBarrier.Get() });
		}

		std::array<ID3D12Resource*, N_OF_RTVS> getRtvResources() const {
			std::array<ID3D12Resource*, N_OF_RTVS> resourcePtrs;

			// Extract raw pointers from ComPtr
			for (size_t i = 0; i < resourcePtrs.size(); i++) {
				resourcePtrs[i] = m_rtvResources[i].Get();
			}

			return resourcePtrs;
		}

		ID3D12Resource* getDepthStencilResource() const {
			return m_depthStencilBuffer.Get();
		}

	private:
		ID3D12RootSignature* getRootSignature() const {
			return m_rootSignature.Get();
		}
		ID3D12PipelineState* getPso(const EnumKey key) {
			if (m_psos.find(key) != m_psos.end()) return m_psos.at(key).Get();
			return createPso(key).Get();
		}
		ID3D12DescriptorHeap* getRtvDescHeap() const {
			return m_rtvHeap.Get();
		}
		ID3D12DescriptorHeap* getDsvDescHeap() const {
			return m_dsvHeap.Get();
		}
		inline uint32_t getRenderTargetsSize() {
			return N_OF_RTVS;
		}

		std::span<D3D12_CLEAR_VALUE> getRtvsClearValues() {
			return std::span<D3D12_CLEAR_VALUE>(m_rtvClearValues);
		}
		void populateRtvClearValues() {
			for (uint32_t i = 0; i < N_OF_RTVS; i++) {
				m_rtvClearValues[i].Format = m_rtvFormats[i];
			}

			// Manually assign default clear values without looping
			m_rtvClearValues[0].Color[0] = 0.0f; // Albedo/Diffuse (Black)
			m_rtvClearValues[0].Color[1] = 0.0f;
			m_rtvClearValues[0].Color[2] = 0.0f;
			m_rtvClearValues[0].Color[3] = 1.0f; // Opaque Alpha

			m_rtvClearValues[1].Color[0] = 0.0f; // World Normals (Neutral)
			m_rtvClearValues[1].Color[1] = 0.0f;
			m_rtvClearValues[1].Color[2] = 0.0f;
			m_rtvClearValues[1].Color[3] = 0.0f; // unused

			m_rtvClearValues[2].Color[0] = 0.0f; // World Position (Neutral)
			m_rtvClearValues[2].Color[1] = 0.0f;
			m_rtvClearValues[2].Color[2] = 0.0f;
			m_rtvClearValues[2].Color[3] = 0.0f; //unused

			m_rtvClearValues[3].Color[0] = 0.0f; // Roughness-Metallic (Default)
			m_rtvClearValues[3].Color[1] = 0.0f;
			m_rtvClearValues[3].Color[2] = 1.0f; // Ao (Black)
			m_rtvClearValues[3].Color[3] = 0.0f; // unused


			m_rtvClearValues[4].Color[0] = 0.0f; // Emissive
			m_rtvClearValues[4].Color[1] = 0.0f;
			m_rtvClearValues[4].Color[2] = 0.0f;
			m_rtvClearValues[4].Color[3] = 0.0f; // unused

			m_rtvClearValues[5].Color[0] = 0.0f; // Motion Vectors
			m_rtvClearValues[5].Color[1] = 0.0f;
			m_rtvClearValues[5].Color[2] = 0.0f;
			m_rtvClearValues[5].Color[3] = 0.0f; // unused

			// Material ID (UINT does not use Color values, uses DepthStencil instead)
			m_rtvClearValues[6].DepthStencil.Depth = 0.0f;
			m_rtvClearValues[6].DepthStencil.Stencil = 0;
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

			for (uint32_t i = 0; i < N_OF_RTVS; i++) {
				rtvDesc.Format = m_rtvFormats[i];
				ThrowIfFailed(m_device->CreateCommittedResource(
					&rtvProp,
					D3D12_HEAP_FLAG_NONE,
					&rtvDesc,
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
					&m_rtvClearValues[i],
					IID_PPV_ARGS(&m_rtvResources[i])));
			}
		}
		void createRtvsDescriptorHeap() {
			D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};

			rtvHeapDesc.NumDescriptors = N_OF_RTVS;
			rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			ThrowIfFailed(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));

			UINT m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
			D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();

			for (uint32_t i = 0; i < N_OF_RTVS; i++) {
				auto& res = m_rtvResources[i];
				m_device->CreateRenderTargetView(res.Get(), nullptr, rtvHandle);
				rtvHandle.ptr += m_rtvDescriptorSize;
			}
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
			psoDesc.NumRenderTargets = N_OF_RTVS;
			for (uint32_t i = 0; i < N_OF_RTVS; i++) {
				psoDesc.RTVFormats[i] = m_rtvFormats[i];
			}
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


		ComPtr<ID3D12Resource> m_rtvResources[N_OF_RTVS];
		DXGI_FORMAT m_rtvFormats[N_OF_RTVS] = {
			DXGI_FORMAT_R16G16B16A16_FLOAT, // albedo/diffues
			DXGI_FORMAT_R32G32B32A32_FLOAT, // world normals + zero
			DXGI_FORMAT_R32G32B32A32_FLOAT, // world posiiton + zero
			DXGI_FORMAT_R16G16B16A16_FLOAT, // RM + Ao + Zero
			DXGI_FORMAT_R16G16B16A16_FLOAT, // Emissive + Zero
			DXGI_FORMAT_R16G16B16A16_FLOAT, // ScreenSpace Motion Vectors + Zero
			DXGI_FORMAT_R32_UINT, // MaterialId
		};
		D3D12_CLEAR_VALUE m_rtvClearValues[N_OF_RTVS];

		std::unordered_map<EnumKey, ComPtr<ID3D12PipelineState>, EnumKeyHash> m_psos;
		ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
		ComPtr<ID3D12Device> m_device;
		ComPtr<ID3D12RootSignature> m_rootSignature;
		D3D12_INPUT_ELEMENT_DESC m_inputElementDescs[4] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 3, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};

		UINT m_width;
		UINT m_height;

		ComPtr<ID3D12Resource> m_depthStencilBuffer;
		ComPtr<ID3D12DescriptorHeap> m_dsvHeap;
		std::unique_ptr<PSOShader> m_shaders;

		ComPtr<ID3D12CommandAllocator> m_commandAllocator;
		ComPtr<ID3D12CommandAllocator> m_commandAllocatorBarrier;

		ComPtr<ID3D12GraphicsCommandList> m_commandList;
		ComPtr<ID3D12GraphicsCommandList> m_commandListBarrier;
		Engine::BindlessHeapDescriptor& m_bindlessHeapDescriptor = Engine::BindlessHeapDescriptor::GetInstance();

		CD3DX12_VIEWPORT m_viewport;
		CD3DX12_RECT m_scissorRect;
	};
}
