#include "stdafx.h"
#include "GLTFSceneObject.h"

Engine::GLTFSceneObject::GLTFSceneObject(const std::filesystem::path& path) : m_path(path) {
	m_modelId = ModelLoader::getInstance().queueGLTF(m_path);
	m_model = &m_modelManager.get(m_modelId);
	std::thread(&GLTFSceneObject::onLoadComplete, this).detach();
}
bool Engine::GLTFSceneObject::isLoadComplete() const
{
	return m_model->getIsLoaded();
}
std::vector<std::unique_ptr<Engine::ISceneRenderable>>& Engine::GLTFSceneObject::getRenderables()
{
	return m_renderables;
	// TODO: insert return statement here
}
void Engine::GLTFSceneObject::onLoadComplete()
{
	m_model->waitForIsLoaded();

	auto gpuMeshes = m_gpuMeshManager.getMany(m_model->getGPUMeshIds());
	for (auto& gpuMesh : gpuMeshes) {
		auto meshRenderable = std::make_unique<MeshRenderable>(gpuMesh);
		m_renderables.push_back(std::move(meshRenderable));
	}	
};