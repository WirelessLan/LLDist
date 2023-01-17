#pragma once

namespace Utils {
	void Trim(std::string& s);
	RE::TESForm* GetFormFromIdentifier(const std::string& pluginName, const uint32_t formId);
    RE::TESForm* GetFormFromIdentifier(const std::string& pluginName, const std::string& formIdStr);
}
