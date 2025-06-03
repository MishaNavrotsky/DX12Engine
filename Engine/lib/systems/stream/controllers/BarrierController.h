#include "stdafx.h"

#pragma once

namespace Engine::System::Streaming {
	class BarrierController {
	public:
		template<typename T>
		struct BarrierGroup {
			T m_barriers[16];
			uint8_t m_currentIndex = 0;

			void add(T&& barrier) {
				if (m_currentIndex >= 16)
					throw std::runtime_error("[BarrierGroup] Overflow: too many barriers");
				m_barriers[m_currentIndex++] = std::move(barrier);
			}

			std::span<T> getAll() {
				return std::span<T>(m_barriers, m_currentIndex);
			}
		};

		template<typename T>
		void addBarrier(T&& barrier) {
			auto& group = getOrCreateGroup<T>();
			group.add(std::forward<T>(barrier));
		}

		template<typename T>
		std::span<T> getAllBarriers() {
			auto it = m_groups.find(std::type_index(typeid(T)));
			if (it == m_groups.end()) return {};
			auto* group = static_cast<BarrierGroup<T>*>(it->second.get());
			return group->getAll();
		}

		void setWasExecuted() {
			m_wasExecuted.store(true, std::memory_order_release);
		}
		bool getWasExecuted() {
			return m_wasExecuted.load(std::memory_order_acquire);
		}
	private:
		std::unordered_map<std::type_index, std::unique_ptr<void, void(*)(void*)>> m_groups;
		std::atomic<bool> m_wasExecuted{ false };
		template<typename T>
		BarrierGroup<T>& getOrCreateGroup() {
			auto key = std::type_index(typeid(T));
			auto it = m_groups.find(key);
			if (it != m_groups.end()) {
				return *static_cast<BarrierGroup<T>*>(it->second.get());
			}

			auto* newGroup = new BarrierGroup<T>();
			auto deleter = [](void* ptr) { delete static_cast<BarrierGroup<T>*>(ptr); };
			std::unique_ptr<void, void(*)(void*)> erasedPtr(newGroup, deleter);

			m_groups.insert({ key, std::move(erasedPtr) });
			return *newGroup;
		}
	};
}
