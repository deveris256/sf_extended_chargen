/* 
 * https://github.com/Starfield-Reverse-Engineering/CommonLibSF
 * This plugin template links against CommonLibSF
 */

#define BETTERAPI_IMPLEMENTATION
#include "betterapi.h"
#include "CCF_API.h"
#include <nlohmann/json.hpp>
#include "NiAVObject.h"

#include "LogWrapper.h"
#include "Sniffer.h"

// SFSEPlugin_Version
DLLEXPORT constinit auto SFSEPlugin_Version = []() noexcept {
	SFSE::PluginVersionData data{};

	data.PluginVersion(Plugin::Version);
	data.PluginName(Plugin::NAME);
	data.AuthorName(Plugin::AUTHOR);

	data.CompatibleVersions({ SFSE::RUNTIME_LATEST });

	return data;
}();

static std::string getPluginFolder()
{
	char    path[MAX_PATH];
	HMODULE hModule = nullptr;  // nullptr == current DLL

	GetModuleFileNameA(hModule, path, MAX_PATH);

	std::string fullPath = path;
	size_t      lastSlash = fullPath.find_last_of("\\/");
	return fullPath.substr(0, lastSlash) + "\\Data\\SFSE\\Plugins\\ExtendedChargen";  // Return only the folder path
}

using json = nlohmann::json;

static const BetterAPI*             API = NULL;
static const struct simple_draw_t*  UI = NULL;
static LogBufferHandle              LogHandle = 0;
static auto                         lastExecutionTime = std::chrono::steady_clock::now();

static bool appearanceChanged = false;

static std::string pluginFolder = getPluginFolder();
static std::string iniFilePath = pluginFolder + "\\ExtendedChargen.ini";
static std::atomic<bool> hasLoaded = false;

void MessageCallback(SFSE::MessagingInterface::Message* a_msg) noexcept
{
	switch (a_msg->type) {
	case SFSE::MessagingInterface::kPostDataLoad:
		{
			hasLoaded = true;
		}
		break;
	default:
		break;
	}
}

namespace ExtendedChargen
{
	static RE::NiPointer<RE::TESObjectREFR> GetRefrFromHandle(uint32_t handle)
	{
		RE::NiPointer<RE::TESObjectREFR>                                    result;
		REL::Relocation<void(RE::NiPointer<RE::TESObjectREFR>&, uint32_t*)> func(REL::ID(72399));
		func(result, &handle);
		return result;
	}

	static RE::NiPointer<RE::TESObjectREFR> GetConsoleRefr()
	{
		REL::Relocation<uint64_t**>                      consoleReferencesManager(REL::ID(879512));
		REL::Relocation<uint32_t*(uint64_t*, uint32_t*)> GetConsoleHandle(REL::ID(166314));
		uint32_t                                         outId = 0;
		GetConsoleHandle(*consoleReferencesManager, &outId);
		return GetRefrFromHandle(outId);
	}

	static RE::Actor* GetSelActorOrPlayer()
	{
		auto sel = GetConsoleRefr();

		if (sel != nullptr and sel.get()->IsActor()) {
			return sel->As<RE::Actor>();
		}

		return RE::PlayerCharacter::GetSingleton();
	}

	// Save JSON data
	static void saveData(json data)
	{
		if (!std::filesystem::exists(pluginFolder)) {
			std::filesystem::create_directories(pluginFolder);
		}

		std::ofstream dataFile(pluginFolder);
		auto          prettyData = data.dump(4);
		dataFile << prettyData;

		dataFile.close();
	}

	// AVM: Get type from string
	static RE::AVMData::Type getAVMTypeFromString(const std::string& typeString)
	{
		return (typeString == "kSimpleGroup")  ? RE::AVMData::Type::kSimpleGroup :
		       (typeString == "kComplexGroup") ? RE::AVMData::Type::kComplexGroup :
		       (typeString == "kModulation")   ? RE::AVMData::Type::kModulation :
		                                         RE::AVMData::Type::kNone;
	}

	// AVM: Get string from type
	static std::string getStringTypeFromAVM(RE::AVMData::Type type)
	{
		return (type == RE::AVMData::Type::kNone)         ? "kNone" :
		       (type == RE::AVMData::Type::kSimpleGroup)  ? "kSimpleGroup" :
		       (type == RE::AVMData::Type::kComplexGroup) ? "kComplexGroup" :
		       (type == RE::AVMData::Type::kModulation)   ? "kModulation" :
		                                                    "kNone";
	}

