#include "stdafx.h"

#pragma once

#include "./GpuBufferFinalizer.h"
#include "../../../scene/assets/AssetStructures.h"
#include "GpuUploadPlannerStructures.h"
#include "../StreamingStructures.h"

namespace Engine::System::Streaming {
	class UploadExecutor {
	public:
		static void ExecuteMesh(ftl::TaskScheduler* ts, void* arg) {
			auto args = reinterpret_cast<MeshArgs*>(arg);
			args->step = StreamingStep::UploadExecutor;
			auto event = args->event;
			auto scene = args->streamingSystemArgs->getScene();

			auto* asset = event.asset;
			asset->status.store(Scene::Asset::Status::Loading, std::memory_order_release);
			if (args->uploadPlan.uploadType == GpuUploadType::DirectStorage) {
				auto dqueue = args->streamingSystemArgs->getDqueue();
				auto dfactory = args->streamingSystemArgs->getDfactory();
				auto& uploadTypeData = std::get<DSMeshUploadTypeData>(args->uploadPlan.uploadTypeData);
				if (uploadTypeData.attReq)
					dqueue->EnqueueRequest(&uploadTypeData.attReq.value());

				if (uploadTypeData.indReq)
					dqueue->EnqueueRequest(&uploadTypeData.indReq.value());

				if (uploadTypeData.skiReq)
					dqueue->EnqueueRequest(&uploadTypeData.skiReq.value());


				dqueue->EnqueueSignal(args->fence.Get(), ++args->fenceValue);
				dqueue->Submit();
				while (args->fence->GetCompletedValue() < args->fenceValue) {
					ftl::YieldThread();
				}
				auto* device = args->device;
				WPtr<ID3D12GraphicsCommandList> commandList;
				WPtr<ID3D12CommandAllocator> commandAllocator;
				ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator)));
				ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList)));
				if (uploadTypeData.attReq) {
					auto* res = uploadTypeData.attReq.value().Destination.Buffer.Resource;
					CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
						res, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
					commandList->ResourceBarrier(1, &barrier);
				}
				if (uploadTypeData.indReq) {
					auto* res = uploadTypeData.indReq.value().Destination.Buffer.Resource;
					CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
						res, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);
					commandList->ResourceBarrier(1, &barrier);
				}
				if (uploadTypeData.skiReq) {
					auto* res = uploadTypeData.skiReq.value().Destination.Buffer.Resource;
					CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
						res, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
					commandList->ResourceBarrier(1, &barrier);
				}
				ThrowIfFailed(commandList->Close());
				ID3D12CommandList* ppCommandLists[] = { commandList.Get() };
				args->commandQueue->ExecuteCommandLists(1, ppCommandLists);
				args->commandQueue->Signal(args->fence.Get(), ++args->fenceValue);
				while (args->fence->GetCompletedValue() < args->fenceValue) {
					ftl::YieldThread();
				}

				asset->status.store(Scene::Asset::Status::Loaded, std::memory_order_release);
				ts->AddTask({ GpuBufferFinalizer::FinalizeMesh, arg }, ftl::TaskPriority::Normal);
			}
		}
	};
}
