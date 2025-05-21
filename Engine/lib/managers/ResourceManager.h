#include "stdafx.h"

#pragma once

#include "../DXSampleHelper.h"
#include "../Device.h"
#include "../memory/Resource.h"

namespace Engine {
	class ResourceManager {
    public:
        enum class ManagerState {
			Readonly,
			Writable,
		};
        struct ResourceSlot {
            std::unique_ptr<Memory::Resource> resource;
            uint32_t generation = 0;
            bool active = false;
        };
        static ResourceManager& GetInstance() {
            static ResourceManager instance;
            return instance;
        }
        Memory::Resource::Handle add(std::unique_ptr<Memory::Resource>&& resource) {
            std::lock_guard<std::mutex> lock(m_mutex);
			if (m_state == ManagerState::Readonly) {
				throw std::runtime_error("ResourceManager is in readonly state.");
			}

            if (!m_freeSlots.empty()) {
                uint32_t index = m_freeSlots.back();
                m_freeSlots.pop_back();

                m_slots[index].active = true;
                ++m_slots[index].generation;
                resource->setId(Memory::Resource::PackHandle(index, m_slots[index].generation));
                m_slots[index].resource = std::move(resource);

                return { index, m_slots[index].generation };
            }

			uint32_t index = static_cast<uint32_t>(m_slots.size());
            uint32_t generation = 1;


            resource->setId(Memory::Resource::PackHandle(index, generation));
            m_slots.push_back({ std::move(resource), generation, true });
            return { index, generation };
        }

        Memory::Resource* get(const Memory::Resource::Handle& handle) {
            std::unique_lock<std::mutex> lock(m_mutex, std::defer_lock);
			if (m_state == ManagerState::Writable) {
                lock.lock();
            }
            if (handle.index >= m_slots.size()) return nullptr;
            const auto& slot = m_slots[handle.index];
            if (!slot.active || slot.generation != handle.generation) return nullptr;
            return slot.resource.get();
        }

        void remove(const Memory::Resource::Handle& handle) {
            std::lock_guard<std::mutex> lock(m_mutex);
			if (m_state == ManagerState::Readonly) {
				throw std::runtime_error("ResourceManager is in readonly state.");
			}

            if (handle.index >= m_slots.size()) return;
            auto& slot = m_slots[handle.index];
            if (!slot.active || slot.generation != handle.generation) return;
            slot.resource = nullptr;
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
            const auto& slot = m_slots[index];
            if (!slot.active || slot.generation != generation) return nullptr;
            return slot.resource.get();
        }

        void remove(const Memory::Resource::PackedHandle& handle) {
            std::lock_guard<std::mutex> lock(m_mutex);
			if (m_state == ManagerState::Readonly) {
				throw std::runtime_error("ResourceManager is in readonly state.");
			}

            uint32_t index = Memory::Resource::ExtractIndex(handle);
            uint32_t generation = Memory::Resource::ExtractGeneration(handle);

            if (index >= m_slots.size()) return;
            auto& slot = m_slots[index];
            if (!slot.active || slot.generation != generation) return;
            slot.resource = nullptr;
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

                ++m_slots[index].generation;
                m_slots[index].active = false; // Reserved but not active
                return { index, m_slots[index].generation };
            }

            uint32_t index = static_cast<uint32_t>(m_slots.size());
            uint32_t generation = 1;
            m_slots.push_back({ nullptr, generation, false });
            return { index, generation };
        }

        void assign(const Memory::Resource::Handle& handle, std::unique_ptr<Memory::Resource>&& resource) {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_state == ManagerState::Readonly) {
                throw std::runtime_error("ResourceManager is in readonly state.");
            }

            if (handle.index >= m_slots.size()) {
                throw std::runtime_error("Invalid handle index.");
            }

            auto& slot = m_slots[handle.index];
            if (slot.active || slot.generation != handle.generation) {
                throw std::runtime_error("Slot already active or generation mismatch.");
            }

            resource->setId(Memory::Resource::PackHandle(handle.index, handle.generation));
            slot.resource = std::move(resource);
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