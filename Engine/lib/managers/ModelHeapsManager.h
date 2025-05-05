#include "stdafx.h"

#pragma once

#include "helpers.h"
#include "IManager.h"
#include "../mesh/ModelHeaps.h"

namespace Engine {
	using Microsoft::WRL::ComPtr;

	class ModelHeapsManager : public IManager<ModelHeaps, ModelHeapsManager> {};
}