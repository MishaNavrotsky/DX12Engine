#include "stdafx.h"

#pragma once

#include "../descriptors/BindlessHeapDescriptor.h"
#include "../../../ecs/classes/ClassTransform.h"
#include "../../../scene/Scene.h"
#include "../../../ecs/entity/Entity.h"
#include "../memory/Resource.h"
#include "../../../helpers.h"
#include "../../../ecs/components/ComponentTransform.h"


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

		std::pair<Memory::Resource*, uint32_t> update() {
			auto dirtyTransforms = m_scene->entityManager.viewDirty<ECS::Component::ComponentTransform>();
			for (auto entity : dirtyTransforms) {
				const auto optTransform = m_scene->entityManager.getComponent<ECS::Component::ComponentTransform>(entity);
				const ECS::Component::ComponentTransform& transform = *optTransform;
				const ECS::Class::ClassTransform classTransform(transform);
				auto const tranformData = classTransform.getTransformData();

				auto itt = m_entityTransformPosition.find(entity);
				if (itt == m_entityTransformPosition.end()) {
					if (m_entityTransformPosition.size() == TRANSFORM_COUNT) throw std::runtime_error("[TransformMatrixManager] too many transform matrices");
					auto position = m_lastOffset.fetch_add(1, std::memory_order_seq_cst);
					auto offset = position * ELEMENT_SIZE;
					m_resource->writeDataD(tranformData.get(), offset, ELEMENT_SIZE);
					m_entityTransformPosition.emplace(entity, position);
					continue;
				}

				auto offset = m_entityTransformPosition.at(entity) * ELEMENT_SIZE;
				m_resource->writeDataD(tranformData.get(), offset, ELEMENT_SIZE);
			}
			return { m_resource.get(), m_bindlessSlot };
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
		std::atomic<size_t> m_lastOffset{0};
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
