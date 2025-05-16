#include "stdafx.h"

#pragma once

namespace Engine {
	struct GUIDHash {
		std::size_t operator()(const GUID& guid) const noexcept {
			const uint64_t* data = reinterpret_cast<const uint64_t*>(&guid);
			std::size_t hash = std::hash<uint64_t>{}(data[0]);

			// Mix second half more effectively
			hash ^= std::hash<uint64_t>{}(data[1]) * 0x9e3779b9 + (hash << 6) + (hash >> 2);

			return hash;
		}
	};

	struct GUIDEqual {
		bool operator()(const GUID& lhs, const GUID& rhs) const noexcept {
			return IsEqualGUID(lhs, rhs);
		}
	};

	struct GUIDComparator {
		bool operator()(const GUID& lhs, const GUID& rhs) const {
			return memcmp(&lhs, &rhs, sizeof(GUID)) < 0; // Binary ordering
		}
	};

	struct GUIDPairHash {
		std::size_t operator()(const std::pair<GUID, GUID>& p) const noexcept {
			GUIDHash hash_fn;
			std::size_t h1 = hash_fn(p.first);
			std::size_t h2 = hash_fn(p.second);
			return h1 ^ (h2 * 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
		}
	};

	struct GUIDPairEqual {
		bool operator()(const std::pair<GUID, GUID>& lhs, const std::pair<GUID, GUID>& rhs) const noexcept {
			GUIDEqual eq;
			return eq(lhs.first, rhs.first) && eq(lhs.second, rhs.second);
		}
	};
}
