#include "PresetsUtils.h"
#include <nlohmann/json.hpp>
#include "ChargenUtils.h"
#include "Utils.h"

//
// Getting data from NPC
//


nlohmann::json presets::getJsonMorphsData(RE::TESNPC* npc)
{
	nlohmann::json j;

	// Morph definitions
	nlohmann::json j_defs;

	auto npcMorphDefinitions = chargen::availableMorphDefinitions(npc);

	if (npcMorphDefinitions != nullptr) {
		for (const auto& innerPair : *npcMorphDefinitions) {
			j_defs[innerPair.key.c_str()] = (float)innerPair.value;
		}
	}

	// Facebones
	nlohmann::json j_fb;

	RE::BSTHashMap2<std::uint32_t, float>* npcFaceBones = chargen::availableFacebones(npc);

	if (npcFaceBones != nullptr) {
		for (const auto& pair : *npcFaceBones) {
			j_fb[std::to_string(pair.key).c_str()] = (float)pair.value;
		}
	}

	// Shape-blends
	nlohmann::json j_sb;

	auto shapeBlends = chargen::availableShapeBlends(npc);

	if (shapeBlends != nullptr) {
		for (const auto& [key, value] : *shapeBlends) {
			j_sb[key.c_str()] = value;
		}
	}

	j["MorphRegions"] = j_defs;
	j["FaceBones"] = j_fb;
	j["ShapeBlends"] = j_sb;

	return j;
}

nlohmann::json presets::getJsonColorData(RE::TESNPC* npc)
{
	nlohmann::json j;

	j["SkinToneIndex"] = npc->skinToneIndex;
	j["EyebrowColor"] = npc->eyebrowColor;
	j["EyeColor"] = npc->eyeColor;
	j["JewelryColor"] = npc->jewelryColor;
	j["FacialHairColor"] = npc->facialColor;
	j["HairColor"] = npc->hairColor;

	return j;
}


nlohmann::json presets::getJsonHeadpartData(RE::TESNPC* npc)
{
	nlohmann::json j_headpart_array = nlohmann::json::array();

	auto Headparts = npc->headParts;
	auto guard = npc->headParts.lock();

	for (RE::BGSHeadPart* part : *guard) {
		nlohmann::json j_headpart_single;

		std::string editorID = part->GetFormEditorID();
		j_headpart_single["EditorID"] = editorID;

		j_headpart_array.push_back(j_headpart_single);
	}
	return j_headpart_array;
}

nlohmann::json presets::getJsonAVMData(RE::TESNPC* npc)
{
	nlohmann::json j_avm_array = nlohmann::json::array();
	auto AVMTints = npc->tintAVMData;

	for (auto& avmd : AVMTints) {
		nlohmann::json j_avm_single;

		auto        type = avmd.type;
		std::string typeString = chargen::getStringTypeFromAVM(type);

		std::string category = avmd.category.c_str();
		std::string unkName = avmd.unk10.name.c_str();
		std::string texturePath = avmd.unk10.texturePath.c_str();
		uint32_t    intensity = avmd.unk10.intensity;

		j_avm_single["Type"] = typeString;
		j_avm_single["Category"] = category;
		j_avm_single["Name"] = unkName;
		j_avm_single["TexturePath"] = texturePath;
		j_avm_single["Intensity"] = intensity;

		// Colors
		nlohmann::json j_avm_colors;
		j_avm_colors["Alpha"] = avmd.unk10.color.alpha;
		j_avm_colors["Blue"] = avmd.unk10.color.blue;
		j_avm_colors["Green"] = avmd.unk10.color.green;
		j_avm_colors["Red"] = avmd.unk10.color.red;

		j_avm_single["Color"] = j_avm_colors;

		j_avm_array.push_back(j_avm_single);
	}
	return j_avm_array;
}

std::string presets::getRaceString(RE::TESNPC* npc) {
	if (npc->formRace != nullptr) {
		auto* raceStr = npc->formRace->GetFormEditorID();
		return raceStr;
	}
	return "";
}

nlohmann::json presets::getPresetData(RE::TESNPC* npc)
{
	std::string            n = "\n";
	std::list<RE::AVMData> avmlist;

	nlohmann::json j;

	j["AVM"] = getJsonAVMData(npc);
	j["Headparts"] = getJsonHeadpartData(npc);
	j["Colors"] = getJsonColorData(npc);
	j["Race"] = getRaceString(npc);
	j["Morphs"] = getJsonMorphsData(npc);

	return j;
}

//
// Preset data loaders
//

void presets::applyDataMorphs(RE::TESNPC* npc, nlohmann::json morphdata)
{
	nlohmann::json j_morphRegions = morphdata["MorphRegions"];
	nlohmann::json j_faceBones = morphdata["FaceBones"];
	nlohmann::json j_shapeBlends = morphdata["ShapeBlends"];

	RE::BSTHashMap<RE::BSFixedStringCS, float>* npcMorphDefinitions = chargen::availableMorphDefinitions(npc);

	if (npcMorphDefinitions != nullptr) {
		npcMorphDefinitions->clear();

		for (const auto& md : j_morphRegions.items()) {
			npcMorphDefinitions->insert(std::make_pair(RE::BSFixedStringCS(md.key()), static_cast<float>(md.value())));
		}
	}

	auto faceBones = chargen::availableFacebones(npc);

	if (faceBones != nullptr) {
		faceBones->clear();

		for (const auto& fb : j_faceBones.items()) {
			faceBones->insert(std::make_pair(static_cast<uint32_t>(std::stoul(fb.key())), (float)fb.value()));
		}
	}
	
	auto shapeBlends = chargen::availableShapeBlends(npc);

	if (shapeBlends != nullptr) {
		shapeBlends->clear();

		for (const auto& sb : j_shapeBlends.items()) {
			shapeBlends->insert(std::make_pair(RE::BSFixedStringCS(sb.key()), (float)sb.value()));
		}
	}
}

