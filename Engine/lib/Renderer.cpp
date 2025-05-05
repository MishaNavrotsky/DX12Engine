#include "stdafx.h"
#include "Renderer.h"

struct CBVCameraData {
	XMFLOAT4X4 projectionMatrix;
	XMFLOAT4X4 projectionReverseDepthMatrix;
	XMFLOAT4X4 viewMatrix;
	XMFLOAT4X4 viewProjectionReverseDepthMatrix;
	XMFLOAT4 position;
};

struct CBVMeshData {
	XMFLOAT4X4 modelMatrix;

	XMUINT4 diffuseNormalOcclusionEmisiveTexturesIds;
	XMUINT4 MRTextureIds;

	XMUINT4 diffuseNormalOcclusionEmisiveSamplersIds;
	XMUINT4 MRSamplersIds;
};

Renderer::Renderer(UINT width, UINT height, std::wstring name) :
	DXSample(width, height, name),
	m_frameIndex(0),
	m_viewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)),
	m_scissorRect(0, 0, static_cast<LONG>(width), static_cast<LONG>(height)),
	m_rtvDescriptorSize(0),
	m_camera(XMConvertToRadians(60.0f), width, height, 0.1f, 100000.0f)
{
}

void Renderer::OnInit()
{
	LoadPipeline();
	LoadAssets();
}

void Renderer::LoadPipeline()
{
	UINT dxgiFactoryFlags = 0;

#if defined(_DEBUG)
	{
		ComPtr<ID3D12Debug> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		{
			debugController->EnableDebugLayer();

			dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
		}
	}
#endif

	ComPtr<IDXGIFactory4> factory;
	ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));

	if (m_useWarpDevice)
	{
		ComPtr<IDXGIAdapter> warpAdapter;
		ThrowIfFailed(factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));

		ThrowIfFailed(D3D12CreateDevice(
			warpAdapter.Get(),
			D3D_FEATURE_LEVEL_12_2,
			IID_PPV_ARGS(&m_device)
		));
	}
	else
	{
		ComPtr<IDXGIAdapter1> hardwareAdapter;
		GetHardwareAdapter(factory.Get(), &hardwareAdapter);

		ThrowIfFailed(D3D12CreateDevice(
			hardwareAdapter.Get(),
			D3D_FEATURE_LEVEL_12_2,
			IID_PPV_ARGS(&m_device)
		));
	}

	Engine::GPUUploadQueue::getInstance().registerDevice(m_device);


	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = FrameCount;
	swapChainDesc.Width = m_width;
	swapChainDesc.Height = m_height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;

	ComPtr<IDXGISwapChain1> swapChain;
	ThrowIfFailed(factory->CreateSwapChainForHwnd(
		m_commandQueue.Get(), Win32Application::GetHwnd(),
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain
	));

	ThrowIfFailed(factory->MakeWindowAssociation(Win32Application::GetHwnd(), DXGI_MWA_NO_ALT_ENTER));

	ThrowIfFailed(swapChain.As(&m_swapChain));
	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

	{
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = FrameCount;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));

		m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}
	{
		D3D12_CLEAR_VALUE clearValue = {};
		clearValue.Format = DXGI_FORMAT_D32_FLOAT; // Match the depth buffer format
		clearValue.DepthStencil.Depth = 0.0f; // Default depth clear value
		clearValue.DepthStencil.Stencil = 0; // Default stencil clear value


		D3D12_RESOURCE_DESC depthBufferDesc = {};
		depthBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		depthBufferDesc.Width = GetWidth();
		depthBufferDesc.Height = GetHeight();
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
	{
		auto sizeInBytesWithAlignment = (static_cast<UINT>(sizeof(CBVCameraData)) + 255) & ~255;

		D3D12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeInBytesWithAlignment);
		bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		D3D12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

		ThrowIfFailed(m_device->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&bufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_cameraBuffer)));
		m_cameraBuffer->SetName(L"Camera Buffer");
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

	ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)));
}

