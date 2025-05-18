#include "stdafx.h"

#pragma once

#include "../DXSampleHelper.h"
#include "../Device.h"

namespace Engine::Memory {
	using namespace Microsoft::WRL;

	class Resource {
	public:
		static std::unique_ptr<Resource> Create(D3D12_HEAP_TYPE type, UINT size, D3D12_RESOURCE_STATES state, std::unique_ptr<D3D12_CLEAR_VALUE> clearValue = nullptr, D3D12_HEAP_FLAGS flag = D3D12_HEAP_FLAG_NONE) {
			D3D12_HEAP_PROPERTIES props = CD3DX12_HEAP_PROPERTIES(type);
			D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(size);
			auto resource = new Resource();
			resource->m_heapType = type;
			resource->m_clearValue = std::move(clearValue);
			resource->Initialize(&props, flag, &desc, state);
			resource->saveSize(desc);
			return std::unique_ptr<Resource>(resource);
		}

		static std::unique_ptr<Resource> Create(D3D12_HEAP_PROPERTIES props, D3D12_RESOURCE_DESC desc, D3D12_RESOURCE_STATES state, std::unique_ptr<D3D12_CLEAR_VALUE> clearValue = nullptr, D3D12_HEAP_FLAGS flag = D3D12_HEAP_FLAG_NONE) {
			auto resource = new Resource();
			resource->m_clearValue = std::move(clearValue);
			resource->m_heapType = props.Type;
			resource->Initialize(&props, flag, &desc, state);
			resource->saveSize(desc);
			return std::unique_ptr<Resource>(resource);
		}

		~Resource() {
			totalAllocatedMemory.fetch_sub(m_allocatedSize, std::memory_order_relaxed);
		}

		static UINT64 GetTotalAllocatedMemory() {
			return totalAllocatedMemory.load(std::memory_order_relaxed);
		}

		ID3D12Resource* getResource() const {
			return m_resource.Get();
		}

		UINT64 copyData(ID3D12GraphicsCommandList* cmdList, Resource* src, UINT64 sourceOffset,UINT64 size, UINT64 alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT) {
			// Align current offset
			UINT64 alignedOffset = Align(m_currentOffset, alignment);

			// Ensure space
			if (alignedOffset + size > m_allocatedSize) {
				throw std::runtime_error("Resource: Not enough space to copy data.");
			}

			// Perform GPU copy
			cmdList->CopyBufferRegion(
				this->m_resource.Get(), alignedOffset,
				src->m_resource.Get(), sourceOffset,
				size
			);

			// Advance pointer
			m_currentOffset = alignedOffset + size;

			return alignedOffset;
		}
		UINT64 writeData(const void* data, UINT64 size, UINT64 alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT) {
			if (m_heapType != D3D12_HEAP_TYPE_UPLOAD && m_heapType != D3D12_HEAP_TYPE_GPU_UPLOAD) {
				throw std::runtime_error("WriteData is only valid on UPLOAD or READBACK heap types");
			}

			UINT64 alignedOffset = Align(m_currentOffset, alignment);

			if (alignedOffset + size > m_allocatedSize) {
				throw std::runtime_error("Resource: Not enough space for WriteData.");
			}

			D3D12_RANGE readRange = { 0, 0 };
			void* mappedData = nullptr;
			ThrowIfFailed(m_resource->Map(0, &readRange, &mappedData));

			std::memcpy(static_cast<uint8_t*>(mappedData) + alignedOffset, data, size);

			m_resource->Unmap(0, nullptr);

			m_currentOffset = alignedOffset + size;

			return alignedOffset;
		}

		void readData(void* destination, UINT64 size, UINT64 offset = 0) {
			if (m_heapType != D3D12_HEAP_TYPE_READBACK) {
				throw std::runtime_error("ReadData is only valid on READBACK heap type.");
			}

			if (offset + size > m_allocatedSize) {
				throw std::runtime_error("ReadData out of bounds.");
			}

			D3D12_RANGE readRange = { offset, offset + size };
			void* mappedData = nullptr;
			ThrowIfFailed(m_resource->Map(0, &readRange, &mappedData));

			std::memcpy(destination, static_cast<const uint8_t*>(mappedData) + offset, size);

			m_resource->Unmap(0, nullptr);
		}

		void resetOffset() {
			m_currentOffset = 0;
		}

		D3D12_HEAP_TYPE getType() const {
			return m_heapType;
		}
	private:
		void saveSize(D3D12_RESOURCE_DESC& desc) {
			static auto device = Device::GetDevice();
			D3D12_RESOURCE_ALLOCATION_INFO allocInfo = device->GetResourceAllocationInfo(
				0,
				1,
				&desc
			);
			totalAllocatedMemory.fetch_add(allocInfo.SizeInBytes, std::memory_order_relaxed);
			m_allocatedSize = allocInfo.SizeInBytes;
		}
		Resource() = default; // Private constructor (only Create() can instantiate)
		void Initialize(_In_  const D3D12_HEAP_PROPERTIES* pHeapProperties,
			D3D12_HEAP_FLAGS HeapFlags,
			_In_  const D3D12_RESOURCE_DESC* pDesc,
			D3D12_RESOURCE_STATES InitialResourceState) {

			static auto device = Device::GetDevice();

			ThrowIfFailed(device->CreateCommittedResource(pHeapProperties, HeapFlags, pDesc, InitialResourceState, m_clearValue.get(), IID_PPV_ARGS(&m_resource)));
		}

		ComPtr<ID3D12Resource> m_resource;

		UINT64 m_allocatedSize = 0;
		std::unique_ptr<D3D12_CLEAR_VALUE> m_clearValue;

		UINT64 m_currentOffset = 0;
		D3D12_HEAP_TYPE m_heapType = D3D12_HEAP_TYPE_DEFAULT;

		static inline std::atomic<UINT64> totalAllocatedMemory{ 0 }; // Tracks total memory usage
	};
}
