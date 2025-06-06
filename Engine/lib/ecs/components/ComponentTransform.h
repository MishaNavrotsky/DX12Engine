#include "stdafx.h"

#pragma once


namespace Engine::ECS::Component {
	struct ComponentTransform {
		DX::XMFLOAT4 position;
		DX::XMFLOAT4 rotation;
		DX::XMFLOAT4 scale;
	};
}
