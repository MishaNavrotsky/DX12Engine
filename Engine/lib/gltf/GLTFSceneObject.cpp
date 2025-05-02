#include "stdafx.h"
#include "GLTFSceneObject.h"

Engine::GLTFSceneObject::GLTFSceneObject(const std::filesystem::path& path) : m_path(path) {};

void Engine::GLTFSceneObject::prepareCPUData()
{
    auto meshes = GLTFLocal::GetMeshesInfo(m_path);
    auto& cpuMaterialManager = CPUMaterialManager::getInstance();
    auto& cpuMeshManager = CPUMeshManager::getInstance();
    auto& cpuTextureManager = CPUTextureManager::getInstance();
    auto& samplerManager = SamplerManager::getInstance();

    for (auto& mesh : *meshes) {
        auto& cpuMesh = cpuMeshManager.get(mesh);
        auto& id = cpuMesh.getMaterialId();
        auto& cpuMaterial = cpuMaterialManager.get(cpuMesh.getMaterialId());
        //m_renderables.push_back(std::move(mesh)); // Move into new vector
    }
    m_isCPULoaded = true;
}

bool Engine::GLTFSceneObject::isCPULoadComplete() const
{
    return m_isCPULoaded;
}

bool Engine::GLTFSceneObject::isGPULoadComplete() const
{
    return m_isGPULoaded;
}

void Engine::GLTFSceneObject::prepareGPUData()
{
    //for (auto& renderable : m_renderables) {
    //    Engine::Mesh* mesh = dynamic_cast<Engine::Mesh*>(renderable.get());
    //    if (mesh) {
    //        mesh->createBuffers(device, commandList);
    //    }
    //}
    //m_isGPULoaded = true;
}
