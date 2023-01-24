#pragma once

#include "Distributors.h"

namespace Configs {
	class ConfigReader {
	public:
		ConfigReader() : dataVec() {}

		void ReadConfigs();

		const std::vector<Distributors::DistData>& GetDataVector() { return dataVec; }

		static ConfigReader* GetSingleton() {
			static ConfigReader self;
			return &self;
		}

	protected:
		void ReadConfigFile(const std::string& path);

		std::vector<Distributors::DistData> dataVec;
	};

	void ReadConfigs();
}
