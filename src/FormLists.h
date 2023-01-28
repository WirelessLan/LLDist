#pragma once

#include <unordered_set>

#include "Distributors.h"

namespace FormLists {
	void Distribute(RE::BGSListForm* flst,
		std::unordered_set<uint32_t>& clearSet,
		std::unordered_map<uint32_t, std::vector<Distributors::DistData>>& addMap,
		std::unordered_map<uint32_t, std::vector<Distributors::DistData>>& delMap);
}
