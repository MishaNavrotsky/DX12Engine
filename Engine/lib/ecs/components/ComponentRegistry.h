#include "stdafx.h"

#pragma once

#include <iostream>
#include <bitset>
#include <unordered_map>
#include <typeindex>
#include <typeinfo>

namespace Engine::ECS {
    constexpr uint32_t MAX_COMPONENTS = 64;
    struct ComponentOps {
        std::function<void(void*)> construct;
        std::function<void(void*, const void*)> copy;
        std::function<void(void*, void*)> move;
        std::function<void(void*)> destroy;
    };

    using ComponentId = size_t;
    using ComponentRegitsryData = std::tuple<ComponentId, size_t, ComponentOps>; // id, size

    class ComponentRegistry {
    public:
        template <typename T>
        static size_t RegisterComponent() {
            auto it = componentIDs.find(typeid(T));
            if (it != componentIDs.end()) {
                return std::get<0>(it->second);  
            }

            size_t newID = nextID++;
            componentIDs[typeid(T)] = ComponentRegitsryData(newID, sizeof(T), makeComponentOps<T>());

			// Store the component data in a list for easier access
			componentDataList.push_back(componentIDs[typeid(T)]);
            return newID;
        }

        template <typename T>
        static ComponentId GetComponentID() {
            return std::get<0>(componentIDs[typeid(T)]);
        }

        template <typename T>
        static size_t GetComponentSize() {
            return std::get<1>(componentIDs[typeid(T)]);
        }

        template <typename T>
        static std::tuple<size_t, size_t, ComponentOps> GetComponent() {
			return componentIDs[typeid(T)];
        }

		static const ComponentRegitsryData& GetComponentById(size_t id) {
			for (const auto& [type, data] : componentIDs) {
				if (std::get<0>(data) == id) {
                    return data;
				}
			}
			throw std::runtime_error("Component ID not found");
		}
		static const std::vector<ComponentRegitsryData>& GetComponentDataList() {
			return componentDataList;
		}

    private:
        static inline ComponentId nextID = 0;
        static inline std::unordered_map<std::type_index, ComponentRegitsryData> componentIDs;
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
                }
            };
        }
    };

}
