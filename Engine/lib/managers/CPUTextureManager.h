#include "stdafx.h"

#pragma once

#include "../mesh/CPUTexture.h"
#include "IManager.h"

namespace Engine {
	class CPUTextureManager : public IManager<CPUTexture, CPUTextureManager> {};
}