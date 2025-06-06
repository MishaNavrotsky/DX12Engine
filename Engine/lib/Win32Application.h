#pragma once

#include <iostream>
#include "external/imgui.h"
#include "external/imgui_impl_win32.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "./lib/Engine.h"


class Win32Application
{
public:
    static int Run(Engine::Engine* engine, HINSTANCE hInstance, int nCmdShow);
    static HWND GetHwnd() { return m_hwnd; }
	static DX::Keyboard* GetKeyboard() { return m_keyboard.get(); }
	static DX::Mouse* GetMouse() { return m_mouse.get(); }

protected:
    static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
    static char RunMainEventLoop();
	static void RunMainEngineLoop(Engine::Engine* engine);
    static void CreateMainWindow(RECT windowRect, const WCHAR* title, HINSTANCE hInstance, int nCmdShow);
    static HWND m_hwnd;
    static std::unique_ptr<DX::Keyboard> m_keyboard;
    static std::unique_ptr<DX::Mouse> m_mouse;
	static std::atomic<bool> m_windowClosed;
	static tbb::concurrent_queue<DX::Keyboard::State> m_keyboardStateQueue[2];
	static tbb::concurrent_queue<DX::Mouse::State> m_mouseStateQueue[2];
    static std::atomic<uint32_t> m_frontBufferIndex;

};