	static json getJsonAVMData(RE::TESNPC* npc)
	{
		json j_avm_array = json::array();
		auto AVMTints = npc->tintAVMData;

		for (auto& avmd : AVMTints) {
			json j_avm_single;

			auto        type = avmd.type;
			std::string typeString = getStringTypeFromAVM(type);

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
			json j_avm_colors;
			j_avm_colors["Alpha"] = avmd.unk10.color.alpha;
			j_avm_colors["Blue"] = avmd.unk10.color.blue;
			j_avm_colors["Green"] = avmd.unk10.color.green;
			j_avm_colors["Red"] = avmd.unk10.color.red;

			j_avm_single["Color"] = j_avm_colors;

			j_avm_array.push_back(j_avm_single);
		}
		return j_avm_array;
	}

	static json getJsonHeadpartData(RE::TESNPC* npc)
	{
		json j_headpart_array = json::array();

		auto Headparts = npc->headParts;
		auto guard = npc->headParts.lock();

		for (RE::BGSHeadPart* part : *guard) {
			json j_headpart_single;

			std::string editorID = std::to_string(part->GetFormID());
			std::string model = part->model.data();
			std::string chargenModel = part->chargenModel.model.data();
			std::string colorMapping = part->colorMapping.data();
			std::string mask = part->mask.data();
			//std::string textureSet = part->textureSet->materialName.data(); // Crashes

			// Morphs
			//json j_headpart_morph;
			//auto morph = part->morphableObject.;
			//std::string morphEditorID = std::to_string(morph->GetFormID());
			//std::string performanceMorph = morph->performanceMorph.data();
			//std::string chargenMorph = morph->chargenMorph.data();
			//j_headpart_morph["EditorID"] = morphEditorID;
			//j_headpart_morph["PerformanceMorph"] = performanceMorph;
			//j_headpart_morph["ChargenMorph"] = chargenMorph;

			//json j_extra_parts = json::array();
			//for (auto& extraPart : part->extraParts) {
			//	json j_extra_part;
			//
			//	std::string extra_formID = std::to_string(part->GetFormID());
			//	std::string extra_model = part->model.data();
			//	std::string extra_chargenModel = part->chargenModel.model.data();
			//	std::string extra_colorMapping = part->colorMapping.data();
			//	std::string extra_mask = part->mask.data();
			//
			//	j_extra_part["FormID"] = extra_formID;
			//	j_extra_part["Model"] = extra_model;
			//	j_extra_part["ChargenModel"] = extra_chargenModel;
			//	j_extra_part["ColorMapping"] = extra_colorMapping;
			//	j_extra_part["Mask"] = extra_mask;
			//
			//	j_extra_parts.push_back(j_extra_part);
			//}

			// Setting headpart data
			j_headpart_single["EditorID"] = editorID;
			//j_headpart_single["Model"] = model;
			//j_headpart_single["ChargenModel"] = chargenModel;
			//j_headpart_single["ExtraParts"] = j_extra_parts;
			//j_headpart_single["ColorMapping"] = colorMapping;
			//j_headpart_single["Mask"] = mask;

			j_headpart_array.push_back(j_headpart_single);
		}
		return j_headpart_array;
	}

	// Load preset data
	static json getPresetData(RE::TESNPC* npc)
	{
		std::string            n = "\n";
		std::list<RE::AVMData> avmlist;

		json j;

		j["AVM"] = getJsonAVMData(npc);
		j["Headparts"] = getJsonHeadpartData(npc);

		return j;
	}

