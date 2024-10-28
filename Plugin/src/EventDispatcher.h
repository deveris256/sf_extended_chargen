#pragma once
#include "LogWrapper.h"
#include "Utils.h"

namespace events
{
	class EventBase
	{
	public:
		virtual ~EventBase() = default;
	};

	class ArmorOrApparelEquippedEvent : public EventBase
	{
	public:
		ArmorOrApparelEquippedEvent(RE::Actor* a_actor, RE::TESObjectARMO* a_armorOrApparel, bool a_equipped) :
			actor(a_actor),
			armorOrApparel(a_armorOrApparel),
			equipped(a_equipped)
		{}

		RE::Actor*         actor;
		RE::TESObjectARMO* armorOrApparel;
		bool               equipped;
	};

	class ActorLoadedEvent : public EventBase
	{
	public:
		ActorLoadedEvent(RE::Actor* a_actor, bool a_loaded) :
			actor(a_actor),
			loaded(a_loaded)
		{}

		RE::Actor* actor;
		bool       loaded;
	};

	template <class _Event_T>
		requires std::derived_from<_Event_T, EventBase>
	class EventDispatcher
	{
	public:
		class Listener
		{
		public:
			virtual ~Listener() = default;
			virtual void OnEvent(const _Event_T& a_event, EventDispatcher<_Event_T>* a_dispatcher) = 0;
		};

		virtual ~EventDispatcher() = default;

		std::vector<std::weak_ptr<Listener>> listeners;
		std::mutex                           mtx;

		void AddListener(std::shared_ptr<Listener> a_listener)
		{
			std::lock_guard lock(mtx);
			listeners.emplace_back(a_listener);
		}

		void RemoveListener(Listener* a_listener)
		{
			std::lock_guard lock(mtx);
			listeners.erase(
				std::remove_if(
					listeners.begin(),
					listeners.end(),
					[a_listener](const std::weak_ptr<Listener>& weak_listener) {
						if (auto listener = weak_listener.lock()) {
							return listener.get() == a_listener;
						}
						return false;
					}),
				listeners.end());
		}

		void Dispatch(_Event_T a_event)
		{
			std::vector<std::shared_ptr<Listener>> active_listeners;

			{
				std::lock_guard lock(mtx);
				for (auto& weak_listener : listeners) {
					if (auto listener = weak_listener.lock()) {
						active_listeners.push_back(listener);
					}
				}
			}

			for (auto& listener : active_listeners) {
				listener->OnEvent(a_event, this);
			}
		}
	};
}