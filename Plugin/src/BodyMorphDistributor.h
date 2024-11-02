#pragma once
#include "SFEventHandler.h"

namespace ExtendedChargen
{
	class BodyMorphDistributor : public events::ActorLoadedEventDispatcher::Listener
	{
	public:
		virtual ~BodyMorphDistributor() = default;

		static BodyMorphDistributor* GetSingleton()
		{
			static BodyMorphDistributor singleton;
			return &singleton;
		}

		void OnEvent(const events::ActorLoadedEvent& a_event, events::EventDispatcher<events::ActorLoadedEvent>* a_dispatcher) override
		{
			logger::c_info("Changing Actor Body Morph: {}", utils::make_str(a_event.actor));
			auto actor = a_event.actor;
			auto npc = a_event.actor->GetNPC();
			if (!npc) {
				return;
			}

			auto skin = npc->formSkin;
			if (!skin) {
				return;
			}

			for (auto& model : skin->modelArray) {
				logger::c_info("Skin ARMA: {}", model.armorAddon->GetFormEditorID());
			}

		}

		void Register()
		{
			events::ActorLoadedEventDispatcher::GetSingleton()->AddStaticListener(this);
		}

	private:
		BodyMorphDistributor() = default;
	};
}