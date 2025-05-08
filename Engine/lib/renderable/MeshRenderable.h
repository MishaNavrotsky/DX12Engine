#pragma once

#include "../mesh/GPUMesh.h"
#include "../mesh/CPUMesh.h"
#include "../mesh/CPUMaterial.h"
#include "../mesh/GPUMaterial.h"


namespace Engine {
	class MeshRenderable {
	public:
		MeshRenderable(CPUMesh& cpuMesh, GPUMesh& gpuMesh, CPUMaterial& cpuMaterial, GPUMaterial& gpuMaterial) : 
			m_cpuMesh(cpuMesh), m_gpuMesh(gpuMesh),
			m_cpuMaterial(cpuMaterial), m_gpuMaterial(gpuMaterial)
		{};
		void render(ID3D12GraphicsCommandList* commandList);

		CPUMesh& getCPUMesh() {
			return m_cpuMesh;
		}

		GPUMesh& getGPUMesh() {
			return m_gpuMesh;
		}

		CPUMaterial& getCPUMaterial() {
			return m_cpuMaterial;
		}

		GPUMaterial& getGPUMaterial() {
			return m_gpuMaterial;
		}

	private:
		CPUMesh& m_cpuMesh;
		GPUMesh& m_gpuMesh;
		CPUMaterial& m_cpuMaterial;
		GPUMaterial& m_gpuMaterial;
	};
}
