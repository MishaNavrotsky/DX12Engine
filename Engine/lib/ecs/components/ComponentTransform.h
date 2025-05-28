#include "stdafx.h"

#pragma once

#include "ComponentWrapper.h"

namespace Engine::ECS::Component {
	struct ComponentTransform {
		DX::XMFLOAT4 position;
		DX::XMFLOAT4 rotation;
		DX::XMFLOAT4 scale;
	};

	using WComponentTransform = ComponentWrapper<ComponentTransform>;
}
