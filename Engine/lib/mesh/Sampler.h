#include "stdafx.h"

#pragma once

#include "helpers.h"

namespace Engine {
	class Sampler: public IID {
	public:
		Sampler() = default;
		Sampler(const Sampler&) = delete;
		Sampler& operator=(const Sampler&) = delete;

		D3D12_SAMPLER_DESC samplerDesc = {};

		std::unordered_set<GUID, GUIDHash, GUIDEqual>& getCpuTextureIds() {
			return m_cpuTextureIds;
		}
		void addCpuTextureId(const GUID& id) {
			m_cpuTextureIds.insert(id);
		}
		void removeCpuTextureId(const GUID& id) {
			m_cpuTextureIds.erase(id);
		}
		void clearCpuTextureIds() {
			m_cpuTextureIds.clear();
		}
		void setCpuTextureIds(std::unordered_set<GUID, GUIDHash, GUIDEqual>&& ids) {
			m_cpuTextureIds = std::move(ids);
		}

		uint32_t bindlessHeapSlot = 0;
	private:
		std::unordered_set<GUID, GUIDHash, GUIDEqual> m_cpuTextureIds;
	};
}
