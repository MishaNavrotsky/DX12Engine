#include "stdafx.h"

#pragma once

namespace Engine {
	class ISceneRenderable {
	public:
		virtual void render(ID3D12GraphicsCommandList* commandList) = 0;
	protected:

	};
}
