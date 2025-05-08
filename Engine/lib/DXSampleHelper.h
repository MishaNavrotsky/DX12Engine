#include "stdafx.h"

#pragma once
#include <stdexcept>
#include <comdef.h>
#include <iostream>

using Microsoft::WRL::ComPtr;

inline std::string HrToString(HRESULT hr)
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

#define SAFE_RELEASE(p) if (p) (p)->Release()

inline void ThrowIfFailed(HRESULT hr)
{
	if (FAILED(hr))
	{
		_com_error err(hr);
		const wchar_t* errMsg = err.ErrorMessage();
		wprintf(L"Error: %s\n", errMsg);

		throw HrException(hr);
	}
}

inline void GetAssetsPath(_Out_writes_(pathSize) WCHAR* path, UINT pathSize)
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

inline HRESULT ReadDataFromFile(LPCWSTR filename, byte** data, UINT* size)
{
	using namespace Microsoft::WRL;

#if WINVER >= _WIN32_WINNT_WIN8
	CREATEFILE2_EXTENDED_PARAMETERS extendedParams = {};
	extendedParams.dwSize = sizeof(CREATEFILE2_EXTENDED_PARAMETERS);
	extendedParams.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
	extendedParams.dwFileFlags = FILE_FLAG_SEQUENTIAL_SCAN;
	extendedParams.dwSecurityQosFlags = SECURITY_ANONYMOUS;
	extendedParams.lpSecurityAttributes = nullptr;
	extendedParams.hTemplateFile = nullptr;

	Wrappers::FileHandle file(CreateFile2(filename, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, &extendedParams));
#else
	Wrappers::FileHandle file(CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN | SECURITY_SQOS_PRESENT | SECURITY_ANONYMOUS, nullptr));
#endif
	if (file.Get() == INVALID_HANDLE_VALUE)
	{
		throw std::exception();
	}

	FILE_STANDARD_INFO fileInfo = {};
	if (!GetFileInformationByHandleEx(file.Get(), FileStandardInfo, &fileInfo, sizeof(fileInfo)))
	{
		throw std::exception();
	}

	if (fileInfo.EndOfFile.HighPart != 0)
	{
		throw std::exception();
	}

	*data = reinterpret_cast<byte*>(malloc(fileInfo.EndOfFile.LowPart));
	*size = fileInfo.EndOfFile.LowPart;

	if (!ReadFile(file.Get(), *data, fileInfo.EndOfFile.LowPart, nullptr, nullptr))
	{
		throw std::exception();
	}

	return S_OK;
}

inline HRESULT ReadDataFromDDSFile(LPCWSTR filename, byte** data, UINT* offset, UINT* size)
{
	if (FAILED(ReadDataFromFile(filename, data, size)))
	{
		return E_FAIL;
	}

	static const UINT DDS_MAGIC = 0x20534444;
	UINT magicNumber = *reinterpret_cast<const UINT*>(*data);
	if (magicNumber != DDS_MAGIC)
	{
		return E_FAIL;
	}

	struct DDS_PIXELFORMAT
	{
		UINT size;
		UINT flags;
		UINT fourCC;
		UINT rgbBitCount;
		UINT rBitMask;
		UINT gBitMask;
		UINT bBitMask;
		UINT aBitMask;
	};

	struct DDS_HEADER
	{
		UINT size;
		UINT flags;
		UINT height;
		UINT width;
		UINT pitchOrLinearSize;
		UINT depth;
		UINT mipMapCount;
		UINT reserved1[11];
		DDS_PIXELFORMAT ddsPixelFormat;
		UINT caps;
		UINT caps2;
		UINT caps3;
		UINT caps4;
		UINT reserved2;
	};

	auto ddsHeader = reinterpret_cast<const DDS_HEADER*>(*data + sizeof(UINT));
	if (ddsHeader->size != sizeof(DDS_HEADER) || ddsHeader->ddsPixelFormat.size != sizeof(DDS_PIXELFORMAT))
	{
		return E_FAIL;
	}

	const ptrdiff_t ddsDataOffset = sizeof(UINT) + sizeof(DDS_HEADER);
	*offset = ddsDataOffset;
	*size = *size - ddsDataOffset;

	return S_OK;
}

#if defined(_DEBUG) || defined(DBG)
inline void SetName(ID3D12Object* pObject, LPCWSTR name)
{
	pObject->SetName(name);
}
inline void SetNameIndexed(ID3D12Object* pObject, LPCWSTR name, UINT index)
{
	WCHAR fullName[50];
	if (swprintf_s(fullName, L"%s[%u]", name, index) > 0)
	{
		pObject->SetName(fullName);
	}
}
#else
inline void SetName(ID3D12Object*, LPCWSTR)
{
}
inline void SetNameIndexed(ID3D12Object*, LPCWSTR, UINT)
{
}
#endif

