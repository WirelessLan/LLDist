#include "LeveledLists.h"
#include "Configs.h"
#include "Utils.h"

#include <algorithm>
#include <unordered_set>

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

	void PrepareDistData(const std::vector<LeveledLists::DistData>& dataVec, std::unordered_set<RE::TESForm*>& modSet, std::unordered_set<uint32_t>& clearSet,
		std::unordered_map<uint32_t, std::vector<RE::LEVELED_OBJECT>>& addMap, std::unordered_map<uint32_t, std::vector<std::pair<uint16_t, RE::TESForm*>>>& delMap) {
		for (const auto& data : dataVec) {
			RE::TESForm* llForm = Utils::GetFormFromString(data.llForm);
			if (!llForm) {
				logger::warn(FMT_STRING("Invalid leveled list form: {}"), data.llForm);
				continue;
			}

			RE::TESLeveledList* lvList = llForm->As<RE::TESLeveledList>();
			if (!lvList) {
				logger::warn(FMT_STRING("Invalid leveled list: {}"), data.llForm);
				continue;
			}

			switch (data.type) {
			case DistData::kClear:
				clearSet.insert(llForm->formID);
				break;
			case DistData::kAdd: {
				RE::TESForm* addForm = Utils::GetFormFromString(data.targetForm);
				if (!addForm) {
					logger::warn(FMT_STRING("Invalid add target form: {}"), data.targetForm);
					continue;
				}

				auto add_it = addMap.find(llForm->formID);
				if (add_it == addMap.end()) {
					auto ins_res = addMap.insert(std::make_pair(llForm->formID, std::vector<RE::LEVELED_OBJECT>()));
					if (!ins_res.second)
						return;

					add_it = ins_res.first;
				}

				add_it->second.push_back({ addForm, nullptr, data.count, data.level, static_cast<int8_t>(data.chanceNone) });
				break;
			}
			case DistData::kDelete: {
				RE::TESForm* delForm = Utils::GetFormFromString(data.targetForm);
				if (!delForm) {
					logger::warn(FMT_STRING("Invalid delete target form: {}"), data.targetForm);
					continue;
				}

				auto del_it = delMap.find(llForm->formID);
				if (del_it == delMap.end()) {
					auto ins_res = delMap.insert(std::make_pair(llForm->formID, std::vector<std::pair<uint16_t, RE::TESForm*>>()));
					if (!ins_res.second)
						return;

					del_it = ins_res.first;
				}

				del_it->second.push_back(std::make_pair(data.level, delForm));
				break;
			}
			default:
				continue;
				break;
			}

			modSet.insert(llForm);
		}
	}

	void ReadConfigs() {
		Configs::ConfigReader* g_configReader = Configs::ConfigReader::GetSingleton();
		g_configReader->ReadConfigs();
	}

	void Distribute() {
		static bool isDone = false;
		if (isDone)
			return;

		Configs::ConfigReader* g_configReader = Configs::ConfigReader::GetSingleton();
		const auto& dataVec = g_configReader->GetDataVector();

		std::unordered_set<RE::TESForm*> modSet;
		std::unordered_set<uint32_t> clearSet;
		std::unordered_map<uint32_t, std::vector<RE::LEVELED_OBJECT>> addMap;
		std::unordered_map<uint32_t, std::vector<std::pair<uint16_t, RE::TESForm*>>> delMap;

		PrepareDistData(dataVec, modSet, clearSet, addMap, delMap);

		logger::info("======================== Distribution Start ========================");
		for (RE::TESForm* form : modSet) {
			RE::TESLeveledList* lvlList = form->As<RE::TESLeveledList>();
			if (!lvlList)
				continue;

			logger::info(FMT_STRING("┌──────────────── Start: {:08X} ────────────────┐"), form->formID);

			logger::info(FMT_STRING(" >\tGet {:08X}'s original LL entries... count[{}]"), form->formID, lvlList->baseListCount);
			std::vector<RE::LEVELED_OBJECT> llVec = GetLL(lvlList);

			if (clearSet.find(form->formID) != clearSet.end()) {
				logger::info(FMT_STRING(" >\tClear {:08X}'s original LL entries"), form->formID);
				llVec.clear();
			}

			auto del_it = delMap.find(form->formID);
			if (del_it != delMap.end()) {
				for (const std::pair<uint16_t, RE::TESForm*>& delPair : del_it->second) {
					logger::info(FMT_STRING(" >\tDelete {:08X} from {:08X}'s LL entries... level[{}]"), delPair.second->formID, form->formID, delPair.first);
					llVec.erase(std::remove_if(llVec.begin(), llVec.end(), [&](const RE::LEVELED_OBJECT& x) {
						return x.level == delPair.first && x.form == delPair.second;
						}), llVec.end());
				}
			}

			auto add_it = addMap.find(form->formID);
			if (add_it != addMap.end()) {
				for (const RE::LEVELED_OBJECT& addLvlObj : add_it->second) {
					logger::info(FMT_STRING(" >\tAdd {:08X} to {:08X}'s LL entries... level[{}] count[{}] chanceNone[{}]"), 
						addLvlObj.form->formID, form->formID, addLvlObj.level, addLvlObj.count, static_cast<uint8_t>(addLvlObj.chanceNone));
					llVec.push_back(addLvlObj);
				}
			}

			std::sort(llVec.begin(), llVec.end(), [](const RE::LEVELED_OBJECT& a, const RE::LEVELED_OBJECT& b) {
				return a.level < b.level;
			});
			
			logger::info(FMT_STRING(" >\tUpdate {:08X}'s LL entries... count[{}]"), form->formID, llVec.size());
			SetLL(lvlList, llVec);

			logger::info(FMT_STRING("└──────────────── End:   {:08X} ────────────────┘"), form->formID);
			logger::info("");
		}
		logger::info("======================== Distribution End ========================");

		isDone = true;
	}
}
