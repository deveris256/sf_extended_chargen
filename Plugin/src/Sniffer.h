#pragma once
#include "LogWrapper.h"
#include "RE/E/Events.h"

namespace sniffer
{
	class EquipEventHandler : public RE::BSTEventSink<RE::TESEquipEvent>
	{
	public:
		using EventResult = RE::BSEventNotifyControl;
		using Event = RE::TESEquipEvent;

		static EquipEventHandler* GetSingleton()
		{
			static EquipEventHandler singleton;
			return &singleton;
		}

		EventResult ProcessEvent(const Event& a_event, RE::BSTEventSource<Event>* a_eventSource) override
		{
			if (a_event.equipped) {
				logger::c_info("Item actor_who {}({:X}), baseObject {:X}, origRef {:X} equipped", a_event.actor->GetDisplayFullName(), a_event.actor->GetFormID(), a_event.baseObject, a_event.origRef);
			} else {
				logger::c_info("Item actor_who {}({:X}), baseObject {:X}, origRef {:X} unequipped", a_event.actor->GetDisplayFullName(), a_event.actor->GetFormID(), a_event.baseObject, a_event.origRef);
			}
			return EventResult::kContinue;
		}

		void Register()
		{
			Event::GetEventSource()->RegisterSink(this);
		}
	};

	class ActorItemEquippedEventHandler : public RE::BSTEventSink<RE::ActorItemEquipped::Event>
	{
		/// <summary>
		/// Seemingly accepts weapons/armor/apparels.
		/// </summary>
	public:
		using EventResult = RE::BSEventNotifyControl;
		using Event = RE::ActorItemEquipped::Event;

		static ActorItemEquippedEventHandler* GetSingleton()
		{
			static ActorItemEquippedEventHandler singleton;
			return &singleton;
		}

		EventResult ProcessEvent(const Event& a_event, RE::BSTEventSource<Event>* a_eventSource) override
		{
			auto actor = a_event.actor;
			auto item = a_event.item;
			logger::c_info("Item actor_who {}({:X}), item_formID {:X} equipped", actor->GetDisplayFullName(), actor->GetFormID(), item->GetFormID());
			return EventResult::kContinue;
		}

		void Register()
		{
			Event::GetEventSource()->RegisterSink(this);
		}
	};

	void RegisterForAllEvents();
}