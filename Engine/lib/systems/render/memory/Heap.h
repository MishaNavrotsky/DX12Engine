#include "stdafx.h"

#pragma once

#include "../Device.h"
#include "Resource.h"
#include "../managers/ResourceManager.h"
#include "../../../helpers.h"

namespace Engine::Render::Memory {
	class Heap {
	public:
		using HeapId = uint64_t;
		Heap(const Heap&) = delete;
		Heap& operator=(const Heap&) = delete;
		Heap(Heap&& other) noexcept {
			moveFrom(std::move(other));
		}

		void initialize(Manager::ResourceManager* resourceManager) {
			m_resourceManager = resourceManager;
		}

		Heap& operator=(Heap&& other) noexcept {
			if (this != &other) {
				moveFrom(std::move(other));
			}
			return *this;
		}
		static std::unique_ptr<Heap> Create(HeapId id, D3D12_HEAP_TYPE type, uint64_t sizeInBytes, uint64_t alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT, D3D12_HEAP_FLAGS flag = D3D12_HEAP_FLAG_NONE) {
			if (sizeInBytes == 0) {
				throw std::runtime_error("[Heap] Size cannot be zero.");
			}

			if (alignment < D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT) {
				throw std::runtime_error("[Heap] Alignment must be at least D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT.");
			}

			if (id == 0) {
				throw std::runtime_error("[Heap] Heap ID cannot be zero.");
			}

			if (sizeInBytes != Helpers::Align(sizeInBytes, alignment)) {
				throw std::runtime_error("[Heap] Size is not aligned.");
			}

			D3D12_HEAP_PROPERTIES props = CD3DX12_HEAP_PROPERTIES(type);
			auto heap = new Heap();
			heap->m_heapId = id;
			heap->m_heapType = type;
			heap->Initialize(props, flag, sizeInBytes, alignment);
			return std::unique_ptr<Heap>(heap);
		}

		static Heap CreateV(HeapId id, D3D12_HEAP_TYPE type, uint64_t sizeInBytes, uint64_t alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT, D3D12_HEAP_FLAGS flag = D3D12_HEAP_FLAG_NONE) {
			if (sizeInBytes == 0) {
				throw std::runtime_error("[Heap] Size cannot be zero.");
			}

			if (alignment < D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT) {
				throw std::runtime_error("[Heap] Alignment must be at least D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT.");
			}

			if (id == 0) {
				throw std::runtime_error("[Heap] Heap ID cannot be zero.");
			}

			if (sizeInBytes != Helpers::Align(sizeInBytes, alignment)) {
				throw std::runtime_error("[Heap] Size is not aligned.");
			}

			D3D12_HEAP_PROPERTIES props = CD3DX12_HEAP_PROPERTIES(type);
			Heap heap;
			heap.m_heapId = id;
			heap.m_heapType = type;
			heap.Initialize(props, flag, sizeInBytes, alignment);
			return heap;
		}

		bool hasSequentialSpace(uint64_t sizeInBytes) {
			uint64_t alignedSizeInBytes = Helpers::Align(sizeInBytes, m_alignment);
			if (sizeInBytes != alignedSizeInBytes) {
				throw std::runtime_error("[Heap] Size is not aligned.");
			}
			for (auto it = m_freeMemory.begin(); it != m_freeMemory.end(); ++it) {
				uint64_t freeStart = it->first;
				uint64_t freeEnd = it->second;
				uint64_t freeSize = freeEnd - freeStart + 1;

				if (freeSize >= sizeInBytes) return true;
			}

			return false;
		}

