#include "stdafx.h"

#pragma once

#include "unordered_map"
#include "../mesh/CPUMaterial.h"
#include "helpers.h"
#include "../DXSampleHelper.h"
#include <mutex>

namespace Engine {
	class CPUMaterialManager {
	public:
		static CPUMaterialManager& getInstance() {
			static CPUMaterialManager instance;
			return instance;
		}

		GUID add(std::unique_ptr<CPUMaterial>&& cpuMaterial) {
			GUID guid;
			ThrowIfFailed(CoCreateGuid(&guid));
			m_mutex.lock();
			cpuMaterial->setID(guid);
			m_cpuMaterials.emplace(guid, std::move(cpuMaterial));
			m_mutex.unlock();
			return guid;
		}

		GUID remove(const GUID& id) {
			m_mutex.lock();
			m_cpuMaterials.erase(id);
			m_mutex.unlock();

		}

		CPUMaterial& get(const GUID& id) {
			m_mutex.lock();
			auto& material = *m_cpuMaterials.at(id);
			m_mutex.unlock();
			return material;
		}
	private:
		std::unordered_map<GUID, std::unique_ptr<CPUMaterial>, GUIDHash, GUIDEqual> m_cpuMaterials;
		std::mutex m_mutex;
	};
}
