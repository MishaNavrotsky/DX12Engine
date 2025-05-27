#include "stdafx.h"

#pragma once

#include "../Device.h"
#include "../DXSampleHelper.h"
#include "PSOShader.h"
#include "../camera/Camera.h"
#include "../mesh/CPUMesh.h"
#include "../mesh/CPUMaterial.h"
#include "../scene/SceneNode.h"
#include "../geometry/PlaneGeometry.h"
#include "../helpers.h"
#include "../memory/Resource.h"

namespace Engine {
	using namespace Microsoft::WRL;

	class CompositionPass {
	public:
		CompositionPass(ComPtr<ID3D12Device> device, UINT width, UINT height) : m_device(device), m_width(width), m_height(height) {
			Engine::PSOShaderCreate psoSC;
			psoSC.PS = L"assets\\shaders\\composition.hlsl";
			psoSC.VS = L"assets\\shaders\\composition.hlsl";
			psoSC.PSEntry = L"PSMain";
			psoSC.VSEntry = L"VSMain";
			m_shaders = PSOShader::Create(psoSC);

			createRtvResource();
			createRtvsDescriptorHeap();
			createFullScreenQuad();
			createRootSignature();
			createPso();
			createSrvDescriptorHeap();

			ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)));
			ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocatorBarrier)));

			ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_commandList)));
			ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocatorBarrier.Get(), nullptr, IID_PPV_ARGS(&m_commandListBarrier)));

			ThrowIfFailed(m_commandList->Close());
			ThrowIfFailed(m_commandListBarrier->Close());

			m_viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(m_width), static_cast<float>(height));
			m_scissorRect = CD3DX12_RECT(0, 0, static_cast<LONG>(m_width), static_cast<LONG>(height));
		}
		std::array<ID3D12CommandList*, 2> renderComposition(Memory::Resource* lightningPassTexture, Memory::Resource* gizmosPassTexture, Memory::Resource* uiPassTexture, Camera* camera) {
			ThrowIfFailed(m_commandAllocator->Reset());
			ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), nullptr));
			{
				CD3DX12_RESOURCE_BARRIER barrierBack = CD3DX12_RESOURCE_BARRIER::Transition(lightningPassTexture->getResource(),
					D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

				m_commandList->ResourceBarrier(1, &barrierBack);
			}
			m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());
			m_commandList->SetGraphicsRootConstantBufferView(1, camera->getResource()->GetGPUVirtualAddress());

			populateSrvDescriptorHeap(lightningPassTexture->getResource(), gizmosPassTexture->getResource(), uiPassTexture->getResource());
			ID3D12DescriptorHeap* heaps[] = { m_srvDescriptorHeap.Get()};
			m_commandList->SetDescriptorHeaps(_countof(heaps), heaps);
			m_commandList->SetGraphicsRootDescriptorTable(0, m_srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

			m_commandList->RSSetViewports(1, &m_viewport);
			m_commandList->RSSetScissorRects(1, &m_scissorRect);

			m_commandList->OMSetRenderTargets(1, &m_rtvHandle, TRUE, nullptr);
			m_commandList->ClearRenderTargetView(m_rtvHandle, m_rtvResource->getClearValue()->Color, 0, nullptr);
			m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			m_commandList->SetPipelineState(m_pso.Get());


			m_scene->draw(m_commandList.Get(), camera, false, [&](CPUMesh& mesh, CPUMaterial& material, SceneNode* node) {
				return false;
				});


			ThrowIfFailed(m_commandList->Close());


			ThrowIfFailed(m_commandAllocatorBarrier->Reset());
			ThrowIfFailed(m_commandListBarrier->Reset(m_commandAllocatorBarrier.Get(), nullptr));
			{
				CD3DX12_RESOURCE_BARRIER barrierBack = CD3DX12_RESOURCE_BARRIER::Transition(lightningPassTexture->getResource(),
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
					D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

				m_commandListBarrier->ResourceBarrier(1, &barrierBack);
			}

			ThrowIfFailed(m_commandListBarrier->Close());

			return std::array<ID3D12CommandList*, 2>({ m_commandList.Get(), m_commandListBarrier.Get() });

		}

		Memory::Resource* getRtvResource() const {
			return m_rtvResource.get();
		}
	private:
		void createRtvsDescriptorHeap() {
			D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};

			rtvHeapDesc.NumDescriptors = N_OF_RTVS;
			rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			ThrowIfFailed(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvDescriptorHeap)));

			UINT rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
			m_rtvHandle = m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
			m_device->CreateRenderTargetView(m_rtvResource->getResource(), nullptr, m_rtvHandle);
		}
		void createRtvResource() {
			D3D12_RESOURCE_DESC overlayDesc = {};
			overlayDesc.MipLevels = 1;
			overlayDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			overlayDesc.Width = m_width;
			overlayDesc.Height = m_height;
			overlayDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
			overlayDesc.SampleDesc.Count = 1;
			overlayDesc.SampleDesc.Quality = 0;
			overlayDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
			overlayDesc.DepthOrArraySize = 1;
			overlayDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

			D3D12_CLEAR_VALUE rtvClearValue;
			rtvClearValue.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			rtvClearValue.Color[0] = 0.0f;
			rtvClearValue.Color[1] = 0.0f;
			rtvClearValue.Color[2] = 0.0f;
			rtvClearValue.Color[3] = 1.0f;

			auto props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
			m_rtvResource = Memory::Resource::Create(
				props,
				overlayDesc,
				D3D12_RESOURCE_STATE_RENDER_TARGET,
				&rtvClearValue);

			m_rtvResource->getResource()->SetName(L"Overlay Texture");
		}
		void createFullScreenQuad() {
			static auto& cpuMaterialManager = CPUMaterialManager::GetInstance();
			static auto& cpuMeshManager = CPUMeshManager::GetInstance();
			//static auto& modelLoader = Engine::ModelLoader::GetInstance();
			//static auto& uploadQueue = Engine::GPUUploadQueue::GetInstance();


			std::vector<GUID> meshGUIDs;

			auto fullscreenQuad = Engine::Geometry::GeneratePlane(2.0f, 2.0f, 1, 1);
			auto cpuMesh = std::make_unique<CPUMesh>();
			cpuMesh->setVertices(Helpers::FlattenXMFLOAT3Vector(fullscreenQuad.vertices));
			cpuMesh->setIndices(std::move(fullscreenQuad.indices));
			cpuMesh->topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			cpuMesh->topologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			cpuMesh->setCPUMaterialId(GUID_NULL);
			meshGUIDs.push_back(cpuMeshManager.add(std::move(cpuMesh)));
			//auto modelSceneNode = ModelSceneNode::CreateFromGeometry(meshGUIDs);
			//modelLoader.waitForQueueEmpty();
			//uploadQueue.execute();
			//modelSceneNode->waitUntilLoadComplete();
			//m_scene->addNode(std::move(modelSceneNode));
		}
		void createPso() {
			CD3DX12_DEPTH_STENCIL_DESC depthStencilDesc = {};
			depthStencilDesc.DepthEnable = FALSE;
			depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
			depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
			depthStencilDesc.StencilEnable = FALSE;

			D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
			psoDesc.InputLayout = { m_inputElementDescs, _countof(m_inputElementDescs) };


			psoDesc.DepthStencilState = depthStencilDesc;
			psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
			psoDesc.VS = CD3DX12_SHADER_BYTECODE(m_shaders->getVS()->GetBufferPointer(), m_shaders->getVS()->GetBufferSize());
			psoDesc.PS = CD3DX12_SHADER_BYTECODE(m_shaders->getPS()->GetBufferPointer(), m_shaders->getPS()->GetBufferSize());
			psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
			psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
			psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			psoDesc.pRootSignature = m_rootSignature.Get();

			psoDesc.SampleMask = UINT_MAX;
			psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			psoDesc.NumRenderTargets = 1;
			psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
			psoDesc.SampleDesc.Count = 1;

			ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pso)));
		}
		void createRootSignature() {
			CD3DX12_ROOT_PARAMETER rootParameters[2];

			CD3DX12_DESCRIPTOR_RANGE range;
			range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 0); // 3 SRVs starting at t0

			rootParameters[0].InitAsDescriptorTable(1, &range, D3D12_SHADER_VISIBILITY_PIXEL);
			rootParameters[1].InitAsConstantBufferView(0, 0);  // Camera buffer (CBV at slot 0)


			CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc;
			rootSigDesc.Init(_countof(rootParameters), rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

			ComPtr<ID3DBlob> signature;
			ComPtr<ID3DBlob> error;
			D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &signature, &error);
			m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature));
		}
		void createSrvDescriptorHeap() {
			D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
			srvHeapDesc.NumDescriptors = 3; // Gizmos and Lighting textures and UI
			srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			ThrowIfFailed(m_device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_srvDescriptorHeap)));

			m_cbvSrvUavDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}
		void populateSrvDescriptorHeap(ID3D12Resource* lightingTexture, ID3D12Resource* gizmosTexture, ID3D12Resource* uiTexture) {
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Texture2D.MipLevels = 1;
			auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
			m_device->CreateShaderResourceView(lightingTexture, &srvDesc, handle);

			handle.Offset(1, m_cbvSrvUavDescriptorSize);
			m_device->CreateShaderResourceView(gizmosTexture, &srvDesc, handle);

			handle.Offset(1, m_cbvSrvUavDescriptorSize);
			m_device->CreateShaderResourceView(uiTexture, &srvDesc, handle);
		}
		ComPtr<ID3D12RootSignature> m_rootSignature;
		ComPtr<ID3D12PipelineState> m_pso;

		ComPtr<ID3D12Device> m_device;
		UINT m_width;
		UINT m_height;

		std::unique_ptr<PSOShader> m_shaders;

		ComPtr<ID3D12CommandAllocator> m_commandAllocator, m_commandAllocatorBarrier;
		ComPtr<ID3D12GraphicsCommandList> m_commandList, m_commandListBarrier;
		D3D12_INPUT_ELEMENT_DESC m_inputElementDescs[4] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 3, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};

		CD3DX12_VIEWPORT m_viewport;
		CD3DX12_RECT m_scissorRect;

		ComPtr<ID3D12DescriptorHeap> m_rtvDescriptorHeap, m_srvDescriptorHeap;
		UINT m_cbvSrvUavDescriptorSize;
		std::unique_ptr<Memory::Resource> m_rtvResource;
		D3D12_CPU_DESCRIPTOR_HANDLE m_rtvHandle;

		std::unique_ptr<Scene> m_scene = std::make_unique<Engine::Scene>();
	};
}
