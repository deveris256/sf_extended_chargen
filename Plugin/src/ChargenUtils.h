#pragma once
#include "GameForms.h"
#include "NiAVObject.h"

namespace chargen
{
	//
	// General Utils
	//

	// AVM: Get string from AVM type
	std::string getStringTypeFromAVM(RE::AVMData::Type type);

	// AVM: Get AVM type from string
	RE::AVMData::Type getAVMTypeFromString(const std::string& typeString);

	//
	// Chargen Utils
	//

	// Available Morph Definitions (Chin, Eyes, Mouth, etc) sliders
	RE::BSTHashMap<RE::BSFixedStringCS, float>* availableMorphDefinitions(RE::TESNPC* npc);

	// Available FaceBones sliders
	RE::BSTHashMap2<std::uint32_t, float>* availableFacebones(RE::TESNPC* npc);

	// Available morphs (Also shows custom morphs here)
	RE::BSTHashMap<RE::BSFixedStringCS, float>* availableShapeBlends(RE::TESNPC* npc);

	// Available weight (Heavy, Thin, Muscular)
	std::vector<std::pair<std::string, float*>>* availableMorphWeight(RE::TESNPC* npc);

	// Gets performance morphs
	float* getPerformanceMorphs(RE::Actor* actor);


	// Available Eye Colors
	std::vector<std::string> availableEyeColor();

	//
	// Updating data
	//

	// Updates chargen appearance of an actor
	void updateActorAppearance(RE::Actor* actor);

	// Updates actor appearance fully, sometimes is prone to cause
	// invisible body (if updateBody is true), or head,
	// especially if called too frequently.
	void updateActorAppearanceFully(RE::Actor* actor, bool updateBody, bool raceChange);

	// Debug: Loads default race appearance
	void loadDefaultRaceAppearance(RE::Actor* actor);

	//
	// Skin/Equipment Utils
	//
	RE::TESObjectARMO* getActorSkin(RE::Actor* actor);

	bool equipObject(RE::Actor* actor, RE::BGSObjectInstance& object, RE::BGSEquipSlot* slot, bool queueEquip, bool forceEquip, bool playSounds, bool applyNow, bool locked);

	bool unequipObject(RE::Actor* actor, RE::BGSObjectInstance& object, RE::BGSEquipSlot* slot, bool queueUnequip, bool forceUnequip, bool playSounds, bool applyNow, RE::BGSEquipSlot* slotBeingReplaced);

	namespace externs
	{
		extern REL::Relocation<void**>                      manager_singleton_ptr;
		extern REL::Relocation<bool (*)(void*, RE::Actor*, RE::BGSObjectInstance&, RE::BGSEquipSlot*, bool, bool, bool, bool, bool)> equipObject_func;
		extern REL::Relocation<bool (*)(void*, RE::Actor*, RE::BGSObjectInstance&, RE::BGSEquipSlot*, bool, bool, bool, bool, RE::BGSEquipSlot*)> unequipObject_func;
	}
}