void Renderer::LoadAssets()
{
	{

#if defined(_DEBUG)
		UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
		UINT compileFlags = 0;
#endif
		ComPtr<IDxcCompiler3> compiler = nullptr;
		ComPtr<IDxcLibrary> library = nullptr;
		ThrowIfFailed(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler)));
		ThrowIfFailed(DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&library)));

		m_vertexShader = CompileShaderFromFile(compiler.Get(), library.Get(), GetAssetFullPath(L"assets\\shaders\\shaders.hlsl").c_str(), L"VSMain", L"vs_6_6");
		m_pixelShader = CompileShaderFromFile(compiler.Get(), library.Get(), GetAssetFullPath(L"assets\\shaders\\shaders.hlsl").c_str(), L"PSMain", L"ps_6_6");
	}
	{
		ThrowIfFailed(m_device->CreateRootSignature(0, m_vertexShader->GetBufferPointer(), m_vertexShader->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
	}

	{
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 3, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
		//psoDesc.pRootSignature = m_rootSignature.Get();
		CD3DX12_DEPTH_STENCIL_DESC depthStencilDesc = {};
		depthStencilDesc.DepthEnable = TRUE;
		depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_GREATER;
		depthStencilDesc.StencilEnable = FALSE;

		psoDesc.DepthStencilState = depthStencilDesc;
		psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		psoDesc.VS = CD3DX12_SHADER_BYTECODE(m_vertexShader->GetBufferPointer(), m_vertexShader->GetBufferSize());
		psoDesc.PS = CD3DX12_SHADER_BYTECODE(m_pixelShader->GetBufferPointer(), m_pixelShader->GetBufferSize());
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.SampleDesc.Count = 1;
		ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
	}

	ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), m_pipelineState.Get(), IID_PPV_ARGS(&m_commandList)));

	ThrowIfFailed(m_commandList->Close());


	{
		m_scene.addObject(std::make_unique<Engine::GLTFSceneObject>(L"assets\\models\\alicev2rigged.glb"));
		m_modelLoader.waitForQueueEmpty();
		m_uploadQueue.execute().get();
	}


	// Create the vertex buffer.
	{
		// Define the geometry for a triangle.
		Vertex triangleVertices[] =
		{
			{ { 0.0f, 0.25f, 0.0f }, },
			{ { 0.25f, -0.25f, 0.0f }, },
			{ { -0.25f, -0.25f, 0.0f }, }
		};

		const UINT vertexBufferSize = sizeof(triangleVertices);

		// Note: using upload heaps to transfer static data like vert buffers is not 
		// recommended. Every time the GPU needs it, the upload heap will be marshalled 
		// over. Please read up on Default Heap usage. An upload heap is used here for 
		// code simplicity and because there are very few verts to actually transfer.
		auto heapUploadProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		auto heapUploadDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
		ThrowIfFailed(m_device->CreateCommittedResource(
			&heapUploadProps,
			D3D12_HEAP_FLAG_NONE,
			&heapUploadDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_vertexBuffer)));

		// Copy the triangle data to the vertex buffer.
		UINT8* pVertexDataBegin;
		CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
		ThrowIfFailed(m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
		memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
		m_vertexBuffer->Unmap(0, nullptr);

		// Initialize the vertex buffer view.
		m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
		m_vertexBufferView.StrideInBytes = sizeof(Vertex);
		m_vertexBufferView.SizeInBytes = vertexBufferSize;
	}

	{
		ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
		m_fenceValue = 1;

		m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (m_fenceEvent == nullptr)
		{
			ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
		}

		WaitForPreviousFrame();
	}
}

void Renderer::OnUpdate()
{

}

static void setCursorToCenterOfTheWindow() {
	HWND hWnd = GetFocus();
	if (!hWnd) return;
	//set cursor to center of window if window is selected
	RECT rect;
	if (!GetClientRect(hWnd, &rect)) return;

	POINT center;
	center.x = (rect.right - rect.left) / 2;
	center.y = (rect.bottom - rect.top) / 2;

	if (!ClientToScreen(hWnd, &center)) return;

	// Set the cursor position to the calculated center
	SetCursorPos(center.x, center.y);
}



void Renderer::OnRender()
{
	{
		this->OnMouseMove();
	}

	{
		this->OnKeyDown();
	}
	PopulateCommandList();

	ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	ThrowIfFailed(m_swapChain->Present(1, 0));

	WaitForPreviousFrame();
}

void Renderer::OnDestroy()
{
	WaitForPreviousFrame();

	CloseHandle(m_fenceEvent);
}

