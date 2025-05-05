#include "stdafx.h"

#pragma once

#include "../mesh/GPUTexture.h"
#include "IManager.h"

namespace Engine {
	class GPUTextureManager : public IManager<GPUTexture, GPUTextureManager> {};
}