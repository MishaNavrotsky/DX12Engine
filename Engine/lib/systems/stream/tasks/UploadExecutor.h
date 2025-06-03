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

			auto& asset = scene->assetManager.getMeshAsset(event.id);
			if (args->uploadPlan.uploadType == GpuUploadType::DirectStorage) {
				auto dqueue = args->streamingSystemArgs->getDqueue();
				auto dfactory = args->streamingSystemArgs->getDfactory();
				auto& uploadTypeData = std::get<DSMeshUploadTypeData>(args->uploadPlan.uploadTypeData);
				if (uploadTypeData.attReq.second) {
					dqueue->EnqueueRequest(&uploadTypeData.attReq.first);
				}
				if (uploadTypeData.indReq.second) {
					dqueue->EnqueueRequest(&uploadTypeData.indReq.first);
				}
				if (uploadTypeData.skiReq.second) {
					dqueue->EnqueueRequest(&uploadTypeData.skiReq.first);
				}

				dqueue->EnqueueSignal(args->fence.Get(), ++args->fenceValue);
				dqueue->Submit();
				while (args->fence->GetCompletedValue() < args->fenceValue) {
					ftl::YieldThread();
				}
				ts->AddTask({ GpuBufferFinalizer::FinalizeMesh, arg }, ftl::TaskPriority::Normal);
			}
		}
	};
}
