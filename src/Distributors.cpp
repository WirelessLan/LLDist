#include "Distributors.h"

#include <algorithm>
#include <unordered_set>

#include "LeveledLists.h"
#include "FormLists.h"
#include "Configs.h"
#include "Utils.h"

namespace Distributors {
	void PrepareDistData(const std::vector<DistData>& dataVec,
		std::unordered_set<RE::TESForm*>& distSet,
		std::unordered_set<uint32_t>& clearSet,
		std::unordered_map<uint32_t, std::vector<DistData>>& setMap,
		std::unordered_map<uint32_t, std::vector<DistData>>& addMap,
		std::unordered_map<uint32_t, std::vector<DistData>>& delMap) {
		logger::info("======================== Prepare Distribution Start ========================");
		for (const DistData& data : dataVec) {
			DistArg distFormArg;
			if (!data.GetValue("DistTargetForm", distFormArg))
				continue;

			std::string& distFormStr = distFormArg.stringValue;
			RE::TESForm* distTargetForm = Utils::GetFormFromString(distFormStr);
			if (!distTargetForm) {
				logger::warn(FMT_STRING("Invalid form: {}"), distFormStr);
				continue;
			}

			switch (data.type) {
			case DistData::Type::kClear:
				clearSet.insert(distTargetForm->formID);
				break;
			case DistData::Type::kSet: {
				auto set_it = setMap.find(distTargetForm->formID);
				if (set_it == setMap.end()) {
					auto ins_res = setMap.insert(std::make_pair(distTargetForm->formID, std::vector<DistData>()));
					if (!ins_res.second)
						return;

					set_it = ins_res.first;
				}

				set_it->second.push_back(data);
				break;
			}
			case DistData::Type::kAdd: {
				auto add_it = addMap.find(distTargetForm->formID);
				if (add_it == addMap.end()) {
					auto ins_res = addMap.insert(std::make_pair(distTargetForm->formID, std::vector<DistData>()));
					if (!ins_res.second)
						return;

					add_it = ins_res.first;
				}

				add_it->second.push_back(data);
				break;
			}
			case DistData::Type::kDelete: {
				auto del_it = delMap.find(distTargetForm->formID);
				if (del_it == delMap.end()) {
					auto ins_res = delMap.insert(std::make_pair(distTargetForm->formID, std::vector<DistData>()));
					if (!ins_res.second)
						return;

					del_it = ins_res.first;
				}

				del_it->second.push_back(data);
				break;
			}
			default:
				continue;
				break;
			}

			distSet.insert(distTargetForm);
		}
		logger::info("======================== Prepare Distribution End ========================");
		logger::info("");
	}

	void Distribute() {
		static bool isDone = false;
		if (isDone)
			return;

		Configs::ConfigReader* g_configReader = Configs::ConfigReader::GetSingleton();
		const std::vector<Distributors::DistData>& dataVec = g_configReader->GetDataVector();

		std::unordered_set<RE::TESForm*> distSet;
		std::unordered_set<uint32_t> clearSet;
		std::unordered_map<uint32_t, std::vector<DistData>> setMap;
		std::unordered_map<uint32_t, std::vector<DistData>> addMap;
		std::unordered_map<uint32_t, std::vector<DistData>> delMap;

		PrepareDistData(dataVec, distSet, clearSet, setMap, addMap, delMap);

		logger::info("======================== Distribution Start ========================");

		for (RE::TESForm* form : distSet) {
			RE::TESLeveledList* lvlList = form->As<RE::TESLeveledList>();
			if (lvlList)
				LeveledLists::Distribute(form, lvlList, clearSet, setMap, addMap, delMap);

			RE::BGSListForm* flst = form->As<RE::BGSListForm>();
			if (flst)
				FormLists::Distribute(flst, clearSet, addMap, delMap);
		}

		logger::info("======================== Distribution End ========================");

		isDone = true;
	}
}
