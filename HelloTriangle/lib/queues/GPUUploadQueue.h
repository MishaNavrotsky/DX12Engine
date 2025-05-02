#include "stdafx.h"
#pragma once

namespace Engine {
	using namespace Microsoft::WRL;

	class GPUUploadQueue {
	public:
		GPUUploadQueue(ComPtr<ID3D12CommandQueue> uploadCommandQueue): m_uploadCommandQueue(uploadCommandQueue) {
			
		}
	private:
		ComPtr<ID3D12CommandQueue> m_uploadCommandQueue;

	};
}