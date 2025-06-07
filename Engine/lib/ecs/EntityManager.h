#include "stdafx.h"

#pragma once
#include <ranges>
#include <algorithm>
#include <optional>

#include "entity/Entity.h"
#include "components/ComponentRegistry.h"
#include "components/ComponentGroup.h"
#include "components/ComponentArchetype.h"

namespace Engine::ECS {
	class EntityManager {
	public:
		EntityManager() {
			m_nextEntityId.store(1); // Start from 1 to avoid zero entity ID
			m_componentArchetypes.reserve(MAX_COMPONENTS);
			m_entityArchetypes.reserve((2ULL << 10) * MAX_COMPONENTS);
		}

		Entity createEntity() {
			Entity entity = m_nextEntityId.fetch_add(1);
			m_entityArchetypes.emplace(entity, nullptr); // Initialize with empty signature
			return entity;
		}

		void destroyEntity(Entity entity) {
			auto it = m_entityArchetypes.find(entity);
			if (it == m_entityArchetypes.end()) return;
			auto* archetype = it->second;

			archetype->removeEntity(entity);
			m_entityArchetypes.unsafe_erase(entity);
		}

		template<typename... Components>
		void addComponent(Entity entity, Components&&... components) {
			auto entityArchetypeItt = m_entityArchetypes.find(entity);
			if (entityArchetypeItt == m_entityArchetypes.end()) return;

			std::vector<std::pair<ComponentId, std::unique_ptr<void, ComponentDeleter>>> componentsData;
			(setComponentPair<Components>(std::forward<Components>(components), componentsData), ...);


			std::bitset<MAX_COMPONENTS> archetypeBitset;
			(setBit<Components>(archetypeBitset), ...);
			if (entityArchetypeItt->second == nullptr) {
				auto* archetype = createOrGetArchetype(archetypeBitset);

				archetype->addEntity(entity, componentsData);
				entityArchetypeItt->second = archetype;
				return;
			}

			auto& entitiesArchetypeBitset = entityArchetypeItt->second->getSignature();
			if ((archetypeBitset | entitiesArchetypeBitset) == entitiesArchetypeBitset) {
				entityArchetypeItt->second->updateEntityComponents(entity, componentsData);
				return;
			}


			auto oldArchetypeEntityComponents = entityArchetypeItt->second->getAllComponents(entity);
			for (auto& component : componentsData) {
				oldArchetypeEntityComponents.push_back(std::move(component));
			}
			entityArchetypeItt->second->removeEntity(entity);

			archetypeBitset |= entityArchetypeItt->second->getSignature();
			auto* archetype = createOrGetArchetype(archetypeBitset);

			archetype->addEntity(entity, oldArchetypeEntityComponents);
			entityArchetypeItt->second = archetype;
		}

		template<typename... Components>
		void removeComponent(Entity entity) {
			auto entityArchetypeItt = m_entityArchetypes.find(entity);
			if (entityArchetypeItt == m_entityArchetypes.end()) return;

			std::bitset<MAX_COMPONENTS> archetypeBitset = entityArchetypeItt->second->getSignature();
			(archetypeBitset.reset(ComponentRegistry::GetComponentId<Components>()), ...);  // Remove the components from the bitset
			auto* archetype = createOrGetArchetype(archetypeBitset);

			auto oldComponents = entityArchetypeItt->second->getAllComponents(entity);
			entityArchetypeItt->second->removeEntity(entity);
			archetype->addEntity(entity, oldComponents);
			entityArchetypeItt->second = archetype;
		}

		template<typename... Components>
		std::vector<Entity> viewDirty(uint64_t sinceFrame) const {
			ComponentSignature signature;
			(setBit<Components>(signature), ...);

			std::vector<Entity> dirtyEntities;

			for (auto& [bitset, archetype] : m_componentArchetypes) {
				if ((signature & bitset) != signature) continue;
				auto dirtyEntitiesForArchetype = archetype->getDirtyEntitiesBySignature(signature, sinceFrame);
				dirtyEntities.insert(dirtyEntities.end(), dirtyEntitiesForArchetype.begin(), dirtyEntitiesForArchetype.end());

			}

			return dirtyEntities;
		}

		template<typename... Components>
		std::vector<Entity> view() const {
			ComponentSignature signature;
			(setBit<Components>(signature), ...);

			std::vector<Entity> entities;

			for (auto& [bitset, archetype] : m_componentArchetypes) {
				if ((signature & bitset) != signature) continue;
				auto entitiesArchetype = archetype->getAllEntities();
				entities.insert(entities.end(), entitiesArchetype.begin(), entitiesArchetype.end());

			}

			return entities;
		}

		template<typename Component>
		std::optional<Component> getComponent(Entity entity) {
			auto itt = m_entityArchetypes.find(entity);
			if (itt == m_entityArchetypes.end()) return std::nullopt;

			return itt->second->getComponent<Component>(entity);
		}

		template<typename Component>
		bool isComponentDirty(Entity entity) {
			auto itt = m_entityArchetypes.find(entity);
			if (itt == m_entityArchetypes.end()) return false;

			return itt->second->isComponentDirty<Component>(entity);
		}
	private:
		template<typename T>
		void setBit(std::bitset<MAX_COMPONENTS>& bitset) const {
			using Component = std::remove_cvref_t<T>;
			const auto componentId = ComponentRegistry::GetComponentId<Component>();
			bitset.set(componentId, true);

		}
		template<typename T>
		void setComponentPair(T&& component, std::vector<std::pair<ComponentId, std::unique_ptr<void, ComponentDeleter>>>& componentsData) {
			using Component = std::remove_cvref_t<T>;

			const auto [cid, size, ops] = ComponentRegistry::GetComponentData<Component>();


			void* componentPtr = operator new(size);
			new (componentPtr) Component(std::forward<T>(component));
			std::unique_ptr<void, ComponentDeleter> componentUniquePtr(componentPtr, ComponentDeleter{ cid });

			componentsData.push_back({ cid, std::move(componentUniquePtr) });
		}

		ComponentArchetype* createOrGetArchetype(std::bitset<MAX_COMPONENTS>& bitset) {
			auto it = m_componentArchetypes.find(bitset);
			if (it == m_componentArchetypes.end()) {
				return m_componentArchetypes.emplace(bitset, std::make_unique<ComponentArchetype>(bitset)).first->second.get();
			}
			return it->second.get();
		}

		std::atomic<Entity> m_nextEntityId;
		tbb::concurrent_unordered_map<Entity, ComponentArchetype*> m_entityArchetypes;
		ankerl::unordered_dense::map<std::bitset<MAX_COMPONENTS>, std::unique_ptr<ComponentArchetype>> m_componentArchetypes;
	};
}
