#include "HookManager.h"

bool hooks::addrs::Hook_MAYBE_ActorInitializer_Func(RE::Actor* a_actor, bool a_unk_switch1, bool a_unk_switch2)
{
	events::ActorInitializedEventDispatcher::GetSingleton()->Dispatch({ a_actor, a_unk_switch1, a_unk_switch2 });

	return hooks::HookManager::GetSingleton()->m_original_MAYBE_ActorInitializer_Func(a_actor, a_unk_switch1, a_unk_switch2);
}

void hooks::addrs::Hook_MAYBE_FormPostInitialized_Func(RE::TESForm** a_form)
{

	/*if (!a_form) {
		return;
	}
	auto form = *a_form;
	if (!form) {
		return;
	}
	if (auto actor = form->As<RE::Actor>(); actor) {
		events::ActorInitializedEventDispatcher::GetSingleton()->Dispatch({ actor, false, false });
	}*/

	return hooks::HookManager::GetSingleton()->m_original_MAYBE_FormPostInitialized_Func(a_form);
}

void hooks::addrs::Hook_MAYBE_ActorEquipAllItems_Func(RE::Actor* a_actor, int64_t a_unk_switch1, bool a_unk_switch2)
{
	events::ActorInitializedEventDispatcher::GetSingleton()->Dispatch({ a_actor, false, false });

	hooks::HookManager::GetSingleton()->m_original_MAYBE_ActorEquipAllItems_Func(a_actor, a_unk_switch1, a_unk_switch2);
}
