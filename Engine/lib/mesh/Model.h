#include "stdafx.h"

#pragma once

#include "helpers.h"
#include <condition_variable>

namespace Engine {
	class Model: public IID {
	public:
		void setCPUMeshIds(std::vector<GUID>&& cpuMeshIds) noexcept {
			m_cpuMeshIds = std::move(cpuMeshIds);
		}
		void setGPUMeshIds(std::vector<GUID>&& gpuMeshIds) noexcept {
			m_gpuMeshIds = std::move(gpuMeshIds);
		}
		std::vector<GUID>& getCPUMeshIds() {
			return m_cpuMeshIds;
		}
		std::vector<GUID>& getGPUMeshIds() {
			return m_gpuMeshIds;
		}

		void setIsLoaded() {
			isLoaded = true;
			m_cv.notify_all();
		}
		bool getIsLoaded() const {
			return isLoaded;
		}
		void waitForIsLoaded() {
			std::unique_lock<std::mutex> lock(m_mutex);
			m_cv.wait(lock, [this] { return isLoaded.load(); });
		}
	private:
		std::vector<GUID> m_cpuMeshIds;
		std::vector<GUID> m_gpuMeshIds;
		std::atomic<bool> isLoaded = false;
		std::condition_variable m_cv;
		std::mutex m_mutex;
	};
}
