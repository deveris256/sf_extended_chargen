#include "PresetsUtils.h"

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

	// Weight
	nlohmann::json j_wght;
	j_wght["Overweight"] = npc->morphWeight.fat;
	j_wght["Thin"] = npc->morphWeight.thin;
	j_wght["Strong"] = npc->morphWeight.muscular;

	j["MorphRegions"] = j_defs;
	j["FaceBones"] = j_fb;
	j["ShapeBlends"] = j_sb;
	j["Weights"] = j_wght;

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

void presets::applyDataMorphs(RE::TESNPC* npc, nlohmann::json morphdata, bool additive)
{
	// Face morph regions
	RE::BSTHashMap<RE::BSFixedStringCS, float>* npcMorphDefinitions = chargen::availableMorphDefinitions(npc);
	
	std::set<std::string> morphDefinitionNames;

	if (npcMorphDefinitions != nullptr && morphdata.contains("MorphRegions")) {
		nlohmann::json j_morphRegions = morphdata["MorphRegions"];
		if (!additive)
		{
			npcMorphDefinitions->clear();
		}

		for (const auto& md : j_morphRegions.items()) {
			if (npcMorphDefinitions->contains(md.key())) {
				npcMorphDefinitions->find(md.key()).operator->()->value = static_cast<float>(md.value());
			} else {
				npcMorphDefinitions->insert(std::make_pair(RE::BSFixedStringCS(md.key()), static_cast<float>(md.value())));
			}
		}
	}

	// Facebones
	auto faceBones = chargen::availableFacebones(npc);

	if (faceBones != nullptr && morphdata.contains("FaceBones")) {
		nlohmann::json j_faceBones = morphdata["FaceBones"];
		if (!additive)
		{
			faceBones->clear();
		}

		for (const auto& fb : j_faceBones.items()) {
			if (faceBones->contains(std::stoul(fb.key())))
			{
				faceBones->find(std::stoul(fb.key()))->value = std::stoul(fb.key());
			}
			else {
				faceBones->insert(std::make_pair(static_cast<uint32_t>(std::stoul(fb.key())), (float)fb.value()));
			}
			
		}
	}
	
	// Shape-Blends
	auto shapeBlends = chargen::availableShapeBlends(npc);

	if (shapeBlends != nullptr && morphdata.contains("ShapeBlends")) {
		nlohmann::json j_shapeBlends = morphdata["ShapeBlends"];

		if (!additive)
		{
			shapeBlends->clear();
		}

		for (const auto& sb : j_shapeBlends.items()) {
			if (shapeBlends->contains(sb.key()))
			{
				shapeBlends->find(sb.key())->value = (float)sb.value();
			}
			else {
				shapeBlends->insert(std::make_pair(RE::BSFixedStringCS(sb.key()), (float)sb.value()));
			}
		}
	}

	// Weights
	if (morphdata.contains("Weights")) {
		nlohmann::json j_weight = morphdata["Weights"];
		npc->morphWeight.fat = j_weight["Overweight"];
		npc->morphWeight.thin = j_weight["Thin"];
		npc->morphWeight.muscular = j_weight["Strong"];
	}
}

void presets::applyDataAVM(RE::TESNPC* npc, nlohmann::json avmdata, bool additive) {
	
	std::vector<std::string> avmCategoryList;

	if (!additive)
	{
		npc->tintAVMData.clear();
	}

	for (int i = 0; i < npc->tintAVMData.size(); i++)
	{
		auto& avm = npc->tintAVMData[i];

		avmCategoryList.emplace_back(std::string(avm.category.c_str()), i);
	}
	
	for (auto& avmItem : avmdata)
	{
		RE::AVMData avm;

		for (int avmIndex = 0; avmIndex < avmCategoryList.size(); avmIndex++)
		{
			std::string avmCategoryName = avmCategoryList[avmIndex];
			if (avmCategoryName == avmItem.value("Category", ""))
			{
				avm = npc->tintAVMData[avmIndex];
				break;
			}
		}

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

void presets::applyDataHeadparts(RE::TESNPC* npc, nlohmann::json headpartsData, bool additive)
{
	RE::BSGuarded<RE::BSTArray<RE::BGSHeadPart*>, RE::BSNonReentrantSpinLock>* headparts = &npc->headParts;

	std::set<std::string> headpartEditorIDs;

	auto guard = headparts->lock();

	if (!additive)
	{
		guard.operator->().clear();
	}
	else {
		
		for (auto& headpart : guard.operator->())
		{
			headpartEditorIDs.emplace(headpart->formEditorID);
		}
	}

	for (auto& hpItem : headpartsData) {
		auto headpartVal = hpItem.value("EditorID", "");

		if (headpartVal != "") {
			if (headpartEditorIDs.contains(headpartVal))
			{
				continue;
			}
			auto headpart = RE::TESForm::LookupByEditorID(headpartVal);

			if (headpart != nullptr && headpart->formType == RE::FormType::kHDPT) {
				guard.operator->().emplace_back(static_cast<RE::BGSHeadPart*>(headpart));
			}
		}
	}
}

void presets::loadPresetData(RE::Actor* actor, nlohmann::json j, bool additive)
{
	RE::TESNPC* npc = actor->GetNPC();

	// AVM
	if (j["AVM"].size() >= 1) {
		applyDataAVM(npc, j["AVM"], additive);
	}

	// Colors
	if (j["Colors"].size() >= 1) {
		applyDataColors(npc, j["Colors"]);
	}
	
	// Headparts
	if (j["Headparts"].size() >= 1) {
		applyDataHeadparts(npc, j["Headparts"], additive);
	}

	// Morphs
	if (j["Morphs"].size() >= 1) {
		applyDataMorphs(npc, j["Morphs"], additive);
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

