#include "stdafx.h"

#pragma once

namespace Engine::System::Streaming {
	class GpuBufferFinalizer {
	public:
		static void FinalizeMesh(ftl::TaskScheduler* ts, void* arg) {
			auto args = reinterpret_cast<MeshArgs*>(arg);
			args->step = StreamingStep::GpuBufferFinalizer;
			auto event = args->event;
			auto scene = args->streamingSystemArgs->getScene();
			auto* asset = event.asset;
			std::cerr << "Finalize " << event.id << "\n";
			args->finalize();
		}
	};
}
