#pragma once
#include "LogWrapper.h"
#include "RE/E/Events.h"

#include "EventDispatcher.h"

namespace events
{
	class ArmorOrApparelEquippedEventDispatcher : 
		public RE::BSTEventSink<RE::TESEquipEvent>, 
		public EventDispatcher<ArmorOrApparelEquippedEvent>
	{
	public:
		using EventResult = RE::BSEventNotifyControl;
		using Event = RE::TESEquipEvent;

		static ArmorOrApparelEquippedEventDispatcher* GetSingleton()
		{
			static ArmorOrApparelEquippedEventDispatcher singleton;
			return &singleton;
		}

		EventResult ProcessEvent(const Event& a_event, RE::BSTEventSource<Event>* a_eventSource) override
		{
			//logger::c_info("Item actor_who {}({:X}), baseObject {:X}, origRef {:X} {}", a_event.actor->GetDisplayFullName(), a_event.actor->GetFormID(), a_event.baseObject, a_event.origRef, a_event.equipped ? "Equipped" : "Unequipped");
			
			auto actor = a_event.actor.get();
			auto object_form = RE::TESObjectREFR::LookupByID(a_event.baseObject);

			if (auto armo_form = object_form->As<RE::TESObjectARMO>(); armo_form != nullptr && actor != nullptr) {
				logger::c_info("Item actor {}, armo {}, {}", utils::make_str(actor), utils::make_str(armo_form), a_event.equipped ? "Equipped" : "Unequipped");
			
				this->Dispatch({ actor, armo_form, a_event.equipped });
			}
				
			return EventResult::kContinue;
		}

		void Register()
		{
			Event::GetEventSource()->RegisterSink(this);
		}
	};

	class ActorLoadedEventDispatcher : 
		public RE::BSTEventSink<RE::TESObjectLoadedEvent>,
		public EventDispatcher<ActorLoadedEvent>
	{
	public:
		using EventResult = RE::BSEventNotifyControl;
		using Event = RE::TESObjectLoadedEvent;

		static ActorLoadedEventDispatcher* GetSingleton()
		{
			static ActorLoadedEventDispatcher singleton;
			return &singleton;
		}

		EventResult ProcessEvent(const Event& a_event, RE::BSTEventSource<Event>* a_eventSource) override
		{
			//logger::c_info("Object {} formID {:X}", a_event.loaded ? "Loaded" : "Unloaded", a_event.formID);

			auto object_form = RE::TESObjectREFR::LookupByID(a_event.formID);

			if (auto actor_form = object_form->As<RE::Actor>(); actor_form != nullptr) {
				logger::c_info("Actor {} {}", utils::make_str(actor_form), a_event.loaded ? "Loaded" : "Unloaded");

				this->Dispatch({ actor_form, a_event.loaded });
			}

			return EventResult::kContinue;
		}

		void Register()
		{
			Event::GetEventSource()->RegisterSink(this);
		}
	};

	inline void RegisterForAllEvents() {
		ArmorOrApparelEquippedEventDispatcher::GetSingleton()->Register();
		ActorLoadedEventDispatcher::GetSingleton()->Register();
	}
}