#pragma once
#include "SFEventHandler.h"
#include "ActorDynamicMorphManager.h"

namespace ExtendedChargen
{
	class ArmorKeywordMorphManager : 
		public events::ArmorOrApparelEquippedEventDispatcher::Listener,
		public events::GameDataLoadedEventDispatcher::Listener,
		public events::EventDispatcher<events::ActorUpdateEvent>::Listener,
		public events::EventDispatcher<events::ActorFirstUpdateEvent>::Listener
	{
	public:
		class KeywordMorphData
		{
		public:
			enum class Mode : uint8_t
			{
				kAdd,
				kSet
			};

			KeywordMorphData(RE::BGSKeyword* a_keyword) :
				keyword(a_keyword)
			{
				this->LoadKeywordMorphData(a_keyword);
			}

			void LoadKeywordMorphData(RE::BGSKeyword* a_keyword)
			{
				// Load morph data from keyword
				std::string_view script = a_keyword->unk50.c_str();

				// Parse script
				// Example: Overweight --add 0.5 --priority 1 // Increase overweight morph by 0.5
				// Example: Strong --set 1.0 --priority 2 // Set strong morph to 1.0
				utils::parser::ArmorKeywordMorphParser parser;
				if (parser.parse(script)) {
					morph_name = parser.getMorphName();
					add = parser.getAdd();
					set = parser.getSet();
					if (parser.hasSet()) {
						mode = Mode::kSet;
					}
					priority = parser.getPriority();
					is_active = true;
				} else {
					auto kw_str = utils::make_str(a_keyword);
					logger::c_error("Failed to parse morph keyword script: kw:{} script:{}", kw_str, script);
					logger::error("Failed to parse morph keyword script: kw:{} script:{}", kw_str, script);
				}
			}

			RE::BGSKeyword*  keyword;
			uint32_t         priority = 0;
			std::string      morph_name;
			float            add = 0.0;
			float            set = 0.0;
			bool 		     is_active = false;
			Mode 			 mode = Mode::kAdd;

		protected:
			KeywordMorphData() = default;
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
			auto actor = a_event.actor;
			auto armo = a_event.armorOrApparel;
			if (!actor || !m_actor_watchlist.contains(actor->formID)) {
				return;
			}

			auto npc = actor->GetNPC();
			if (!npc) {
				return;
			}

			switch (equip_type) {
			case events::ArmorOrApparelEquippedEvent::EquipType::kEquip:
				//logger::c_info("Armor Keyword Morph: {} equipped", utils::make_str(a_event.actor));
				m_actors_pending_reevaluation.insert(actor);
				break;
			case events::ArmorOrApparelEquippedEvent::EquipType::kUnequip:
				//logger::c_info("Armor Keyword Morph: {} unequipped", utils::make_str(a_event.actor));
				m_actors_pending_reevaluation.insert(actor);
				break;
			}
		}

		void OnEvent(const events::GameLoadedEvent& a_event, events::EventDispatcher<events::GameLoadedEvent>* a_dispatcher) override
		{
			if (a_event.messageType != SFSE::MessagingInterface::MessageType::kPostPostDataLoad) {
				return;
			}
			// Load settings & Scan for morph keywords for caching (optional)
			return;
		}

		void OnEvent(const events::ActorUpdateEvent& a_event, events::EventDispatcher<events::ActorUpdateEvent>* a_dispatcher) override
		{
			//logger::c_info("Actor {} updated with deltaTime: {} ms, timeStamp {}", utils::make_str(a_event.actor), a_event.deltaTime * 1000, a_event.when());
			auto actor = a_event.actor;
			if (!actor || !m_actor_watchlist.contains(actor->formID)) {
				return;
			}
			auto npc = actor->GetNPC();
			if (!npc) {
				return;
			}

			if (m_actors_pending_reevaluation.contains(actor)) {
				{
					std::lock_guard lock(m_actors_pending_reevaluation_erase_lock);
					auto            kw = ScanEquippedArmor(actor, {});
					if (this->ReevaluateActorMorph(actor, kw)) {
						actor->UpdateChargenAppearance();
					}
					m_actors_pending_reevaluation.unsafe_erase(actor);
				}
			}
		}

		void OnEvent(const events::ActorFirstUpdateEvent& a_event, events::EventDispatcher<events::ActorFirstUpdateEvent>* a_dispatcher) override
		{
			logger::c_info("Actor {} first updated with deltaTime: {} ms, timeStamp {}", utils::make_str(a_event.actor), a_event.deltaTime * 1000, a_event.when());
			
			auto actor = a_event.actor;
			if (!actor || !m_actor_watchlist.contains(actor->formID)) {
				return;
			}
			auto npc = actor->GetNPC();
			if (!npc) {
				return;
			}

			auto kw = ScanEquippedArmor(actor, {});
			if (this->ReevaluateActorMorph(actor, kw)) {
				actor->UpdateChargenAppearance();
			}
		}

		void Watch(RE::Actor* a_actor) 
		{
			if (a_actor) {
				m_actor_watchlist.insert(a_actor->formID);
				m_actors_pending_reevaluation.insert(a_actor);
			}
		}

		void Unwatch(RE::Actor* a_actor)
		{
			if (a_actor) {
				std::lock_guard lock(m_actor_watchlist_erase_lock);
				m_actor_watchlist.unsafe_erase(a_actor->formID);
			}
		}

		void Register()
		{
			events::ArmorOrApparelEquippedEventDispatcher::GetSingleton()->AddStaticListener(this);
			events::GameDataLoadedEventDispatcher::GetSingleton()->AddStaticListener(this);
			events::ActorUpdatedEventDispatcher::GetSingleton()->EventDispatcher<events::ActorUpdateEvent>::AddStaticListener(this);
			events::ActorUpdatedEventDispatcher::GetSingleton()->EventDispatcher<events::ActorFirstUpdateEvent>::AddStaticListener(this);
		}

		std::unordered_map<RE::BGSKeyword*, std::unique_ptr<KeywordMorphData>> m_morph_keywords;

		static constexpr std::string_view _morph_keyword_prefix = "ECArmorMorph_";

		bool IsMorphKeyword(RE::BGSKeyword* a_keyword)
		{
			if (m_morph_keywords.contains(a_keyword)) {
				return true;
			} else if (this->IsMorphKeyword_Impl(a_keyword)) {
				m_morph_keywords[a_keyword] = std::make_unique<KeywordMorphData>(a_keyword);
				return true;
			}
			return false;
		};

		std::unordered_set<RE::BGSKeyword*> ScanEquippedArmor(RE::Actor* a_actor, std::unordered_set<RE::TESBoundObject*> exclude_list = {});

		bool ReevaluateActorMorph(RE::Actor* a_actor, std::unordered_set<RE::BGSKeyword*>& a_morph_kws)
		{
			std::vector<KeywordMorphData*> morph_data;
			for (auto& kw : a_morph_kws) {
				if (this->IsMorphKeyword(kw)) {
					auto data = m_morph_keywords[kw].get();
					if (data->is_active) {
						morph_data.push_back(data);
					}
				}
			}

			if (morph_data.size() == 0) {
				return false;
			}

			// Sort by priority
			std::sort(morph_data.begin(), morph_data.end(), [](KeywordMorphData* a, KeywordMorphData* b) {
				return a->priority < b->priority;
			});

			std::unordered_map<std::string, float> new_offset_batch;
			for (auto& data : morph_data) {
				switch (data->mode) {
					case KeywordMorphData::Mode::kAdd:
						new_offset_batch[data->morph_name] += data->add;
						break;
					case KeywordMorphData::Mode::kSet:
						new_offset_batch[data->morph_name] = data->set;
						break;
				}
			}
			return ActorDynamicMorphManager::GetSingleton()->ReevaluateMorph(a_actor, new_offset_batch);
		}

	private:
		std::mutex                                m_actors_pending_reevaluation_erase_lock;
		tbb::concurrent_unordered_set<RE::Actor*> m_actors_pending_reevaluation;

		std::mutex                                   m_actor_watchlist_erase_lock;
		tbb::concurrent_unordered_set<RE::TESFormID> m_actor_watchlist{ 0x14 };  // Player_ref

		inline bool IsMorphKeyword_Impl(RE::BGSKeyword* a_keyword) { 
			return a_keyword->formEditorID.contains(_morph_keyword_prefix);
		};

		ArmorKeywordMorphManager(size_t estimated_count)
		{
			m_morph_keywords.reserve(estimated_count);
		};
	};
};