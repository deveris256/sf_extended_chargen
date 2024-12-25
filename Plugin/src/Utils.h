#pragma once
#include <chrono>
#include <ctime>
#include <format>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <regex>

#include <RE/T/TESForm.h>
#include <nlohmann/json.hpp>

namespace utils
{
	namespace traits
	{
		template <class _FORM_T>
		concept _is_form = std::derived_from<_FORM_T, RE::TESForm>;

	}

	inline constexpr std::string_view GetPluginName()
	{
		return Plugin::NAME;
	};

	// Return only the folder path
	std::string GetPluginFolder();

	std::string GetPluginIniFile();

	std::string_view GetPluginLogFile();

	std::string GetCurrentTimeString(std::string fmt = "%d.%m.%Y %H:%M:%S");

	RE::Actor* GetSelActorOrPlayer();

	inline bool IsActorMenuActor(RE::Actor* a_actor)
	{
		return a_actor->boolFlags.underlying() == 4 && a_actor->boolFlags2.underlying() == 8458272;  // From experience
	}

	void saveDataJSON(nlohmann::json, std::string subfolder, std::string name);

	template <class _FORM_T>
	requires traits::_is_form<_FORM_T>
	inline std::string make_str(_FORM_T* a_form)
	{
		return std::format("'{}'({:X})", a_form->GetFormEditorID(), a_form->GetFormID());
	}

	template<>
	inline std::string make_str<RE::Actor>(RE::Actor* a_form)
	{
		auto npc = a_form->GetNPC();
		if (npc) {
			return std::format("NPC:'{}'({:X})", npc->GetFormEditorID(), a_form->GetFormID());
		}
		else {
			return std::format("'{}'({:X})", a_form->GetDisplayFullName(), a_form->GetFormID()); // Could be potentially crashy
		}
	}

	bool caseInsensitiveCompare(const std::string& str, const char* cstr);

	std::vector<nlohmann::json> getJsonConfigs(std::string subfolderName);
}