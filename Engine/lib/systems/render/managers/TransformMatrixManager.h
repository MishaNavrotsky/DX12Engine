#include "stdafx.h"

#pragma once

#include "../descriptors/BindlessHeapDescriptor.h"
#include "../../../ecs/classes/ClassTransform.h"
#include "../../../scene/Scene.h"
#include "../../../ecs/entity/Entity.h"
#include "../memory/Resource.h"
#include "../../../helpers.h"
#include "../../../ecs/components/Components.h"


namespace Engine::Render::Manager {
	class TransformMatrixManager {
		static constexpr size_t ELEMENT_SIZE = sizeof(ECS::Class::ClassTransform::TransformData);
		static constexpr uint64_t TRANSFORM_COUNT = 100000;
		static constexpr size_t RESOURCE_SIZE = (TRANSFORM_COUNT * ELEMENT_SIZE + D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT - 1) & ~(D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT - 1);
	public:
		void initialize(Scene::Scene* scene, Descriptor::BindlessHeapDescriptor* bindlessHeap) {
			m_scene = scene;
			m_entityTransformPosition.reserve(TRANSFORM_COUNT);
			createTrasfromMatricesResource();
			createSrv(bindlessHeap);
		}

		Memory::Resource* update() {
			auto& registry = m_scene->entityManager.getRegistry();
			const auto& group = registry.group_if_exists<ECS::Component::ComponentTransform, ECS::Component::ComponentTransformDirty>();
			for (const auto& [entity, transform] : group.each()) {
				const ECS::Class::ClassTransform classTransform(transform);
				auto const tranformData = classTransform.getTransformData();

				auto itt = m_entityTransformPosition.find(entity);
				if (itt == m_entityTransformPosition.end()) {
					if (m_entityTransformPosition.size() == TRANSFORM_COUNT) throw std::runtime_error("[TransformMatrixManager] too many transform matrices");
					auto position = m_lastOffset.fetch_add(1, std::memory_order_seq_cst);
					auto offset = position * ELEMENT_SIZE;
					m_resource->writeDataD(tranformData.get(), offset, ELEMENT_SIZE);
					m_entityTransformPosition.emplace(entity, position);

					m_scene->entityManager.removeComponent<ECS::Component::ComponentTransformDirty>(entity);
					continue;
				}

				auto offset = itt->second * ELEMENT_SIZE;
				m_resource->writeDataD(tranformData.get(), offset, ELEMENT_SIZE);
				m_scene->entityManager.removeComponent<ECS::Component::ComponentTransformDirty>(entity);
			}

			return m_resource.get();
		}

		uint32_t getBindlessSlot() {
			return m_bindlessSlot;
		}

		std::optional<size_t> getEntityTransformPosition(ECS::Entity entity) {
			auto itt = m_entityTransformPosition.find(entity);
			if (itt == m_entityTransformPosition.end()) return std::nullopt;

			return itt->second;
		}
	private:
		Scene::Scene* m_scene;
		std::unique_ptr<Memory::Resource> m_resource;
		ankerl::unordered_dense::map<ECS::Entity, size_t> m_entityTransformPosition;
		std::atomic<size_t> m_lastOffset{ 0 };
		uint32_t m_bindlessSlot = 0;

		void createTrasfromMatricesResource() {
			m_resource = Memory::Resource::Create(D3D12_HEAP_TYPE_GPU_UPLOAD, RESOURCE_SIZE);
		}

		void createSrv(Descriptor::BindlessHeapDescriptor* bindlessHeap) {
			D3D12_SHADER_RESOURCE_VIEW_DESC desc{};
			desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
			desc.Format = DXGI_FORMAT_UNKNOWN;
			desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			desc.Buffer.NumElements = TRANSFORM_COUNT;
			desc.Buffer.StructureByteStride = ELEMENT_SIZE;
			desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

			m_bindlessSlot = bindlessHeap->addSrv(m_resource.get(), desc);
		}

	};
}