	static void loadPresetData(RE::Actor* actor)
	{
		std::ifstream jsonFile(iniFilePath);
		json          j;

		RE::TESNPC* npc = actor->GetNPC();

		try {
			jsonFile >> j;
		} catch (json::parse_error& e) {
			jsonFile.close();

			API->LogBuffer->Append(LogHandle, e.what());
			return;
		}
		jsonFile.close();

		//AVM
		npc->tintAVMData.clear();

		int avmSize = j["AVM"].size();
		int index = 0;

		API->LogBuffer->Append(LogHandle, "LOADING PRESET");
		API->LogBuffer->Append(LogHandle, std::to_string(avmSize).c_str());

		for (auto& item : j["AVM"]) {
			RE::AVMData avm;

			avm.type = getAVMTypeFromString(item.value("Type", "kNone"));

			avm.category = RE::BSFixedString(item.value("Category", "").c_str());
			avm.unk10.name = RE::BSFixedString(item.value("Name", "").c_str());
			avm.unk10.texturePath = RE::BSFixedString(item.value("TexturePath", "").c_str());
			avm.unk10.intensity = std::uint32_t(item.value("Intensity", 64));

			API->LogBuffer->Append(LogHandle, "Loading color data.");

			auto color = item["Color"];

			avm.unk10.color = RE::Color(
				uint8_t(color.value("Red", 64)),
				uint8_t(color.value("Green", 64)),
				uint8_t(color.value("Blue", 64)),
				uint8_t(color.value("Alpha", 64)));

			API->LogBuffer->Append(LogHandle, avm.category.c_str());
			API->LogBuffer->Append(LogHandle, avm.unk10.name.c_str());
			API->LogBuffer->Append(LogHandle, avm.unk10.texturePath.c_str());
			API->LogBuffer->Append(LogHandle, std::to_string(avm.unk10.intensity).c_str());

			npc->tintAVMData.emplace_back(std::move(avm));
		}

		jsonFile.close();
		actor->UpdateChargenAppearance();
	}

