#pragma once

#include "DXSample.h"
#include <iostream>
#include "external/imgui.h"
#include "external/imgui_impl_win32.h"
#include "Keyboard.h"
#include "Mouse.h"

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
    static HWND m_hwnd;
    static std::unique_ptr<DirectX::Keyboard> m_keyboard;
    static std::unique_ptr<DirectX::Mouse> m_mouse;
};
