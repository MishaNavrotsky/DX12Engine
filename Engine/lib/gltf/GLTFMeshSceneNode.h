#include "stdafx.h"

#pragma once

#include "../scene/SceneNode.h"
#include "../loaders/ModelLoader.h"
#include "../renderable/MeshRenderable.h"
#include "../mesh/CPUMesh.h"
#include "../mesh/GPUMesh.h"
#include "../mesh/CPUMaterial.h"
#include "../mesh/GPUMaterial.h"
#include "../helpers.h"


namespace Engine {
	struct CBVMeshData {
		XMFLOAT4X4 modelMatrix;
		XMFLOAT4X4 prevModelMatrix;
		XMFLOAT4X4 MVrP;

		XMUINT4 cbvDataBindlessHeapSlot;
	};
	class GLTFMeshSceneNode : public SceneNode {
	public:
		static std::shared_ptr<GLTFMeshSceneNode> CreateFromMesh(CPUMesh& cpuMesh, GPUMesh& gpuMesh, CPUMaterial& cpuMaterial, GPUMaterial& gpuMaterial) {
			return std::shared_ptr<GLTFMeshSceneNode>(new GLTFMeshSceneNode(cpuMesh, gpuMesh, cpuMaterial, gpuMaterial));
		}

		ID3D12Resource* getResource() override {
			return res.Get();
		}

		void draw(ID3D12GraphicsCommandList* commandList, Camera* camera, const std::function<bool(CPUMesh&, GPUMesh&, SceneNode* node)>& callback) override {
			CBVMeshData cbvData = {};
			cbvData.cbvDataBindlessHeapSlot.x = m_cpuMaterial.getCBVDataBindlessHeapSlot();
			XMStoreFloat4x4(&cbvData.modelMatrix, XMMatrixTranspose(m_worldMatrix));
			XMStoreFloat4x4(&cbvData.prevModelMatrix, XMMatrixTranspose(m_prevWorldMatrix));
			XMStoreFloat4x4(&cbvData.MVrP, XMMatrixTranspose(m_worldMatrix * camera->getViewProjReverseDepth()));
			{
				void* mappedData = nullptr;
				ThrowIfFailed(res->Map(0, nullptr, &mappedData));
				memcpy(mappedData, &cbvData, sizeof(CBVMeshData));
				res->Unmap(0, nullptr);
			}

			//auto& cameraFrustum = camera->getFrustum();
			//if (!Helpers::IsAABBInFrustum(cameraFrustum, m_worldSpaceAABB.min, m_worldSpaceAABB.max)) return;

			if(callback(m_cpuMesh, m_gpuMesh, this)) return;

			m_meshRenderable->render(commandList);
		}

		SceneNodeType getType() const override {
			return SceneNodeType::Mesh;
		}
		
		AABB& getWorldSpaceAABB() override {
			return m_worldSpaceAABB;
		}

		void update(const XMMATRIX& parentWorldMatrix = XMMatrixIdentity()) override {
			bool oldShouldUpdate = shouldUpdate;
			SceneNode::update(parentWorldMatrix);

			if (oldShouldUpdate) {
				Helpers::TransformAABB_ObjectToWorld(m_cpuMesh.getAABB(), m_worldMatrix, m_worldSpaceAABB);
			}
			 
		}
	private:
		GLTFMeshSceneNode(CPUMesh& cpuMesh, GPUMesh& gpuMesh, CPUMaterial& cpuMaterial, GPUMaterial& gpuMaterial): m_cpuMesh(cpuMesh), m_gpuMesh(gpuMesh), m_cpuMaterial(cpuMaterial), m_gpuMaterial(gpuMaterial) {
			m_meshRenderable = std::make_unique<MeshRenderable>(cpuMesh, gpuMesh, cpuMaterial, gpuMaterial);

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
		}


		std::unique_ptr<MeshRenderable> m_meshRenderable;
		ComPtr<ID3D12Resource> res;
		CPUMesh& m_cpuMesh;
		GPUMesh& m_gpuMesh;
		CPUMaterial& m_cpuMaterial;
		GPUMaterial& m_gpuMaterial;
		AABB m_worldSpaceAABB;
	};
}
