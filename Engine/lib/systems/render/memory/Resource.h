#include "stdafx.h"

#pragma once

#include "../../../helpers.h"
#include "../Device.h"

namespace Engine::Render::Memory {
	enum class ResourceTypes {
		Commited,
		Placed,
		Reserved,
	};
	class Resource {

	public:
		struct Handle {
			uint32_t index;
			uint32_t generation;

			bool operator==(const Handle& other) const {
				return index == other.index && generation == other.generation;
			}
		};
		using PackedHandle = uint64_t;

		static inline PackedHandle PackHandle(uint32_t index, uint32_t generation) {
			return (static_cast<uint64_t>(generation) << 32) | index;
		}

		static inline uint32_t ExtractIndex(PackedHandle handle) {
			return static_cast<uint32_t>(handle & 0xFFFFFFFF);
		}

		static inline uint32_t ExtractGeneration(PackedHandle handle) {
			return static_cast<uint32_t>(handle >> 32);
		}
		Resource(const Resource&) = delete;
		Resource& operator=(const Resource&) = delete;
		Resource(Resource&& other) noexcept {
			moveFrom(std::move(other));
		}

		Resource& operator=(Resource&& other) noexcept {
			if (this != &other) {
				moveFrom(std::move(other));
			}
			return *this;
		}
		static std::unique_ptr<Resource> Create(D3D12_HEAP_TYPE type, UINT size, D3D12_RESOURCE_STATES state, const D3D12_CLEAR_VALUE* clearValue = nullptr, D3D12_HEAP_FLAGS flag = D3D12_HEAP_FLAG_NONE) {
			D3D12_HEAP_PROPERTIES props = CD3DX12_HEAP_PROPERTIES(type);
			D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(size);
			auto resource = new Resource();
			resource->m_heapType = type;
			resource->setClearValue(clearValue);
			resource->Initialize(&props, flag, &desc, state);
			resource->saveSize(desc);
			return std::unique_ptr<Resource>(resource);
		}

		static Resource CreateV(D3D12_HEAP_TYPE type, UINT size, D3D12_RESOURCE_STATES state, const D3D12_CLEAR_VALUE* clearValue = nullptr, D3D12_HEAP_FLAGS flag = D3D12_HEAP_FLAG_NONE) {
			D3D12_HEAP_PROPERTIES props = CD3DX12_HEAP_PROPERTIES(type);
			D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(size);
			Resource resource;
			resource.m_heapType = type;
			resource.setClearValue(clearValue);
			resource.Initialize(&props, flag, &desc, state);
			resource.saveSize(desc);
			return resource;
		}

		static std::unique_ptr<Resource> Create(D3D12_HEAP_PROPERTIES props, D3D12_RESOURCE_DESC desc, D3D12_RESOURCE_STATES state, const D3D12_CLEAR_VALUE* clearValue = nullptr, D3D12_HEAP_FLAGS flag = D3D12_HEAP_FLAG_NONE) {
			auto resource = new Resource();
			resource->setClearValue(clearValue);
			resource->m_heapType = props.Type;
			resource->Initialize(&props, flag, &desc, state);
			resource->saveSize(desc);
			return std::unique_ptr<Resource>(resource);
		}

		static Resource CreateV(D3D12_HEAP_PROPERTIES props, D3D12_RESOURCE_DESC desc, D3D12_RESOURCE_STATES state, const D3D12_CLEAR_VALUE* clearValue = nullptr, D3D12_HEAP_FLAGS flag = D3D12_HEAP_FLAG_NONE) {
			Resource resource;
			resource.setClearValue(clearValue);
			resource.m_heapType = props.Type;
			resource.Initialize(&props, flag, &desc, state);
			resource.saveSize(desc);
			return resource;
		}

		ID3D12Resource* getResource() const {
			return m_resource.Get();
		}

		uint64_t copyData(ID3D12GraphicsCommandList* cmdList, Resource* src, uint64_t sourceOffset, uint64_t size, uint64_t alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT) {
			uint64_t alignedOffset = Helpers::Align(m_currentOffset, alignment);

			if (alignedOffset + size > m_allocatedSize) {
				throw std::runtime_error("Resource: Not enough space to copy data.");
			}

			cmdList->CopyBufferRegion(
				this->m_resource.Get(), alignedOffset,
				src->m_resource.Get(), sourceOffset,
				size
			);

			m_currentOffset = alignedOffset + size;

			return alignedOffset;
		}

		void copyResource(ID3D12GraphicsCommandList* cmdList, Resource* src) {
			cmdList->CopyResource(m_resource.Get(), src->m_resource.Get());

			m_allocatedSize = src->m_allocatedSize;
			m_currentOffset = src->m_currentOffset;
		}

