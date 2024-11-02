#pragma once
#include <functional>

#include "EventDispatcher.h"
#include "VFuncHook.h"

namespace vfunc
{
	template <typename _Instance_T, typename... _Args>
	class VirtualFunctionCalledEvent : public events::EventBase
	{
	public:
		VirtualFunctionCalledEvent(_Instance_T* instance, size_t v_func_id, _Args... args) :
			instance(instance),
			v_func_id(v_func_id),
			args(args...)
		{}
		virtual ~VirtualFunctionCalledEvent() = default;

		_Instance_T*         instance;
		size_t               v_func_id;
		std::tuple<_Args...> args;

		template <size_t arg_index, typename _Arg_T = std::tuple_element_t<arg_index, std::tuple<_Args...>>>
		_Arg_T GetArg() const
		{
			return std::get<arg_index>(args);
		}
	};

	template <size_t v_func_id, typename _Instance_T, typename _Rtn_T, typename... _Args>
	class VirtualFunctionCalledEventDispatcher :
		public events::EventDispatcher<VirtualFunctionCalledEvent<_Instance_T, _Args...>>
	{
	public:
		using _VFunc_T = _Rtn_T (*)(_Instance_T*, _Args...);
		using _VFunc_STD_FUNCTION_T = std::function<_Rtn_T(_Instance_T*, _Args...)>;
		using _VHook_T = VirtualFunctionHook<_Instance_T, _Rtn_T, _Args...>;

	protected:
		class _Ensure_Unique_Hook
		{
			/*
			* This class is used to ensure that only one hook is active at a time. RAII style.
			*/
		public:
			_Ensure_Unique_Hook(_Instance_T* instance)
			{
				// Temporarily restore vtables other than the target instance's vtable
				void** target_vtable(*(void***)instance);
				auto   _dispatcher = VirtualFunctionCalledEventDispatcher::GetInstance();

				for (auto hook : _dispatcher->m_hooks) {
					auto hook_instance_vtable = *(void***)hook.first;

					if (hook_instance_vtable != target_vtable && m_protected_vtables.find(hook_instance_vtable) == m_protected_vtables.end()) {
						auto hook_instance_hook = hook.second;

						hook_instance_hook->Pause();

						m_protected_vtables[hook_instance_vtable] = hook_instance_hook;
					}
				}
			}

			~_Ensure_Unique_Hook()
			{
				for (auto [vtable, hook] : m_protected_vtables) {
					if (auto hook_shared = hook.lock()) {
						hook_shared->Restore();
					}
				}
			}

			std::unordered_map<void**, std::weak_ptr<_VHook_T>> m_protected_vtables;
		};

	public:
		virtual ~VirtualFunctionCalledEventDispatcher()
		{
			m_vfunc_retrieval.clear();
			m_hooks.clear();
		}

		static VirtualFunctionCalledEventDispatcher* GetInstance()
		{
			static VirtualFunctionCalledEventDispatcher instance;
			return &instance;
		}

		void WatchInstance(_Instance_T* instance)
		{
			if (IsWatching(instance)) {
				return;
			}

			void* v_func_ptr = GetVTableFuncPtr((void*)instance, v_func_id);

			if (Retrieve(instance, v_func_ptr)) {
				return;
			}

			_VFunc_T lambda = [](_Instance_T* instance, _Args... args) -> _Rtn_T {
				auto _dispatcher = VirtualFunctionCalledEventDispatcher::GetInstance();

				std::lock_guard lock(_dispatcher->mutex_hook_busy);
				_dispatcher->_hook_busy = true;

				_Ensure_Unique_Hook hook_lock(instance);

				if (_dispatcher->IsWatching(instance)) {
					auto event = VirtualFunctionCalledEvent<_Instance_T, _Args...>(instance, v_func_id, args...);
					_dispatcher->Dispatch(event);
				}

				if constexpr (std::is_void_v<_Rtn_T>) {
					GlobalVFuncHookManager::GetInstance()->CallOriginal<v_func_id, _Instance_T, _Rtn_T, _Args...>(instance, args...);
					_dispatcher->_hook_busy = false;
					return;
				} else {
					_Rtn_T rtn = GlobalVFuncHookManager::GetInstance()->CallOriginal<v_func_id, _Instance_T, _Rtn_T, _Args...>(instance, args...);
					_dispatcher->_hook_busy = false;
					return rtn;
				}
			};

			std::shared_ptr<_VHook_T> hook(new _VHook_T(instance, v_func_id, lambda));

			if (hook.get()->IsValid()) {
				// Add the hook to the list
				m_hooks[instance] = hook;
				m_vfunc_retrieval[v_func_ptr] = hook;
			}
		}

		bool UnwatchInstance(_Instance_T* instance)
		{
			if (!IsWatching(instance)) {
				return false;
			}

			m_hooks.erase(instance);
			return true;
		}

		inline bool IsWatching(_Instance_T* instance)
		{
			return m_hooks.find(instance) != m_hooks.end();
		}

		inline size_t NumWatching()
		{
			return m_hooks.size();
		}

		inline bool IsHookBusy()
		{
			return _hook_busy;
		}

	protected:
		VirtualFunctionCalledEventDispatcher() = default;

		[[nodiscard]] bool Retrieve(_Instance_T* instance, void* v_func_ptr)
		{
			if (m_vfunc_retrieval.find(v_func_ptr) != m_vfunc_retrieval.end()) {
				if (auto shared_p = m_vfunc_retrieval[v_func_ptr].lock()) {
					m_hooks[instance] = shared_p;
					return true;
				} else {
					m_vfunc_retrieval.erase(v_func_ptr);
					return false;
				}
			}
			return false;
		}

		std::unordered_map<void*, std::weak_ptr<_VHook_T>>          m_vfunc_retrieval;
		std::unordered_map<_Instance_T*, std::shared_ptr<_VHook_T>> m_hooks;

		bool       _hook_busy{ false };
		std::mutex mutex_hook_busy;
	};

	template <size_t v_func_id, typename _Instance_T, typename _Rtn_T, typename... _Args>
	class VirtualFunctionCalledEventListenerBase :
		public VirtualFunctionCalledEventDispatcher<v_func_id, _Instance_T, _Rtn_T, _Args...>::Listener
	{
	public:
		using vfunc_type = _Rtn_T (*)(_Instance_T*, _Args...);
		using vf_dispatcher_type = VirtualFunctionCalledEventDispatcher<v_func_id, _Instance_T, _Rtn_T, _Args...>;
	};
}