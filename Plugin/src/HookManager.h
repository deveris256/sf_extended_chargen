#pragma once
#include "SFEventHandler.h"
#include <detours/detours.h>
#include <type_traits>


namespace hooks
{
	namespace addrs
	{
		inline constexpr REL::ID MAYBE_ActorInitializer_Func{ 150902 };
		bool                     Hook_MAYBE_ActorInitializer_Func(RE::Actor* a_actor, bool a_unk_switch1, bool a_unk_switch2);
		inline constexpr REL::ID MAYBE_FormPostInitialized_Func{ 34242 };
		void                     Hook_MAYBE_FormPostInitialized_Func(RE::TESForm** a_form);
		inline constexpr REL::ID MAYBE_ActorEquipAllItems_Func{ 150387 };
		void                     Hook_MAYBE_ActorEquipAllItems_Func(RE::Actor* a_actor, int64_t a_unk_switch1, bool a_unk_switch2);
	}

	class HookManager
	{
	public:
		enum class HookType
		{
			kCall,
			kJump
		};

		static HookManager* GetSingleton()
		{
			static HookManager singleton;
			return &singleton;
		}

		HookManager* Initialize(size_t a_trampolineSize = 0x1000)
		{
			//void* mem_start = SFSE::GetTrampolineInterface()->AllocateFromLocalPool(a_trampolineSize);

			//auto deleter = [](void* a_mem, std::size_t) { REX::W32::VirtualFree(a_mem, 0, MEM_RELEASE); };

			//m_trampoline.set_trampoline(mem_start, a_trampolineSize, deleter);

			//m_trampoline.create(a_trampolineSize);

			return this;
		};

		/*HookManager* InstallHook(HookType a_hookType, REL::ID a_addrLibId, void* a_dstFunction, size_t N = 5)
		{
			switch (a_hookType) {
			case HookType::kCall:
				if (N == 5)
					m_trampoline.write_call<5>(a_addrLibId.address(), a_dstFunction);
				else if (N == 6)
					m_trampoline.write_call<6>(a_addrLibId.address(), a_dstFunction);
				break;
			case HookType::kJump:
				if (N == 5)
					m_trampoline.write_branch<5>(a_addrLibId.address(), a_dstFunction);
				else if (N == 6)
					m_trampoline.write_branch<6>(a_addrLibId.address(), a_dstFunction);
				break;
			}

			return this;
		};*/

		HookManager* InstallAllHooks()
		{
			//InstallHook(HookType::kCall, addrs::MAYBE_ActorInitializer_Func, addrs::Hook_MAYBE_ActorInitializer_Func);

			//InstallHook(HookType::kCall, addrs::MAYBE_FormPostInitialized_Func, addrs::Hook_MAYBE_FormPostInitialized_Func);

			DetourTransactionBegin();
			DetourUpdateThread(GetCurrentThread());

			//m_original_MAYBE_ActorInitializer_Func = (decltype(addrs::Hook_MAYBE_ActorInitializer_Func)*)addrs::MAYBE_ActorInitializer_Func.address();
			//DetourAttach(&(PVOID&)m_original_MAYBE_ActorInitializer_Func, addrs::Hook_MAYBE_ActorInitializer_Func);
			
			//m_original_MAYBE_FormPostInitialized_Func = (decltype(addrs::Hook_MAYBE_FormPostInitialized_Func)*)addrs::MAYBE_FormPostInitialized_Func.address();
			//DetourAttach(&(PVOID&)m_original_MAYBE_FormPostInitialized_Func, addrs::Hook_MAYBE_FormPostInitialized_Func);

			m_original_MAYBE_ActorEquipAllItems_Func = (decltype(addrs::Hook_MAYBE_ActorEquipAllItems_Func)*)addrs::MAYBE_ActorEquipAllItems_Func.address();
			DetourAttach(&(PVOID&)m_original_MAYBE_ActorEquipAllItems_Func, addrs::Hook_MAYBE_ActorEquipAllItems_Func);

			DetourTransactionCommit();

			return this;
		};

		decltype(addrs::Hook_MAYBE_ActorInitializer_Func)*    m_original_MAYBE_ActorInitializer_Func;
		decltype(addrs::Hook_MAYBE_FormPostInitialized_Func)* m_original_MAYBE_FormPostInitialized_Func;
		decltype(addrs::Hook_MAYBE_ActorEquipAllItems_Func)*  m_original_MAYBE_ActorEquipAllItems_Func;

	private:
		//SFSE::Trampoline m_trampoline;
		HookManager() = default;
	};
}