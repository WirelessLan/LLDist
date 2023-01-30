#pragma once

namespace Distributors {
	struct DistArg {
		enum ArgType : uint32_t {
			kString,
			kUint8,
			kUint16
		};

		static DistArg SetArg(const std::string& strVal) {
			return { ArgType::kString, strVal, 0, 0 };
		}

		static DistArg SetArg(uint16_t u16Val) {
			return { ArgType::kUint16, std::string(), 0, u16Val };
		}

		static DistArg SetArg(uint8_t u8Val) {
			return { ArgType::kUint8, std::string(), u8Val, 0 };
		}

		ArgType type;
		std::string stringValue;
		uint8_t u8Value;
		uint16_t u16Value;
	};

	struct DistData {
		enum class Type : uint16_t {
			kNone,
			kClear,
			kAdd,
			kDelete,
			kSet
		};

		bool GetValue(const std::string& name, DistArg& value) const {
			auto it = args.find(name);
			if (it != args.end()) {
				value = it->second;
				return true;
			}
			return false;
		}

		Type type = Type::kNone;
		std::unordered_map<std::string, DistArg> args;
	};

	void Distribute();
}
