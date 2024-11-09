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

#include "ConditionalMorphManager.h"

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

void MessageCallback(SFSE::MessagingInterface::Message* a_msg) noexcept
{
	events::GameDataLoadedEventDispatcher::GetSingleton()->Dispatch({ SFSE::MessagingInterface::MessageType(a_msg->type) });

	switch (a_msg->type) {
	case SFSE::MessagingInterface::kPostDataLoad:
		{
			hasLoaded = true;
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
		const char* tabHeaders[] = { "AVM", "Headparts", "Sliders", "Race", "Performance morphs", "Presets (WIP)" };

		uint32_t tabCount = 6;
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