		ID3D12Heap* getHeap() const {
			return m_heap.Get();
		}
		uint64_t getSize() const {
			return m_sizeInBytes;
		}
		D3D12_HEAP_TYPE getHeapType() const {
			return m_heapType;
		}
		uint64_t getHeapId() const {
			return m_heapId;
		}
		std::optional<Resource::PackedHandle> tryCreatePlacedResource(D3D12_RESOURCE_DESC desc, D3D12_RESOURCE_STATES state, const D3D12_RESOURCE_ALLOCATION_INFO* info = nullptr, const D3D12_CLEAR_VALUE* clearValue = nullptr) {
			static auto device = Device::GetDevice();
			D3D12_RESOURCE_ALLOCATION_INFO allocInfo = info ? *info : device->GetResourceAllocationInfo(
				0,
				1,
				&desc
			);

			auto handle = m_resourceManager->reserve();
			auto packedHandle = Resource::PackHandle(handle.index, handle.generation);

			Resource resource;
			auto size = Helpers::Align(allocInfo.SizeInBytes, m_alignment);

			auto sizes = allocate(packedHandle, size);
			if (sizes.first == 0 && sizes.second == 0) {
				m_resourceManager->remove(handle);
				return std::nullopt;
			}


			device->CreatePlacedResource(
				m_heap.Get(),
				sizes.first,
				&desc,
				state,
				clearValue,
				IID_PPV_ARGS(&resource.m_resource)
			);

			resource.m_heapType = m_heapType;
			resource.m_resourceType = ResourceTypes::Placed;
			resource.m_clearValue = clearValue == nullptr ? nullptr : std::make_unique<D3D12_CLEAR_VALUE>(*clearValue);
			resource.m_allocatedSize = size;
			resource.m_placedHeapId = m_heapId;
			resource.m_id = packedHandle;

			m_resourceManager->assign(handle, std::move(resource));
			return packedHandle;
		}

		void removePlacedResource(Resource::PackedHandle resourceId) {
			deallocate(resourceId);
			m_resourceManager->remove(resourceId);
		}

		uint64_t getCommitedResourceSize() {
			return m_resourceMap.size();
		}

		uint64_t getFreeSpace() {
			uint64_t freeSpace = 0;
			for (auto& m : m_freeMemory) {
				freeSpace += m.second - m.first + 1;
			}
			return freeSpace;
		}

		~Heap()
		{
			if (m_heapType == D3D12_HEAP_TYPE_UPLOAD || m_heapType == D3D12_HEAP_TYPE_READBACK || m_heapType == D3D12_HEAP_TYPE_GPU_UPLOAD) {
				cpuTotalAllocatedMemory.fetch_sub(m_sizeInBytes, std::memory_order_relaxed);
			}
			else if (m_heapType == D3D12_HEAP_TYPE_DEFAULT) {
				gpuTotalAllocatedMemory.fetch_sub(m_sizeInBytes, std::memory_order_relaxed);
			}
			else if (m_heapType == D3D12_HEAP_TYPE_CUSTOM) {
				//throw std::runtime_error("[Heap] D3D12_HEAP_TYPE_CUSTOM not yet supported");
				//gpuTotalAllocatedMemory.fetch_add(allocInfo.SizeInBytes, std::memory_order_relaxed);
			}
		}
	private:
		Heap() = default;
		void moveFrom(Heap&& other) {
			// Take ownership
			m_heap = std::move(other.m_heap);
			m_sizeInBytes = other.m_sizeInBytes;
			m_heapType = other.m_heapType;
			m_alignment = other.m_alignment;
			m_heapId = other.m_heapId;
			m_resourceMap = std::move(other.m_resourceMap);
			m_freeMemory = std::move(other.m_freeMemory);
			m_resourceManager = other.m_resourceManager;

			other.m_sizeInBytes = 0;
			other.m_heapType = D3D12_HEAP_TYPE_CUSTOM;
			other.m_heapId = 0;
			other.m_resourceManager = nullptr;
			other.m_resourceMap.clear();
			other.m_freeMemory.clear();
			other.m_heap.Reset();
		}
		void Initialize(D3D12_HEAP_PROPERTIES& props, D3D12_HEAP_FLAGS flag, uint64_t size, uint64_t alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT) {
			m_resourceMap.reserve(256);
			static auto device = Device::GetDevice();
			m_sizeInBytes = Helpers::Align(size, alignment);
			m_alignment = alignment;
			m_heapType = props.Type;
			D3D12_HEAP_DESC heapDesc = {};
			heapDesc.SizeInBytes = size;
			heapDesc.Properties = props;
			heapDesc.Flags = flag;
			heapDesc.Alignment = alignment;

			ThrowIfFailed(device->CreateHeap(&heapDesc, IID_PPV_ARGS(&m_heap)));

			if (m_heapType == D3D12_HEAP_TYPE_UPLOAD || m_heapType == D3D12_HEAP_TYPE_READBACK || m_heapType == D3D12_HEAP_TYPE_GPU_UPLOAD) {
				cpuTotalAllocatedMemory.fetch_add(m_sizeInBytes, std::memory_order_relaxed);
			}
			else if (m_heapType == D3D12_HEAP_TYPE_DEFAULT) {
				gpuTotalAllocatedMemory.fetch_add(m_sizeInBytes, std::memory_order_relaxed);
			}
			else if (m_heapType == D3D12_HEAP_TYPE_CUSTOM) {
				throw std::runtime_error("[Heap] D3D12_HEAP_TYPE_CUSTOM not yet supported");
				//gpuTotalAllocatedMemory.fetch_add(allocInfo.SizeInBytes, std::memory_order_relaxed);
			}

			m_freeMemory.insert({ 0, m_sizeInBytes - 1 });
		}

