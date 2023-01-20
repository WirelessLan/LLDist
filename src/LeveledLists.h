#pragma once

namespace LeveledLists {
	struct DistData {
		enum class Type : uint16_t {
			kNone,
			kClear,
			kAdd,
			kDelete,
			kSet
		};

		enum class SetName : uint16_t {
			kNone,
			kLVLD,
			kLVLM,
			kLVLF
		};

		Type type = Type::kNone;
		std::string llForm = std::string();
		std::string targetForm = std::string();
		uint16_t level = 0;
		uint16_t count = 0;
		uint8_t chanceNone = 0;
		SetName setName = SetName::kNone;
		uint8_t setValue = 0;
	};

	void ReadConfigs();
	void Distribute();
}
