#include "stdafx.h"
#pragma once

#include "ISceneRenderable.h"
#include "../geometry/ModelMatrix.h"

namespace Engine {
	class ISceneObject {
	public:
		Engine::ModelMatrix modelMatrix;

		virtual bool isLoadComplete() const = 0;

		virtual std::vector<std::unique_ptr<ISceneRenderable>>& getRenderables() = 0;
		virtual ~ISceneObject() = default;
	};
}
