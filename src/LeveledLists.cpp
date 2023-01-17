#include "LeveledLists.h"
#include "Configs.h"

#include <algorithm>

namespace LeveledLists {
	void FreeLL(RE::LEVELED_OBJECT* a_lobj, uint32_t arg2 = 0x3) {
		if (!a_lobj)
			return;

		using func_t = decltype(&FreeLL);
		const REL::Relocation<func_t> func{ REL::ID(296092) };
		return func(a_lobj, arg2);
	}

	struct LL_ALLOC {
		uint32_t count;
		RE::LEVELED_OBJECT ll[];
	};

	LL_ALLOC* AllocateLL(uint32_t size) {
		RE::MemoryManager mm = RE::MemoryManager::GetSingleton();
		return (LL_ALLOC*)mm.Allocate(size, 0, 0);
	}

	std::vector<RE::LEVELED_OBJECT> GetLL(RE::TESLeveledList* a_lvl) {
		std::vector<RE::LEVELED_OBJECT> retVec;

		if (!a_lvl || !a_lvl->leveledLists || a_lvl->baseListCount == 0)
			return retVec;

		for (uint32_t ii = 0; ii < static_cast<uint32_t>(a_lvl->baseListCount); ii++)
			retVec.push_back(a_lvl->leveledLists[ii]);

		return retVec;
	}

	void SetLL(RE::TESLeveledList* a_lvl, const std::vector<RE::LEVELED_OBJECT>& a_llad) {
		if (!a_lvl || !a_lvl->leveledLists || a_lvl->baseListCount == 0)
			return;

		memset(a_lvl->leveledLists, 0, a_lvl->baseListCount * sizeof(RE::LEVELED_OBJECT));
		FreeLL(a_lvl->leveledLists);

		a_lvl->leveledLists = nullptr;
		a_lvl->baseListCount = 0;

		if (a_llad.empty())
			return;

		uint32_t vecCnt = static_cast<uint32_t>(a_llad.size());
		LL_ALLOC* nLL = AllocateLL(sizeof(RE::LEVELED_OBJECT) * vecCnt + sizeof(uint32_t));
		if (!nLL)
			return;

		nLL->count = vecCnt;
		for (uint32_t ii = 0; ii < vecCnt; ii++)
			nLL->ll[ii] = a_llad[ii];

		a_lvl->leveledLists = nLL->ll;
		a_lvl->baseListCount = static_cast<int8_t>(vecCnt);
	}

	void Distribute() {
		static bool isDone = false;
		if (isDone)
			return;

		Configs::ConfigReader* g_configReader = Configs::ConfigReader::GetSingleton();
		g_configReader->ReadConfigs();

		const auto& modSet = g_configReader->GetModSet();
		const auto& clearSet = g_configReader->GetClearSet();
		const auto& addMap = g_configReader->GetAddMap();
		const auto& delMap = g_configReader->GetDeleteMap();

		for (RE::TESForm* form : modSet) {
			RE::TESLeveledList* lvlList = form->As<RE::TESLeveledList>();

			std::vector<RE::LEVELED_OBJECT> llVec = GetLL(lvlList);

			if (clearSet.find(form->formID) != clearSet.end())
				llVec.clear();

			auto del_it = delMap.find(form->formID);
			if (del_it != delMap.end()) {
				for (const std::pair<uint16_t, RE::TESForm*>& delPair : del_it->second)
					llVec.erase(std::remove_if(llVec.begin(), llVec.end(), [&](const RE::LEVELED_OBJECT& x) {
						return x.level == delPair.first && x.form == delPair.second;
					}), llVec.end());
			}

			auto add_it = addMap.find(form->formID);
			if (add_it != addMap.end()) {
				for (const RE::LEVELED_OBJECT& addLvlObj : add_it->second)
					llVec.push_back(addLvlObj);
			}

			std::sort(llVec.begin(), llVec.end(), [](const RE::LEVELED_OBJECT& a, const RE::LEVELED_OBJECT& b) {
				return a.level < b.level;
			});

			SetLL(lvlList, llVec);
		}

		isDone = true;
	}
}
