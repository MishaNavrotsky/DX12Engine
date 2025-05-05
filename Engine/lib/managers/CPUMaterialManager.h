#include "stdafx.h"

#pragma once

#include "../mesh/CPUMaterial.h"
#include "IManager.h"

namespace Engine {
	class CPUMaterialManager : public IManager<CPUMaterial, CPUMaterialManager> {};
}
