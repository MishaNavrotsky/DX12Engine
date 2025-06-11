#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers.
#endif
#ifndef NOMINMAX
# define NOMINMAX
#endif
#include <windows.h>
#include <initguid.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include "include/d3dx12/d3dx12.h"
#include <dxcapi.h>
#include <dstorage.h>
#include <objbase.h>
#include <vector>
#include <functional>
#include <syncstream>
#include <filesystem>
#include <thread>
#include <mutex>
#include <atomic>
#include <future>
#include <iostream>
#include <set>
#include <map>
#include <unordered_set>
#include <iomanip>
#include <span>
#include <array>
#include <variant>
#include <string>
#include <wrl.h>
#include <shellapi.h>
#include <stdexcept>
#include <comdef.h>
#include "ftl/task_scheduler.h"
#include "ftl/wait_group.h"
#include "external/unordered_dense.h"
#include <tbb/concurrent_unordered_map.h>
#include <tbb/concurrent_vector.h>
#include <tbb/concurrent_queue.h>
#include <tbb/scalable_allocator.h>
#include <external/entt/entt.hpp>
#include <typeindex>


static std::string HrToString(HRESULT hr)
{
	char s_str[64] = {};
	sprintf_s(s_str, "HRESULT of 0x%08X", static_cast<UINT>(hr));
	return std::string(s_str);
}

class HrException : public std::runtime_error
{
public:
	HrException(HRESULT hr) : std::runtime_error(HrToString(hr)), m_hr(hr) {}
	HRESULT Error() const { return m_hr; }
private:
	const HRESULT m_hr;
};


static void ThrowIfFailed(HRESULT hr)
{
	if (FAILED(hr))
	{
		_com_error err(hr);
		const wchar_t* errMsg = err.ErrorMessage();
		wprintf(L"Error: %s\n", errMsg);

		throw HrException(hr);
	}
}

static void GetAssetsPath(_Out_writes_(pathSize) WCHAR* path, UINT pathSize)
{
	if (path == nullptr)
	{
		throw std::exception();
	}

	DWORD size = GetModuleFileName(nullptr, path, pathSize);
	if (size == 0 || size == pathSize)
	{
		throw std::exception();
	}

	WCHAR* lastSlash = wcsrchr(path, L'\\');
	if (lastSlash)
	{
		*(lastSlash + 1) = L'\0';
	}
}
static auto programStart = std::chrono::high_resolution_clock::now();


static void PrintCurrentTimestampNS() {
	auto now = std::chrono::high_resolution_clock::now();
	float ms = std::chrono::duration<float, std::milli>(now - programStart).count();
	std::cout << "Elapsed time since program start: " << ms << " ms" << std::endl;
}

template<typename T>
using WPtr = Microsoft::WRL::ComPtr<T>;

namespace DX = DirectX;