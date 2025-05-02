#include "stdafx.h"

#pragma once

#include "unordered_map"
#include "../mesh/GPUMaterial.h"
#include "helpers.h"
#include "../DXSampleHelper.h"
#include <mutex>

namespace Engine {
	class GPUMaterialManager {
	public:
		static GPUMaterialManager& getInstance() {
			static GPUMaterialManager instance;
			return instance;
		}

		GUID add(std::unique_ptr<GPUMaterial>&& gpuMaterial) {
			GUID guid;
			ThrowIfFailed(CoCreateGuid(&guid));
			m_mutex.lock();
			gpuMaterial->setID(guid);
			m_gpuMaterials.emplace(guid, std::move(gpuMaterial));
			m_mutex.unlock();
			return guid;
		}

		GUID remove(const GUID& id) {
			m_mutex.lock();
			m_gpuMaterials.erase(id);
			m_mutex.unlock();
		}

		GPUMaterial& get(const GUID& id) {
			m_mutex.lock();
			auto& gpuMaterial = *m_gpuMaterials.at(id);
			m_mutex.unlock();
			return gpuMaterial;
		}
	private:
		std::unordered_map<GUID, std::unique_ptr<GPUMaterial>, GUIDHash, GUIDEqual> m_gpuMaterials;
		std::mutex m_mutex;
	};
}