		std::pair<uint64_t, uint64_t> allocate(Resource::PackedHandle id, uint64_t alignedSize) {
			if (alignedSize == 0) {
				throw std::runtime_error("[Heap] Cannot allocate 0 bytes.");
			}

			for (auto it = m_freeMemory.begin(); it != m_freeMemory.end(); ++it) {
				uint64_t freeStart = it->first;
				uint64_t freeEnd = it->second;
				uint64_t freeSize = freeEnd - freeStart + 1;

				if (freeSize < alignedSize) continue;

				uint64_t allocStart = freeStart;
				uint64_t allocEnd = allocStart + alignedSize - 1;

				m_freeMemory.erase(it);

				if (allocEnd < freeEnd) {
					m_freeMemory.insert({ allocEnd + 1, freeEnd });
				}

				m_resourceMap[id] = { allocStart, allocEnd };
				return { allocStart, allocEnd };
			}

			return { 0, 0 };
		}

		void deallocate(Resource::PackedHandle resourceId) {
			auto it = m_resourceMap.find(resourceId);
			if (it == m_resourceMap.end()) {
				throw std::runtime_error("[Heap] Resource ID not found.");
			}

			uint64_t start = it->second.first;
			uint64_t end = it->second.second;
			m_resourceMap.unsafe_erase(it);

			auto next = m_freeMemory.lower_bound(start);
			if (next != m_freeMemory.begin()) {
				auto prev = std::prev(next);
				if (prev->second + 1 == start) {
					start = prev->first;
					m_freeMemory.erase(prev);
				}
			}

			next = m_freeMemory.lower_bound(end + 1);
			if (next != m_freeMemory.end() && next->first == end + 1) {
				end = next->second;
				m_freeMemory.erase(next);
			}

			m_freeMemory[start] = end;
		}

		WPtr<ID3D12Heap> m_heap;
		uint64_t m_sizeInBytes;
		D3D12_HEAP_TYPE m_heapType;
		uint64_t m_alignment;

		HeapId m_heapId = 0;
		tbb::concurrent_unordered_map<Resource::PackedHandle, std::pair<uint64_t, uint64_t>> m_resourceMap; // [alloctStart, allocEnd]
		std::map<uint64_t, uint64_t> m_freeMemory; // key = start, value = end

		Manager::ResourceManager* m_resourceManager;

		static inline std::atomic<uint64_t> gpuTotalAllocatedMemory{ 0 };
		static inline std::atomic<uint64_t> cpuTotalAllocatedMemory{ 0 };
	};
}