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
		void initialize(D3D12_HEAP_TYPE heapType, uint64_t heapSize) {
			m_heapType = heapType;
			m_heapSize = heapSize;
			m_device = Device::GetDevice().Get();
		}
		std::optional<AllocateResult> allocate(D3D12_RESOURCE_DESC desc, D3D12_RESOURCE_STATES state, const D3D12_CLEAR_VALUE* clearValue = nullptr) {
			D3D12_RESOURCE_ALLOCATION_INFO allocInfo = m_device->GetResourceAllocationInfo(
				0,
				1,
				&desc
			);

			if (allocInfo.SizeInBytes > m_heapSize) {
				throw std::runtime_error("[HeapPool] Placed resource size is too big to allocate");
			}


			for (auto& heap : m_heaps) {
				if (heap.second.hasSequentialSpace(allocInfo.SizeInBytes)) {
					Resource::PackedHandle resourceHandle = heap.second.createPlacedResource(desc, state, &allocInfo, clearValue);
					return 	AllocateResult{ .heapId = heap.first, .resourceHandle = resourceHandle };
				}
			}

			if (true) { //enough memory
				auto nextHeapId = generateNextHeapId();
				m_heaps.emplace(nextHeapId, std::move(Heap::CreateV(nextHeapId, m_heapType, m_heapSize)));
				Resource::PackedHandle resourceHandle = m_heaps.at(nextHeapId).createPlacedResource(desc, state, &allocInfo, clearValue);
				return AllocateResult{ .heapId = nextHeapId, .resourceHandle = resourceHandle };
			}


			return std::nullopt;
		}
		void deallocate(AllocateResult res) {

		}
	private:
		D3D12_HEAP_TYPE m_heapType;
		uint64_t m_heapSize;
		ID3D12Device* m_device;
		Heap::HeapId generateNextHeapId() {
			return m_nextHeapId.fetch_add(1, std::memory_order_seq_cst);
		}
		std::atomic<Heap::HeapId> m_nextHeapId{ 1 };
		ankerl::unordered_dense::map<Heap::HeapId, Heap> m_heaps;

	};
}