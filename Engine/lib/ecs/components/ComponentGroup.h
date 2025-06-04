#include "stdafx.h"

#pragma once

#include "../entity/Entity.h"
#include "ComponentRegistry.h"


namespace Engine::ECS {
	using ComponentSignature = std::bitset<MAX_COMPONENTS>;

	class IComponentGroup {
	public:
		virtual ~IComponentGroup() = default;


		virtual void removeEntity(Entity entity) = 0;
		virtual std::vector<Entity>& getEntities() = 0;
		virtual const ComponentSignature& getSignature() const = 0;
		virtual bool isEmpty() const = 0;
	};

	template<typename Component>
	class ComponentGroup : public IComponentGroup {
	public:
		ComponentGroup() {
			setBitForComponent();
			m_entitiesPosition.reserve(2ULL << 10);
			m_entities.reserve(2ULL << 10);
			m_components.reserve(2ULL << 10);
		}

		void addEntity(Entity entity, Component&& component) {
			if (!m_entitiesPosition.contains(entity)) {
				auto pos = m_entitiesPosition[entity];
				m_components[pos] = std::forward<Component>(component);
				return;
			}
			m_entitiesPosition[entity] = m_entities.size();
			m_entities.push_back(entity);
			m_components.push_back(std::forward<Component>(component));
		}

		void removeEntity(Entity entity) override {
			auto pos = m_entitiesPosition[entity];
			if (pos < m_entities.size()) {
				m_entities[pos] = m_entities.back();
				m_components[pos] = std::move(m_components.back());
				m_entitiesPosition[m_entities[pos]] = pos;
			}
			m_entities.pop_back();
			m_components.pop_back();
			m_entitiesPosition.erase(entity);
		}

		std::vector<Entity>& getEntities() override {
			return m_entities;
		}

		std::vector<Component>& getComponents() {
			return m_components;
		}

		const ComponentSignature& getSignature() const override {
			return m_bitset;
		}

		inline Component& getComponent(Entity entity) {
			return m_components[m_entitiesPosition[entity]];
		}

		bool isEmpty() const override {
			return m_entities.empty();
		}

	private:
		ComponentSignature m_bitset;

		void setBitForComponent() {
			size_t componentID = ComponentRegistry::GetComponentID<Component>();
			m_bitset.set(componentID);
		}

		tbb::concurrent_unordered_map<Entity, uint64_t> m_entitiesPosition; //for fast access without find
		std::vector<Entity> m_entities;
		std::vector<Component> m_components;
	};
}