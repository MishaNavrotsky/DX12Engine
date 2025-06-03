#include "stdafx.h"

#pragma once

#include <d3dx12.h>
#include <DirectXTex.h>
#include "structures.h"


namespace Engine::Helpers {
	template<size_t N>
	inline std::vector<float> FlattenXMFLOAT3Array(const std::array<DX::XMFLOAT3, N>& arr) {
		std::vector<float> flat;
		flat.reserve(N * 3);
		for (const auto& v : arr) {
			flat.push_back(v.x);
			flat.push_back(v.y);
			flat.push_back(v.z);
		}
		return flat;
	}

	inline std::vector<float> FlattenXMFLOAT3Vector(const std::vector<DX::XMFLOAT3>& vec) {
		std::vector<float> flat;
		flat.reserve(vec.size() * 3);
		for (const auto& v : vec) {
			flat.push_back(v.x);
			flat.push_back(v.y);
			flat.push_back(v.z);
		}
		return flat;
	}

	inline void TransformAABB_ObjectToWorld(const Structures::AABB& objectAABB, const 	DX::XMMATRIX& worldMatrix, Structures::AABB& aabb) {
		using namespace DirectX;
		// Load min and max
		XMVECTOR minV = objectAABB.min;
		XMVECTOR maxV = objectAABB.max;

		// Generate all 8 corners of the AABB
		const XMVECTOR corners[8] = {
			XMVectorSet(minV.m128_f32[0], minV.m128_f32[1], minV.m128_f32[2], 1.0f),
			XMVectorSet(minV.m128_f32[0], minV.m128_f32[1], maxV.m128_f32[2], 1.0f),
			XMVectorSet(minV.m128_f32[0], maxV.m128_f32[1], minV.m128_f32[2], 1.0f),
			XMVectorSet(minV.m128_f32[0], maxV.m128_f32[1], maxV.m128_f32[2], 1.0f),
			XMVectorSet(maxV.m128_f32[0], minV.m128_f32[1], minV.m128_f32[2], 1.0f),
			XMVectorSet(maxV.m128_f32[0], minV.m128_f32[1], maxV.m128_f32[2], 1.0f),
			XMVectorSet(maxV.m128_f32[0], maxV.m128_f32[1], minV.m128_f32[2], 1.0f),
			XMVectorSet(maxV.m128_f32[0], maxV.m128_f32[1], maxV.m128_f32[2], 1.0f),
		};

		// Initialize new min/max
		XMVECTOR transformedMin = XMVectorReplicate(std::numeric_limits<float>::max());
		XMVECTOR transformedMax = XMVectorReplicate(std::numeric_limits<float>::lowest());

		// Transform each corner and expand bounds
		for (int i = 0; i < 8; ++i) {
			XMVECTOR transformed = XMVector3Transform(corners[i], worldMatrix);
			transformedMin = XMVectorMin(transformedMin, transformed);
			transformedMax = XMVectorMax(transformedMax, transformed);
		}

		aabb.min = transformedMin;
		aabb.max = transformedMax;
	}

	inline bool AABBInFrustum(const DX::XMVECTOR* frustumPlanes, const DX::XMFLOAT3& minBounds, const DX::XMFLOAT3& maxBounds)
	{
		using namespace DirectX;

		for (int i = 0; i < 6; i++)
		{
			XMVECTOR plane = frustumPlanes[i];

			// Select the most positive vertex for the plane test
			XMFLOAT3 positiveVertex = {
				(XMVectorGetX(plane) >= 0) ? maxBounds.x : minBounds.x,
				(XMVectorGetY(plane) >= 0) ? maxBounds.y : minBounds.y,
				(XMVectorGetZ(plane) >= 0) ? maxBounds.z : minBounds.z
			};

			// Perform the plane test
			if (XMVectorGetX(plane) * positiveVertex.x +
				XMVectorGetY(plane) * positiveVertex.y +
				XMVectorGetZ(plane) * positiveVertex.z +
				XMVectorGetW(plane) < 0)
			{
				return false; // AABB is outside the frustum
			}
		}
		return true; // AABB is inside or intersecting the frustum
	}

