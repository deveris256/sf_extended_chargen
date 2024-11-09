#pragma once
#include "SFEventHandler.h"
#include "DynamicMorphSession.h"

namespace DEPRECATED
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
				// Example: Thin -a -0.5 -p 3 // Decrease thin morph by 0.5, priority 3, short names for arguments are also supported
				utils::parser::ArmorKeywordMorphParser parser;
				if (parser.parse(script)) {
					if (parser.hasSet()) {
						mode = Mode::kSet;
					}
					else if (parser.hasAdd()) {
						mode = Mode::kAdd;
					} else {
						std::string error_msg = std::format("Failed to parse morph keyword script: kw:{}, script:{}, error:{}", utils::make_str(a_keyword), script, "No --add or --set specified");
						logger::c_error(error_msg.c_str());
						logger::error(error_msg.c_str());
						is_active = false;
						return;
					}
					add = parser.getAdd();
					set = parser.getSet();
					morph_name = parser.getMorphName();
					priority = parser.getPriority();
				} else {
					std::string error_msg = std::format("Failed to parse morph keyword script: kw:{}, script:{}, error:{}", utils::make_str(a_keyword), script, parser.getLastError());
					logger::c_error(error_msg.c_str());
					logger::error(error_msg.c_str());
					is_active = false;
					return;
				}
				is_active = true;
				return;
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
			static ArmorKeywordMorphManager singleton;
			return &singleton;
		}

		void OnEvent(const events::ArmorOrApparelEquippedEvent& a_event, events::EventDispatcher<events::ArmorOrApparelEquippedEvent>* a_dispatcher) override
		{	
			auto equip_type = a_event.equipType;
			auto actor = a_event.actor;
			auto armo = a_event.armorOrApparel;

			{
				tbb::concurrent_hash_map<RE::TESFormID, time_t>::accessor acc;
				if (!actor || !m_actor_watchlist.find(acc, actor->formID)) {
					return;
				}
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
			if (IsActorMenuActor(a_event.actor) && a_event.when() - menu_actor_last_update_time > 200) {
				menu_actor_last_update_time = a_event.when();
				if (this->ReevaluateActorMorph(actor)) {
					actor->UpdateChargenAppearance();
				}
				return;
			}

			{
				tbb::concurrent_hash_map<RE::TESFormID, time_t>::accessor acc;
				if (!actor || !m_actor_watchlist.find(acc, actor->formID)) {
					return;
				}

				if (m_actors_pending_reevaluation.contains(actor)) {
					std::lock_guard lock(m_actors_pending_reevaluation_erase_lock);
					acc->second = a_event.when();
					if (this->ReevaluateActorMorph(actor)) {
						actor->UpdateChargenAppearance();
					}
					m_actors_pending_reevaluation.unsafe_erase(actor);
					return;
				}
			}
		}

		void OnEvent(const events::ActorFirstUpdateEvent& a_event, events::EventDispatcher<events::ActorFirstUpdateEvent>* a_dispatcher) override
		{
			logger::c_info("Actor {} first updated with deltaTime: {} ms, timeStamp {}", utils::make_str(a_event.actor), a_event.deltaTime * 1000, a_event.when());
			
			auto actor = a_event.actor;
			{
				tbb::concurrent_hash_map<RE::TESFormID, time_t>::accessor acc;
				if (!actor || !m_actor_watchlist.find(acc, actor->formID)) {
					return;
				}

				acc->second = a_event.when();
				if (this->ReevaluateActorMorph(actor)) {
					actor->UpdateChargenAppearance();
				}
			}
		}

		void Watch(RE::Actor* a_actor) 
		{
			if (a_actor) {
				tbb::concurrent_hash_map<RE::TESFormID, time_t>::accessor acc;
				m_actor_watchlist.insert(acc, { a_actor->formID, 0 });
				m_actors_pending_reevaluation.insert(a_actor);
			}
		}

		void Unwatch(RE::Actor* a_actor)
		{
			if (a_actor) {
				std::lock_guard lock(m_actor_watchlist_erase_lock);
				m_actor_watchlist.erase(a_actor->formID);
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

		// Thread-safe
		std::unordered_set<RE::BGSKeyword*> ScanEquippedArmor(RE::Actor* a_actor, std::unordered_set<RE::TESBoundObject*> exclude_list = {});

		bool ReevaluateActorMorph(RE::Actor* a_actor)
		{
			// Critical section
			daf::DynamicMorphSession manager(daf::tokens::general_offset, a_actor);
			// End of critical section

			auto morph_kws = ScanEquippedArmor(a_actor);
			
			std::vector<KeywordMorphData*> morph_data;
			for (auto& kw : morph_kws) {
				auto data = m_morph_keywords[kw].get();
				if (data->is_active) {
					morph_data.push_back(data);
				}
			}

			if (morph_data.size() == 0) {
				return false;
			}

			// Sort by priority
			std::sort(morph_data.begin(), morph_data.end(), [](KeywordMorphData* a, KeywordMorphData* b) {
				return a->priority < b->priority;
			});

			for (auto& data : morph_data) {
				switch (data->mode) {
					case KeywordMorphData::Mode::kAdd:
						manager.MorphOffsetCommit(data->morph_name, data->add);
						break;
					case KeywordMorphData::Mode::kSet:
						manager.MorphTargetCommit(data->morph_name, data->set);
						break;
				}
			}
			
			// Critical section
			return manager.PushCommits() > 1E-3;
			// End of critical section
		}

	private:
		std::mutex                                m_actors_pending_reevaluation_erase_lock;
		tbb::concurrent_unordered_set<RE::Actor*> m_actors_pending_reevaluation;

		std::mutex                                   m_actor_watchlist_erase_lock;
		tbb::concurrent_hash_map<RE::TESFormID, time_t> m_actor_watchlist{ { 0x14, 0 } };  // Player_ref

		time_t menu_actor_last_update_time = 0;

		inline bool IsMorphKeyword_Impl(RE::BGSKeyword* a_keyword) { 
			return a_keyword->formEditorID.contains(_morph_keyword_prefix);
		};

		inline bool IsActorMenuActor(RE::Actor* a_actor) {
			return a_actor->boolFlags.underlying() == 4 && a_actor->boolFlags2.underlying() == 8458272; // From experience
		}

		ArmorKeywordMorphManager()
		{};
	};
};