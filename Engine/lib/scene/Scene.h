#include "stdafx.h"
#pragma once

#include "../ecs/EntityManager.h"
#include "graph/SceneGraph.h"
#include "assets/AssetManager.h"
#include "../systems/render/managers/ResourceManager.h"

namespace Engine::Scene {
	struct Scene {
		ECS::EntityManager entityManager;
		SceneGraph sceneGraph;
		AssetManager assetManager;
		Render::Manager::ResourceManager resourceManager;
	};
}
