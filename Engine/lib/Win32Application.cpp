#include "stdafx.h"
#include "Win32Application.h"


extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

HWND Win32Application::m_hwnd = nullptr;
std::unique_ptr<DX::Keyboard> Win32Application::m_keyboard = std::make_unique<DX::Keyboard>();
std::unique_ptr<DX::Mouse> Win32Application::m_mouse = std::make_unique<DX::Mouse>();
ThreadSafeQueue<DX::Keyboard::State> Win32Application::m_keyboardStateQueue;
ThreadSafeQueue<DX::Mouse::State> Win32Application::m_mouseStateQueue;

std::atomic<bool> Win32Application::m_windowClosed = false;

void Win32Application::RunMainEngineLoop(Engine::Engine* engine)
{
	using namespace std::chrono;
	auto previousTime = high_resolution_clock::now();


	while (!m_windowClosed.load(std::memory_order_relaxed))
	{
		auto currentTime = high_resolution_clock::now();
		auto deltaTime = duration<float, std::milli>(currentTime - previousTime).count();
		previousTime = currentTime;

		auto keyboardStates = m_keyboardStateQueue.popAll();
		auto mouseStates = m_mouseStateQueue.popAll();
		if (keyboardStates.size() > 0)
		{
			engine->onKeyboardUpdate(keyboardStates.back());

		}
		if (mouseStates.size() > 0)
		{
			engine->onMouseUpdate(mouseStates.back());
		}

		engine->update(deltaTime);

		//sleap
		std::this_thread::sleep_for(std::chrono::milliseconds(30)); // Roughly 60 FPS
	}
}

void Win32Application::CreateMainWindow(RECT windowRect, const WCHAR* title, HINSTANCE hInstance, int nCmdShow)
{
	WNDCLASSEX windowClass = { 0 };
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = WindowProc;
	windowClass.hInstance = hInstance;
	windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	windowClass.lpszClassName = L"DXSampleClass";
	RegisterClassEx(&windowClass);

	m_hwnd = CreateWindow(
		windowClass.lpszClassName,
		title,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		windowRect.right - windowRect.left,
		windowRect.bottom - windowRect.top,
		nullptr,        // We have no parent window.
		nullptr,        // We aren't using menus.
		hInstance,
		nullptr);

	ShowWindow(m_hwnd, nCmdShow);
}

char Win32Application::RunMainEventLoop()
{
	MSG msg = { 0 };
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return static_cast<char>(msg.wParam);
}

int Win32Application::Run(Engine::Engine* engine, HINSTANCE hInstance, int nCmdShow)
{
	int argc;
	LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	engine->parseCommandLineArgs(argv, argc);
	LocalFree(argv);

	RECT windowRect = { 0, 0, static_cast<LONG>(engine->getWidth()), static_cast<LONG>(engine->getHeight()) };
	AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

	CreateMainWindow(windowRect, engine->getTitle().c_str(), hInstance, nCmdShow);
	m_mouse->SetWindow(m_hwnd);
	ShowCursor(true);

	engine->initialize(m_hwnd);

	std::thread mainEngineLoopThread(RunMainEngineLoop, engine);

	auto exitCode = RunMainEventLoop();
	m_windowClosed.store(true);

	mainEngineLoopThread.join();

	engine->destroy();

	return exitCode;
}

// Main message handler for the sample.
LRESULT CALLBACK Win32Application::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (ImGui::GetCurrentContext()) {
		auto& io = ImGui::GetIO();
		if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam)) {
			if (io.WantCaptureMouse)
			{
				return 0;
			}
		}
	}

	switch (message)
	{
	case WM_CREATE:
	{
	}
	return 0;
	case WM_ACTIVATEAPP:
		m_keyboard->ProcessMessage(message, wParam, lParam);
		m_mouse->ProcessMessage(message, wParam, lParam);
		m_keyboardStateQueue.push(m_keyboard->GetState());
		m_mouseStateQueue.push(m_mouse->GetState());
		break;
	case WM_ACTIVATE:
	case WM_INPUT:
	case WM_MOUSEMOVE:
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_MOUSEWHEEL:
	case WM_XBUTTONDOWN:
	case WM_XBUTTONUP:
	case WM_MOUSEHOVER:
		m_mouse->ProcessMessage(message, wParam, lParam);
		m_mouseStateQueue.push(m_mouse->GetState());
		break;

	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_SYSKEYUP:
		m_keyboard->ProcessMessage(message, wParam, lParam);
		m_keyboardStateQueue.push(m_keyboard->GetState());
		break;

	case WM_SYSKEYDOWN:
		m_keyboard->ProcessMessage(message, wParam, lParam);
		m_keyboardStateQueue.push(m_keyboard->GetState());
		if (wParam == VK_RETURN && (lParam & 0x60000000) == 0x20000000)
		{

		}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	// Handle any messages the switch statement didn't.
	return DefWindowProc(hWnd, message, wParam, lParam);
}
