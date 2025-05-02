#include "stdafx.h"

#pragma once

#include "unordered_map"
#include "../mesh/Sampler.h"
#include "helpers.h"
#include "../DXSampleHelper.h"
#include <mutex>

namespace Engine {
	class SamplerManager {
	public:
		static SamplerManager& getInstance() {
			static SamplerManager instance;
			return instance;
		}

		GUID add(std::unique_ptr<Sampler>&& sampler) {
			GUID guid;
			ThrowIfFailed(CoCreateGuid(&guid));
			m_mutex.lock();
			sampler->setID(guid);
			m_samplers.emplace(guid, std::move(sampler));
			m_mutex.unlock();
			return guid;
		}

		GUID remove(const GUID& id) {
			m_mutex.lock();
			m_samplers.erase(id);
			m_mutex.unlock();
		}

		Sampler& get(const GUID& id) {
			m_mutex.lock();
			auto& samplers = *m_samplers.at(id);
			m_mutex.unlock();
			return samplers;
		}
	private:
		std::unordered_map<GUID, std::unique_ptr<Sampler>, GUIDHash, GUIDEqual> m_samplers;
		std::mutex m_mutex;
	};
}
