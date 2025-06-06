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

		template <typename T>
		void addEntity(Entity entity, T&& component) {
			using U = std::remove_cvref_t<T>;
			static_assert(std::is_same_v<U, Component>, "Type mismatch in addEntity");

			auto it = m_entitiesPosition.find(entity);
			if (it != m_entitiesPosition.end()) {
				m_components[it->second] = std::forward<T>(component);
				return;
			}

			size_t pos = m_entities.size();
			m_entitiesPosition[entity] = pos;
			m_entities.push_back(entity);
			m_components.push_back(std::forward<T>(component));
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
			m_entitiesPosition.unsafe_erase(entity);
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