#define NAME_D3D12_OBJECT(x) SetName((x).Get(), L#x)
#define NAME_D3D12_OBJECT_INDEXED(x, n) SetNameIndexed((x)[n].Get(), L#x, n)

inline UINT CalculateConstantBufferByteSize(UINT byteSize)
{
	return (byteSize + (D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1)) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1);
}

#ifdef D3D_COMPILE_STANDARD_FILE_INCLUDE
inline Microsoft::WRL::ComPtr<ID3DBlob> CompileShader(
	const std::wstring& filename,
	const D3D_SHADER_MACRO* defines,
	const std::string& entrypoint,
	const std::string& target)
{
	UINT compileFlags = 0;
#if defined(_DEBUG) || defined(DBG)
	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	HRESULT hr;

	Microsoft::WRL::ComPtr<ID3DBlob> byteCode = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> errors;
	hr = D3DCompileFromFile(filename.c_str(), defines, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		entrypoint.c_str(), target.c_str(), compileFlags, 0, &byteCode, &errors);

	if (errors != nullptr)
	{
		OutputDebugStringA((char*)errors->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	return byteCode;
}
#endif

template<class T>
void ResetComPtrArray(T* comPtrArray)
{
	for (auto& i : *comPtrArray)
	{
		i.Reset();
	}
}


template<class T>
void ResetUniquePtrArray(T* uniquePtrArray)
{
	for (auto& i : *uniquePtrArray)
	{
		i.reset();
	}
}


inline HRESULT CompileShaderWithMessage(const wchar_t* shaderFilePath, const char* entryPoint, const char* target, const UINT flags, ID3DBlob** shaderBlob) {
	ID3DBlob* errorBlob = nullptr;

	HRESULT hr = D3DCompileFromFile(
		shaderFilePath, nullptr, nullptr, entryPoint, target, flags, 0, shaderBlob, &errorBlob);

	if (FAILED(hr)) {
		if (errorBlob) {
			std::cerr << "Shader compilation error: " << static_cast<const char*>(errorBlob->GetBufferPointer()) << std::endl;
			errorBlob->Release();
		}
		else {
			_com_error err(hr);
			std::wcerr << L"Shader compilation failed with HRESULT: " << err.ErrorMessage() << std::endl;
		}
	}

	return hr;
}

inline ComPtr<IDxcBlob> CompileShaderFromFile(IDxcCompiler3* compiler, IDxcLibrary* library, const wchar_t* shaderFilePath, const wchar_t* entryPoint, const wchar_t* targetProfile) {
	IDxcBlobEncoding* sourceBlob = nullptr;
	ThrowIfFailed(library->CreateBlobFromFile(shaderFilePath, nullptr, &sourceBlob));

	DxcBuffer sourceBuffer = {};
	sourceBuffer.Ptr = sourceBlob->GetBufferPointer();
	sourceBuffer.Size = sourceBlob->GetBufferSize();
	sourceBuffer.Encoding = DXC_CP_UTF8;
	const wchar_t* arguments[] = {
		L"-E", entryPoint,
		L"-T", targetProfile,
		L"-Zi",
		L"-Fd", L"shader.pdb",
		L"-Qembed_debug"// Specify PDB output file name
	};

	ComPtr<IDxcResult> result;
	ThrowIfFailed(compiler->Compile(
		&sourceBuffer,          // Shader source
		arguments,              // Compilation arguments
		_countof(arguments),    // Number of arguments
		nullptr,                // Include handler (optional)
		IID_PPV_ARGS(&result)   // Output result
	));

	if (sourceBlob) sourceBlob->Release();
	ComPtr<IDxcBlob> shaderBlob;
	ComPtr<IDxcBlobUtf8> errors;

	auto hr = result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
	if (errors && errors->GetStringLength() > 0) {
		std::cerr << "Shader compilation errors:\n" << errors->GetStringPointer() << std::endl;
	}

	ThrowIfFailed(hr);
	ThrowIfFailed(result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr));

	return shaderBlob;
}


struct DefaultPBRTextures {
	ComPtr<ID3D12Resource> baseColor;
	ComPtr<ID3D12Resource> metallicRoughness;
	ComPtr<ID3D12Resource> normal;
	ComPtr<ID3D12Resource> emissive;
	ComPtr<ID3D12Resource> occlusion;
};

inline ComPtr<ID3D12Resource> CreateAndUpload1x1Texture(
	ID3D12Device* device,
	ID3D12GraphicsCommandList* cmdList,
	const void* pixelData,
	UINT pixelSize,
	DXGI_FORMAT format,
	ComPtr<ID3D12Resource>& uploadBufferOut)
{
	// --- Create default heap texture (GPU)
	D3D12_RESOURCE_DESC texDesc = {};
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Width = 1;
	texDesc.Height = 1;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.Format = format;
	texDesc.SampleDesc.Count = 1;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	D3D12_HEAP_PROPERTIES defaultHeapProps = {};
	defaultHeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

	ComPtr<ID3D12Resource> texture;
	ThrowIfFailed(device->CreateCommittedResource(
		&defaultHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&texture)));

	// --- Create upload buffer
	UINT64 uploadSize = 0;
	device->GetCopyableFootprints(&texDesc, 0, 1, 0, nullptr, nullptr, nullptr, &uploadSize);

	D3D12_HEAP_PROPERTIES uploadHeapProps = {};
	uploadHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

	auto uploadDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadSize);
	ThrowIfFailed(device->CreateCommittedResource(
		&uploadHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&uploadDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&uploadBufferOut))); // Pass the upload buffer to the caller

	// --- Copy data to upload buffer
	uint8_t* mappedData = nullptr;
	ThrowIfFailed(uploadBufferOut->Map(0, nullptr, reinterpret_cast<void**>(&mappedData)));
	memcpy(mappedData, pixelData, pixelSize);
	uploadBufferOut->Unmap(0, nullptr);

	// --- Upload to GPU texture
	D3D12_SUBRESOURCE_DATA subresource = {};
	subresource.pData = pixelData;
	subresource.RowPitch = pixelSize;
	subresource.SlicePitch = pixelSize;

	UpdateSubresources(cmdList, texture.Get(), uploadBufferOut.Get(), 0, 0, 1, &subresource);

	// --- Transition texture to shader resource state
	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		texture.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	cmdList->ResourceBarrier(1, &barrier);

	return texture;
}

