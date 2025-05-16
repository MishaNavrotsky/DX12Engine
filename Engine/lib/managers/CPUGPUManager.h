#include "stdafx.h"

#pragma once

#include "IManager.h"

namespace Engine {
	class CPUGPU: public IID{
	public:
		GUID gpuId;
	};
	class CPUGPUManager : public IManager<CPUGPU, CPUGPUManager> {};
}
