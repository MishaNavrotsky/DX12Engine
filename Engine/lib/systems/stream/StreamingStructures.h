#include "stdafx.h"

#pragma once

#include "StreamingSystemArgs.h"
#include "../../scene/assets/AssetStructures.h"
#include "tasks/GpuUploadPlannerStructures.h"


namespace Engine::System::Streaming {
	enum class StreamingStep {
		MetadataLoader,
		GpuUploadPlanner,
		UploadExecutor,
		GpuBufferFinalizer
	};
	using StreamingRequestId = uint64_t;
	struct Args {
		std::atomic<StreamingStep> step;
		StreamingSystemArgs* streamingSystemArgs;
		StreamingRequestId streamingRequestId;
		WPtr<ID3D12Fence> fence;
		uint64_t fenceValue;
		std::function<void()> finalize;
		virtual ~Args() = default;
	};
	struct MeshArgs : Args {
		Scene::Asset::MeshAssetEvent event;
		MeshGpuUploadPlan uploadPlan;
	};
}
