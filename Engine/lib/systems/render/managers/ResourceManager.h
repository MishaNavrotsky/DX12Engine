#include "stdafx.h"

#pragma once

#include "../Device.h"
#include "../memory/Resource.h"

namespace Engine::Render::Manager {
	class ResourceManager {
    public:
        enum class ManagerState {
			Readonly,
			Writable,
		};
        struct ResourceSlot {
            std::optional<Memory::Resource> resource;
            uint32_t generation = 0;
            bool active = false;
        };
        ResourceManager() {
            m_slots.reserve(2048);
            m_freeSlots.reserve(2048);
        }

        Memory::Resource::Handle add(Memory::Resource&& resource) {
            std::lock_guard<std::mutex> lock(m_mutex);
			if (m_state == ManagerState::Readonly) {
				throw std::runtime_error("ResourceManager is in readonly state.");
			}

            if (!m_freeSlots.empty()) {
                uint32_t index = m_freeSlots.back();
                m_freeSlots.pop_back();

                auto& slot = m_slots[index];
                slot.active = true;
                ++slot.generation;
                resource.setId(Memory::Resource::PackHandle(index, slot.generation));
                slot.resource.emplace(std::move(resource));

                return { index, slot.generation };
            }

			uint32_t index = static_cast<uint32_t>(m_slots.size());
            uint32_t generation = 1;


            resource.setId(Memory::Resource::PackHandle(index, generation));
            m_slots.push_back({ std::optional<Memory::Resource>(std::move(resource)), generation, true });
            return { index, generation };
        }

        Memory::Resource* get(const Memory::Resource::Handle& handle) {
            std::unique_lock<std::mutex> lock(m_mutex, std::defer_lock);
			if (m_state == ManagerState::Writable) {
                lock.lock();
            }
            if (handle.index >= m_slots.size()) return nullptr;
            auto& slot = m_slots[handle.index];
            if (!slot.active || slot.generation != handle.generation) return nullptr;
            return &slot.resource.value();
        }

        void remove(const Memory::Resource::Handle& handle) {
            std::lock_guard<std::mutex> lock(m_mutex);
			if (m_state == ManagerState::Readonly) {
				throw std::runtime_error("ResourceManager is in readonly state.");
			}

            if (handle.index >= m_slots.size()) return;
            auto& slot = m_slots[handle.index];
            if (!slot.active || slot.generation != handle.generation) return;
            slot.resource = std::nullopt;
            slot.active = false;
            m_freeSlots.push_back(handle.index);
        }

        Memory::Resource* get(const Memory::Resource::PackedHandle& handle) {
            std::unique_lock<std::mutex> lock(m_mutex, std::defer_lock);
            if (m_state == ManagerState::Writable) {
                lock.lock();
            }
			uint32_t index = Memory::Resource::ExtractIndex(handle);
			uint32_t generation = Memory::Resource::ExtractGeneration(handle);

            if (index >= m_slots.size()) return nullptr;
            auto& slot = m_slots.at(index);
            if (!slot.active || slot.generation != generation) return nullptr;
            return &slot.resource.value();
        }

        void remove(const Memory::Resource::PackedHandle& handle) {
            std::lock_guard<std::mutex> lock(m_mutex);
			if (m_state == ManagerState::Readonly) {
				throw std::runtime_error("ResourceManager is in readonly state.");
			}

            uint32_t index = Memory::Resource::ExtractIndex(handle);
            uint32_t generation = Memory::Resource::ExtractGeneration(handle);

            if (index >= m_slots.size()) return;
            auto& slot = m_slots.at(index);
            if (!slot.active || slot.generation != generation) return;
            slot.resource = std::nullopt;
            slot.active = false;
            m_freeSlots.push_back(index);
        }

        Memory::Resource::Handle reserve() {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_state == ManagerState::Readonly) {
                throw std::runtime_error("ResourceManager is in readonly state.");
            }

            if (!m_freeSlots.empty()) {
                uint32_t index = m_freeSlots.back();
                m_freeSlots.pop_back();

                auto& slot = m_slots.at(index);
                ++slot.generation;
                slot.active = false; // Reserved but not active
                return { index, slot.generation };
            }

            uint32_t index = static_cast<uint32_t>(m_slots.size());
            uint32_t generation = 1;
            m_slots.push_back({ std::nullopt, generation, false });
            return { index, generation };
        }

        void assign(const Memory::Resource::Handle& handle, Memory::Resource&& resource) {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_state == ManagerState::Readonly) {
                throw std::runtime_error("ResourceManager is in readonly state.");
            }

            if (handle.index >= m_slots.size()) {
                throw std::runtime_error("Invalid handle index.");
            }

            auto& slot = m_slots.at(handle.index);
            if (slot.active || slot.generation != handle.generation) {
                throw std::runtime_error("Slot already active or generation mismatch.");
            }

            resource.setId(Memory::Resource::PackHandle(handle.index, handle.generation));
            slot.resource.emplace(std::move(resource));
            slot.active = true;
        }

		void setState(ManagerState state) {
			std::lock_guard<std::mutex> lock(m_mutex);
			m_state = state;
		}
    private:
        std::vector<ResourceSlot> m_slots;
        std::vector<uint32_t> m_freeSlots;
		std::mutex m_mutex;
		ManagerState m_state = ManagerState::Writable;
	};
}