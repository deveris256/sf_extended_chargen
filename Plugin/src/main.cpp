/* 
 * https://github.com/Starfield-Reverse-Engineering/CommonLibSF
 * This plugin template links against CommonLibSF
 */

#define BETTERAPI_IMPLEMENTATION
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

static bool appearanceChanged = false;

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

		// UI
		UI->Text(actor->GetDisplayFullName());
		if (UI->Button("Update appearance fully")) {
			chargen::updateActorAppearanceFully(actor, false, false);
		}

		if (UI->Button("Update appearance fully (Race)")) {
			chargen::updateActorAppearanceFully(actor, false, true);
		}

		// Settings
		//UI->DragInt("Minimum slider value", &min, -16, 0);
		//UI->DragInt("Minimum slider value", &max, 1, 16);

		// Tabs
		void (*TabBarPtr)(const char* const* const, uint32_t, int*) = UI->TabBar;
		const char* tabHeaders[] = { "AVM", "Headparts", "Sliders", "Race", "Performance morphs", "Presets (WIP)", "Custom chargen" };

		uint32_t tabCount = 7;
		int      activeTab = 1;

		TabBarPtr(tabHeaders, tabCount, &activeTab);

		if (activeTab == 0) {  // AVM tab
			uint32_t selEyeColor = 0;
			auto     eyeColorsAVM = chargen::availableEyeColor();

			UI->VboxTop(0.4f, 0.0f);

			if (UI->SelectionList(&selEyeColor, &eyeColorsAVM, eyeColorsAVM.size(), selectionListCallback)) {
				actorNpc->eyeColor = eyeColorsAVM[selEyeColor];
				chargen::updateActorAppearanceFully(actor, false, false);
			}

			UI->VBoxBottom();
			UI->VBoxEnd();

			auto AVMTints = actorNpc->tintAVMData;
			//std::list<RE::AVMData> avmlist;

			for (auto& avmd : AVMTints) {
				//avmlist.push_front(avmd);

				std::string typeString = chargen::getStringTypeFromAVM(avmd.type).c_str();

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
			UI->Text(actor->race->GetFullName());

		} else if (activeTab == 4) {  //
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
		} else if (activeTab == 5) {
			if (UI->Button("Save preset")) {
				utils::saveDataJSON(presets::getPresetData(actorNpc));
			}

			if (UI->Button("Load preset")) {
				presets::loadPresetData(actor);
				chargen::updateActorAppearanceFully(actor, false, true);
			}

			if (UI->Button("DEBUG: Load default headparts for this race")) {
				chargen::loadDefaultRaceAppearance(actor);
				chargen::updateActorAppearanceFully(actor, false, true);
			}
		} else if (activeTab == 6) { // Custom morphs
			auto morphs = chargen::availableShapeBlends(actorNpc);
			
			std::vector<float> minMax;

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
							nlohmann::json& morph = layoutPart;

							if (morph.contains("Morph") && morph.value("Morph", "") != "") {
								morphs->insert({ morph.value("Morph", ""), 0.0 });
								auto shapeBlend = morphs->find(morph.value("Morph", "").c_str());

								if (UI->SliderFloat(
										morph.value("Name", " ").c_str(),
										(float*)&shapeBlend->value,
										0.0f,
										1.0f,
										NULL)) {
									chargen::updateActorAppearance(actor);
								}

							} else if (morph.contains("MorphMin") &&
									   morph.contains("MorphMax") &&
									   morph.value("MorphMin", "") != "" &&
									   morph.value("MorphMax", "") != "") { 

								morphs->insert({ morph.value("MorphMin", ""), 0.0 });
								morphs->insert({ morph.value("MorphMax", ""), 0.0 });

								auto shapeBlendMin = morphs->find(morph.value("MorphMin", "").c_str());
								auto shapeBlendMax = morphs->find(morph.value("MorphMax", "").c_str());

								if (i >= minMax.size()) {
									minMax.resize(i + 1, 0.0f);
								}

								minMax[i] = (shapeBlendMin->value > 0.0f) ? -shapeBlendMin->value : shapeBlendMax->value;

								if (UI->SliderFloat(
										morph.value("Name", " ").c_str(),
										&minMax[i],
										-1.0f,
										1.0f,
										NULL)) {
									shapeBlendMax->value = (minMax[i] > 0.0) ? minMax[i] : 0.0;
									shapeBlendMin->value = (minMax[i] < 0.0) ? abs(minMax[i]) : 0.0;

									chargen::updateActorAppearance(actor);
								}
							}
						}
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
	logger::info("Extended Chargen loaded.");

	SFSE::Init(a_sfse, false);
	SFSE::GetMessagingInterface()->RegisterListener(MessageCallback);

	return true;
}
