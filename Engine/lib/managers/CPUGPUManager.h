#include "stdafx.h"

#pragma once

#include "IManager.h"

namespace Engine {
	struct CPUGPU {
		GUID cpuId;
		GUID gpuId;
	};
	class CPUGPUManager : public IManager<CPUGPU, CPUGPUManager> {};
}
