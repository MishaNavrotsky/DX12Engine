#include "stdafx.h"

#pragma once

#include "../../Device.h"
#include "../../DXSampleHelper.h"
#include "../PSOShader.h"
#include "../../camera/Camera.h"
#include "../../mesh/CPUMesh.h"
#include "../../mesh/CPUMaterial.h"
#include "../../mesh/GPUMesh.h"
#include "../../scene/SceneNode.h"
#include "../../scene/Scene.h"
#include "../../geometry/CubeGeometry.h"
#include "../../geometry/PlaneGeometry.h"
#include "../../helpers.h"

#include "../../managers/CPUMaterialManager.h"
#include "../../managers/CPUMeshManager.h"
#include "../../managers/ModelManager.h"

#include "../../nodes/ModelSceneNode.h"
#include "../../memory/Resource.h"


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

			createRtvCommitedResources();
			createRtvsDescriptorHeap();
			createDsv();
			createRootSignature();

			ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)));
			ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocatorBarrier)));

			ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_commandList)));
			ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocatorBarrier.Get(), nullptr, IID_PPV_ARGS(&m_commandListBarrier)));

			ThrowIfFailed(m_commandList->Close());
			ThrowIfFailed(m_commandListBarrier->Close());


			m_viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));
			m_scissorRect = CD3DX12_RECT(0, 0, static_cast<LONG>(width), static_cast<LONG>(height));
		}
		std::array<ID3D12CommandList*, 2> renderGizmos(Scene* scene, Camera* camera, Memory::Resource* srcDepthBuffer) {
			ThrowIfFailed(m_commandAllocator->Reset());
			ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), nullptr));
			{
				CD3DX12_RESOURCE_BARRIER barrierBack = CD3DX12_RESOURCE_BARRIER::Transition(m_rtvResource->getResource(),
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
					D3D12_RESOURCE_STATE_RENDER_TARGET);

				m_commandList->ResourceBarrier(1, &barrierBack);
			}
			cloneDepthBuffer(srcDepthBuffer);
			m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());
			m_commandList->SetGraphicsRootConstantBufferView(0, camera->getResource()->GetGPUVirtualAddress());

			//ID3D12DescriptorHeap* heaps[] = { m_bindlessHeapDescriptor.getSrvDescriptorHeap(), m_bindlessHeapDescriptor.getSamplerDescriptorHeap() };
			//m_commandList->SetDescriptorHeaps(_countof(heaps), heaps);
			//m_commandList->SetGraphicsRootDescriptorTable(2, m_bindlessHeapDescriptor.getSrvDescriptorHeap()->GetGPUDescriptorHandleForHeapStart());
			//m_commandList->SetGraphicsRootDescriptorTable(3, m_bindlessHeapDescriptor.getSamplerDescriptorHeap()->GetGPUDescriptorHandleForHeapStart());

			m_commandList->RSSetViewports(1, &m_viewport);
			m_commandList->RSSetScissorRects(1, &m_scissorRect);

			auto rtvHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_rtvHeap.Get()->GetCPUDescriptorHandleForHeapStart());
			auto dsvHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_dsvHeap.Get()->GetCPUDescriptorHandleForHeapStart());
			m_commandList->OMSetRenderTargets(1, &rtvHandle, TRUE, &dsvHandle);
			m_commandList->ClearRenderTargetView(rtvHandle, m_rtvResource->getClearValue()->Color, 0, nullptr);

			//populateScene(scene, camera);

			m_scene->draw(m_commandList.Get(), camera, false, [&](CPUMesh& mesh, CPUMaterial& material, SceneNode* node) {
				m_commandList->IASetPrimitiveTopology(mesh.topology);
				m_commandList->SetPipelineState(getPso({ material.cullMode, mesh.topologyType }));
				m_commandList->SetGraphicsRootConstantBufferView(1, node->getResource()->GetGPUVirtualAddress());
				return false;
				});


			ThrowIfFailed(m_commandList->Close());


			ThrowIfFailed(m_commandAllocatorBarrier->Reset());
			ThrowIfFailed(m_commandListBarrier->Reset(m_commandAllocatorBarrier.Get(), nullptr));
			{
				CD3DX12_RESOURCE_BARRIER barrierBack = CD3DX12_RESOURCE_BARRIER::Transition(m_rtvResource->getResource(),
					D3D12_RESOURCE_STATE_RENDER_TARGET,
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

				m_commandListBarrier->ResourceBarrier(1, &barrierBack);
			}

			ThrowIfFailed(m_commandListBarrier->Close());


			return std::array<ID3D12CommandList*, 2>({ m_commandList.Get(), m_commandListBarrier.Get() });
		}

		Memory::Resource* getRtvResource() const {
			return m_rtvResource.get();
		}

		Memory::Resource* getDepthStencilResource() const {
			return m_depthStencilBuffer.get();
		}
	private:

		void populateScene(Scene* oScene, Camera* camera) {
			m_scene->getSceneRootNodes().clear();
			static auto& cpuMaterialManager = CPUMaterialManager::GetInstance();
			static auto& cpuMeshManager = CPUMeshManager::GetInstance();
			static auto& modelLoader = Engine::ModelLoader::GetInstance();
			static auto& uploadQueue = Engine::GPUUploadQueue::GetInstance();

			auto meshNodes = oScene->getAllMeshNodes();

			std::vector<GUID> meshGUIDs;

			for (auto& meshNode : meshNodes) {
				auto worldSpace = *(&meshNode->getWorldSpaceAABB());
				DirectX::XMFLOAT3 min;
				DirectX::XMStoreFloat3(&min, worldSpace.min);
				DirectX::XMFLOAT3 max;
				DirectX::XMStoreFloat3(&max, worldSpace.max);



				auto cube = Geometry::GenerateCubeFromPoints(min, max);
				auto cpuMesh = std::make_unique<CPUMesh>();
				cpuMesh->setIndices(std::vector<uint32_t>(cube.lineListIndices.begin(), cube.lineListIndices.end()));
				cpuMesh->setVertices(Helpers::FlattenXMFLOAT3Array(cube.vertices));
				cpuMesh->setNormals(Helpers::FlattenXMFLOAT3Array(cube.normals));
				cpuMesh->topology = D3D_PRIMITIVE_TOPOLOGY_LINELIST;
				cpuMesh->topologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
				cpuMesh->setCPUMaterialId(GUID_NULL);

				meshGUIDs.push_back(cpuMeshManager.add(std::move(cpuMesh)));
			}

			//auto& frustrum = camera->getFrustum();
			//for (int i = 0; i < std::size(frustrum.planes); ++i) {
			//	auto plane = frustrum.planes[i];
			//	auto p = Geometry::GeneratePlaneQuad(plane);
			//	std::array<uint32_t, 8> lineIndices = {
			//		0, 1, 1, 2, 2, 3, 3, 0
			//	};

			//	auto cpuMesh = std::make_unique<CPUMesh>();
			//	cpuMesh->setVertices(Helpers::FlattenXMFLOAT3Array(p));
			//	cpuMesh->setIndices(std::vector<uint32_t>(lineIndices.begin(), lineIndices.end()));
			//	cpuMesh->topology = D3D_PRIMITIVE_TOPOLOGY_LINELIST;
			//	cpuMesh->topologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
			//	cpuMesh->setCPUMaterialId(GUID_NULL);
			//	meshGUIDs.push_back(cpuMeshManager.add(std::move(cpuMesh)));
			//}

			auto o = ModelSceneNode::CreateFromGeometry(meshGUIDs);
			modelLoader.waitForQueueEmpty();
			uploadQueue.execute();
			o->waitUntilLoadComplete();
			m_scene->addNode(std::move(o));
		}
		void cloneDepthBuffer(Memory::Resource* srcDepthBuffer) {
			auto b = CD3DX12_RESOURCE_BARRIER::Transition(
				srcDepthBuffer->getResource(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_COPY_SOURCE);
			m_commandList->ResourceBarrier(1, &b);

			auto b1 = CD3DX12_RESOURCE_BARRIER::Transition(
				m_depthStencilBuffer->getResource(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_COPY_DEST);
			m_commandList->ResourceBarrier(1, &b1);

			// Copy entire depth buffer
			m_depthStencilBuffer->copyResource(m_commandList.Get(), srcDepthBuffer);

			// Restore original states
			auto b2 = CD3DX12_RESOURCE_BARRIER::Transition(
				srcDepthBuffer->getResource(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);
			m_commandList->ResourceBarrier(1, &b2);

			auto b3 = CD3DX12_RESOURCE_BARRIER::Transition(
				m_depthStencilBuffer->getResource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_DEPTH_WRITE);
			m_commandList->ResourceBarrier(1, &b3);
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

			D3D12_CLEAR_VALUE clearValue = {};
			clearValue.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			clearValue.Color[0] = 0.0f;
			clearValue.Color[1] = 0.0f;
			clearValue.Color[2] = 0.0f;
			clearValue.Color[3] = 0.0f; //for alpha


			m_rtvResource = Memory::Resource::Create(
				rtvProp,
				rtvDesc,
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
				&clearValue);
		}
		void createRtvsDescriptorHeap() {
			D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};

			rtvHeapDesc.NumDescriptors = 1;
			rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			ThrowIfFailed(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));

			m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
			D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
			m_device->CreateRenderTargetView(m_rtvResource->getResource(), nullptr, rtvHandle);
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

			m_depthStencilBuffer = Memory::Resource::Create(
				heapProps,
				depthBufferDesc,
				D3D12_RESOURCE_STATE_DEPTH_WRITE,
				&clearValue);

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
				m_depthStencilBuffer->getResource(),
				&dsvDesc,
				m_dsvHeap->GetCPUDescriptorHandleForHeapStart()
			);
		}
		ID3D12PipelineState* getPso(const EnumKey key) {
			if (m_psos.find(key) != m_psos.end()) return m_psos.at(key).Get();
			return createPso(key).Get();
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


			D3D12_BLEND_DESC blendDesc = {};
			blendDesc.AlphaToCoverageEnable = FALSE;
			blendDesc.IndependentBlendEnable = FALSE;

			D3D12_RENDER_TARGET_BLEND_DESC rtBlendDesc = {};
			rtBlendDesc.BlendEnable = TRUE;
			rtBlendDesc.LogicOpEnable = FALSE;

			rtBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
			rtBlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
			rtBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;

			rtBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
			rtBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
			rtBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;

			rtBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			blendDesc.RenderTarget[0] = rtBlendDesc;

			psoDesc.BlendState = blendDesc;

			ComPtr<ID3D12PipelineState> pso;
			ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso)));
			m_psos.insert({ key, pso });

			return pso;
		}
		void createRootSignature() {
			D3D12_ROOT_PARAMETER rootParameters[2] = {};

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


			D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
			rootSignatureDesc.NumParameters = std::size(rootParameters);
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

		std::unordered_map<EnumKey, ComPtr<ID3D12PipelineState>, EnumKeyHash> m_psos;
		ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
		ComPtr<ID3D12RootSignature> m_rootSignature;
		D3D12_INPUT_ELEMENT_DESC m_inputElementDescs[4] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 3, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};

		std::unique_ptr<Memory::Resource> m_depthStencilBuffer;
		std::unique_ptr<Memory::Resource> m_rtvResource;

		ComPtr<ID3D12DescriptorHeap> m_dsvHeap;
		std::unique_ptr<PSOShader> m_shaders;

		ComPtr<ID3D12CommandAllocator> m_commandAllocator;
		ComPtr<ID3D12CommandAllocator> m_commandAllocatorBarrier;

		ComPtr<ID3D12GraphicsCommandList> m_commandList;
		ComPtr<ID3D12GraphicsCommandList> m_commandListBarrier;
		Engine::BindlessHeapDescriptor& m_bindlessHeapDescriptor = Engine::BindlessHeapDescriptor::GetInstance();

		CD3DX12_VIEWPORT m_viewport;
		CD3DX12_RECT m_scissorRect;

		UINT m_rtvDescriptorSize = 0;

		std::unique_ptr<Scene> m_scene = std::make_unique<Engine::Scene>();
	};
}
