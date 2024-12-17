/* 
 * https://github.com/Starfield-Reverse-Engineering/CommonLibSF
 * This plugin template links against CommonLibSF
 */

#define BETTERAPI_IMPLEMENTATION
#include <clipboardxx.hpp>
#include "CCF_API.h"
#include "NiAVObject.h"
#include "betterapi.h"
#include "PresetsUtils.h"
#include "Utils.h"
#include "ChargenUtils.h"


#include "LogWrapper.h"
#include "SFEventHandler.h"
#include "HookManager.h"

//#include "ConditionalMorphManager.h"

// SFSEPlugin_Version
DLLEXPORT constinit auto SFSEPlugin_Version = []() noexcept {
	SFSE::PluginVersionData data{};

	data.PluginVersion(Plugin::Version);
	data.PluginName(Plugin::NAME);
	data.AuthorName(Plugin::AUTHOR);

	data.CompatibleVersions({ SFSE::RUNTIME_LATEST });

	return data;
}();

using json = nlohmann::json;

static const BetterAPI*            API = NULL;
static const struct simple_draw_t* UI = NULL;
static LogBufferHandle             LogHandle = 0;
static auto                        lastExecutionTime = std::chrono::steady_clock::now();

static std::atomic<bool> hasLoaded = false;
static nlohmann::json customConfig;

void MessageCallback(SFSE::MessagingInterface::Message* a_msg) noexcept
{
	events::GameDataLoadedEventDispatcher::GetSingleton()->Dispatch({ SFSE::MessagingInterface::MessageType(a_msg->type) });

	switch (a_msg->type) {
	case SFSE::MessagingInterface::kPostDataLoad:
		{
			hasLoaded = true;
			customConfig = utils::getChargenConfig();
		}
		break;
	case SFSE::MessagingInterface::kPostLoad:
		{
			events::RegisterHandlers();
			
			hooks::InstallHooks();
		}
		break;
	default:
		break;
	}
}

