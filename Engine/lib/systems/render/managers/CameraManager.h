#include "stdafx.h"

#pragma once

#include "../../../scene/Scene.h"
#include "../../../ecs/components/ComponentCamera.h"
#include "../../../ecs/components/ComponentTransform.h"
#include "../../../ecs/classes/ClassCamera.h"
#include "../memory/Resource.h"


namespace Engine::Render::Manager {
	class CameraManager {
	public:
		void initialize(Scene::Scene* scene) {
			m_scene = scene;
			createCameraBuffer();
		}

		Render::Memory::Resource* update() {
			auto cameras = m_scene->entityManager.viewDirty<ECS::Component::ComponentCamera, ECS::Component::ComponentTransform>();
			uint64_t mainCameraEntity = 0;
			for (auto& camera : cameras) {
				auto com = m_scene->entityManager.getComponent<ECS::Component::ComponentCamera>(camera);
				if (com && (*com).isMain) {
					mainCameraEntity = camera;
				}
			}

			if (!mainCameraEntity) return m_resource.get();

			ECS::Component::ComponentTransform componentTransform = *m_scene->entityManager.getComponent<ECS::Component::ComponentTransform>(mainCameraEntity);
			ECS::Component::ComponentCamera componentCamera = *m_scene->entityManager.getComponent<ECS::Component::ComponentCamera>(mainCameraEntity);

			ECS::Class::ClassCamera camera(componentCamera, componentTransform);

			auto cameraData = camera.getCameraData();
			m_resource->writeDataD(cameraData.get(), 0, sizeof(ECS::Class::ClassCamera::CameraData));
			return m_resource.get();
		}
	private:
		Scene::Scene* m_scene;
		std::unique_ptr<Render::Memory::Resource> m_resource;

		void createCameraBuffer() {
			m_resource = Render::Memory::Resource::Create(D3D12_HEAP_TYPE_GPU_UPLOAD, sizeof(ECS::Class::ClassCamera::CameraData));
			//D3D12_FEATURE_DATA_D3D12_OPTIONS16 options16 = {};
			//bool GPUUploadHeapSupported = false;
			//if (SUCCEEDED(m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS16, &options16, sizeof(options16))))
			//{
			//	GPUUploadHeapSupported = options16.GPUUploadHeapSupported;
			//}
		}

	};
}
