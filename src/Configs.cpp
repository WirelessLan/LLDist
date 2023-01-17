#include "Configs.h"
#include "Utils.h"

#include <regex>
#include <fstream>

namespace Configs {
    void ConfigReader::ReadConfigs() {
		const std::filesystem::path configDir{ "Data\\F4SE\\Plugins\\" + std::string(Version::PROJECT) };

		const std::regex filter(".*\\.cfg", std::regex_constants::icase);
		const std::filesystem::directory_iterator dir_iter(configDir);
		for (auto& iter : dir_iter) {
			if (!std::filesystem::is_regular_file(iter.status()))
				continue;

			if (!std::regex_match(iter.path().filename().string(), filter))
				continue;

			ReadConfigFile(iter.path().string());
		}
    }

	uint8_t GetNextChar(const std::string& line, uint32_t& index) {
		if (index < line.length())
			return line[index++];

		return 0xFF;
	}

	std::string GetNextData(const std::string& line, uint32_t& index, char delimeter) {
		uint8_t ch;
		std::string retVal = "";

		while ((ch = GetNextChar(line, index)) != 0xFF) {
			if (ch == '#') {
				if (index > 0) index--;
				break;
			}

			if (delimeter != 0 && ch == delimeter)
				break;

			retVal += static_cast<char>(ch);
		}

		Utils::Trim(retVal);
		return retVal;
	}

	void ConfigReader::ReadConfigFile(const std::string& path) {
		std::ifstream configFile(path);

		logger::info(FMT_STRING("Reading Config file: {}"), path);
		if (!configFile.is_open()) {
			logger::warn(FMT_STRING("Cannot open the config file: {}"), path);
			return;
		}

		std::string line;
		std::string lineType, llFormPluginName, llFormId, targetFormPluginName, targetFormId, levelStr, countStr, chanceNoneStr;
		while (std::getline(configFile, line)) {
			Utils::Trim(line);
			if (line.empty() || line[0] == '#')
				continue;

			uint32_t index = 0;

			lineType = GetNextData(line, index, '|');
			if (lineType.empty()) {
				logger::warn(FMT_STRING("Cannot read the lineType: {}"), line);
				continue;
			}

			llFormPluginName = GetNextData(line, index, '|');
			if (llFormPluginName.empty()) {
				logger::warn(FMT_STRING("Cannot read the llFormPluginName: {}"), line);
				continue;
			}

			if (lineType == "CLEAR")
				llFormId = GetNextData(line, index, 0);
			else
				llFormId = GetNextData(line, index, '|');
			if (llFormId.empty()) {
				logger::warn(FMT_STRING("Cannot read the llFormId: {}"), line);
				continue;
			}

			if (lineType == "ADD" || lineType == "DELETE") {
				targetFormPluginName = GetNextData(line, index, '|');
				if (targetFormPluginName.empty()) {
					logger::warn(FMT_STRING("Cannot read the targetFormPluginName: {}"), line);
					continue;
				}

				targetFormId = GetNextData(line, index, '|');
				if (targetFormId.empty()) {
					logger::warn(FMT_STRING("Cannot read the targetFormId: {}"), line);
					continue;
				}

				if (lineType == "DELETE")
					levelStr = GetNextData(line, index, 0);
				else
					levelStr = GetNextData(line, index, '|');
				if (levelStr.empty()) {
					logger::warn(FMT_STRING("Cannot read the levelStr: {}"), line);
					continue;
				}

				if (lineType == "ADD") {
					countStr = GetNextData(line, index, '|');
					if (countStr.empty()) {
						logger::warn(FMT_STRING("Cannot read the countStr: {}"), line);
						continue;
					}

					chanceNoneStr = GetNextData(line, index, 0);
					if (chanceNoneStr.empty()) {
						logger::warn(FMT_STRING("Cannot read the chanceNoneStr: {}"), line);
						continue;
					}
				}
			}

			if (lineType != "CLEAR" && lineType != "ADD" && lineType != "DELETE") {
				logger::warn(FMT_STRING("Invalid lineType: {}"), line);
				continue;
			}

			RE::TESForm* llForm = Utils::GetFormFromIdentifier(llFormPluginName, llFormId);
			if (!llForm) {
				logger::warn(FMT_STRING("Invalid leveled list: {} | {}"), llFormPluginName, llFormId);
				continue;
			}

			RE::TESLeveledList* lvList = llForm->As<RE::TESLeveledList>();
			if (!lvList) {
				logger::warn(FMT_STRING("Invalid leveled list: {} | {}"), llFormPluginName, llFormId);
				continue;
			}

			modSet.insert(llForm);

			if (lineType == "CLEAR") {
				clearSet.insert(llForm->formID);
				logger::info(FMT_STRING("Clear: LeveledList[{:08X}]"), llForm->formID);
			}
			else if (lineType == "ADD") {
				RE::TESForm* addForm = Utils::GetFormFromIdentifier(targetFormPluginName, targetFormId);
				if (!addForm) {
					logger::warn(FMT_STRING("Invalid add target form: {} | {}"), targetFormPluginName, targetFormId);
					continue;
				}

				uint16_t level;
				try {
					level = static_cast<uint16_t>(std::stoul(levelStr));
				}
				catch (...) {
					logger::warn(FMT_STRING("Failed to parse the level: {}"), line);
					continue;
				}

				uint16_t count;
				try {
					count = static_cast<uint16_t>(std::stoul(countStr));
				}
				catch (...) {
					logger::warn(FMT_STRING("Failed to parse the count: {}"), line);
					continue;
				}

				int8_t chanceNone;
				try {
					chanceNone = static_cast<int8_t>(std::stol(chanceNoneStr));
				}
				catch (...) {
					logger::warn(FMT_STRING("Failed to parse the chanceNone: {}"), line);
					continue;
				}

				auto add_it = addMap.find(llForm->formID);
				if (add_it == addMap.end()) {
					auto ins_res = addMap.insert(std::make_pair(llForm->formID, std::vector<RE::LEVELED_OBJECT>()));
					if (!ins_res.second)
						return;

					add_it = ins_res.first;
				}

				add_it->second.push_back({ addForm, nullptr, count, level, chanceNone });

				logger::info(FMT_STRING("Add: LeveledList[{:08X}] Form[{:08X}] Count[{}] Level[{}] ChanceNone[{}]"), llForm->formID, addForm->formID, count, level, chanceNone);
			}
			else {	// DELETE
				RE::TESForm* delForm = Utils::GetFormFromIdentifier(targetFormPluginName, targetFormId);
				if (!delForm) {
					logger::warn(FMT_STRING("Invalid delete target form: {} | {}"), targetFormPluginName, targetFormId);
					continue;
				}

				uint16_t level;
				try {
					level = static_cast<uint16_t>(std::stoul(levelStr));
				}
				catch (...) {
					logger::warn(FMT_STRING("Failed to parse the level: {}"), line);
					continue;
				}

				auto del_it = delMap.find(llForm->formID);
				if (del_it == delMap.end()) {
					auto ins_res = delMap.insert(std::make_pair(llForm->formID, std::vector<std::pair<uint16_t, RE::TESForm*>>()));
					if (!ins_res.second)
						return;

					del_it = ins_res.first;
				}

				del_it->second.push_back(std::make_pair(level, delForm));

				logger::info(FMT_STRING("Delete: LeveledList[{:08X}] Form[{:08X}] Level[{}]"), llForm->formID, delForm->formID, level);
			}
		}
	}
}