		uint64_t writeData(const void* data, uint64_t size, uint64_t alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT) {
			if (m_heapType != D3D12_HEAP_TYPE_UPLOAD && m_heapType != D3D12_HEAP_TYPE_GPU_UPLOAD) {
				throw std::runtime_error("WriteData is only valid on UPLOAD or READBACK heap types");
			}

			uint64_t alignedOffset = Helpers::Align(m_currentOffset, alignment);

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

		void readData(void* destination, uint64_t size, uint64_t offset = 0) {
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

		uint64_t getSize() const {
			return m_allocatedSize;
		}

		ResourceTypes getResourceType() const {
			return m_resourceType;
		}

		uint64_t getCurrentOffset() const {
			return m_currentOffset;
		}

		void resetOffset() {
			m_currentOffset = 0;
		}

		D3D12_HEAP_TYPE getHeapType() const {
			return m_heapType;
		}

		D3D12_CLEAR_VALUE* getClearValue() const {
			return m_clearValue.get();
		}

		uint64_t getPlacedHeapId() const {
			return m_placedHeapId;
		}

		PackedHandle getId() const {
			return m_id;
		}
		void setId(PackedHandle id) {
			m_id = id;
		}

		~Resource() {
			if (m_resourceType != ResourceTypes::Commited) return;
			if (m_heapType == D3D12_HEAP_TYPE_UPLOAD || m_heapType == D3D12_HEAP_TYPE_READBACK || m_heapType == D3D12_HEAP_TYPE_GPU_UPLOAD) {
				cpuTotalAllocatedMemory.fetch_sub(m_allocatedSize, std::memory_order_relaxed);
			}
			else if (m_heapType == D3D12_HEAP_TYPE_DEFAULT) {
				gpuTotalAllocatedMemory.fetch_sub(m_allocatedSize, std::memory_order_relaxed);
			}
			else if (m_heapType == D3D12_HEAP_TYPE_CUSTOM) {
				//throw std::runtime_error("[Resource] D3D12_HEAP_TYPE_CUSTOM not yet supported");
				//gpuTotalAllocatedMemory.fetch_add(allocInfo.SizeInBytes, std::memory_order_relaxed);
			}
		}
		static uint64_t GetTotalAllocatedMemoryCPU() {
			return cpuTotalAllocatedMemory.load(std::memory_order_relaxed);
		}
		static uint64_t GetTotalAllocatedMemoryGPU() {
			return gpuTotalAllocatedMemory.load(std::memory_order_relaxed);
		}
	private:
		Resource() = default;
		void moveFrom(Resource&& other) noexcept {
			m_resource = std::move(other.m_resource);
			m_allocatedSize = other.m_allocatedSize;
			m_clearValue = std::move(other.m_clearValue);
			m_currentOffset = other.m_currentOffset;
			m_heapType = other.m_heapType;
			m_resourceType = other.m_resourceType;
			m_placedHeapId = other.m_placedHeapId;
			m_id = other.m_id;

			other.m_resource.Reset();
			other.m_allocatedSize = 0;
			other.m_clearValue = nullptr;
			other.m_currentOffset = 0;
			other.m_heapType = D3D12_HEAP_TYPE_DEFAULT;
			other.m_resourceType = ResourceTypes::Commited;
			other.m_placedHeapId = 0;
			other.m_id = 0;
		}
		virtual void saveSize(D3D12_RESOURCE_DESC& desc) {
			if (m_resourceType != ResourceTypes::Commited) return;
			static auto device = Device::GetDevice();

			D3D12_RESOURCE_ALLOCATION_INFO allocInfo = device->GetResourceAllocationInfo(
				0,
				1,
				&desc
			);
			if (m_heapType == D3D12_HEAP_TYPE_UPLOAD || m_heapType == D3D12_HEAP_TYPE_READBACK || m_heapType == D3D12_HEAP_TYPE_GPU_UPLOAD) {
				cpuTotalAllocatedMemory.fetch_add(allocInfo.SizeInBytes, std::memory_order_relaxed);
			}
			else if (m_heapType == D3D12_HEAP_TYPE_DEFAULT) {
				gpuTotalAllocatedMemory.fetch_add(allocInfo.SizeInBytes, std::memory_order_relaxed);
			}
			else if (m_heapType == D3D12_HEAP_TYPE_CUSTOM) {
				throw std::runtime_error("[Resource] D3D12_HEAP_TYPE_CUSTOM not yet supported");
				//gpuTotalAllocatedMemory.fetch_add(allocInfo.SizeInBytes, std::memory_order_relaxed);
			}
			m_allocatedSize = allocInfo.SizeInBytes;
		}
		void setClearValue(const D3D12_CLEAR_VALUE* clearValue) {
			if (clearValue) {
				m_clearValue = std::make_unique<D3D12_CLEAR_VALUE>(*clearValue);
			}
			else {
				m_clearValue = nullptr;
			}
		}
		void Initialize(_In_  const D3D12_HEAP_PROPERTIES* pHeapProperties,
			D3D12_HEAP_FLAGS HeapFlags,
			_In_  const D3D12_RESOURCE_DESC* pDesc,
			D3D12_RESOURCE_STATES InitialResourceState) {

			static auto device = Device::GetDevice();

			ThrowIfFailed(device->CreateCommittedResource(pHeapProperties, HeapFlags, pDesc, InitialResourceState, m_clearValue.get(), IID_PPV_ARGS(&m_resource)));
		}

		WPtr<ID3D12Resource> m_resource;

		uint64_t m_allocatedSize = 0;
		std::unique_ptr<D3D12_CLEAR_VALUE> m_clearValue;

		uint64_t m_currentOffset = 0;
		D3D12_HEAP_TYPE m_heapType = D3D12_HEAP_TYPE_DEFAULT;
		ResourceTypes m_resourceType = ResourceTypes::Commited;

		uint64_t m_placedHeapId = 0;

		PackedHandle m_id = 0;

		static inline std::atomic<uint64_t> gpuTotalAllocatedMemory{ 0 };
		static inline std::atomic<uint64_t> cpuTotalAllocatedMemory{ 0 };

		friend class Heap;
	};
}
