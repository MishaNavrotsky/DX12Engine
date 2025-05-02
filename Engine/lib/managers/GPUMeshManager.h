#include "stdafx.h"

#pragma once

#include "unordered_map"
#include "../mesh/GPUMesh.h"
#include "helpers.h"
#include "../DXSampleHelper.h"
#include <mutex>

namespace Engine {
	class GPUMeshManager {
	public:
		static GPUMeshManager& getInstance() {
			static GPUMeshManager instance;
			return instance;
		}

		GUID add(std::unique_ptr<GPUMesh>&& gpuMesh) {
			GUID guid;
			ThrowIfFailed(CoCreateGuid(&guid));
			m_mutex.lock();
			gpuMesh->setID(guid);
			m_gpuMeshes.emplace(guid, std::move(gpuMesh));
			m_mutex.unlock();
			return guid;
		}

		GUID remove(const GUID& id) {
			m_mutex.lock();
			m_gpuMeshes.erase(id);
			m_mutex.unlock();
		}

		GPUMesh& get(const GUID& id) {
			m_mutex.lock();
			auto& gpuMesh = *m_gpuMeshes.at(id);
			m_mutex.unlock();
			return gpuMesh;
		}
	private:
		std::unordered_map<GUID, std::unique_ptr<GPUMesh>, GUIDHash, GUIDEqual> m_gpuMeshes;
		std::mutex m_mutex;
	};
}