	static void ECDrawCallback(void* imgui_context)
	{
		if (hasLoaded == false) {
			return;
		}
		// Used for sliders
		//int min = -1;
		//int max = 16;

		// Setting actor
		RE::Actor* actor = GetSelActorOrPlayer();
		RE::TESNPC* actorNpc = actor->GetNPC();

		// UI
		UI->Text(actor->GetDisplayFullName());

		// Settings
		//UI->DragInt("Minimum slider value", &min, -16, 0);
		//UI->DragInt("Minimum slider value", &max, 1, 16);

		// Preset saving: TODO
		//if (UI->Button("Save preset")) {
		//	saveData(getPresetData(actorNpc));
		//}
		//if (UI->Button("Load preset")) {
		//	loadPresetData(actor);
		//	SFSE::GetTaskInterface()->AddTask(
		//		[actor]() {
		//			actor->UpdateChargenAppearance();
		//		});
		//}
	
		// Tabs
		void (*TabBarPtr)(const char* const* const, uint32_t, int*) = UI->TabBar;
		const char* tabHeaders[] = { "AVM", "Headparts", "Sliders", "Race", "Performance morphs" };
		
		uint32_t    tabCount = 5;
		int         activeTab = 1;
	
		TabBarPtr(tabHeaders, tabCount, &activeTab);
	
		if (activeTab == 0) {  // AVM tab
			auto AVMTints = actorNpc->tintAVMData;
			//std::list<RE::AVMData> avmlist;
	
			for (auto& avmd : AVMTints) {
				//avmlist.push_front(avmd);
	
				std::string typeString = getStringTypeFromAVM(avmd.type).c_str();
	
				UI->Text(avmd.category.c_str());
				UI->Text(avmd.unk10.name.c_str());
				UI->Text(avmd.unk10.texturePath.c_str());
				UI->Text(std::to_string(avmd.unk10.intensity).c_str());
				UI->Separator();
			}
		} else if (activeTab == 1)  // Headparts tab
		{
			auto Headparts = actorNpc->headParts;
			auto guard = Headparts.lock();
	
			for (RE::BGSHeadPart* part : *guard) {
				UI->Text(part->formEditorID.c_str());
				UI->Text(part->colorMapping.c_str());
				UI->Text(part->mask.c_str());
				UI->Separator();
			}
		} else if (activeTab == 2)  // Morphs tab
		{
			UI->Text("Weight");
			if (UI->SliderFloat(
					"Overweight",
					&actorNpc->morphWeight.fat,
					0.0f,
					1.0f,
					NULL)) {
				SFSE::GetTaskInterface()->AddTask(
					[actor]() {
						actor->UpdateChargenAppearance();
					});
			}

			if (UI->SliderFloat(
					"Thin",
					&actorNpc->morphWeight.thin,
					0.0f,
					1.0f,
					NULL)) {
				SFSE::GetTaskInterface()->AddTask(
					[actor]() {
						actor->UpdateChargenAppearance();
					});
			}

			if (UI->SliderFloat(
					"Strong",
					&actorNpc->morphWeight.muscular,
					0.0f,
					1.0f,
					NULL)) {
				SFSE::GetTaskInterface()->AddTask(
					[actor]() {
						actor->UpdateChargenAppearance();
					});
			}
	
			auto ShapeBlends = actorNpc->shapeBlendData;

			if (ShapeBlends != nullptr) {
				UI->Text("Shape Blends (Morphs)");  // Also shows custom morphs here
				for (const auto& pair : *ShapeBlends) {
					//UI->Text(std::to_string().c_str());
					if (UI->SliderFloat(
							pair.key.c_str(),
							(float*)&pair.value,
							0.0f,
							1.0f,
							NULL)) {
						SFSE::GetTaskInterface()->AddTask(
							[actor]() {
								actor->UpdateChargenAppearance();
							});
					}
				}
			}
	
			auto Morphs1 = actorNpc->unk3E0;
	
			UI->Text("Morphs1");
			if (Morphs1 != nullptr) {
				for (const auto& outerPair : *Morphs1) {
					std::uint32_t                               outerKey = outerPair.key;
					RE::BSTHashMap<RE::BSFixedStringCS, float>* innerMap = outerPair.value;
	
					if (innerMap) {
						for (const auto& innerPair : *innerMap) {
							const RE::BSFixedStringCS& innerKey = innerPair.key;
	
							if (UI->SliderFloat(
									innerKey.c_str(),
									(float*)&innerPair.value,
									0.0f,
									1.0f,
									NULL)) {
								SFSE::GetTaskInterface()->AddTask(
									[actor]() {
										actor->UpdateChargenAppearance();
									});
							}
						}
					} else {
						UI->Text("Inner map is null");
					}
				}
			} else {
				UI->Text("Morphs1 is null");
			}
	
			// Facebones. TODO: Read all from file
			UI->Text("Facebones");
			auto Morphs2 = actorNpc->unk3D8;
			if (Morphs2 != nullptr) {
				for (const auto& pair : *Morphs2) {
					std::uint32_t key = pair.key;
	
					if (UI->SliderFloat(
							std::to_string(key).c_str(),
							(float*)&pair.value,
							-16.0f,
							16.0f,
							NULL)) {
						SFSE::GetTaskInterface()->AddTask(
							[actor]() {
								actor->UpdateChargenAppearance();
							});
					}
				}
			}
		} else if (activeTab == 3)  // Race tab
		{
			UI->Text(actor->race->GetFullName());
		} else if (activeTab == 4) {  //
			auto actorLoadedData = actor->loadedData.lock_write();
			auto actorData3D = actorLoadedData->data3D;

			if (actorData3D != nullptr) {
				auto actor3D = static_cast<RE::BGSFadeNode*>(actorData3D.get());

				auto facegen = actor3D->bgsModelNode->facegenNodes.data[0]->faceGenAnimData;

				if (facegen != nullptr) {
					auto facegenMorphs = facegen->morphs;
					auto morphNames = facegen->morphNames;

					for (int i = 0; i < 104; i++) {
						auto morph = &facegenMorphs[i];

						UI->SliderFloat(
							std::to_string(i).c_str(),
							morph,
							0.0f,
							1.0f,
							NULL);
					}
				}
			}
		}

		UI->Separator();
		UI->ShowLogBuffer(LogHandle, true);
	}
}

static int OnBetterConsoleLoad(const struct better_api_t* BetterAPI)
{
	RegistrationHandle ec_mod_handle = BetterAPI->Callback->RegisterMod("Extended Chargen");

	// Registering any callback is optional, but you will probably want
	// to at least register the draw callback to show a mod menu
	BetterAPI->Callback->RegisterDrawCallback(ec_mod_handle, &ExtendedChargen::ECDrawCallback);

	LogHandle = BetterAPI->LogBuffer->Create("Extended Chargen Log", NULL);

	API = BetterAPI;

	UI = BetterAPI->SimpleDraw;

	return 0;
}

DLLEXPORT bool SFSEAPI SFSEPlugin_Load(const SFSE::LoadInterface* a_sfse)
{
//#ifndef NDEBUG
//	MessageBoxA(NULL, "EXTENDED CHARGEN LOADED. SORRY FOR THE INTERRUPTION...", Plugin::NAME.data(), NULL);
//#endif
	//logger::info("Extended Chargen loaded.");
	
	SFSE::Init(a_sfse, false);
	SFSE::GetMessagingInterface()->RegisterListener(MessageCallback);

	//sniffer::RegisterForAllEvents();

	return true;
}