	inline bool AABBInFrustum(const DX::XMVECTOR* frustumPlanes, const DX::XMVECTOR minBounds, const DX::XMVECTOR maxBounds)
	{
		using namespace DirectX;

		for (int i = 0; i < 6; i++)
		{
			XMVECTOR plane = frustumPlanes[i];

			// Select the most positive vertex for the plane test
			XMVECTOR positiveVertex = XMVectorSet(
				(XMVectorGetX(plane) >= 0) ? XMVectorGetX(maxBounds) : XMVectorGetX(minBounds),
				(XMVectorGetY(plane) >= 0) ? XMVectorGetY(maxBounds) : XMVectorGetY(minBounds),
				(XMVectorGetZ(plane) >= 0) ? XMVectorGetZ(maxBounds) : XMVectorGetZ(minBounds),
				1.0f // Homogeneous coordinate
			);

			// Perform the plane test
			if (XMVectorGetX(plane) * XMVectorGetX(positiveVertex) +
				XMVectorGetY(plane) * XMVectorGetY(positiveVertex) +
				XMVectorGetZ(plane) * XMVectorGetZ(positiveVertex) +
				XMVectorGetW(plane) < 0)
			{
				return false; // AABB is outside the frustum
			}
		}
		return true; // AABB is inside or intersecting the frustum
	}

	inline uint64_t Align(uint64_t size, uint64_t alignment) {
		return (size + alignment - 1) & ~(alignment - 1);
	}

	struct DefaultPBRTextures {
		WPtr<ID3D12Resource> baseColor;
		WPtr<ID3D12Resource> metallicRoughness;
		WPtr<ID3D12Resource> normal;
		WPtr<ID3D12Resource> emissive;
		WPtr<ID3D12Resource> occlusion;
	};

	inline WPtr<ID3D12Resource> CreateAndUpload1x1Texture(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList,
		const void* pixelData,
		UINT pixelSize,
		DXGI_FORMAT format,
		WPtr<ID3D12Resource>& uploadBufferOut)
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

		WPtr<ID3D12Resource> texture;
		ThrowIfFailed(device->CreateCommittedResource(
			&defaultHeapProps,
			D3D12_HEAP_FLAG_NONE,
			&texDesc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&texture)));

		// --- Create upload buffer
		uint64_t uploadSize = 0;
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
		WPtr<ID3D12CommandAllocator> cmdAllocator;
		WPtr<ID3D12GraphicsCommandList> cmdList;
		WPtr<ID3D12CommandQueue> queue;

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
		WPtr<ID3D12Resource> uploadBuffer0;
		WPtr<ID3D12Resource> uploadBuffer1;
		WPtr<ID3D12Resource> uploadBuffer2;
		WPtr<ID3D12Resource> uploadBuffer3;
		WPtr<ID3D12Resource> uploadBuffer4;


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
		WPtr<ID3D12Fence> fence;
		ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
		HANDLE fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		uint64_t fenceValue = 1;

		ThrowIfFailed(queue->Signal(fence.Get(), fenceValue));
		if (fence->GetCompletedValue() < fenceValue)
		{
			ThrowIfFailed(fence->SetEventOnCompletion(fenceValue, fenceEvent));
			WaitForSingleObject(fenceEvent, INFINITE);
		}
		CloseHandle(fenceEvent);

		return textures;
	}

	inline std::vector<DX::XMVECTOR> convertToXMVectors(const std::vector<float>& vertices) {
		size_t count = vertices.size() / 3;
		std::vector<DX::XMVECTOR> result;
		result.reserve(count);

		for (size_t i = 0; i < count; ++i) {
			result.push_back(DX::XMVectorSet(
				vertices[i * 3 + 0],
				vertices[i * 3 + 1],
				vertices[i * 3 + 2],
				1.0f  // default w for position
			));
		}

		return result;
	}

	inline WPtr<IDxcBlob> CompileShaderFromFile(IDxcCompiler3* compiler, IDxcLibrary* library, const wchar_t* shaderFilePath, const wchar_t* entryPoint, const wchar_t* targetProfile) {
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

		WPtr<IDxcResult> result;
		ThrowIfFailed(compiler->Compile(
			&sourceBuffer,          // Shader source
			arguments,              // Compilation arguments
			_countof(arguments),    // Number of arguments
			nullptr,                // Include handler (optional)
			IID_PPV_ARGS(&result)   // Output result
		));

		if (sourceBlob) sourceBlob->Release();
		WPtr<IDxcBlob> shaderBlob;
		WPtr<IDxcBlobUtf8> errors;

		auto hr = result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
		if (errors && errors->GetStringLength() > 0) {
			std::cerr << "Shader compilation errors:\n" << errors->GetStringPointer() << std::endl;
		}

		ThrowIfFailed(hr);
		ThrowIfFailed(result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr));

		return shaderBlob;
	}

	inline size_t CalculateBufferSize(const DX::ScratchImage& scratchImage) {
		size_t totalSize = 0;
		const size_t imageCount = scratchImage.GetImageCount();
		const DX::Image* images = scratchImage.GetImages();

		for (size_t i = 0; i < imageCount; ++i) {
			const DX::Image& img = images[i];
			totalSize += img.rowPitch * img.height;
		}

		return totalSize;
	}
}