void presets::applyDataAVM(RE::TESNPC* npc, nlohmann::json avmdata) {
	npc->tintAVMData.clear();

	for (auto& avmItem : avmdata) {
		RE::AVMData avm;

		avm.type = chargen::getAVMTypeFromString(avmItem.value("Type", "kNone"));

		avm.category = RE::BSFixedString(avmItem.value("Category", "").c_str());
		avm.unk10.name = RE::BSFixedString(avmItem.value("Name", "").c_str());
		avm.unk10.texturePath = RE::BSFixedString(avmItem.value("TexturePath", "").c_str());
		avm.unk10.intensity = std::uint32_t(avmItem.value("Intensity", 64));

		auto color = avmItem["Color"];

		avm.unk10.color = RE::Color(
			uint8_t(color.value("Red", 64)),
			uint8_t(color.value("Green", 64)),
			uint8_t(color.value("Blue", 64)),
			uint8_t(color.value("Alpha", 64)));

		npc->tintAVMData.emplace_back(std::move(avm));
	}
}

void presets::applyDataRace(RE::TESNPC* npc, std::string raceStr) {
	if (raceStr == "") {
		return;
	}

	auto* race = RE::TESForm::LookupByEditorID(raceStr);

	if (race != nullptr && race->formType == RE::FormType::kRACE) {
		npc->formRace = static_cast<RE::TESRace*>(race);
	}
}

void presets::applyDataRace(RE::TESNPC* npc, uint32_t raceID)
{
	if (raceID == 0) {
		return;
	}
	auto* race = RE::TESForm::LookupByID(raceID);

	if (race != nullptr && race->formType == RE::FormType::kRACE) {
		npc->formRace = static_cast<RE::TESRace*>(race);
	}
}

void presets::applyDataRace(RE::TESNPC* npc, RE::TESRace* race)
{
	npc->formRace = race;
}

void presets::applyDataColors(RE::TESNPC* npc, nlohmann::json colorData)
{
	npc->skinToneIndex = colorData.value("SkinToneIndex", 0);
	npc->eyebrowColor = colorData.value("EyebrowColor", "");
	npc->eyeColor = colorData.value("EyeColor", "");
	npc->jewelryColor = colorData.value("JewelryColor", "");
	npc->facialColor = colorData.value("FacialHairColor", "");
	npc->hairColor = colorData.value("HairColor", "");
}

void presets::applyDataHeadparts(RE::TESNPC* npc, nlohmann::json headpartsData)
{
	RE::BSGuarded<RE::BSTArray<RE::BGSHeadPart*>, RE::BSNonReentrantSpinLock>* headparts = &npc->headParts;

	auto guard = headparts->lock();

	guard.operator->().clear();

	for (auto& hpItem : headpartsData) {
		auto headpartVal = hpItem.value("EditorID", "");

		if (headpartVal != "") {
			auto headpart = RE::TESForm::LookupByEditorID(headpartVal);

			if (headpart != nullptr && headpart->formType == RE::FormType::kHDPT) {
				guard.operator->().emplace_back(static_cast<RE::BGSHeadPart*>(headpart));
			}
		}
	}
}

void presets::loadPresetData(RE::Actor* actor, nlohmann::json j)
{
	RE::TESNPC* npc = actor->GetNPC();

	// AVM
	if (j["AVM"].size() >= 1) {
		applyDataAVM(npc, j["AVM"]);
	}

	// Colors
	if (j["Colors"].size() >= 1) {
		applyDataColors(npc, j["Colors"]);
	}
	
	// Headparts
	if (j["Headparts"].size() >= 1) {
		applyDataHeadparts(npc, j["Headparts"]);
	}

	// Morphs
	if (j["Morphs"].size() >= 1) {
		applyDataMorphs(npc, j["Morphs"]);
	}

	// Race
	applyDataRace(npc, j.value("Race", ""));
}

std::string presets::morphListToQuickPreset(std::vector<std::pair<std::string, float>> morphList)
{
	nlohmann::json quickPreset;

	for (std::pair morph : morphList) {
		quickPreset[morph.first] = morph.second;
	}

	return quickPreset.dump(0);
}

std::vector<std::pair<std::string, float>> presets::quickPresetToMorphList(std::string quickPreset)
{
	std::vector<std::pair<std::string, float>> validMorphs;

	nlohmann::json jMorph;


	try {
		jMorph = nlohmann::json::parse(quickPreset);
	} catch (nlohmann::json::parse_error& e) {
		return validMorphs;
	}

	for (auto& morph : jMorph.items()) {
		validMorphs.push_back({ morph.key(), morph.value() });
	}

	return validMorphs;
}

