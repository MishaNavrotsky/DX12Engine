#include "stdafx.h"

#pragma once

#include "unordered_map"
#include "../mesh/CPUTexture.h"
#include "helpers.h"
#include "../DXSampleHelper.h"
#include <mutex>

namespace Engine {
	class CPUTextureManager {
	public:
		static CPUTextureManager& getInstance() {
			static CPUTextureManager instance;
			return instance;
		}

		GUID add(std::unique_ptr<CPUTexture>&& cpuTexture) {
			GUID guid;
			ThrowIfFailed(CoCreateGuid(&guid));
			m_mutex.lock();
			cpuTexture->setID(guid);
			m_cpuTextures.emplace(guid, std::move(cpuTexture));
			m_mutex.unlock();
			return guid;
		}

		GUID remove(const GUID& id) {
			m_mutex.lock();
			m_cpuTextures.erase(id);
			m_mutex.unlock();
		}

		CPUTexture& get(const GUID& id) {
			m_mutex.lock();
			auto& cpuTexture = *m_cpuTextures.at(id);
			m_mutex.unlock();
			return cpuTexture;
		}
	private:
		std::unordered_map<GUID, std::unique_ptr<CPUTexture>, GUIDHash, GUIDEqual> m_cpuTextures;
		std::mutex m_mutex;
	};
}
