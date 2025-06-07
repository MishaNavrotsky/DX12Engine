#include "stdafx.h"

#pragma once

#include <iostream>
#include <bitset>
#include <unordered_map>
#include <typeindex>
#include <typeinfo>

namespace Engine::ECS {
	constexpr uint32_t MAX_COMPONENTS = 64;
	using ComponentId = uint64_t;

	struct ComponentOps {
		std::function<void(void*)> construct;
		std::function<void(void*, const void*)> copy;
		std::function<void(void*, void*)> move;
		std::function<void(void*)> destroy;
		bool isTriviallyDestructible = false;
	};

	using ComponentRegitsryData = std::tuple<ComponentId, uint64_t, ComponentOps>; // id, size


	class ComponentRegistry {
	public:
		template <typename T>
		static uint64_t RegisterComponent() {
			if constexpr (!std::is_standard_layout_v<T>) {
				std::cerr << "Warning: Component " << typeid(T).name() << " is not standard layout.\n";
			}
			if constexpr (!std::is_trivial_v<T>) {
				std::cerr << "Warning: Component " << typeid(T).name() << " is not trivial.\n";
			}
			if constexpr (!std::is_trivially_destructible_v<T>) {
				std::cerr << "Warning: Component " << typeid(T).name() << " is not trivially destructible.\n";
			}

			auto it = componentIDs.find(typeid(T));
			if (it != componentIDs.end()) {
				return std::get<0>(it->second);
			}

			uint64_t newID = nextID++;
			componentIDs[typeid(T)] = ComponentRegitsryData(newID, sizeof(T), makeComponentOps<T>());

			// Store the component data in a list for easier access
			componentDataList.push_back(componentIDs.at(typeid(T)));
			return newID;
		}

		template <typename T>
		static ComponentId GetComponentId() {
			return std::get<0>(componentIDs[typeid(T)]);
		}

		template <typename T>
		static uint64_t GetComponentSize() {
			return std::get<1>(componentIDs[typeid(T)]);
		}

		template <typename T>
		static ComponentRegitsryData& GetComponentData() {
			return componentIDs.at(typeid(T));
		}

		static const std::vector<ComponentRegitsryData>& GetComponentDataList() {
			return componentDataList;
		}

		static ComponentRegitsryData& GetComponentDataById(ComponentId id) {
			return componentDataList.at(id);
		}

	private:
		static inline ComponentId nextID = 0;
		static inline ankerl::unordered_dense::map<std::type_index, ComponentRegitsryData> componentIDs;
		static inline std::vector<ComponentRegitsryData> componentDataList;

		template<typename T>
		static inline ComponentOps makeComponentOps() {
			return {
				// Default construct
				[](void* dest) {
					new (dest) T();
				},
				// Copy construct
				[](void* dest, const void* src) {
					new (dest) T(*reinterpret_cast<const T*>(src));
				},
				// Move construct
				[](void* dest, void* src) {
					new (dest) T(std::move(*reinterpret_cast<T*>(src)));
				},
				// Destruct
				[](void* dest) {
					reinterpret_cast<T*>(dest)->~T();
				},
				std::is_trivially_destructible_v<T>

			};
		}
	};

	struct ComponentDeleter {
		ComponentId cid;

		ComponentDeleter(ComponentId id)
			: cid(id) {
		}

		void operator()(void* ptr) const {
			if (ptr == nullptr) {
				return;
			}

			const auto& [_, size, ops] = ComponentRegistry::GetComponentDataById(cid);
			if (!ops.isTriviallyDestructible) ops.destroy(ptr);
			operator delete(ptr, size);
		}
	};

}
