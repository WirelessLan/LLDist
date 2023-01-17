#pragma once

#include <unordered_set>

namespace Configs {
	class ConfigReader {
	public:
		ConfigReader() : modSet(), clearSet(), addMap(), delMap() {}

		void ReadConfigs();

		const std::unordered_set<RE::TESForm*>& GetModSet() { return modSet; }
		const std::unordered_set<uint32_t>& GetClearSet() { return clearSet; }
		const std::unordered_map<uint32_t, std::vector<RE::LEVELED_OBJECT>>& GetAddMap() { return addMap; }
		const std::unordered_map<uint32_t, std::vector<std::pair<uint16_t, RE::TESForm*>>>& GetDeleteMap() { return delMap; }

		static ConfigReader* GetSingleton() {
			static ConfigReader self;
			return &self;
		}

	protected:
		void ReadConfigFile(const std::string& path);

		std::unordered_set<RE::TESForm*> modSet;
		std::unordered_set<uint32_t> clearSet;
		std::unordered_map<uint32_t, std::vector<RE::LEVELED_OBJECT>> addMap;
		std::unordered_map<uint32_t, std::vector<std::pair<uint16_t, RE::TESForm*>>> delMap;
	};
}
