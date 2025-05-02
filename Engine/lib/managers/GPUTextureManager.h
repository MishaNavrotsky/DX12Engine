#include "stdafx.h"

#pragma once

#include "unordered_map"
#include "../mesh/GPUTexture.h"
#include "helpers.h"
#include "../DXSampleHelper.h"
#include <mutex>

namespace Engine {
	class GPUTextureManager {
	public:
		static GPUTextureManager& getInstance() {
			static GPUTextureManager instance;
			return instance;
		}

		GUID add(std::unique_ptr<GPUTexture>&& gpuTexture) {
			GUID guid;
			ThrowIfFailed(CoCreateGuid(&guid));
			m_mutex.lock();
			gpuTexture->setID(guid);
			m_gpuTextures.emplace(guid, std::move(gpuTexture));
			m_mutex.unlock();
			return guid;
		}

		GUID remove(const GUID& id) {
			m_mutex.lock();
			m_gpuTextures.erase(id);
			m_mutex.unlock();
		}

		GPUTexture& get(const GUID& id) {
			m_mutex.lock();
			auto& gpuTexture = *m_gpuTextures.at(id);
			m_mutex.unlock();
			return gpuTexture;
		}

		std::vector<std::reference_wrapper<GPUTexture>> getMany(const std::vector<GUID>& ids) {
			std::vector<std::reference_wrapper<GPUTexture>> textures;
			m_mutex.lock();
			for (const auto& id : ids) {
				textures.push_back(*m_gpuTextures.at(id));
			}
			m_mutex.unlock();
			return textures;
		}
	private:
		std::unordered_map<GUID, std::unique_ptr<GPUTexture>, GUIDHash, GUIDEqual> m_gpuTextures;
		std::mutex m_mutex;
	};
}
