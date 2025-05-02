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
}
