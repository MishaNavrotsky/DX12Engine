#include "stdafx.h"

#pragma once

#include "../mesh/Sampler.h"
#include "IManager.h"
namespace Engine {
	class SamplerManager : public IManager<Sampler, SamplerManager> {};
}