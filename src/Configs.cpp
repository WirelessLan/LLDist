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

	bool IsEndOfLine(const std::string& line, uint32_t& index) {
		if (index < line.length()) {
			if (line[index] == '#')
				return true;
			return false;
		}

		return true;
	}

	void ConfigReader::ReadConfigFile(const std::string& path) {
		std::ifstream configFile(path);

		if (!configFile.is_open()) {
			logger::warn(FMT_STRING("Cannot open the config file: {}"), path);
			return;
		}

		std::string line;
		std::string lineType, distTargetFormPluginName, distTargetFormId, modFormPluginName, modFormId, levelStr, countStr, chanceNoneStr, setName, setValueStr;
		while (std::getline(configFile, line)) {
			Utils::Trim(line);
			if (line.empty() || line[0] == '#')
				continue;

			uint32_t index = 0;
			uint32_t argCount = 0;

			lineType = GetNextData(line, index, '|');
			if (lineType.empty()) {
				logger::warn(FMT_STRING("Cannot read the lineType: {}"), line);
				continue;
			}

			distTargetFormPluginName = GetNextData(line, index, '|');
			if (distTargetFormPluginName.empty()) {
				logger::warn(FMT_STRING("Cannot read the distTargetFormPluginName: {}"), line);
				continue;
			}

			if (lineType == "CLEAR")
				distTargetFormId = GetNextData(line, index, 0);
			else
				distTargetFormId = GetNextData(line, index, '|');
			if (distTargetFormId.empty()) {
				logger::warn(FMT_STRING("Cannot read the distTargetFormId: {}"), line);
				continue;
			}

			argCount++;

			if (lineType == "ADD" || lineType == "DELETE") {
				modFormPluginName = GetNextData(line, index, '|');
				if (modFormPluginName.empty()) {
					logger::warn(FMT_STRING("Cannot read the modFormPluginName: {}"), line);
					continue;
				}

				// End of FLST DistData
				modFormId = GetNextData(line, index, '|');
				if (modFormId.empty()) {
					logger::warn(FMT_STRING("Cannot read the modFormId: {}"), line);
					continue;
				}

				argCount++;

				// For LL DistData
				if (!IsEndOfLine(line, index)) {
					if (lineType == "DELETE")
						levelStr = GetNextData(line, index, 0);
					else
						levelStr = GetNextData(line, index, '|');
					if (levelStr.empty()) {
						logger::warn(FMT_STRING("Cannot read the levelStr: {}"), line);
						continue;
					}

					argCount++;

					if (lineType == "ADD") {
						countStr = GetNextData(line, index, '|');
						if (countStr.empty()) {
							logger::warn(FMT_STRING("Cannot read the countStr: {}"), line);
							continue;
						}

						argCount++;

						chanceNoneStr = GetNextData(line, index, 0);
						if (chanceNoneStr.empty()) {
							logger::warn(FMT_STRING("Cannot read the chanceNoneStr: {}"), line);
							continue;
						}

						argCount++;
					}
				}
			}
			else if (lineType == "SET") {
				setName = GetNextData(line, index, '|');
				if (setName.empty()) {
					logger::warn(FMT_STRING("Cannot read the setName: {}"), line);
					continue;
				}

				argCount++;

				setValueStr = GetNextData(line, index, 0);
				if (setName.empty()) {
					logger::warn(FMT_STRING("Cannot read the setValue: {}"), line);
					continue;
				}

				argCount++;
			}

			if (lineType != "CLEAR" && lineType != "ADD" && lineType != "DELETE" && lineType != "SET") {
				logger::warn(FMT_STRING("Invalid lineType: {}"), line);
				continue;
			}

			Distributors::DistData distData;
			distData.args["DistTargetForm"] = Distributors::DistArg::SetArg(distTargetFormPluginName + "|" + distTargetFormId);

			if (lineType == "CLEAR") {
				distData.type = Distributors::DistData::Type::kClear;
				logger::info(FMT_STRING("Clear: DistTargetForm[{}]"), distData.args["DistTargetForm"].stringValue);
			}
			else if (lineType == "ADD") {
				distData.type = Distributors::DistData::Type::kAdd;
				distData.args["AddForm"] = Distributors::DistArg::SetArg(modFormPluginName + "|" + modFormId);

				// For FLST DistData
				if (argCount == 2) {
					logger::info(FMT_STRING("Add: DistTargetForm[{}] AddForm[{}]"),
						distData.args["DistTargetForm"].stringValue, distData.args["AddForm"].stringValue);
				}
				// For LL DistData
				else {
					try {
						distData.args["Level"] = Distributors::DistArg::SetArg(static_cast<uint16_t>(std::stoul(levelStr)));
					}
					catch (...) {
						logger::warn(FMT_STRING("Failed to parse the level: {}"), line);
						continue;
					}

					try {
						distData.args["Count"] = Distributors::DistArg::SetArg(static_cast<uint16_t>(std::stoul(countStr)));
					}
					catch (...) {
						logger::warn(FMT_STRING("Failed to parse the count: {}"), line);
						continue;
					}

					try {
						distData.args["ChanceNone"] = Distributors::DistArg::SetArg(static_cast<uint8_t>(std::stol(chanceNoneStr)));
					}
					catch (...) {
						logger::warn(FMT_STRING("Failed to parse the chanceNone: {}"), line);
						continue;
					}

					logger::info(FMT_STRING("Add: DistTargetForm[{}] AddForm[{}] Level[{}] Count[{}] ChanceNone[{}]"),
						distData.args["DistTargetForm"].stringValue, distData.args["AddForm"].stringValue,
						distData.args["Level"].u16Value, distData.args["Count"].u16Value, distData.args["ChanceNone"].u8Value);
				}
			}
			else if (lineType == "DELETE") {
				distData.type = Distributors::DistData::Type::kDelete;
				distData.args["DeleteForm"] = Distributors::DistArg::SetArg(modFormPluginName + "|" + modFormId);

				// For FLST DistData
				if (argCount == 2) {
					logger::info(FMT_STRING("Delete: DistTargetForm[{}] DeleteForm[{}]"),
						distData.args["DistTargetForm"].stringValue, distData.args["DeleteForm"].stringValue);
				}
				// For LL DistData
				else {
					try {
						distData.args["Level"] = Distributors::DistArg::SetArg(static_cast<uint16_t>(std::stoul(levelStr)));
					}
					catch (...) {
						logger::warn(FMT_STRING("Failed to parse the level: {}"), line);
						continue;
					}

					logger::info(FMT_STRING("Delete: DistTargetForm[{}] DeleteForm[{}] Level[{}]"),
						distData.args["DistTargetForm"].stringValue, distData.args["DeleteForm"].stringValue, distData.args["Level"].u16Value);
				}
			}
			else if (lineType == "SET") {
				distData.type = Distributors::DistData::Type::kSet;
				
				if (setName != "LVLD" && setName != "LVLM" && setName != "LVLF") {
					logger::warn(FMT_STRING("Invalid setName: {}"), line);
					continue;
				}

				distData.args["SetName"] = Distributors::DistArg::SetArg(setName);

				try {
					distData.args["SetValue"] = Distributors::DistArg::SetArg(static_cast<uint8_t>(std::stol(setValueStr)));
				}
				catch (...) {
					logger::warn(FMT_STRING("Failed to parse the setValue: {}"), line);
					continue;
				}

				if (distData.args["SetName"].stringValue == "LVLF" && distData.args["SetValue"].u8Value > 7) {
					logger::warn(FMT_STRING("Invalid LVLF setValue: {}"), line);
					continue;
				}

				logger::info(FMT_STRING("Set: DistTargetForm[{}] SetName[{}] SetValue[{}]"), 
					distData.args["DistTargetForm"].stringValue, distData.args["SetName"].stringValue, distData.args["SetValue"].u8Value);
			}

			dataVec.push_back(distData);
		}
	}

	void ReadConfigs() {
		Configs::ConfigReader* g_configReader = Configs::ConfigReader::GetSingleton();
		g_configReader->ReadConfigs();
	}
}
