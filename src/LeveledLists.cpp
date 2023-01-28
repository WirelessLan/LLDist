#include "LeveledLists.h"

#include "Utils.h"

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

	LL_ALLOC* AllocateLL(uint32_t entryCnt) {
		RE::MemoryManager mm = RE::MemoryManager::GetSingleton();
		return (LL_ALLOC*)mm.Allocate(sizeof(RE::LEVELED_OBJECT) * entryCnt + sizeof(size_t), 0, 0);
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
		LL_ALLOC* nLL = AllocateLL(vecCnt);
		if (!nLL)
			return;

		nLL->count = vecCnt;
		for (uint32_t ii = 0; ii < vecCnt; ii++)
			nLL->ll[ii] = a_llad[ii];

		a_lvl->leveledLists = nLL->ll;
		a_lvl->baseListCount = static_cast<int8_t>(vecCnt);
	}

	void Distribute(RE::TESForm* llForm, RE::TESLeveledList* lvList, 
		std::unordered_set<uint32_t>& clearSet, 
		std::unordered_map<uint32_t, std::vector<Distributors::DistData>>& setMap, 
		std::unordered_map<uint32_t, std::vector<Distributors::DistData>>& addMap,
		std::unordered_map<uint32_t, std::vector<Distributors::DistData>>& delMap) {
		bool isCleared = false, isDeleted = false, isAdded = false;

		logger::info(FMT_STRING("┌──────────────────────────────── Start: {:08X} ────────────────────────────────┐"), llForm->formID);

		std::vector<RE::LEVELED_OBJECT> llVec;

		if (clearSet.find(llForm->formID) != clearSet.end()) {
			logger::info(FMT_STRING(" >\tClear {:08X}'s original LL entries"), llForm->formID);
			isCleared = true;
		}
		else if (addMap.size() > 0 || delMap.size() > 0) {
			logger::info(FMT_STRING(" >\tGet {:08X}'s original LL entries... count[{}]"), llForm->formID, lvList->baseListCount);
			llVec = GetLL(lvList);
		}

		auto set_it = setMap.find(llForm->formID);
		if (set_it != setMap.end()) {
			for (const Distributors::DistData& setData : set_it->second) {
				Distributors::DistArg setNameArg, setValueArg;
				if (!setData.GetValue("SetName", setNameArg) || !setData.GetValue("SetValue", setValueArg))
					continue;

				std::string& setName = setNameArg.stringValue;
				uint8_t setValue = setValueArg.u8Value;

				if (setName == "LVLD") {
					logger::info(FMT_STRING(" >\tSet {:08X}'s LVLD {} to {}"), llForm->formID, static_cast<uint8_t>(lvList->chanceNone), setValue);
					lvList->chanceNone = static_cast<int8_t>(setValue);
				}
				else if (setName == "LVLM") {
					logger::info(FMT_STRING(" >\tSet {:08X}'s LVLM {} to {}"), llForm->formID, static_cast<uint8_t>(lvList->maxUseAllCount), setValue);
					lvList->maxUseAllCount = static_cast<int8_t>(setValue);
				}
				else if (setName == "LVLF") {
					logger::info(FMT_STRING(" >\tSet {:08X}'s LVLF {} to {}"), llForm->formID, static_cast<uint8_t>(lvList->llFlags), setValue);
					lvList->llFlags = static_cast<int8_t>(setValue);
				}
			}
		}

		if (!isCleared) {
			auto del_it = delMap.find(llForm->formID);
			if (del_it != delMap.end()) {
				auto pre_del_size = llVec.size();
				for (const Distributors::DistData& delData : del_it->second) {
					if (delData.args.size() != 3)
						continue;

					Distributors::DistArg delFormStrArg, levelArg;
					if (!delData.GetValue("DeleteForm", delFormStrArg) || !delData.GetValue("Level", levelArg))
						continue;

					std::string delFormStr = delFormStrArg.stringValue;
					uint16_t level = levelArg.u16Value;

					RE::TESForm* delForm = Utils::GetFormFromString(delFormStr);
					if (!delForm) {
						logger::warn(FMT_STRING(" >\tInvalid delete form {}"), delFormStr);
						continue;
					}

					logger::info(FMT_STRING(" >\tDelete {:08X} from {:08X}'s LL entries... level[{}]"), delForm->formID, llForm->formID, level);
					llVec.erase(std::remove_if(llVec.begin(), llVec.end(), [&](const RE::LEVELED_OBJECT& x) {
						return x.level == level && x.form == delForm;
						}), llVec.end());
				}
				if (pre_del_size != llVec.size())
					isDeleted = true;
			}
		}

		auto add_it = addMap.find(llForm->formID);
		if (add_it != addMap.end()) {
			for (const Distributors::DistData& addData : add_it->second) {
				if (addData.args.size() != 5)
					continue;

				Distributors::DistArg addFormStrArg, levelArg, countArg, chanceNoneArg;
				if (!addData.GetValue("AddForm", addFormStrArg) || !addData.GetValue("Level", levelArg) 
					|| !addData.GetValue("Count", countArg) || !addData.GetValue("ChanceNone", chanceNoneArg))
					continue;

				std::string addFormStr = addFormStrArg.stringValue;
				uint16_t level = levelArg.u16Value;
				uint16_t count = countArg.u16Value;
				uint8_t chanceNone = chanceNoneArg.u8Value;

				RE::TESForm* addForm = Utils::GetFormFromString(addFormStr);
				if (!addForm) {
					logger::warn(FMT_STRING(" >\tInvalid add form {}"), addFormStr);
					continue;
				}

				logger::info(FMT_STRING(" >\tAdd {:08X} to {:08X}'s LL entries... level[{}] count[{}] chanceNone[{}]"),
					addForm->formID, llForm->formID, level, count, chanceNone);
				llVec.push_back({ addForm, nullptr, count, level, static_cast<int8_t>(chanceNone) });
			}
			isAdded = true;
		}

		std::sort(llVec.begin(), llVec.end(), [](const RE::LEVELED_OBJECT& a, const RE::LEVELED_OBJECT& b) {
			return a.level < b.level;
			});

		if (isCleared || isDeleted || isAdded) {
			logger::info(FMT_STRING(" >\tUpdate {:08X}'s LL entries... count[{}]"), llForm->formID, llVec.size());
			SetLL(lvList, llVec);
		}

		logger::info(FMT_STRING("└──────────────────────────────── End:   {:08X} ────────────────────────────────┘"), llForm->formID);
		logger::info("");
	}
}
