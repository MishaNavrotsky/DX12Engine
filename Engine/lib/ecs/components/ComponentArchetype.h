#pragma once
#include "stdafx.h"

#pragma once

#include "../entity/Entity.h"
#include "ComponentRegistry.h"
#include "../../global.h"


namespace Engine::ECS {
	using ComponentSignature = std::bitset<MAX_COMPONENTS>;

	struct ComponentArchetypeChunk {
		static constexpr uint64_t ChunkSize = 64;
		ComponentArchetypeChunk() {
			entityVersion.reserve(ChunkSize);
		}
		uint64_t entityCount = 0;
		Entity entities[ChunkSize];
		ankerl::unordered_dense::map<ComponentId, void*> componentArrays;
		ankerl::unordered_dense::map<ComponentId, std::array<uint64_t, ChunkSize>> componentVersions;
		ankerl::unordered_dense::map<Entity, uint64_t> entityVersion;

		uint64_t chunkVersion = 0;

		void markComponentUpdated(ComponentId cid, size_t entityIndex) {
			auto frame = Global::CurrentFrame;
			componentVersions[cid][entityIndex] = frame;
			entityVersion[entities[entityIndex]] = frame;
			chunkVersion = frame;
		}

		bool isComponentDirty(ComponentId cid, size_t entityIndex, uint64_t sinceFrame) const {
			auto it = componentVersions.find(cid);
			if (it == componentVersions.end()) return false;
			return it->second[entityIndex] >= sinceFrame;
		}

		bool isChunkDirty(uint64_t sinceFrame) const {
			return chunkVersion >= sinceFrame;
		}

		~ComponentArchetypeChunk() {
			for (const auto& [id, ptr] : componentArrays) {
				const auto& [_, size, ops] = ComponentRegistry::GetComponentDataById(id);
				if (!ops.isTriviallyDestructible)
					for (size_t i = 0; i < ChunkSize; ++i) {
						void* obj = static_cast<char*>(ptr) + i * size;
						ops.destroy(obj);
					}

				operator delete(ptr);
			}
		}
	};

	class ComponentArchetype {
	public:
		ComponentArchetype(ComponentSignature signature) : m_bitset(signature) {};
		const ComponentSignature& getSignature() const {
			return m_bitset;
		}

		void addEntity(Entity entity, const std::vector<std::pair<ComponentId, void*>>& componentData) {
			ComponentArchetypeChunk* chunk = nullptr;
			if (m_chunks.empty() || m_chunks.back()->entityCount == ComponentArchetypeChunk::ChunkSize) {
				chunk = createChunk();
				m_chunks.emplace_back(chunk);
			}
			else {
				chunk = m_chunks.back().get();
			}
			size_t idx = chunk->entityCount;

			chunk->entities[idx] = entity;

			for (size_t i = 0; i < MAX_COMPONENTS; ++i) {
				if (m_bitset.test(i)) {
					const auto& [id, size, ops] = ComponentRegistry::GetComponentDataById(i);
					void* dest = static_cast<char*>(chunk->componentArrays[i]) + idx * size;

					auto it = std::find_if(componentData.begin(), componentData.end(),
						[i](const auto& p) { return p.first == i; });
					if (it != componentData.end()) {
						ops.copy(dest, it->second);
						chunk->markComponentUpdated(id, idx);
					}
				}
			}

			chunk->entityCount++;
			m_entityLocations[entity] = { chunk, idx };
		}

		void updateEntityComponents(Entity entity, const std::vector<std::pair<ComponentId, void*>>& updatedComponents) {
			auto it = m_entityLocations.find(entity);
			if (it == m_entityLocations.end()) return;

			ComponentArchetypeChunk* chunk = it->second.first;
			size_t idx = it->second.second;

			for (const auto& [cid, dataPtr] : updatedComponents) {
				if (!m_bitset.test(cid)) continue;

				const auto& [id, size, ops] = ComponentRegistry::GetComponentDataById(cid);
				void* dest = static_cast<char*>(chunk->componentArrays[cid]) + idx * size;

				if (!ops.isTriviallyDestructible) {
					ops.destroy(dest);
				}

				ops.copy(dest, dataPtr);

				chunk->markComponentUpdated(cid, idx);
			}
		}