void Renderer::PopulateCommandList()
{
	ThrowIfFailed(m_commandAllocator->Reset());
	ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), m_pipelineState.Get()));

	{
		CBVCameraData cbvData;
		auto cameraProjection = m_camera.getProjectionMatrix();
		auto cameraProjectionReverseDepth = m_camera.getProjectionMatrixForReverseDepth();
		auto cameraView = m_camera.getViewMatrix();
		XMStoreFloat4x4(&cbvData.projectionMatrix, cameraProjection);
		XMStoreFloat4x4(&cbvData.projectionReverseDepthMatrix, cameraProjectionReverseDepth);
		XMStoreFloat4x4(&cbvData.viewMatrix, cameraView);
		XMStoreFloat4x4(&cbvData.viewProjectionReverseDepthMatrix, XMMatrixTranspose(cameraView * cameraProjectionReverseDepth));
		XMStoreFloat4(&cbvData.position, m_camera.getPosition());

		void* mappedData = nullptr;
		m_cameraBuffer->Map(0, nullptr, &mappedData);

		memcpy(mappedData, &cbvData, sizeof(CBVCameraData));

		m_cameraBuffer->Unmap(0, nullptr);
	}





	m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());
	m_commandList->SetGraphicsRootConstantBufferView(0, m_cameraBuffer->GetGPUVirtualAddress());


	m_commandList->RSSetViewports(1, &m_viewport);
	m_commandList->RSSetScissorRects(1, &m_scissorRect);

	auto renderTargetBarrierToRT = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	m_commandList->ResourceBarrier(1, &renderTargetBarrierToRT);

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
	m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

	const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	m_commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 0.0f, 0, 0, nullptr);
	m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	m_scene.render(m_commandList.Get());

	//m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	//m_commandList->DrawInstanced(3, 1, 0, 0);
	auto renderTargetBarrierToSP = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	m_commandList->ResourceBarrier(1, &renderTargetBarrierToSP);

	ThrowIfFailed(m_commandList->Close());
}

void Renderer::WaitForPreviousFrame()
{

	const UINT64 fence = m_fenceValue;
	ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), fence));
	m_fenceValue++;

	if (m_fence->GetCompletedValue() < fence)
	{
		ThrowIfFailed(m_fence->SetEventOnCompletion(fence, m_fenceEvent));
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}

	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
}

void Renderer::OnKeyDown()
{
	HWND hWnd = GetFocus();
	if (!hWnd) return;

	auto cameraPos = m_camera.getPosition();
	auto cameraLookAt = m_camera.getLookAt();
	auto cameraSpeed = 10.0f;

	if (GetAsyncKeyState(VK_SHIFT) & 0x8000) { // Shift key
		cameraSpeed *= 10.0f;
	}

	if (GetAsyncKeyState(VK_LCONTROL) & 0x8000) { // Left Ctrl key
		cameraSpeed *= 0.1f;
	}



	if (GetAsyncKeyState(0x57) & 0x8000) { // W key
		cameraPos += cameraLookAt * cameraSpeed;
	}
	if (GetAsyncKeyState(0x41) & 0x8000) { // A key
		cameraPos += XMVector3Normalize(XMVector3Cross(cameraLookAt, XMVectorSet(0, 1, 0, 0))) * cameraSpeed;
	}
	if (GetAsyncKeyState(0x53) & 0x8000) { // S key
		cameraPos -= cameraLookAt * cameraSpeed;
	}
	if (GetAsyncKeyState(0x44) & 0x8000) { // D key
		cameraPos -= XMVector3Normalize(XMVector3Cross(cameraLookAt, XMVectorSet(0, 1, 0, 0))) * cameraSpeed;;
	}

	m_camera.setPosition(cameraPos);
}

void Renderer::OnMouseMove()
{
	HWND hWnd = GetFocus();
	if (!hWnd) return;
	//set cursor to center of window if window is selected
	RECT rect;
	if (!GetClientRect(hWnd, &rect)) return;

	POINT center;
	center.x = (rect.right - rect.left) / 2;
	center.y = (rect.bottom - rect.top) / 2;

	if (!ClientToScreen(hWnd, &center)) return;

	//get mouse position
	POINT mousePos;
	GetCursorPos(&mousePos);

	int deltaX = mousePos.x - center.x;
	int deltaY = mousePos.y - center.y;

	float speed = 0.1f;

	yaw += -deltaX * speed;
	pitch += -deltaY * speed;
	if (pitch > 89.0f) {
		pitch = 89.0f;
	}
	if (pitch < -89.0f) {
		pitch = -89.0f;
	}
	XMVECTOR cameraPos = m_camera.getPosition();

	XMVECTOR direction = XMVectorSet(
		cosf(XMConvertToRadians(yaw)) * cosf(XMConvertToRadians(pitch)),
		sinf(XMConvertToRadians(pitch)),
		sinf(XMConvertToRadians(yaw)) * cosf(XMConvertToRadians(pitch)),
		1.0f);
	direction = XMVector3Normalize(direction);

	m_camera.setLookAt(direction);

	setCursorToCenterOfTheWindow();
}
