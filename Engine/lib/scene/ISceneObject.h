#include "stdafx.h"
#pragma once

#include "../geometry/ModelMatrix.h"

namespace Engine {
	class ISceneObject {
	public:
		Engine::ModelMatrix modelMatrix;

		virtual bool isLoadComplete() const = 0;
		virtual void render(ID3D12GraphicsCommandList* commandList) = 0;

		virtual ~ISceneObject() = default;
	};
}
