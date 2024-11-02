#pragma once
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <Windows.h>
#include <detours/detours.h>

namespace vfunc
{
	inline void* GetVTableFuncPtr(void* instance, size_t v_func_id)
	{
		void** vtable = *(void***)instance;
		return vtable[v_func_id];
	}

	class VirtualFunctionHookBase
	{
	public:
		VirtualFunctionHookBase() :
			m_originalFunction(nullptr),
			m_hookTarget(nullptr),
			m_detourFunction(nullptr)
		{}

		VirtualFunctionHookBase(VirtualFunctionHookBase&& other) noexcept :
			m_originalFunction(other.m_originalFunction),
			m_hookTarget(other.m_hookTarget),
			m_detourFunction(other.m_detourFunction)
		{
			other.m_originalFunction = nullptr;
			other.m_hookTarget = nullptr;
			other.m_detourFunction = nullptr;
		}

		VirtualFunctionHookBase(const VirtualFunctionHookBase&) = delete;

		virtual ~VirtualFunctionHookBase() = default;
		void* m_originalFunction;
		void* m_hookTarget;
		void* m_detourFunction;
	};

	class GlobalVFuncHookManager
	{
	public:
		static GlobalVFuncHookManager* GetInstance()
		{
			static GlobalVFuncHookManager instance;
			return &instance;
		}

		std::unordered_map<void*, VirtualFunctionHookBase*> m_vfunc_hooks;

		[[nodiscard]] bool HasHook(void* hookTarget) const
		{
			if (m_vfunc_hooks.find(hookTarget) == m_vfunc_hooks.end()) {
				return false;
			}
			return true;
		}

		[[nodiscard]] inline size_t NumHooks() const
		{
			return m_vfunc_hooks.size();
		}

		[[nodiscard]] VirtualFunctionHookBase* GetInstanceVFuncHook(void* instance, size_t v_func_id) const
		{
			void** vtable = *(void***)instance;
			void*  m_hookTarget = vtable[v_func_id];
			if (HasHook(m_hookTarget)) {
				return m_vfunc_hooks.at(m_hookTarget);
			}
			return nullptr;
		}

		bool RegisterHook(void* hookTarget, VirtualFunctionHookBase* hook)
		{
			if (HasHook(hookTarget)) {
				return false;
			}
			m_vfunc_hooks[hookTarget] = hook;
#ifdef _REL_WITH_DEB_INFO
			m_offsets[hookTarget] = GetOffset(hookTarget);
#endif
			return true;
		}

		[[nodiscard]] void* GetOriginalFunction(void* hookTarget) const
		{
			if (!HasHook(hookTarget)) {
				return nullptr;
			}
			return m_vfunc_hooks.at(hookTarget)->m_originalFunction;
		}

		template <size_t v_func_id, typename _Instance_T, typename _Rtn_T, typename... _Args>
		_Rtn_T CallOriginal(_Instance_T* instance, _Args... args)
		{
			void** vtable = *(void***)instance;
			void*  m_hookTarget = vtable[v_func_id];
			if (void* originalFunction = GetOriginalFunction(m_hookTarget)) {
				return ((_Rtn_T(*)(_Instance_T*, _Args...))originalFunction)(instance, args...);
			}
			std::runtime_error("Cannot call the original function, it is not hooked");
		}

		bool ReleaseHook(void* hookTarget)
		{
			if (!HasHook(hookTarget)) {
				return false;
			}
			m_vfunc_hooks.erase(hookTarget);
			return true;
		}

#ifdef _REL_WITH_DEB_INFO

		uintptr_t GetOffset(void* target)
		{
			return (uintptr_t)target - (uintptr_t)image_base;
		}

	private:
		GlobalVFuncHookManager() {
			image_base = (void*)REL::Module::get().base();
		}

		void* image_base = nullptr;
		std::unordered_map<void*, uintptr_t> m_offsets;
#endif
	};

	template <typename _Instance_T, typename _Rtn_T, typename... _Args>
	class VirtualFunctionHook : public VirtualFunctionHookBase
	{
	public:
		using _VFunc_T = _Rtn_T (*)(_Instance_T*, _Args...);

		VirtualFunctionHook(_Instance_T* instance, size_t v_func_id, _VFunc_T hook) :
			VirtualFunctionHookBase()
		{
			Initialize(instance, v_func_id, hook);
		}

		virtual ~VirtualFunctionHook()
		{
			Release();
		}

		void Release()
		{
			if (!IsValid()) {
				return;
			}
			auto manager = GlobalVFuncHookManager::GetInstance();

			if (!manager->ReleaseHook(m_hookTarget)) {
				std::cout << "Cannot release the hook\n";
				return;
			}

			// Detach the detour
			DetourTransactionBegin();
			DetourUpdateThread(GetCurrentThread());

			DetourDetach(&(PVOID&)m_originalFunction, m_detourFunction);

			DetourTransactionCommit();

			is_valid = false;
		}

		bool IsValid()
		{
			return is_valid;
		}

		_Rtn_T CallOriginal(_Instance_T* instance, _Args... args)
		{
			return ((_VFunc_T)m_originalFunction)(instance, args...);
		}

		_Rtn_T CallDetour(_Instance_T* instance, _Args... args)
		{
			return m_detourFunction(instance, args...);
		}

		_Rtn_T CallHookTarget(_Instance_T* instance, _Args... args)
		{
			return ((_VFunc_T)m_hookTarget)(instance, args...);
		}

		void Pause()
		{
			Release();
		}

		void Restore()
		{
			if (IsValid()) {
				return;
			}

			if (m_originalFunction != m_hookTarget) {
				std::runtime_error("Cannot restore the hook, the original function is not the hook target.");
			}

			auto manager = GlobalVFuncHookManager::GetInstance();

			if (!manager->RegisterHook(m_hookTarget, this)) {
				std::cout << "Cannot restore the hook\n";
				return;
			}

			// Detach the detour
			DetourTransactionBegin();
			DetourUpdateThread(GetCurrentThread());

			DetourAttach(&(PVOID&)m_originalFunction, m_detourFunction);

			DetourTransactionCommit();

			is_valid = true;
		}

	protected:
		VirtualFunctionHook() = default;

		bool is_valid = false;

		void Initialize(_Instance_T* instance, size_t v_func_id, _VFunc_T hook)
		{
			// Get the vtable of the instance
			void** vtable = *(void***)instance;

			m_originalFunction = vtable[v_func_id];
			m_hookTarget = vtable[v_func_id];
			m_detourFunction = hook;

			auto manager = GlobalVFuncHookManager::GetInstance();

			if (m_originalFunction && m_detourFunction && instance && manager->HasHook(m_hookTarget)) {
				std::cout << "Cannot hook the function, it is already hooked\n";
				m_originalFunction = nullptr;
				m_hookTarget = nullptr;
				m_detourFunction = nullptr;
				is_valid = false;
				return;
			}

			// Detour the function
			DetourTransactionBegin();
			DetourUpdateThread(GetCurrentThread());

			DetourAttach(&(PVOID&)m_originalFunction, m_detourFunction);

			DetourTransactionCommit();

			manager->RegisterHook(m_hookTarget, this);

			is_valid = true;
		}
	};


	extern GlobalVFuncHookManager const* g_vfuncHookManager; // For eager initialization
}