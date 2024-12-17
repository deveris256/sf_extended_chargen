#include "ChargenUtils.h"

//
// Getters of NPC/Actor data
//

namespace chargen
{
	namespace externs
	{
		REL::Relocation<void**> manager_singleton_ptr{ RE::ID::ActorEquipManager::Singleton };
		REL::Relocation<bool (*)(void*, RE::Actor*, RE::BGSObjectInstance&, RE::BGSEquipSlot*, bool, bool, bool, bool, bool)> equipObject_func { RE::ID::ActorEquipManager::EquipObject };
		REL::Relocation<bool (*)(void*, RE::Actor*, RE::BGSObjectInstance&, RE::BGSEquipSlot*, bool, bool, bool, bool, RE::BGSEquipSlot*)> unequipObject_func { RE::ID::ActorEquipManager::UnequipObject };
	}
}

float* chargen::getPerformanceMorphs(RE::Actor* actor)
{
	auto actorLoadedData = actor->loadedData.lock_write();
	auto actorData3D = actorLoadedData->data3D;

	if (actorData3D == nullptr) {
		return nullptr;
	}

	auto actor3D = static_cast<RE::BGSFadeNode*>(actorData3D.get());

	if (actor3D->bgsModelNode == nullptr ||
		actor3D->bgsModelNode->facegenNodes.data == nullptr) {
		return nullptr;
	}

	auto facegen = actor3D->bgsModelNode->facegenNodes.data[0]->faceGenAnimData;

	if (facegen == nullptr) {
		return nullptr;
	}

	auto facegenMorphs = facegen->morphs;

	return facegenMorphs;
}

std::vector<std::string> chargen::getAVMList(std::string avmName)
{
	auto        sgAVMName = RE::TESForm::LookupByEditorID(avmName);
	BGSAVMData* sgAVM = static_cast<BGSAVMData*>(sgAVMName);

	BGSAVMData::Entry* entryBegin = sgAVM->entryBegin;
	BGSAVMData::Entry* entryEnd = sgAVM->entryEnd;

	std::vector<std::string> v;

	for (BGSAVMData::Entry* i = entryBegin; i != entryEnd; i++) {
		v.emplace_back(i->name);
	}

	return v;
}


RE::BSTHashMap<RE::BSFixedStringCS, float>* chargen::availableMorphDefinitions(RE::TESNPC* npc)
{
	auto* morphDefinitions = npc->unk3E8;

	if (morphDefinitions != nullptr) {
		for (const auto& outerPair : *morphDefinitions) {
			std::uint32_t                               outerKey = outerPair.key;
			RE::BSTHashMap<RE::BSFixedStringCS, float>* innerMap = outerPair.value;

			if (innerMap) {
				return innerMap;
			}
		}
	}

	return nullptr;
}

RE::BSTHashMap2<std::uint32_t, float>* chargen::availableFacebones(RE::TESNPC* npc) {
	auto* faceBones = npc->unk3E0;

	if (faceBones != nullptr) {
		return faceBones;
	}
	return nullptr;
}

RE::BSTHashMap<RE::BSFixedStringCS, float>* chargen::availableShapeBlends(RE::TESNPC* npc) {
	auto* shapeBlends = npc->shapeBlendData;

	if (shapeBlends != nullptr) {

		return shapeBlends;
	}
	return nullptr;
}

std::vector<std::pair<std::string, float*>>* chargen::availableMorphWeight(RE::TESNPC* npc)
{
	std::vector<std::pair<std::string, float*>>* weight = new std::vector<std::pair<std::string, float*>>();

	weight->push_back(std::pair("Heavy", &npc->morphWeight.fat));
	weight->push_back(std::pair("Thin", &npc->morphWeight.thin));
	weight->push_back(std::pair("Muscular", &npc->morphWeight.muscular));

	return weight;
}

//
// General Utils
//

RE::AVMData::Type chargen::getAVMTypeFromString(const std::string& typeString)
{
	return (typeString == "kSimpleGroup")  ? RE::AVMData::Type::kSimpleGroup :
	       (typeString == "kComplexGroup") ? RE::AVMData::Type::kComplexGroup :
	       (typeString == "kModulation")   ? RE::AVMData::Type::kModulation :
	                                         RE::AVMData::Type::kNone;
}

std::string chargen::getStringTypeFromAVM(RE::AVMData::Type type)
{
	return (type == RE::AVMData::Type::kNone)         ? "kNone" :
	       (type == RE::AVMData::Type::kSimpleGroup)  ? "kSimpleGroup" :
	       (type == RE::AVMData::Type::kComplexGroup) ? "kComplexGroup" :
	       (type == RE::AVMData::Type::kModulation)   ? "kModulation" :
	                                                    "kNone";
}

//
// Updating data
//

void chargen::updateActorAppearance(RE::Actor* actor)
{
	SFSE::GetTaskInterface()->AddTask(
		[actor]() {
			actor->UpdateChargenAppearance();
		});
}

void chargen::updateActorAppearanceFully(RE::Actor* actor, bool updateBody, bool raceChange)
{
	SFSE::GetTaskInterface()->AddTask(
		[actor, updateBody, raceChange]() {
			actor->UpdateAppearance(updateBody, 0u, raceChange);
		});

	SFSE::GetTaskInterface()->AddTask(
		[actor]() {
			actor->UpdateChargenAppearance();
		});
}

//
// Moreso for debug purposes
//

void chargen::loadDefaultRaceAppearance(RE::Actor* actor)
{
	auto        race = actor->race;
	RE::TESNPC* npc = actor->GetNPC();

	RE::BSGuarded<RE::BSTArray<RE::BGSHeadPart*>, RE::BSNonReentrantSpinLock>* headparts = &npc->headParts;

	auto raceHeadparts = race->headparts;
	auto raceHeadpartsSize = raceHeadparts->size();
	auto guard = headparts->lock();

	guard.operator->().clear();

	for (auto rhp : raceHeadparts[1]) {  // TODO: Genders
		guard.operator->().emplace_back(rhp);
	}
}

RE::TESObjectARMO* chargen::getActorSkin(RE::Actor* actor)
{
	auto npc = actor->GetNPC();
	if (!npc) {
		return nullptr;
	}

	auto skin = npc->formSkin;
	if (skin) {
		return skin;
	}

	auto race = npc->GetRace();
	if (!race) {
		return nullptr;
	}

	auto race_skin = race->formSkin;
	return race_skin;
}

bool chargen::equipObject(RE::Actor* actor, RE::BGSObjectInstance& object, RE::BGSEquipSlot* slot, bool queueEquip, bool forceEquip, bool playSounds, bool applyNow, bool locked)
{
	return externs::equipObject_func(*externs::manager_singleton_ptr, actor, object, slot, queueEquip, forceEquip, playSounds, applyNow, locked);
}

bool chargen::unequipObject(RE::Actor* actor, RE::BGSObjectInstance& object, RE::BGSEquipSlot* slot, bool queueUnequip, bool forceUnequip, bool playSounds, bool applyNow, RE::BGSEquipSlot* slotBeingReplaced)
{
	return externs::unequipObject_func(*externs::manager_singleton_ptr, actor, object, slot, queueUnequip, forceUnequip, playSounds, applyNow, slotBeingReplaced);
}
