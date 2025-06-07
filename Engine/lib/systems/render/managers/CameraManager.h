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
			auto cameras = m_scene->entityManager.view<ECS::Component::ComponentCamera, ECS::Component::ComponentTransform>();
			auto mainCameraEntity = 0;
			for (auto& camera : cameras) {
				auto com = m_scene->entityManager.getComponent<ECS::Component::ComponentCamera>(camera);
				if (com && (*com).isMain) {
					mainCameraEntity = camera;
				}
			}

			if (!mainCameraEntity) return nullptr;

			ECS::Component::ComponentTransform componentTransform = *m_scene->entityManager.getComponent<ECS::Component::ComponentTransform>(mainCameraEntity);
			ECS::Component::ComponentCamera componentCamera = *m_scene->entityManager.getComponent<ECS::Component::ComponentCamera>(mainCameraEntity);

			ECS::Class::ClassCamera camera(componentCamera, componentTransform);

			auto cameraData = camera.getCameraData();
			m_cameraBuffer->writeData(&cameraData, sizeof(ECS::Class::ClassCamera::CameraData));
			m_cameraBuffer->resetOffset();
			return m_cameraBuffer.get();
		}
	private:
		Scene::Scene* m_scene;
		std::unique_ptr<Render::Memory::Resource> m_cameraBuffer;

		void createCameraBuffer() {
			m_cameraBuffer = Render::Memory::Resource::Create(D3D12_HEAP_TYPE_GPU_UPLOAD, sizeof(ECS::Class::ClassCamera::CameraData));
			//D3D12_FEATURE_DATA_D3D12_OPTIONS16 options16 = {};
			//bool GPUUploadHeapSupported = false;
			//if (SUCCEEDED(m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS16, &options16, sizeof(options16))))
			//{
			//	GPUUploadHeapSupported = options16.GPUUploadHeapSupported;
			//}
		}

	};
}
