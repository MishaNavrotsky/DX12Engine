#include "stdafx.h"

#pragma once

#include "../mesh/CPUMesh.h"
#include "IManager.h"

namespace Engine {
	class CPUMeshManager : public IManager<CPUMesh, CPUMeshManager>{};
}