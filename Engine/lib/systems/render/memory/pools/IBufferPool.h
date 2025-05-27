#include "stdafx.h"

#pragma once

#include "../Resource.h"

namespace Engine::Memory {
	class IBufferPool {
	public:
		virtual ~IBufferPool() = default;
		virtual void* allocate(uint64_t size) = 0;
		virtual void deallocate(void* ptr) = 0;
		virtual size_t getSize() const = 0;
		virtual size_t getUsedSize() const = 0;
	};
}
