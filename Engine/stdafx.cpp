#include "stdafx.h"

//static std::string HrToString(HRESULT hr)
//{
//	char s_str[64] = {};
//	sprintf_s(s_str, "HRESULT of 0x%08X", static_cast<UINT>(hr));
//	return std::string(s_str);
//}
//
//class HrException : public std::runtime_error
//{
//public:
//	HrException(HRESULT hr) : std::runtime_error(HrToString(hr)), m_hr(hr) {}
//	HRESULT Error() const { return m_hr; }
//private:
//	const HRESULT m_hr;
//};
//
//
//static void ThrowIfFailed(HRESULT hr)
//{
//	if (FAILED(hr))
//	{
//		_com_error err(hr);
//		const wchar_t* errMsg = err.ErrorMessage();
//		wprintf(L"Error: %s\n", errMsg);
//
//		throw HrException(hr);
//	}
//}
//
//static void GetAssetsPath(_Out_writes_(pathSize) WCHAR* path, UINT pathSize)
//{
//	if (path == nullptr)
//	{
//		throw std::exception();
//	}
//
//	DWORD size = GetModuleFileName(nullptr, path, pathSize);
//	if (size == 0 || size == pathSize)
//	{
//		throw std::exception();
//	}
//
//	WCHAR* lastSlash = wcsrchr(path, L'\\');
//	if (lastSlash)
//	{
//		*(lastSlash + 1) = L'\0';
//	}
//}