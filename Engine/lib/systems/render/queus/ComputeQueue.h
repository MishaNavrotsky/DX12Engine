#include "stdafx.h"

#pragma once

namespace Engine::Render::Queue {
	class ComputeQueue {
	public:
		void initialize(ID3D12Device* device) {
			D3D12_COMMAND_QUEUE_DESC queueDesc = {};
			queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
			queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
			ThrowIfFailed(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_queue)));
		}
		ID3D12CommandQueue* getQueue() {
			return m_queue.Get();
		}
	private:
		WPtr<ID3D12CommandQueue> m_queue;
	};
}
