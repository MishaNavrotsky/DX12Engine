#include "stdafx.h"

#pragma once
#include <unordered_map>
#include "helpers.h"
#include <mutex>

namespace Engine {
	class CPUGPUBimap {
	public:
		static CPUGPUBimap& getInstance() {
			static CPUGPUBimap instance;
			return instance;
		}

		void add(const GUID& cpuData, const GUID& gpuData) {
			std::lock_guard<std::mutex> lock(m_mutex);
			m_gpuToCpu[gpuData] = cpuData;
			m_cpuToGpu[cpuData] = gpuData;
		}

		GUID getCPU(const GUID& gpuData) {
			std::lock_guard<std::mutex> lock(m_mutex);
			auto it = m_gpuToCpu.find(gpuData);
			return (it != m_gpuToCpu.end()) ? it->second : GUID_NULL;
		}

		GUID getGPU(const GUID& cpuData) {
			std::lock_guard<std::mutex> lock(m_mutex);
			auto it = m_cpuToGpu.find(cpuData);
			return (it != m_cpuToGpu.end()) ? it->second : GUID_NULL;
		}

	private:
		std::unordered_map<GUID, GUID, GUIDHash, GUIDEqual> m_gpuToCpu;
		std::unordered_map<GUID, GUID, GUIDHash, GUIDEqual> m_cpuToGpu;
		std::mutex m_mutex;
	};
}
