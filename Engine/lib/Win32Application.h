#pragma once

#include "DXSample.h"
#include <iostream>
#include "external/imgui.h"
#include "external/imgui_impl_win32.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "ThreadSafeQueue.h"

class DXSample;

class Win32Application
{
public:
    static int Run(DXSample* pSample, HINSTANCE hInstance, int nCmdShow);
    static HWND GetHwnd() { return m_hwnd; }
	static DirectX::Keyboard* GetKeyboard() { return m_keyboard.get(); }
	static DirectX::Mouse* GetMouse() { return m_mouse.get(); }

protected:
    static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
    static char RunMainEventLoop();
	static void RunMainEngineLoop(DXSample* pSample);
    static void CreateMainWindow(RECT windowRect, const WCHAR* title, HINSTANCE hInstance, int nCmdShow);
    static HWND m_hwnd;
    static std::unique_ptr<DirectX::Keyboard> m_keyboard;
    static std::unique_ptr<DirectX::Mouse> m_mouse;
	static std::atomic<bool> m_windowClosed;
	static ThreadSafeQueue<DirectX::Keyboard::State> m_keyboardStateQueue;
	static ThreadSafeQueue<DirectX::Mouse::State> m_mouseStateQueue;

};
