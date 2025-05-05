#include "stdafx.h"

#pragma once

#include "IManager.h"
#include "../mesh/Model.h"

namespace Engine {
	class ModelManager: public IManager<Model, ModelManager> {

	};
}
