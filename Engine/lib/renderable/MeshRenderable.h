#pragma once

#include "../scene/ISceneRenderable.h"
#include "../mesh/GPUMesh.h"

namespace Engine {
	class MeshRenderable : public ISceneRenderable {
	public:
		MeshRenderable(GPUMesh& gpuMesh) : m_gpuMesh(gpuMesh) {};
		void render(ID3D12GraphicsCommandList* commandList) override;
	private:
		GPUMesh& m_gpuMesh;
	};
}
