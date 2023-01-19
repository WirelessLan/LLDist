#pragma once

namespace LeveledLists {
	struct DistData {
		enum Type : uint16_t {
			kNone,
			kClear,
			kAdd,
			kDelete
		};

		Type type = kNone;
		std::string llForm = std::string();
		std::string targetForm = std::string();
		uint16_t level = 0;
		uint16_t count = 0;
		uint8_t chanceNone = 0;
	};

	void ReadConfigs();
	void Distribute();
}
