#include "stdafx.h"

#pragma once

#include "unordered_map"
#include "../mesh/CPUMesh.h"
#include "helpers.h"
#include "../DXSampleHelper.h"
#include <mutex>

namespace Engine {
	class CPUMeshManager {
	public:
		static CPUMeshManager& getInstance() {
			static CPUMeshManager instance;
			return instance;
		}

		GUID add(std::unique_ptr<CPUMesh>&& cpuMesh) {
			GUID guid;
			ThrowIfFailed(CoCreateGuid(&guid));
			m_mutex.lock();
			cpuMesh->setID(guid);
			m_cpuMeshes.emplace(guid, std::move(cpuMesh));
			m_mutex.unlock();
			return guid;
		}

		GUID remove(const GUID& id) {
			m_mutex.lock();
			m_cpuMeshes.erase(id);
			m_mutex.unlock();
		}

		CPUMesh& get(const GUID& id) {
			m_mutex.lock();
			auto& mesh = *m_cpuMeshes.at(id);
			m_mutex.unlock();
			return mesh;
		}
	private:
		std::unordered_map<GUID, std::unique_ptr<CPUMesh>, GUIDHash, GUIDEqual> m_cpuMeshes;
		std::mutex m_mutex;
	};
}
