#include "stdafx.h"
#pragma once

#include "ISceneRenderable.h"
#include "../geometry/ModelMatrix.h"

namespace Engine {
	class ISceneObject {
	public:
		Engine::ModelMatrix modelMatrix;

		virtual void prepareCPUData() = 0;
		virtual void prepareGPUData() = 0;

		virtual bool isCPULoadComplete() const = 0;
		virtual bool isGPULoadComplete() const = 0;

		std::vector<std::unique_ptr<ISceneRenderable>>& getRenderables() {
			return m_renderables;
		}
		virtual ~ISceneObject() {}
	protected:
		std::vector<std::unique_ptr<ISceneRenderable>> m_renderables;
	};
}
