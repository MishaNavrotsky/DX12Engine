#include "stdafx.h"

#pragma once

#include "../mesh/GPUMaterial.h"
#include "IManager.h"

namespace Engine {
	class GPUMaterialManager : public IManager<GPUMaterial, GPUMaterialManager> {};
}