namespace ExtendedChargen
{
	const char* selectionListCallback(const void* userdata, uint32_t index, char* out_buffer, uint32_t out_buffer_size)
	{
		const auto* items = static_cast<const std::vector<std::string>*>(userdata);

		if (index >= items->size()) {
			return nullptr;
		}

		snprintf(out_buffer, out_buffer_size, "%s", (*items)[index].c_str());

		return out_buffer;
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
		RE::Actor*  actor = utils::GetSelActorOrPlayer();
		RE::TESNPC* actorNpc = actor->GetNPC();

		// Settings
		//UI->DragInt("Minimum slider value", &min, -16, 0);
		//UI->DragInt("Minimum slider value", &max, 1, 16);

		UI->Text(actor->GetDisplayFullName());

		// Tabs
		void (*TabBarPtr)(const char* const* const, uint32_t, int*) = UI->TabBar;
		const char* tabHeaders[] = { "AVM", "Headparts (WIP)", "Sliders", "Race (WIP)", "Presets (WIP)", "Custom chargen", "Debug" };

		uint32_t tabCount = 7;
		int      activeTab = 1;

		TabBarPtr(tabHeaders, tabCount, &activeTab);

		if (activeTab == 0) {  // AVM tab
			if (actorNpc == nullptr || actor == nullptr) {
				actor = utils::GetSelActorOrPlayer();
				actorNpc = actor->GetNPC();
			}

			// Eye colors
			uint32_t selEyeColor = 0;

			uint32_t selHairColor = 0;

			auto eyeColorsAVM = chargen::getAVMList("SimpleGroup_EyeColor");

			auto hairColorsAVM = chargen::getAVMList("SimpleGroup_Hair_Long_Straight");

			UI->VboxTop(0.4f, 0.0f);
			UI->HBoxLeft(0.33f, 0.0f);

			UI->Text("Eye color");
			if (UI->SelectionList(&selEyeColor, &eyeColorsAVM, eyeColorsAVM.size(), selectionListCallback)) {
				actorNpc->eyeColor = eyeColorsAVM[selEyeColor];
				chargen::updateActorAppearanceFully(actor, false, false);
			}

			UI->HBoxRight();

			UI->Text("Hair color");
			if (UI->SelectionList(&selHairColor, &hairColorsAVM, hairColorsAVM.size(), selectionListCallback)) {
				actorNpc->hairColor = hairColorsAVM[selHairColor];
				chargen::updateActorAppearanceFully(actor, false, false);
			}

			UI->HBoxRight();

			UI->HBoxEnd();

			UI->VBoxEnd();

			UI->VboxTop(0.4f, 0.0f);


			UI->VBoxEnd();

		} else if (activeTab == 1)  // Headparts tab
		{
			UI->Text("WIP!");
			/*
			if (actorNpc == nullptr || actor == nullptr) {
				actor = utils::GetSelActorOrPlayer();
				actorNpc = actor->GetNPC();
			}
			
			auto Headparts = actorNpc->headParts;
			auto guard = Headparts.lock();
			
			for (RE::BGSHeadPart* part : *guard) {
				UI->Text(part->formEditorID.c_str());
				UI->Text(part->colorMapping.c_str());
				UI->Text(part->mask.c_str());
				UI->Separator();
			}
			*/
		} else if (activeTab == 2)  // Morphs tab
		{
			if (actorNpc == nullptr || actor == nullptr) {
				actor = utils::GetSelActorOrPlayer();
				actorNpc = actor->GetNPC();
			}

			UI->Text("All morphs");
			// Weight
			UI->Text("Weight");
			auto* weights = chargen::availableMorphWeight(actorNpc);

			for (const auto& [str, weightData] : *weights) {
				if (UI->SliderFloat(
						str.c_str(),
						weightData,
						0.0f,
						1.0f,
						NULL))
				{
					chargen::updateActorAppearance(actor);
				}
			}
			delete weights;

			// Shape blends (Also, custom morphs)
			auto* shapeBlends = chargen::availableShapeBlends(actorNpc);

			if (shapeBlends != nullptr) {
				UI->Text("Shape Blends (Morphs)");

				for (const auto& pair : *shapeBlends) {
					if (UI->SliderFloat(
							pair.key.c_str(),
							(float*)&pair.value,
							0.0f,
							1.0f,
							NULL))
					{
						chargen::updateActorAppearance(actor);
					}
				}
			}

			// Morph definition sliders
			auto npcMorphDefinitions = chargen::availableMorphDefinitions(actorNpc);

			if (npcMorphDefinitions != nullptr) {
				UI->Text("Morph Definitions");

				for (const auto& shapeBlendsPair : *npcMorphDefinitions) {
					if (UI->SliderFloat(
							shapeBlendsPair.key.c_str(),
							(float*)&shapeBlendsPair.value,
							0.0f,
							1.0f,
							NULL)) {
						chargen::updateActorAppearance(actor);
					}
				}
			}

			// Facebones sliders
			RE::BSTHashMap2<std::uint32_t, float>* npcFaceBones = chargen::availableFacebones(actorNpc);

			if (npcFaceBones != nullptr) {
				UI->Text("Facebones");

				for (const auto& pair : *npcFaceBones) {

					if (UI->SliderFloat(
							std::to_string(pair.key).c_str(),
							(float*)&pair.value,
							-16.0f,
							16.0f,
							NULL)) {
						chargen::updateActorAppearance(actor);
					}
				}
			}
		} else if (activeTab == 3)  // Race tab
		{
			UI->Text("WIP!");
			/*
			if (actorNpc == nullptr || actor == nullptr) {
				actor = utils::GetSelActorOrPlayer();
				actorNpc = actor->GetNPC();
			}

			UI->Text(actor->race->GetFullName());
			*/
		} else if (activeTab == 4) {
			UI->Text("WIP!");
			/*
			if (actorNpc == nullptr || actor == nullptr) {
				actor = utils::GetSelActorOrPlayer();
				actorNpc = actor->GetNPC();
			}

			if (UI->Button("Save preset")) {
				utils::saveDataJSON(presets::getPresetData(actorNpc));
			}

			if (UI->Button("Load preset")) {
				presets::loadPresetData(actor);
				chargen::updateActorAppearanceFully(actor, false, true);
			}
			*/

		} else if (activeTab == 5) { // Custom morphs
			std::vector<float> minMax;
			std::vector<std::string> morphList;

			if (actorNpc == nullptr || actor == nullptr) {
				actor = utils::GetSelActorOrPlayer();
				actorNpc = actor->GetNPC();
			}

			auto actorMorphs = chargen::availableShapeBlends(actorNpc);

			void (*customConfigTabs)(const char* const* const, uint32_t, int*) = UI->TabBar;
			int customConfigActiveTab = 1;

			// Parsing for tab headers
			std::vector<std::string> customConfigTabHeaders;

			for (nlohmann::json config : customConfig) {
				std::string configName = config.value("Name", "");

				customConfigTabHeaders.push_back(configName.c_str());
			}

			// Preparing the tabs
			size_t headersSize = customConfigTabHeaders.size();

			if (headersSize > 0) {
				const char** configTabHeaders = new const char*[headersSize];

				for (size_t i = 0; i < headersSize; ++i) {
					configTabHeaders[i] = customConfigTabHeaders[i].c_str();
				}

				customConfigTabs(configTabHeaders, headersSize, &customConfigActiveTab);

				delete[] configTabHeaders;

				if (customConfigActiveTab <= headersSize) {
					auto& currentConfig = customConfig[customConfigActiveTab];

					for (int i = 0; i < currentConfig["Layout"].size(); i++) {
						nlohmann::json& layoutPart = currentConfig["Layout"][i];

						// Text parsing
						if (layoutPart.value("Type", "") == "Text") {
							UI->Text(layoutPart.value("Text", "").c_str());
						}

						// Separator parsing
						if (layoutPart.value("Type", "") == "Separator") {
							UI->Separator();
						}

						// Morph parsing
						if (layoutPart.value("Type", "") == "Morph" &&
							(layoutPart["Gender"] == 0 ||
							layoutPart["Gender"] != 0 && layoutPart["Gender"] == actorNpc->IsFemale() + 1))
						{
							if (layoutPart.value("Morph", "") != "") {
								std::string morphName = layoutPart.value("Morph", "");
								morphList.push_back(morphName);

								if (!actorMorphs->contains(morphName)) {
									actorMorphs->insert({ morphName, 0.0 });
								}

								auto shapeBlend = actorMorphs->find(morphName.c_str());

								if (UI->SliderFloat(
										layoutPart.value("Name", " ").c_str(),
										(float*)&shapeBlend->value,
										0.0f,
										1.0f,
										NULL)) {
									chargen::updateActorAppearance(actor);
								}

							} else if (layoutPart.contains("MorphMin") &&
									   layoutPart.contains("MorphMax") &&
									   layoutPart.value("MorphMin", "") != "" &&
									   layoutPart.value("MorphMax", "") != "")
							{

								std::string morphMinName = layoutPart.value("MorphMin", "");
								std::string morphMaxName = layoutPart.value("MorphMax", "");

								morphList.push_back(morphMinName);
								morphList.push_back(morphMaxName);

								if (!actorMorphs->contains(morphMinName)) {
									actorMorphs->insert({ morphMinName, 0.0 });
								}

								if (!actorMorphs->contains(morphMaxName)) {
									actorMorphs->insert({ morphMaxName, 0.0 });
								}

								auto shapeBlendMin = actorMorphs->find(morphMinName.c_str());
								auto shapeBlendMax = actorMorphs->find(morphMaxName.c_str());

								if (i >= minMax.size()) {
									minMax.resize(i + 1, 0.0f);
								}

								minMax[i] = (shapeBlendMin->value > 0.0f) ? -shapeBlendMin->value : shapeBlendMax->value;

								if (UI->SliderFloat(
										layoutPart.value("Name", " ").c_str(),
										&minMax[i],
										-1.0f,
										1.0f,
										NULL))
								{
									shapeBlendMax->value = (minMax[i] > 0.0) ? minMax[i] : 0.0;
									shapeBlendMin->value = (minMax[i] < 0.0) ? abs(minMax[i]) : 0.0;

									chargen::updateActorAppearance(actor);
								}
							}
						}
					}

					UI->Separator();
					UI->Text("Quick preset of the current tab morphs");

					std::vector<const char*> quickPresetLabels = std::vector<const char*> {
						"Copy current sliders to clipboard",
						"Paste sliders relevant for this category"
					};

					switch (UI->ButtonBar(
						quickPresetLabels.data(), quickPresetLabels.size()
					)) 
					{
					case (0): // Copy to clipboard
						{
							std::vector<std::pair<std::string, float>> quickMorphListWithValues;

							for (auto& morph : morphList) {
								if (actorMorphs->contains(morph)) {
									std::pair morphValuePair = std::pair<std::string, float>(morph, actorMorphs->find(morph)->value);
									quickMorphListWithValues.push_back(morphValuePair);
								}
							}

							std::string quickPresetData = presets::morphListToQuickPreset(quickMorphListWithValues);

							clipboardxx::clipboard clipboard;

							clipboard << quickPresetData;
						}
					case (1):
						{
							clipboardxx::clipboard clipboard;
							std::string            pasteText;

							clipboard >> pasteText;

							std::vector<std::pair<std::string, float>> quickMorphs = presets::quickPresetToMorphList(pasteText);

							if (quickMorphs.size() > 0) {
								for (auto& morphPair : quickMorphs) {
									if (!actorMorphs->contains(morphPair.first)) {
										actorMorphs->insert({ morphPair.first, 0.0 });
									}

									auto actorMorph = actorMorphs->find(morphPair.first.c_str());

									actorMorph->value = morphPair.second;
								}

								chargen::updateActorAppearance(actor);
							}
						}
							
					}
				}
			}
			morphList.clear();
			minMax.clear();
		} else if (activeTab == 6) {
			if (actorNpc == nullptr || actor == nullptr) {
				actor = utils::GetSelActorOrPlayer();
				actorNpc = actor->GetNPC();
			}

			UI->Text("For debug purposes only.");

			// UI
			if (UI->Button("Update appearance fully")) {
				chargen::updateActorAppearanceFully(actor, false, false);
			}

			if (UI->Button("Update appearance fully (Race)")) {
				chargen::updateActorAppearanceFully(actor, false, true);
			}

			if (UI->Button("DEBUG: Load default headparts for this race")) {
				chargen::loadDefaultRaceAppearance(actor);
				chargen::updateActorAppearanceFully(actor, false, true);
			}

			auto facegenMorphs = chargen::getPerformanceMorphs(actor);

			if (facegenMorphs != nullptr) {
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
	logger::info("Extended Chargen loaded.");

	SFSE::Init(a_sfse, false);
	SFSE::GetMessagingInterface()->RegisterListener(MessageCallback);

	return true;
}
