#include "stdafx.h"
#include "GLTFSceneObject.h"
#include "../Device.h"

Engine::GLTFSceneObject::GLTFSceneObject(const std::filesystem::path& path) : m_path(path) {
	m_modelId = ModelLoader::GetInstance().queueGLTF(m_path);
	m_model = &m_modelManager.get(m_modelId);
	std::thread(&GLTFSceneObject::onLoadComplete, this).detach();
}
bool Engine::GLTFSceneObject::isLoadComplete() const
{
	return m_model->getIsLoaded();
}

void Engine::GLTFSceneObject::onLoadComplete()
{
	m_model->waitForIsLoaded();

	auto gpuMeshes = m_gpuMeshManager.getMany(m_model->getGPUMeshIds());
	auto cpuMeshes = m_cpuMeshManager.getMany(m_model->getCPUMeshIds());

	auto device = Device::GetDevice();

	for (size_t i = 0; i < gpuMeshes.size(); ++i) {
		auto& cpuMaterial = m_cpuMaterialManager.get(cpuMeshes[i].get().getMaterialId());
		auto& gpuMaterial = m_gpuMaterialManager.get(gpuMeshes[i].get().getGPUMaterialId());

		auto meshRenderable = std::make_unique<MeshRenderable>(cpuMeshes[i], gpuMeshes[i], cpuMaterial, gpuMaterial);
		m_renderables.push_back(std::move(meshRenderable));

		ComPtr<ID3D12Resource> res;

		D3D12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(Align(sizeof(CBVMeshData), 256));
		D3D12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		auto device = Device::GetDevice();

		ThrowIfFailed(device->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&bufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&res)));
		m_cbvs.push_back(std::move(res));
	}
}
void Engine::GLTFSceneObject::render(ID3D12GraphicsCommandList* commandList)
{
	for (size_t i = 0; i < m_renderables.size(); i++) {
		auto& r = m_renderables[i];
		auto& cpuMesh = r->getCPUMesh();
		auto& gpuMesh = r->getGPUMesh();
		auto& cpuMaterial = r->getCPUMaterial();
		auto& gpuMaterial = r->getGPUMaterial();
		auto& cbv = m_cbvs[i];

		CBVMeshData cbvData = {};
		cbvData.cbvDataBindlessHeapSlot.x = cpuMaterial.getCBVDataBindlessHeapSlot();
		XMStoreFloat4x4(&cbvData.modelMatrix, XMMatrixTranspose(modelMatrix.getModelMatrix()));

		{
			void* mappedData = nullptr;
			ThrowIfFailed(cbv->Map(0, nullptr, &mappedData));
			memcpy(mappedData, &cbvData, sizeof(CBVMeshData));
			cbv->Unmap(0, nullptr);
		}

		commandList->SetGraphicsRootConstantBufferView(1, cbv->GetGPUVirtualAddress());
		r->render(commandList);
	}

};