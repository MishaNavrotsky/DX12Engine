#include "stdafx.h"
#pragma once
#include "ISceneObject.h"


namespace Engine {
	class Scene {
	public:
		uint64_t addObject(std::unique_ptr<ISceneObject>&& sceneObject) noexcept {
			std::shared_ptr<ISceneObject> obj = std::move(sceneObject);
			m_scene.push_back(obj);
			return m_idIncreemntal++;
		}

		void render(ID3D12GraphicsCommandList* commandList) const {
			for (auto& sceneObj : m_scene) {
				if (!sceneObj->isCPULoadComplete() || !sceneObj->isGPULoadComplete()) continue;
				auto& renderables = sceneObj->getRenderables();
				for (auto& renderable : renderables) {
					renderable->render(commandList);
				}
			}
		}

		void prepareGPUData(ID3D12Device* device, ID3D12GraphicsCommandList* commandList) {
			for (auto& sceneObj : m_scene) {
				sceneObj->prepareGPUData();
			}
		}

		void releaseUnusedHeaps() {

		}

		void loadAll() {
			for (auto& sceneObj : m_scene) {
				sceneObj->prepareCPUData();
			}
		}

	private:
		std::vector<std::shared_ptr<ISceneObject>> m_scene;
		std::vector<std::shared_ptr<ISceneObject>> m_frustrum;
		uint64_t m_idIncreemntal = 0;
	};
}