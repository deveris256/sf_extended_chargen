#pragma once
#include "SFEventHandler.h"
#include <functional>

namespace ExtendedChargen
{
	class ArmorKeywordMorphManager : 
		public events::ArmorOrApparelEquippedEventDispatcher::Listener,
		public events::ActorLoadedEventDispatcher::Listener,
		public events::GameDataLoadedEventDispatcher::Listener,
		public events::EventDispatcher<events::ActorUpdateEvent>::Listener
	{
	public:
		class KeywordMorphData
		{
		public:
			using MorphFunc = std::function<void(RE::Actor*)>;

			KeywordMorphData(RE::BGSKeyword* a_keyword) :
				keyword(a_keyword)
			{
				this->LoadKeywordMorphData(a_keyword);
			}

			void LoadKeywordMorphData(RE::BGSKeyword* a_keyword)
			{
				// Load morph data from keyword
				std::string_view script = a_keyword->unk50.c_str();
				logger::c_info("Morph script: {}:{}", utils::make_str(a_keyword), script);
			}

			RE::BGSKeyword* keyword;
			uint32_t  priority;
			MorphFunc morph_func;

		protected:
			KeywordMorphData(RE::BGSKeyword* a_keyword, uint32_t a_priority, MorphFunc a_morph_func) :
				keyword(a_keyword),
				priority(a_priority),
				morph_func(a_morph_func)
			{}
		};


		virtual ~ArmorKeywordMorphManager() = default;

		static ArmorKeywordMorphManager* GetSingleton()
		{
			static ArmorKeywordMorphManager singleton(256);
			return &singleton;
		}

		void OnEvent(const events::ArmorOrApparelEquippedEvent& a_event, events::EventDispatcher<events::ArmorOrApparelEquippedEvent>* a_dispatcher) override
		{
			auto equip_type = a_event.equipType;

			/*switch (equip_type) {
				case events::ArmorOrApparelEquippedEvent::EquipType::kEquip:
					logger::c_info("Armor Keyword Morph: {} equipped", utils::make_str(a_event.actor));
					break;
				case events::ArmorOrApparelEquippedEvent::EquipType::kEquip2:
					logger::c_info("Armor Keyword Morph: {} equipped 2", utils::make_str(a_event.actor));
					break;
				case events::ArmorOrApparelEquippedEvent::EquipType::kUnequip:
					logger::c_info("Armor Keyword Morph: {} unequipped", utils::make_str(a_event.actor));
					break;
			}*/

			//logger::c_info("Armor Keyword Morph: {} equipped", utils::make_str(a_event.actor));
			auto actor = a_event.actor;
			auto armo = a_event.armorOrApparel;
			if (!actor) {
				return;
			}

			auto npc = actor->GetNPC();
			if (!npc) {
				return;
			}

			//npc->morphWeight.fat = 1.0f;

			//actor->UpdateChargenAppearance();
		}

		void OnEvent(const events::ActorLoadedEvent& a_event, events::EventDispatcher<events::ActorLoadedEvent>* a_dispatcher) override
		{
			if (a_event.actor->IsPlayerRef()) {
				//logger::c_info("Armor Keyword Morph: {} loaded", utils::make_str(a_event.actor));
			}
			//logger::c_info("Armor Keyword Morph: {} loaded", utils::make_str(a_event.actor));

			auto actor = a_event.actor;
			if (!actor) {
				return;
			}

			auto npc = actor->GetNPC();
			if (!npc) {
				return;
			}

			//npc->morphWeight.fat = 1.0f;

			//actor->UpdateAppearance(true, 0, false);
			//actor->UpdateChargenAppearance();
		}

		void OnEvent(const events::GameLoadedEvent& a_event, events::EventDispatcher<events::GameLoadedEvent>* a_dispatcher) override
		{
			if (a_event.messageType != SFSE::MessagingInterface::MessageType::kPostPostDataLoad) {
				return;
			}

			// Scan for morph keywords for caching (optional)
			return;
		}

		void OnEvent(const events::ActorUpdateEvent& a_event, events::EventDispatcher<events::ActorUpdateEvent>* a_dispatcher) override
		{
			//logger::c_info("Actor {} updated with deltaTime: {}", utils::make_str(a_event.actor), a_event.deltaTime);
		}

		void Register()
		{
			events::ArmorOrApparelEquippedEventDispatcher::GetSingleton()->AddStaticListener(this);
			events::ActorLoadedEventDispatcher::GetSingleton()->AddStaticListener(this);
			events::GameDataLoadedEventDispatcher::GetSingleton()->AddStaticListener(this);
			events::ActorUpdatedEventDispatcher::GetSingleton()->AddStaticListener(this);
		}



		std::unordered_map<RE::BGSKeyword*, KeywordMorphData> m_morph_keywords;

		static constexpr std::string_view _morph_keyword_prefix = "ECArmorMorph_";

		bool IsMorphKeyword(RE::BGSKeyword* a_keyword){
			if (m_morph_keywords.find(a_keyword) != m_morph_keywords.end()) {
				return true;
			} else if (this->IsMorphKeyword_Impl(a_keyword)) {
				m_morph_keywords.insert({ a_keyword, KeywordMorphData(a_keyword) });
				return true;
			}
			return false;
		};

		std::vector<RE::BGSKeyword*> ScanEquippedArmor(RE::Actor* a_actor);


	private:
		bool IsMorphKeyword_Impl(RE::BGSKeyword* a_keyword) { 
			return a_keyword->formEditorID.contains(_morph_keyword_prefix);
		};

		ArmorKeywordMorphManager(size_t estimated_count)
		{
			m_morph_keywords.reserve(estimated_count);
		};
	};
};