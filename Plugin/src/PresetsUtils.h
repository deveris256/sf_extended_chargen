#pragma once
#include "GameForms.h"
#include <iostream>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace presets
{
	//
	// Getters
	//

	// Gets morphs JSON data (Shape blends, Morph regions, Facebones)
	nlohmann::json getJsonMorphsData(RE::TESNPC* npc);

	// Gets JSON data for colors (skin tone, eye color, etc.)
	nlohmann::json getJsonColorData(RE::TESNPC* npc);

	// Gets AVM data (texture overlays)
	nlohmann::json getJsonAVMData(RE::TESNPC* npc);

	// Gets headpart JSON data
	nlohmann::json getJsonHeadpartData(RE::TESNPC* npc);

	// Gets race as string
	std::string getRaceString(RE::TESNPC* npc);

	// Gets full preset data ready for saving to file
	nlohmann::json getPresetData(RE::TESNPC* npc);

	//
	// Setters
	//

	// Applies morph data to an actor
	void applyDataMorphs(RE::TESNPC* npc, nlohmann::json morphdata);

	// Applies AVM data to actor
	void applyDataAVM(RE::TESNPC* npc, nlohmann::json avmdata);

	// Applies race data to an actor
	void applyDataRace(RE::TESNPC* npc, std::string raceStr);
	// Applies race data to an actor
	void applyDataRace(RE::TESNPC* npc, RE::TESRace* race);
	// Applies race data to an actor
	void applyDataRace(RE::TESNPC* npc, uint32_t raceID);

	// Applies color data to an actor
	void applyDataColors(RE::TESNPC* npc, nlohmann::json colorData);

	// Applies headparts data to an actor
	void applyDataHeadparts(RE::TESNPC* npc, nlohmann::json headpartsData);

	// Loads a preset
	void loadPresetData(RE::Actor* actor, nlohmann::json preset);

	// Convert morph list to quick easily copy-able preset
	std::string morphListToQuickPreset(std::vector<std::pair<std::string, float>> morphList);

	std::vector<std::pair<std::string, float>> quickPresetToMorphList(std::string quickPreset);
}