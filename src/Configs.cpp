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

			logger::info(FMT_STRING("=========== Reading Config file: {} ==========="), iter.path().string());
			ReadConfigFile(iter.path().string());
			logger::info("");
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

		if (!configFile.is_open()) {
			logger::warn(FMT_STRING("Cannot open the config file: {}"), path);
			return;
		}

		std::string line;
		std::string lineType, llFormPluginName, llFormId, targetFormPluginName, targetFormId, levelStr, countStr, chanceNoneStr, setName, setValueStr;
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
			else if (lineType == "SET") {
				setName = GetNextData(line, index, '|');
				if (setName.empty()) {
					logger::warn(FMT_STRING("Cannot read the setName: {}"), line);
					continue;
				}

				setValueStr = GetNextData(line, index, 0);
				if (setName.empty()) {
					logger::warn(FMT_STRING("Cannot read the setValue: {}"), line);
					continue;
				}
			}

			if (lineType != "CLEAR" && lineType != "ADD" && lineType != "DELETE" && lineType != "SET") {
				logger::warn(FMT_STRING("Invalid lineType: {}"), line);
				continue;
			}

			LeveledLists::DistData distData;
			distData.llForm = llFormPluginName + "|" + llFormId;

			if (lineType == "CLEAR") {
				distData.type = LeveledLists::DistData::Type::kClear;
				logger::info(FMT_STRING("Clear: LeveledList[{}]"), distData.llForm);
			}
			else if (lineType == "ADD") {
				distData.type = LeveledLists::DistData::Type::kAdd;
				distData.targetForm = targetFormPluginName + "|" + targetFormId;

				try {
					distData.level = static_cast<uint16_t>(std::stoul(levelStr));
				}
				catch (...) {
					logger::warn(FMT_STRING("Failed to parse the level: {}"), line);
					continue;
				}

				try {
					distData.count = static_cast<uint16_t>(std::stoul(countStr));
				}
				catch (...) {
					logger::warn(FMT_STRING("Failed to parse the count: {}"), line);
					continue;
				}

				try {
					distData.chanceNone = static_cast<uint8_t>(std::stol(chanceNoneStr));
				}
				catch (...) {
					logger::warn(FMT_STRING("Failed to parse the chanceNone: {}"), line);
					continue;
				}
				logger::info(FMT_STRING("Add: LeveledList[{}] Form[{}] Count[{}] Level[{}] ChanceNone[{}]"), distData.llForm, distData.targetForm, distData.count, distData.level, distData.chanceNone);
			}
			else if (lineType == "DELETE") {
				distData.type = LeveledLists::DistData::Type::kDelete;
				distData.targetForm = targetFormPluginName + "|" + targetFormId;

				try {
					distData.level = static_cast<uint16_t>(std::stoul(levelStr));
				}
				catch (...) {
					logger::warn(FMT_STRING("Failed to parse the level: {}"), line);
					continue;
				}
				logger::info(FMT_STRING("Delete: LeveledList[{}] Form[{}] Level[{}]"), distData.llForm, distData.targetForm, distData.level);
			}
			else if (lineType == "SET") {
				distData.type = LeveledLists::DistData::Type::kSet;
				
				if (setName != "LVLD" && setName != "LVLM" && setName != "LVLF") {
					logger::warn(FMT_STRING("Invalid setName: {}"), line);
					continue;
				}

				try {
					distData.setValue = static_cast<uint8_t>(std::stol(setValueStr));
				}
				catch (...) {
					logger::warn(FMT_STRING("Failed to parse the setValue: {}"), line);
					continue;
				}

				if (setName == "LVLD") {
					distData.setName = LeveledLists::DistData::SetName::kLVLD;
				}
				else if (setName == "LVLM") {
					distData.setName = LeveledLists::DistData::SetName::kLVLM;
				}
				else if (setName == "LVLF") {
					distData.setName = LeveledLists::DistData::SetName::kLVLF;

					if (distData.setValue > 7) {
						logger::warn(FMT_STRING("Invalid LVLF setValue: {}"), line);
						continue;
					}
				}
				logger::info(FMT_STRING("Set: LeveledList[{}] SetName[{}] SetValue[{}]"), distData.llForm, setName, distData.setValue);
			}

			dataVec.push_back(distData);
		}
	}
}
