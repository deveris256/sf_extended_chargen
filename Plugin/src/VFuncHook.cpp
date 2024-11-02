#include "VFuncHook.h"

namespace vfunc
{
	GlobalVFuncHookManager const* g_vfuncHookManager{ GlobalVFuncHookManager::GetInstance() }; // Eager initialization
}