inline DefaultPBRTextures CreateDefaultPBRTextures(ID3D12Device* m_device)
{
	// Create command objects
	ComPtr<ID3D12CommandAllocator> cmdAllocator;
	ComPtr<ID3D12GraphicsCommandList> cmdList;
	ComPtr<ID3D12CommandQueue> queue;

	ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAllocator)));
	ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAllocator.Get(), nullptr, IID_PPV_ARGS(&cmdList)));

	D3D12_COMMAND_QUEUE_DESC qDesc = {};
	qDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	ThrowIfFailed(m_device->CreateCommandQueue(&qDesc, IID_PPV_ARGS(&queue)));

	// Pixel data
	const uint8_t baseColor[4] = { 255, 255, 255, 255 };
	const uint8_t mrPixel[4] = { 255, 128, 0, 255 };
	const uint8_t normalPixel[4] = { 128, 128, 255, 255 };
	const uint8_t emissivePixel[4] = { 0, 0, 0, 255 };
	const uint8_t occlusionPixel[1] = { 255 };

	// Define a struct to hold the upload buffer
	ComPtr<ID3D12Resource> uploadBuffer0;
	ComPtr<ID3D12Resource> uploadBuffer1;
	ComPtr<ID3D12Resource> uploadBuffer2;
	ComPtr<ID3D12Resource> uploadBuffer3;
	ComPtr<ID3D12Resource> uploadBuffer4;


	// Initialize textures using the upload buffer
	DefaultPBRTextures textures;
	textures.baseColor = CreateAndUpload1x1Texture(m_device, cmdList.Get(), baseColor, sizeof(baseColor), DXGI_FORMAT_R8G8B8A8_UNORM, uploadBuffer0);
	textures.metallicRoughness = CreateAndUpload1x1Texture(m_device, cmdList.Get(), mrPixel, sizeof(mrPixel), DXGI_FORMAT_R8G8B8A8_UNORM, uploadBuffer1);
	textures.normal = CreateAndUpload1x1Texture(m_device, cmdList.Get(), normalPixel, sizeof(normalPixel), DXGI_FORMAT_R8G8B8A8_UNORM, uploadBuffer2);
	textures.emissive = CreateAndUpload1x1Texture(m_device, cmdList.Get(), emissivePixel, sizeof(emissivePixel), DXGI_FORMAT_R8G8B8A8_UNORM, uploadBuffer3);
	textures.occlusion = CreateAndUpload1x1Texture(m_device, cmdList.Get(), occlusionPixel, sizeof(occlusionPixel), DXGI_FORMAT_R8_UNORM, uploadBuffer4);

	// Finish and execute
	ThrowIfFailed(cmdList->Close());
	ID3D12CommandList* lists[] = { cmdList.Get() };
	queue->ExecuteCommandLists(1, lists);

	// Wait for GPU to finish
	ComPtr<ID3D12Fence> fence;
	ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
	HANDLE fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	UINT64 fenceValue = 1;

	ThrowIfFailed(queue->Signal(fence.Get(), fenceValue));
	if (fence->GetCompletedValue() < fenceValue)
	{
		ThrowIfFailed(fence->SetEventOnCompletion(fenceValue, fenceEvent));
		WaitForSingleObject(fenceEvent, INFINITE);
	}
	CloseHandle(fenceEvent);

	return textures;
}