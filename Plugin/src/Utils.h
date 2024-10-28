#pragma once
#include <chrono>
#include <ctime>
#include <format>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

#include <RE/T/TESForm.h>
#include <nlohmann/json.hpp>

namespace utils
{
	inline constexpr std::string_view GetPluginName()
	{
		return Plugin::NAME;
	};

	// Return only the folder path
	std::string_view GetPluginFolder();

	std::string GetPluginIniFile();

	std::string_view GetPluginLogFile();

	std::string GetCurrentTimeString(std::string fmt = "%d.%m.%Y %H:%M:%S");

	RE::Actor* GetSelActorOrPlayer();

	void saveDataJSON(nlohmann::json);

	inline std::string make_str(RE::TESForm* a_form)
	{
		return std::format("{}({:X})", a_form->GetFormEditorID(), a_form->GetFormID());
	}
}