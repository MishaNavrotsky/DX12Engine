#include "stdafx.h"

#pragma once
#include <ranges>
#include <algorithm>
#include <optional>

#include "entity/Entity.h"
#include "components/Components.h"

namespace Engine::ECS {
	class EntityManager {
	private:
		using ECSCommand = std::function<void(entt::basic_registry<Entity>&)>;
	public:
		EntityManager() {
			m_registry.group<Component::ComponentTransform, Component::ComponentTransformDirty>();
			m_registry.group<Component::ComponentCamera>(entt::get<Component::ComponentTransform>);
			m_registry.group<Component::ComponentMesh>(entt::get<Component::ComponentTransform>);
		}

		Entity createEntity() {
			Entity entity = m_nextEntityId.fetch_add(1, std::memory_order_seq_cst);
			return m_registry.create(entity);
		}

		template<typename... Components>
		void addComponent(Entity entity, Components&&... components) {
			(addComponent(entity, std::forward<Components>(components)), ...);
		}

		template<typename... Components>
		void removeComponent(Entity entity) {
			m_commandQueue.push([entity](entt::basic_registry<Entity>& registry) {
				(registry.remove<Components>(entity), ...);
				});
		}

		void destroyEntity(Entity entity) {
			m_commandQueue.push([entity](entt::basic_registry<Entity>& registry) {
				registry.destroy(entity);
				});
		}


		void processCommands() {
			ECSCommand command;
			while (m_commandQueue.try_pop(command)) {
				command(m_registry);
			}
		}
		entt::basic_registry<Entity>& getRegistry() {
			return m_registry;
		}
	private:
		std::atomic<Entity> m_nextEntityId{ 1 };

		tbb::concurrent_queue<ECSCommand> m_commandQueue;
		entt::basic_registry<Entity> m_registry;


		template<typename Component>
		void addComponent(Entity entity, Component&& component) {
			using T = std::decay_t<Component>;

			m_commandQueue.push([entity, comp = T(std::forward<Component>(component))](entt::basic_registry<Entity>& registry) mutable {
				registry.emplace_or_replace<T>(entity, std::move(comp));
				});
		}
	};
}
