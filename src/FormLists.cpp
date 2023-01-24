#include "FormLists.h"

#include "Utils.h"

namespace FormLists {
	void Distribute(RE::BGSListForm* flst, 
		std::unordered_set<uint32_t>& clearSet, 
		std::unordered_map<uint32_t, std::vector<Distributors::DistData>>& addMap, 
		std::unordered_map<uint32_t, std::vector<Distributors::DistData>>& delMap) {
		bool isCleared = false;

		logger::info(FMT_STRING("┌──────────────────────────────── Start: {:08X} ────────────────────────────────┐"), flst->formID);

		if (clearSet.find(flst->formID) != clearSet.end()) {
			logger::info(FMT_STRING(" >\tClear {:08X}'s original FLST entries"), flst->formID);
			flst->arrayOfForms.clear();
			isCleared = true;
		}

		if (!isCleared) {
			auto del_it = delMap.find(flst->formID);
			if (del_it != delMap.end()) {
				for (const Distributors::DistData& delData : del_it->second) {
					if (delData.args.size() != 2)
						continue;

					Distributors::DistArg delFormStrArg;
					if (!delData.GetValue("DeleteForm", delFormStrArg))
						continue;

					std::string delFormStr = delFormStrArg.stringValue;

					RE::TESForm* delForm = Utils::GetFormFromString(delFormStr);
					if (!delForm) {
						logger::warn(FMT_STRING(" >\tInvalid delete form {}"), delFormStr);
						continue;
					}

					logger::info(FMT_STRING(" >\tDelete {:08X} from {:08X}'s FLST entries..."), delForm->formID, flst->formID);
					flst->arrayOfForms.erase(std::remove_if(flst->arrayOfForms.begin(), flst->arrayOfForms.end(), [&](const RE::TESForm* x) {
						return x == delForm;
						}), flst->arrayOfForms.end());
				}
			}
		}

		auto add_it = addMap.find(flst->formID);
		if (add_it != addMap.end()) {
			for (const Distributors::DistData& addData : add_it->second) {
				if (addData.args.size() != 2)
					continue;

				Distributors::DistArg addFormStrArg;
				if (!addData.GetValue("AddForm", addFormStrArg))
					continue;

				std::string addFormStr = addFormStrArg.stringValue;

				RE::TESForm* addForm = Utils::GetFormFromString(addFormStr);
				if (!addForm) {
					logger::warn(FMT_STRING(" >\tInvalid add form {}"), addFormStr);
					continue;
				}

				logger::info(FMT_STRING(" >\tAdd {:08X} to {:08X}'s FLST entries..."), addForm->formID, flst->formID);
				flst->arrayOfForms.push_back(addForm);
			}
		}

		logger::info(FMT_STRING("└──────────────────────────────── End:   {:08X} ────────────────────────────────┘"), flst->formID);
		logger::info("");
	}
}
