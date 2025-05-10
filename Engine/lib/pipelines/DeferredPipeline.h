#include "stdafx.h"

#pragma once

#include "../Device.h"
#include "../DXSampleHelper.h"
#include "PSOShader.h"
#include <span>

namespace Engine {
	using namespace Microsoft::WRL;
	class DeferredPipeline {
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
		DeferredPipeline(ComPtr<ID3D12Device> device, UINT width, UINT height) : m_device(device), m_width(width), m_height(height) {
			Engine::PSOShaderCreate psoSC;
			psoSC.PS = L"assets\\shaders\\gbuffers.hlsl";
			psoSC.VS = L"assets\\shaders\\gbuffers.hlsl";
			psoSC.PSEntry = L"PSMain";
			psoSC.VSEntry = L"VSMain";
			m_shaders = PSOShader::Create(psoSC);
			m_depthStencilDesc.DepthEnable = TRUE;
			m_depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
			m_depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_GREATER;
			m_depthStencilDesc.StencilEnable = FALSE;

			populateRtvClearValues();
			createRtvCommitedResources();
			createRtvsDescriptorHeap();
			createDsv();

			ThrowIfFailed(m_device->CreateRootSignature(0, m_shaders->getPS()->GetBufferPointer(), m_shaders->getPS()->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
		}

		ID3D12RootSignature* getRootSignature() const {
			return m_rootSignature.Get();
		}

		std::array<ID3D12Resource*, 6> getRtvResources() const {
			std::array<ID3D12Resource*, 6> resourcePtrs;

			// Extract raw pointers from ComPtr
			for (size_t i = 0; i < resourcePtrs.size(); i++) {
				resourcePtrs[i] = m_rtvResources[i].Get();
			}

			return resourcePtrs;
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

	private:
		void populateRtvClearValues() {
			for (uint32_t i = 0; i < _countof(m_rtvFormats); i++) {
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

			m_rtvClearValues[4].Color[0] = 0.0f; // Emissive + AO (Default Black)
			m_rtvClearValues[4].Color[1] = 0.0f;
			m_rtvClearValues[4].Color[2] = 0.0f;
			m_rtvClearValues[4].Color[3] = 1.0f;

			// Material ID (UINT does not use Color values, uses DepthStencil instead)
			m_rtvClearValues[5].DepthStencil.Depth = 0.0f;
			m_rtvClearValues[5].DepthStencil.Stencil = 0;
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

			for (uint32_t i = 0; i < _countof(m_rtvResources); i++) {
				rtvDesc.Format = m_rtvFormats[i];
				ThrowIfFailed(m_device->CreateCommittedResource(
					&rtvProp,
					D3D12_HEAP_FLAG_NONE,
					&rtvDesc,
					D3D12_RESOURCE_STATE_RENDER_TARGET,
					&m_rtvClearValues[i],
					IID_PPV_ARGS(&m_rtvResources[i])));
			}
		}
		void createRtvsDescriptorHeap() {
			m_rtvHeapDesc.NumDescriptors = _countof(m_rtvResources);
			m_rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			m_rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			ThrowIfFailed(m_device->CreateDescriptorHeap(&m_rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));

			m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
			D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();

			for (uint32_t i = 0; i < _countof(m_rtvResources); i++) {
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
			D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
			psoDesc.InputLayout = { m_inputElementDescs, _countof(m_inputElementDescs) };


			psoDesc.DepthStencilState = m_depthStencilDesc;
			psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
			psoDesc.VS = CD3DX12_SHADER_BYTECODE(m_shaders->getVS()->GetBufferPointer(), m_shaders->getVS()->GetBufferSize());
			psoDesc.PS = CD3DX12_SHADER_BYTECODE(m_shaders->getPS()->GetBufferPointer(), m_shaders->getPS()->GetBufferSize());
			psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
			psoDesc.RasterizerState.CullMode = key.cullMode;
			psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			psoDesc.pRootSignature = m_rootSignature.Get();

			psoDesc.SampleMask = UINT_MAX;
			psoDesc.PrimitiveTopologyType = key.topology;
			psoDesc.NumRenderTargets = 6;
			for (uint32_t i = 0; i < _countof(m_rtvFormats); i++) {
				psoDesc.RTVFormats[i] = m_rtvFormats[i];
			}
			psoDesc.SampleDesc.Count = 1;

			ComPtr<ID3D12PipelineState> pso;
			ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso)));
			m_psos.insert({ key, pso });

			return pso;
		}
		CD3DX12_DEPTH_STENCIL_DESC m_depthStencilDesc = {};
		D3D12_DESCRIPTOR_HEAP_DESC m_rtvHeapDesc = {};
		UINT m_rtvDescriptorSize = 0;

		ComPtr<ID3D12Resource> m_rtvResources[6];
		DXGI_FORMAT m_rtvFormats[6] = {
			DXGI_FORMAT_R16G16B16A16_FLOAT, // albedo/diffues
			DXGI_FORMAT_R32G32B32A32_FLOAT, // world normals
			DXGI_FORMAT_R32G32B32A32_FLOAT, // world posiiton
			DXGI_FORMAT_R16G16_FLOAT, // RM
			DXGI_FORMAT_R16G16B16A16_FLOAT, // Emissive + Ao
			DXGI_FORMAT_R32_UINT, // MaterialId
		};
		D3D12_CLEAR_VALUE m_rtvClearValues[6];

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
	};
}
