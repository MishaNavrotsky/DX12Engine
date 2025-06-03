#include "stdafx.h"

#pragma once

#include "controllers/BarrierController.h"
#include "../../scene/Scene.h"

namespace Engine::System {
	class StreamingSystemArgs {
	public:
		void initialize(ID3D12Device* device, Scene::Scene* scene) {
			m_scene = scene;
			createCopyQueue(device);
			createDirectStorageQueue(device);
			createCommandList(device);
			createFence(device);

#if defined(_DEBUG)
			m_dstorageFactory->SetDebugFlags(DSTORAGE_DEBUG_SHOW_ERRORS);
#endif 
		}
		void enqueeBarrierController(Streaming::BarrierController& barriers) {
			auto bufferBarriers = barriers.getAllBarriers<D3D12_BUFFER_BARRIER>();
			auto textureBarriers = barriers.getAllBarriers<D3D12_TEXTURE_BARRIER>();

			auto fenceValue = m_fenceValue.fetch_add(1, std::memory_order_seq_cst);

			ThrowIfFailed(m_commandAllocator->Reset());
			ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), nullptr));
			if (bufferBarriers.size()) {
				D3D12_BARRIER_GROUP barrierGroup{};
				barrierGroup.Type = D3D12_BARRIER_TYPE_BUFFER;
				barrierGroup.NumBarriers = static_cast<uint32_t>(bufferBarriers.size());
				barrierGroup.pBufferBarriers = bufferBarriers.data();
				m_commandList7->Barrier(1, &barrierGroup);
			}

			if (textureBarriers.size()) {
				D3D12_BARRIER_GROUP barrierGroup{};
				barrierGroup.Type = D3D12_BARRIER_TYPE_TEXTURE;
				barrierGroup.NumBarriers = static_cast<uint32_t>(bufferBarriers.size());
				barrierGroup.pBufferBarriers = bufferBarriers.data();
				m_commandList7->Barrier(1, &barrierGroup);
			}
			m_commandList7->Close();

			ID3D12CommandList* commandLists[] = { m_commandList7.Get() };
			m_copyCommandQueue->ExecuteCommandLists(1, commandLists);
			ThrowIfFailed(m_copyCommandQueue->Signal(m_fence.Get(), fenceValue));


			while (m_fence->GetCompletedValue() < fenceValue) {
				ftl::YieldThread();
			}
			barriers.setWasExecuted();
		}

		inline IDStorageFactory* getDfactory() {
			return m_dstorageFactory.Get();
		}

		inline IDStorageQueue2* getDqueue() {
			return m_dstorageQueue.Get();
		}

		inline Scene::Scene* getScene() {
			return m_scene;
		}
	private:
		void createCopyQueue(ID3D12Device* device) {
			D3D12_COMMAND_QUEUE_DESC directQueueDesc = {};
			directQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
			directQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
			ThrowIfFailed(device->CreateCommandQueue(&directQueueDesc, IID_PPV_ARGS(&m_copyCommandQueue)));
		}
		void createDirectStorageQueue(ID3D12Device* device) {
			ThrowIfFailed(DStorageGetFactory(IID_PPV_ARGS(&m_dstorageFactory)));

			DSTORAGE_QUEUE_DESC queueDesc = {};
			queueDesc.Capacity = 512;
			queueDesc.Priority = DSTORAGE_PRIORITY_NORMAL;
			queueDesc.SourceType = DSTORAGE_REQUEST_SOURCE_FILE;
			queueDesc.Device = device;

			WPtr<IDStorageQueue> tempQeue = nullptr;

			ThrowIfFailed(m_dstorageFactory->CreateQueue(&queueDesc, IID_PPV_ARGS(&tempQeue)));
			ThrowIfFailed(tempQeue->QueryInterface(IID_PPV_ARGS(&m_dstorageQueue)));

		}
		void createCommandList(ID3D12Device* device) {
			WPtr<ID3D12GraphicsCommandList> commandListTemp;

			ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&m_commandAllocator)));
			ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COPY, m_commandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_commandList)));
			ThrowIfFailed(m_commandList->QueryInterface(IID_PPV_ARGS(&m_commandList7)));
			ThrowIfFailed(m_commandList->Close());

		}
		void createFence(ID3D12Device* device) {
			ThrowIfFailed(device->CreateFence(
				0,
				D3D12_FENCE_FLAG_NONE,
				IID_PPV_ARGS(&m_fence)
			));
		}
		WPtr<ID3D12CommandQueue> m_copyCommandQueue;
		WPtr<IDStorageFactory> m_dstorageFactory;
		WPtr<IDStorageQueue2> m_dstorageQueue;

		WPtr<ID3D12CommandAllocator> m_commandAllocator;
		WPtr<ID3D12GraphicsCommandList> m_commandList;
		WPtr<ID3D12GraphicsCommandList7> m_commandList7;

		WPtr<ID3D12Fence> m_fence;
		std::atomic<uint64_t> m_fenceValue{ 1 };

		Scene::Scene* m_scene;
	};
}
