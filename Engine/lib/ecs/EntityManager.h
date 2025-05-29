#include "stdafx.h"

#pragma once
#include <ranges>
#include <algorithm>
#include <optional>

#include "../../external/unordered_dense.h"
#include "entity/Entity.h"
#include "components/ComponentRegistry.h"
#include "components/ComponentGroup.h"

namespace Engine::ECS {
	class EntityManager {
	public:
		EntityManager() {
			m_nextEntityId.store(1); // Start from 1 to avoid zero entity ID
			m_componentGroups.resize(MAX_COMPONENTS);
			m_entitySignatures.reserve(2ULL << 10 * MAX_COMPONENTS);
		}

		Entity createEntity() {
			Entity entity = m_nextEntityId.fetch_add(1);
			m_entitySignatures[entity] = ComponentSignature(); // Initialize with empty signature
			return entity;
		}

		void destroyEntity(Entity entity) {
			auto& signature = m_entitySignatures[entity];
			for (uint64_t i = 0; i < signature.size(); i++) {
				if (signature.test(i)) {
					auto& group = m_componentGroups[i];
					group->removeEntity(entity);
				}
			}
			m_entitySignatures.erase(entity);
		}

		template<typename... Components>
		void addComponent(Entity entity, Components&&... components) {
			((addSingleComponent<Components>(entity, std::forward<Components>(components))), ...);
		}

		template<typename... Components>
		void removeComponent(Entity entity) {
			((removeSingleComponent<Components>(entity)), ...);
		}

		template<typename... Components>
		std::vector<Entity> view() {
			std::vector<Entity> entities;
			ComponentSignature requiredSignature;
			((requiredSignature.set(ComponentRegistry::GetComponentID<Components>(), true)), ...);
			for (const auto& [entity, signature] : m_entitySignatures) {
				if ((signature & requiredSignature) == requiredSignature) {
					entities.push_back(entity);
				}
			}
			return entities;
		}

		template<typename Component>
		Component& getComponent(Entity entity) {
			auto componentId = ComponentRegistry::GetComponentID<Component>();
			if (!m_entitySignatures[entity].test(componentId)) {
				throw std::runtime_error("Entity does not have the requested component.");
			}
			auto& group = static_cast<ComponentGroup<Component>&>(*m_componentGroups[componentId]);
			return group.getComponent(entity);
		}

		template <typename Component>
		std::optional<std::pair<std::vector<Entity>&, std::vector<Component>&>> fastView() {
			auto componentId = ComponentRegistry::GetComponentID<Component>();
			if (!m_componentGroups[componentId] || m_componentGroups[componentId]->isEmpty()) {
				return std::nullopt;
			}

			auto& group = static_cast<ComponentGroup<Component>&>(*m_componentGroups[componentId]);
			return std::make_optional(std::pair<std::vector<Entity>&, std::vector<Component>&>(
				group.getEntities(),
				group.getComponents()
			));
		}
    private:
		template<typename Component>
		void addSingleComponent(Entity entity, Component&& component) {
			auto componentId = ComponentRegistry::GetComponentID<Component>();
			if (!m_componentGroups[componentId]) {
				m_componentGroups[componentId] = std::make_unique<ComponentGroup<Component>>();
			}
			m_entitySignatures[entity].set(componentId, true);
			auto& group = static_cast<ComponentGroup<Component>&>(*m_componentGroups[componentId]);
			group.addEntity(entity, std::forward<Component>(component));
		}

		template<typename Component>
		void removeSingleComponent(Entity entity) {
			auto componentId = ComponentRegistry::GetComponentID<Component>();
			if (m_componentGroups[componentId]) {				
				m_entitySignatures[entity].set(componentId, false);
				auto& group = static_cast<ComponentGroup<Component>&>(*m_componentGroups[componentId]);
				group.removeEntity(entity);
			}
		}

		std::atomic<Entity> m_nextEntityId;
        ankerl::unordered_dense::map<Entity, ComponentSignature> m_entitySignatures;
		std::vector<std::unique_ptr<IComponentGroup>> m_componentGroups;
	};
}
