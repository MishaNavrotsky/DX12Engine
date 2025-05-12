#include "stdafx.h"
#include "Renderer.h"

struct CBVCameraData {
	XMFLOAT4X4 projectionMatrix;
	XMFLOAT4X4 projectionReverseDepthMatrix;
	XMFLOAT4X4 viewMatrix;
	XMFLOAT4X4 viewProjectionReverseDepthMatrix;

	XMFLOAT4X4 prevProjectionMatrix;
	XMFLOAT4X4 prevProjectionReverseDepthMatrix;
	XMFLOAT4X4 prevViewMatrix;
	XMFLOAT4X4 prevViewProjectionReverseDepthMatrix;
	XMFLOAT4 position;
	XMUINT4 screenDimensions;
};

Renderer::Renderer(UINT width, UINT height, std::wstring name) :
	DXSample(width, height, name),
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

	m_uploadQueue.registerDevice(m_device);
	m_bindlessHeapDescriptor.registerDevice(m_device);
	Engine::Device::SetDevice(m_device);
	m_gbufferPipeline = std::unique_ptr<Engine::GBufferPipeline>(new Engine::GBufferPipeline(m_device, m_width, m_height));
	m_lightingPipeline = std::unique_ptr<Engine::LightingPipeline>(new Engine::LightingPipeline(m_device, m_width, m_height));



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
	{
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = FrameCount;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));

		m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
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
	ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_commandList)));
	ThrowIfFailed(m_commandList->Close());
	ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
	m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (m_fenceEvent == nullptr)
	{
		ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
	}
}

void Renderer::LoadAssets()
{
	{
		m_scene.addObject(std::make_unique<Engine::GLTFSceneObject>(L"assets\\models\\alicev2rigged.glb"));
		auto o = std::make_unique<Engine::GLTFSceneObject>(L"assets\\models\\alicev2rigged.glb");
		o.get()->modelMatrix.setPosition(8000, 0, 0);
		m_scene.addObject(std::move(o));

		m_modelLoader.waitForQueueEmpty();
		m_uploadQueue.execute().get();
		WaitForCommandQueueExecute();
	}

	{
		ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
		m_fenceValue = 1;

		m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (m_fenceEvent == nullptr)
		{
			ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
		}
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
	m_camera.update();
	PopulateCommandList();


	ThrowIfFailed(m_commandAllocator->Reset());
	ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), nullptr));

	auto uavBuffer = m_lightingPipeline->getOutputTexture();
	auto swapChainBuffer = m_renderTargets[m_swapChain->GetCurrentBackBufferIndex()].Get();

	{
		CD3DX12_RESOURCE_BARRIER uavBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
			uavBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
		CD3DX12_RESOURCE_BARRIER swapChainBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
			swapChainBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_DEST);
		D3D12_RESOURCE_BARRIER barriers[] = { uavBarrier,  swapChainBarrier };
		m_commandList->ResourceBarrier(2, barriers);
	}
	m_commandList->CopyResource(swapChainBuffer, uavBuffer);
	{
		CD3DX12_RESOURCE_BARRIER uavBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
			uavBuffer, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		CD3DX12_RESOURCE_BARRIER swapChainBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
			swapChainBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT);
		D3D12_RESOURCE_BARRIER barriers[] = { uavBarrier,  swapChainBarrier };
		m_commandList->ResourceBarrier(2, barriers);
	}

	ThrowIfFailed(m_commandList->Close());
	m_commandQueue->Wait(m_lightingPipeline->getFence(), m_lightingPipeline->getFenceValue());
	ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	WaitForCommandQueueExecute();
	ThrowIfFailed(m_swapChain->Present(1, 0));
}

void Renderer::OnDestroy()
{
	WaitForCommandQueueExecute();

	CloseHandle(m_fenceEvent);
}

void Renderer::PopulateCommandList()
{
	{
		CBVCameraData cbvData;
		auto cameraProjection = m_camera.getProjectionMatrix();
		auto cameraProjectionReverseDepth = m_camera.getProjectionMatrixForReverseDepth();
		auto cameraView = m_camera.getViewMatrix();
		XMStoreFloat4x4(&cbvData.projectionMatrix, cameraProjection);
		XMStoreFloat4x4(&cbvData.projectionReverseDepthMatrix, cameraProjectionReverseDepth);
		XMStoreFloat4x4(&cbvData.viewMatrix, cameraView);
		XMStoreFloat4x4(&cbvData.viewProjectionReverseDepthMatrix, XMMatrixTranspose(cameraView * cameraProjectionReverseDepth));

		auto cameraPrevProjection = m_camera.getPrevProjectionMatrix();
		auto cameraPrevProjectionReverseDepth = m_camera.getPrevProjectionMatrixForReverseDepth();
		auto cameraPrevView = m_camera.getPrevViewMatrix();
		XMStoreFloat4x4(&cbvData.prevProjectionMatrix, cameraPrevProjection);
		XMStoreFloat4x4(&cbvData.prevProjectionReverseDepthMatrix, cameraPrevProjectionReverseDepth);
		XMStoreFloat4x4(&cbvData.prevViewMatrix, cameraPrevView);
		XMStoreFloat4x4(&cbvData.prevViewProjectionReverseDepthMatrix, XMMatrixTranspose(cameraPrevView * cameraPrevProjectionReverseDepth));

		XMStoreFloat4(&cbvData.position, m_camera.getPosition());
		XMVECTOR dimensions = XMVectorSetInt(m_width, m_height, 0, 0);
		XMStoreUInt4(&cbvData.screenDimensions, dimensions);


		void* mappedData = nullptr;
		m_cameraBuffer->Map(0, nullptr, &mappedData);

		memcpy(mappedData, &cbvData, sizeof(CBVCameraData));

		m_cameraBuffer->Unmap(0, nullptr);
	}

	m_gbufferPipeline->renderGBuffers(m_scene, m_cameraBuffer.Get());
	m_lightingPipeline->computeLighting(m_gbufferPipeline.get(), m_cameraBuffer.Get());

}

void Renderer::WaitForCommandQueueExecute()
{

	const UINT64 fence = m_fenceValue;
	ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), fence));
	m_fenceValue++;

	if (m_fence->GetCompletedValue() < fence)
	{
		ThrowIfFailed(m_fence->SetEventOnCompletion(fence, m_fenceEvent));
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}
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
