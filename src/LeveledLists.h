#pragma once

#include <unordered_set>

#include "Distributors.h"

namespace LeveledLists {
	void Distribute(RE::TESForm* llForm, RE::TESLeveledList* lvList,
		std::unordered_set<uint32_t>& clearSet,
		std::unordered_map<uint32_t, std::vector<Distributors::DistData>>& setMap,
		std::unordered_map<uint32_t, std::vector<Distributors::DistData>>& addMap,
		std::unordered_map<uint32_t, std::vector<Distributors::DistData>>& delMap);
}
