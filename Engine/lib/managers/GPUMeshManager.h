#include "stdafx.h"

#pragma once

#include "../mesh/GPUMesh.h"
#include "IManager.h"

namespace Engine {
	class GPUMeshManager : public IManager<GPUMesh, GPUMeshManager> {};
}
