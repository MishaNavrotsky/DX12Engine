#include "stdafx.h"

#pragma once

#include "../Resource.h"
#include "../Heap.h"
#include "../../Device.h"

namespace Engine::Render::Memory {
	class HeapPool {
	public:
		struct AllocateResult {
			Heap::HeapId heapId;
			Resource::PackedHandle resourceHandle;
		};
		void initialize(D3D12_HEAP_TYPE heapType, uint64_t heapSize, D3D12_HEAP_FLAGS heapFlags, Manager::ResourceManager* resourceManager) {
			m_heapType = heapType;
			m_heapSize = heapSize;
			m_heapFlags = heapFlags;
			m_resourceManager = resourceManager;
			m_device = Device::GetDevice();
		}
		std::optional<AllocateResult> allocate(D3D12_RESOURCE_DESC desc, D3D12_RESOURCE_STATES state, const D3D12_CLEAR_VALUE* clearValue = nullptr) {
			std::lock_guard lock(m_allocateMutex);
			D3D12_RESOURCE_ALLOCATION_INFO allocInfo = m_device->GetResourceAllocationInfo(
				0,
				1,
				&desc
			);

			if (allocInfo.SizeInBytes > m_heapSize) {
				throw std::runtime_error("[HeapPool] Placed resource size is too big to allocate");
			}


			for (auto& heap : m_heaps) {
				auto resourceHandle = heap.second.tryCreatePlacedResource(desc, state, &allocInfo, clearValue);
				if (resourceHandle) return 	AllocateResult{ .heapId = heap.first, .resourceHandle = *resourceHandle };
			}

			if (true) { //enough memory
				auto nextHeapId = generateNextHeapId();
				auto heap = Heap::CreateV(nextHeapId, m_heapType, m_heapSize, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT, m_heapFlags);
				heap.initialize(m_resourceManager);
				auto& heapRef = m_heaps.emplace(nextHeapId, std::move(heap)).first->second;
				Resource::PackedHandle resourceHandle = *heapRef.tryCreatePlacedResource(desc, state, &allocInfo, clearValue);
				return AllocateResult{ .heapId = nextHeapId, .resourceHandle = resourceHandle };
			}


			return std::nullopt;
		}
		void deallocate(AllocateResult res) {

		}
	private:
		Manager::ResourceManager* m_resourceManager;
		D3D12_HEAP_TYPE m_heapType;
		D3D12_HEAP_FLAGS m_heapFlags;
		uint64_t m_heapSize;
		ID3D12Device* m_device;
		Heap::HeapId generateNextHeapId() {
			return m_nextHeapId.fetch_add(1, std::memory_order_seq_cst);
		}
		inline static std::atomic<Heap::HeapId> m_nextHeapId{ 1 };
		tbb::concurrent_unordered_map<Heap::HeapId, Heap> m_heaps;
		std::mutex m_allocateMutex;

	};
}