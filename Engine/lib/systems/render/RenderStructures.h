#include "stdafx.h"

#pragma once

#include <Structures.h>

namespace Engine::Render {
	struct VertexAttribute {
		AssetsCreator::Asset::AttributeType type;
		uint32_t typeIndex;
		DXGI_FORMAT format;
		uint64_t sizeInBytes;
	};
}
