#include "stdafx.h"

#pragma once

#include "../../../scene/Scene.h"
#include "../../../ecs/components/ComponentCamera.h"
#include "../../../ecs/components/ComponentTransform.h"
#include "../../../ecs/classes/ClassCamera.h"
#include "../memory/Resource.h"


namespace Engine::Render::Manager {
	class GlobalsManager {
	public:
		struct Globals {
			uint32_t transformsIndex, screenX, screenY, unsued;
		};
		void initialize(Scene::Scene* scene) {
			m_scene = scene;
			createGlobalsBuffer();
		}

		Render::Memory::Resource* update(Globals& globals) {
			if (std::memcmp(&m_globals, &globals, sizeof(Globals)) == 0) {
				return m_resource.get();
			}

			m_resource->writeDataD(&globals, 0, sizeof(Globals));
			m_globals = globals;
			return m_resource.get();
		}
	private:
		Scene::Scene* m_scene;
		Globals m_globals{};
		std::unique_ptr<Render::Memory::Resource> m_resource;

		void createGlobalsBuffer() {
			m_resource = Render::Memory::Resource::Create(D3D12_HEAP_TYPE_GPU_UPLOAD, sizeof(Globals));
		}
	};
}