		void removeEntity(Entity entity) {
			auto it = m_entityLocations.find(entity);
			if (it == m_entityLocations.end()) return;

			ComponentArchetypeChunk* chunk = it->second.first;
			size_t idx = it->second.second;

			size_t lastIndex = chunk->entityCount - 1;

			for (size_t i = 0; i < MAX_COMPONENTS; ++i) {
				if (m_bitset.test(i)) {
					const auto& [id, size, ops] = ComponentRegistry::GetComponentDataById(i);
					if (!ops.isTriviallyDestructible) {
						void* ptr = static_cast<char*>(chunk->componentArrays[i]) + idx * size;
						ops.destroy(ptr);
					}
				}
			}

			if (idx != lastIndex) {
				Entity lastEntity = chunk->entities[lastIndex];
				chunk->entities[idx] = lastEntity;

				for (size_t i = 0; i < MAX_COMPONENTS; ++i) {
					if (m_bitset.test(i)) {
						const auto& [id, size, ops] = ComponentRegistry::GetComponentDataById(i);
						void* dest = static_cast<char*>(chunk->componentArrays[i]) + idx * size;
						void* src = static_cast<char*>(chunk->componentArrays[i]) + lastIndex * size;

						ops.move(dest, src);
						if (!ops.isTriviallyDestructible) {
							ops.destroy(src);
						}
					}
				}

				m_entityLocations[lastEntity] = { chunk, idx };
			}

			chunk->entityCount--;
			m_entityLocations.erase(it);

			if (chunk->entityCount == 0) {
				auto it = std::find_if(m_chunks.begin(), m_chunks.end(), [&](const auto& ptr) {
					return ptr.get() == chunk;
					});
				if (it != m_chunks.end()) {
					m_chunks.erase(it);
				}
			}
		}

		bool hasComponent(Entity, ComponentId id) {
			return m_bitset.test(id);
		}

		template<typename T>
		std::optional<T> getComponent(Entity entity) {
			auto it = m_entityLocations.find(entity);
			if (it == m_entityLocations.end()) return std::nullopt;

			ComponentArchetypeChunk* chunk = it->second.first;
			size_t idx = it->second.second;
			ComponentId id = ComponentRegistry::GetComponentId<T>();

			if (!m_bitset.test(id)) return std::nullopt;

			const auto& [_, size, ops] = ComponentRegistry::GetComponentDataById(id);
			void* ptr = static_cast<char*>(chunk->componentArrays[id]) + idx * size;

			T copy;
			ops.copy(&copy, ptr);
			return copy;
		}

		bool isComponentDirty(Entity entity, ComponentId cid, uint64_t sinceFrame) const {
			auto it = m_entityLocations.find(entity);
			if (it == m_entityLocations.end()) return false;

			if (!it->second.first->isChunkDirty(sinceFrame)) return false;

			return it->second.first->isComponentDirty(cid, it->second.second, sinceFrame);
		}


		std::vector<Entity> getEntities(const ComponentSignature& signature) const {
			std::vector<Entity> result;

			for (auto& [entity, _] : m_entityLocations) {
				result.push_back(entity);
			}

			return result;
		}

		std::vector<Entity> getDirtyEntities(uint64_t sinceFrame) const {
			std::vector<Entity> dirtyEntities;

			// Iterate over all entities in this archetype
			for (const auto& [entity, location] : m_entityLocations) {
				ComponentArchetypeChunk* chunk = location.first;
				size_t idx = location.second;

				if (!chunk->isChunkDirty(sinceFrame)) continue;

				for (auto& [entity, version] : chunk->entityVersion) {
					if (version >= sinceFrame) {
						dirtyEntities.push_back(entity);
					}
				}
			}

			return dirtyEntities;
		}
	private:
		ComponentSignature m_bitset;
		std::vector<std::unique_ptr<ComponentArchetypeChunk>> m_chunks;
		ankerl::unordered_dense::map<Entity, std::pair<ComponentArchetypeChunk*, size_t>> m_entityLocations; //position in vector

		ComponentArchetypeChunk* createChunk() {
			auto chunk = std::make_unique<ComponentArchetypeChunk>();

			for (size_t i = 0; i < MAX_COMPONENTS; i++) {
				if (m_bitset.test(i)) {
					const auto& [id, size, ops] = ComponentRegistry::GetComponentDataById(i);
					void* array = operator new(size * ComponentArchetypeChunk::ChunkSize);
					chunk->componentArrays.emplace(i, array);
					chunk->componentVersions.emplace(i, std::array<uint64_t, ComponentArchetypeChunk::ChunkSize>{});

					for (size_t idx = 0; idx < ComponentArchetypeChunk::ChunkSize; ++idx) {
						ops.construct(static_cast<char*>(array) + idx * size);
					}
				}
			}

			chunk->entityCount = 0;
			return chunk.release();
		}
	};
}