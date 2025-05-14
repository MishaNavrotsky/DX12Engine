#include "stdafx.h"
#include "Renderer.h"

Renderer::Renderer(UINT width, UINT height, std::wstring name) :
	DXSample(width, height, name),
	m_rtvDescriptorSize(0)
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
	m_gbufferPass = std::unique_ptr<Engine::GBufferPass>(new Engine::GBufferPass(m_device, m_width, m_height));
	m_lightingPass = std::unique_ptr<Engine::LightingPass>(new Engine::LightingPass(m_device, m_width, m_height));
	m_gizmosPass = std::unique_ptr<Engine::GizmosPass>(new Engine::GizmosPass(m_device, m_width, m_height));

	m_camera = std::unique_ptr<Engine::Camera>(new Engine::Camera(XMConvertToRadians(60.0f), m_width, m_height, 0.1f, 100000.0f));



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
		const wchar_t* strings[] = { L"brick_wall.glb", L"cute_anime_girl_mage.glb", L"furniture_decor_sculpture_8mb.glb", L"hand_low_poly.glb", L"star_wars_galaxies_-_eta-2_actis_interceptor.glb", L"su-33_flanker-d.glb" };
		//for (uint32_t i = 0; i < std::size(strings); i++) {
		//	std::wstring prefix = L"assets\\models\\";
		//	auto result = prefix + strings[i];
		//	m_scene.addNode(Engine::GLTFModelSceneNode::ReadFromFile(result));
		//}

		m_scene.addNode(Engine::GLTFModelSceneNode::ReadFromFile(L"assets\\models\\alicev2rigged.glb"));
		auto o = Engine::GLTFModelSceneNode::ReadFromFile(L"assets\\models\\alicev2rigged_c.glb");
		Engine::ModelMatrix modelMatrix;
		modelMatrix.setPosition(8000, 0, 0);
		modelMatrix.update();
		o.get()->setLocalModelMatrix(modelMatrix);
		m_scene.addNode(o);

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
	m_camera->update();
	PopulateCommandList();


	ThrowIfFailed(m_commandAllocator->Reset());
	ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), nullptr));

	auto uavBuffer = m_lightingPass->getOutputTexture();
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
	m_commandQueue->Wait(m_lightingPass->getFence(), m_lightingPass->getFenceValue());
	ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	WaitForCommandQueueExecute();
	ThrowIfFailed(m_swapChain->Present(0, 0));
}

void Renderer::OnDestroy()
{
	WaitForCommandQueueExecute();

	CloseHandle(m_fenceEvent);
}

void Renderer::PopulateCommandList()
{
	m_gbufferPass->renderGBuffers(&m_scene, m_camera.get());
	m_lightingPass->computeLighting(m_gbufferPass.get(), m_camera.get());
	{
		m_lightingPass->waitForGPU();
		m_gizmosPass->renderGizmos(&m_scene, m_camera.get(), m_gbufferPass->getDepthStencilResource());
	}
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

	auto cameraPos = m_camera->getPosition();
	auto cameraLookAt = m_camera->getLookAt();
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

	m_camera->setPosition(cameraPos);
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
	XMVECTOR cameraPos = m_camera->getPosition();

	XMVECTOR direction = XMVectorSet(
		cosf(XMConvertToRadians(yaw)) * cosf(XMConvertToRadians(pitch)),
		sinf(XMConvertToRadians(pitch)),
		sinf(XMConvertToRadians(yaw)) * cosf(XMConvertToRadians(pitch)),
		1.0f);
	direction = XMVector3Normalize(direction);

	m_camera->setLookAt(direction);

	setCursorToCenterOfTheWindow();
}
