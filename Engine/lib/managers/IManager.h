#include "stdafx.h"

#pragma once

#include "unordered_map"
#include "helpers.h"
#include "../DXSampleHelper.h"
#include <mutex>
#include <optional>

namespace Engine {
	template<typename V, typename D>
	class IManager {
	public:
		static D& GetInstance() {
			static D instance;
			return instance;
		}

		GUID add(std::unique_ptr<V>&& el) {
			GUID guid;
			ThrowIfFailed(CoCreateGuid(&guid));
			std::lock_guard<std::mutex> lock(m_mutex);
			el->setID(guid);
			v.emplace(guid, std::move(el));
			return guid;
		}

		GUID add(const GUID& id, std::unique_ptr<V>&& el) {
			std::lock_guard<std::mutex> lock(m_mutex);
			el->setID(id);
			v.emplace(id, std::move(el));
			return id;
		}

		void remove(const GUID& id) {
			std::lock_guard<std::mutex> lock(m_mutex);
			v.erase(id);
		}

		V& get(const GUID& id) {
			std::lock_guard<std::mutex> lock(m_mutex);
			auto& item = *v.at(id);
			return item;
		}

		std::optional<std::reference_wrapper<V>> try_get(const GUID& id) {
			std::lock_guard<std::mutex> lock(m_mutex);
			auto it = v.find(id);
			return (it != v.end()) ? std::optional<std::reference_wrapper<V>>{*(it->second)} : std::nullopt;
		}

		std::vector<std::optional<std::reference_wrapper<V>>> try_getMany(const std::vector<GUID>& ids) {
			std::lock_guard<std::mutex> lock(m_mutex);
			std::vector<std::optional<std::reference_wrapper<V>>> results;
			for (const auto& id : ids) {
				auto it = v.find(id);
				results.emplace_back(it != v.end() ? std::optional<std::reference_wrapper<V>>{*(it->second)} : std::nullopt);
			}
			return results;
		}

		std::vector<std::reference_wrapper<V>> getMany(const std::vector<GUID>& ids) {
			std::lock_guard<std::mutex> lock(m_mutex);
			std::vector<std::reference_wrapper<V>> results;
			for (const auto& id : ids) {
				results.emplace_back(*v.at(id));
			}
			return results;
		}

		virtual ~IManager() = default;
	protected:
		std::mutex m_mutex;
		std::unordered_map<GUID, std::unique_ptr<V>, GUIDHash, GUIDEqual> v;
	};
}
