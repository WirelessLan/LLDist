#pragma once

#include "LeveledLists.h"

namespace Configs {
	class ConfigReader {
	public:
		ConfigReader() : dataVec() {}

		void ReadConfigs();

		const std::vector<LeveledLists::DistData>& GetDataVector() { return dataVec; }

		static ConfigReader* GetSingleton() {
			static ConfigReader self;
			return &self;
		}

	protected:
		void ReadConfigFile(const std::string& path);

		std::vector<LeveledLists::DistData> dataVec;
	};
}
