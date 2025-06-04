#include "stdafx.h"

#pragma once


#include "../../../scene/assets/AssetStructures.h"
#include "../../../helpers.h"
#include "../queus/DirectQueue.h"
#include "../Device.h"
#include "./RenderableManagerStructures.h"


namespace Engine::Render::Manager {
	class RenderableManager {
	public:
		void initialize(Queue::DirectQueue& directQueue) {
			auto* device = Device::GetDevice();
			WPtr<ID3D12GraphicsCommandList> commandList;
			WPtr<ID3D12CommandAllocator> commandAllocator;
			WPtr<ID3D12Fence> fence;
			UINT64 fenceValue = 1;

			ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator)));
			ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList)));
			ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
			HANDLE fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
			if (!fenceEvent) {
				ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
			}

			std::vector<std::byte> defaultAttributeData;
			for (auto& defaultAttribute : m_defaultAttributes) {
				auto& v = defaultAttribute.first.data;
				if (v) {
					auto& value = v.value();
					defaultAttributeData.insert(defaultAttributeData.end(), value.begin(), value.end());
				}
			}

			auto uploadResource = Memory::Resource::CreateV(D3D12_HEAP_TYPE_UPLOAD, static_cast<uint32_t>(defaultAttributeData.size()), D3D12_RESOURCE_STATE_COPY_SOURCE);
			m_resource = Memory::Resource::Create(D3D12_HEAP_TYPE_DEFAULT, static_cast<uint32_t>(defaultAttributeData.size()), D3D12_RESOURCE_STATE_COMMON);

			uint64_t offset = 0;
			for (auto& defaultAttribute : m_defaultAttributes) {
				if (defaultAttribute.first.data) {
					defaultAttribute.second.BufferLocation = m_resource->getResource()->GetGPUVirtualAddress() + offset;
					offset += defaultAttribute.first.data.value().size();
				}
			}

			uploadResource.writeData(defaultAttributeData.data(), defaultAttributeData.size());
			m_resource->copyResource(commandList.Get(), &uploadResource);

			CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
				m_resource->getResource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
			commandList->ResourceBarrier(1, &barrier);
			commandList->Close();

			ID3D12CommandList* ppCommandLists[] = { commandList.Get() };
			directQueue.getQueue()->ExecuteCommandLists(1, ppCommandLists);
			ThrowIfFailed(directQueue.getQueue()->Signal(fence.Get(), fenceValue));

			if (fence->GetCompletedValue() < fenceValue) {
				ThrowIfFailed(fence->SetEventOnCompletion(fenceValue, fenceEvent));
				WaitForSingleObject(fenceEvent, INFINITE);
			}
			CloseHandle(fenceEvent);
		}
		void addMeshAsset(Scene::Asset::MeshId meshId, Scene::Asset::Mesh& mesh) {
			RenderableMesh renderableMesh{};

			for (auto& submesh : mesh.subMeshes) {
				RenderableSubMesh renderableSubMesh{};
				renderableSubMesh.normal = m_defaultAttributes[static_cast<uint64_t>(AssetsCreator::Asset::AttributeType::NORMAL)].second;
				renderableSubMesh.tangent = m_defaultAttributes[static_cast<uint64_t>(AssetsCreator::Asset::AttributeType::TANGENT)].second;
				renderableSubMesh.texcoord = m_defaultAttributes[static_cast<uint64_t>(AssetsCreator::Asset::AttributeType::TEXCOORD)].second;

				for (auto& att : submesh.gpuData.attributes) {
					D3D12_VERTEX_BUFFER_VIEW view{};
					view.BufferLocation = *att.gpuVirtualAddress;
					view.SizeInBytes = static_cast<uint32_t>(att.attribute.sizeInBytes);
					view.StrideInBytes = Helpers::GetFormatStride(att.attribute.format);

					if (att.attribute.typeIndex == 0) {
						switch (att.attribute.type) {
						case AssetsCreator::Asset::AttributeType::POSITION:
							renderableSubMesh.position = std::move(view);
							break;
						case AssetsCreator::Asset::AttributeType::NORMAL:
							renderableSubMesh.normal = std::move(view);
							break;
						case AssetsCreator::Asset::AttributeType::TEXCOORD:
							renderableSubMesh.texcoord = std::move(view);
							break;
						case AssetsCreator::Asset::AttributeType::TANGENT:
							renderableSubMesh.tangent = std::move(view);
							break;
						}
					}
				}
				renderableMesh.subMeshes.push_back(renderableSubMesh);
			}

			m_meshRenderables.emplace(meshId, std::move(renderableMesh));
		}
	private:
		tbb::concurrent_unordered_map<Scene::Asset::MeshId, RenderableMesh> m_meshRenderables;

		std::vector<std::pair<Scene::Asset::CpuAttributeData, D3D12_VERTEX_BUFFER_VIEW>> m_defaultAttributes = GenerateGLTFDefaultCPUAttributes();
		std::unique_ptr<Memory::Resource> m_resource;

		static std::vector<std::pair<Scene::Asset::CpuAttributeData, D3D12_VERTEX_BUFFER_VIEW>> GenerateGLTFDefaultCPUAttributes() {
			using namespace AssetsCreator::Asset;

			std::vector<std::pair<Scene::Asset::CpuAttributeData, D3D12_VERTEX_BUFFER_VIEW>> attributes;

			auto add = [&](AssetsCreator::Asset::AttributeType type, uint32_t index, DXGI_FORMAT format, uint64_t sizeInBytes, const void* defaultRawData = nullptr) {
				VertexAttribute attr{
					.type = type,
					.typeIndex = index,
					.format = format,
					.sizeInBytes = sizeInBytes
				};

				std::optional<std::vector<std::byte>> data;
				if (defaultRawData) {
					data.emplace(sizeInBytes);
					std::memcpy(data->data(), defaultRawData, sizeInBytes);
				}
				D3D12_VERTEX_BUFFER_VIEW view{};
				view.SizeInBytes = static_cast<uint32_t>(sizeInBytes);
				view.StrideInBytes = Helpers::GetFormatStride(format);

				attributes.push_back(std::pair{ Scene::Asset::CpuAttributeData{ attr, std::move(data) }, view });
				};

			add(AttributeType::POSITION, 0, DXGI_FORMAT_R32G32B32_FLOAT, 12, nullptr);

			const float normal[3] = { 0.0f, 0.0f, 1.0f };
			add(AttributeType::NORMAL, 0, DXGI_FORMAT_R32G32B32_FLOAT, sizeof(normal), normal);

			const float tangent[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
			add(AttributeType::TANGENT, 0, DXGI_FORMAT_R32G32B32A32_FLOAT, sizeof(tangent), tangent);

			const float bitangent[3] = { 0.0f, 0.0f, 0.0f };
			add(AttributeType::BITANGENT, 0, DXGI_FORMAT_R32G32B32_FLOAT, sizeof(bitangent), bitangent);

			const float texcoord[2] = { 0.0f, 0.0f };
			add(AttributeType::TEXCOORD, 0, DXGI_FORMAT_R32G32_FLOAT, sizeof(texcoord), texcoord);

			const uint8_t joints[4] = { 0, 0, 0, 0 };
			add(AttributeType::JOINT, 0, DXGI_FORMAT_R8G8B8A8_UINT, sizeof(joints), joints);

			const float weights[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
			add(AttributeType::WEIGHT, 0, DXGI_FORMAT_R32G32B32A32_FLOAT, sizeof(weights), weights);

			const float color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
			add(AttributeType::COLOR, 0, DXGI_FORMAT_R32G32B32A32_FLOAT, sizeof(color), color);

			return attributes;
		}

	};
}