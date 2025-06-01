#include "stdafx.h"

#pragma once

namespace Engine::System::Streaming {
	template<typename T>
	class BarrierController {
	public:
		void addBarrier(T&& barrier) {
			if (m_currentIndex >= std::size(m_barriers)) throw std::runtime_error("[BarrierController] addBarrier overflow");
			m_barriers[m_currentIndex++] = std::move(barrier);
		}
		std::vector<T> getAllBarriers() {
			return std::vector<T>(m_barriers, m_barriers + m_currentIndex);
		}

		void setWasExecuted() {
			m_wasExecuted.store(true, std::memory_order_release);
		}
		void getWasExecuted() {
			return m_wasExecuted.load(std::memory_order_acquire);
		}
	private:
		std::atomic<bool> m_wasExecuted{ false };
		uint8_t m_currentIndex = 0;
		T m_barriers[16];
	};
}
