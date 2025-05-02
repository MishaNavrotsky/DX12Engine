#include "stdafx.h"

#pragma once
#include <unordered_map>
#include "helpers.h"

namespace Engine {
	class Bimap {
	public:
		std::unordered_map<GUID, GUID, GUIDHash, GUIDEqual> leftToRight;
		std::unordered_map<GUID, GUID, GUIDHash, GUIDEqual> rightToLeft;

		void insert(GUID left, GUID right) {
			leftToRight[left] = right;
			rightToLeft[right] = left;
		}
	};
}
