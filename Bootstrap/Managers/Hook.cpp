#include <Windows.h>
#include "Hook.h"
#include "../Base/MSDetours/detours.h"
#include "../Addons/HookManager/HookManager.h"

void Hook::Attach(void** target, void* detour)
{
	DetourTransactionBegin();
	DetourAttach(target, detour);
	DetourTransactionCommit();
}

void Hook::Detach(void** target, void* detour)
{
	DetourTransactionBegin();
	DetourDetach(target, detour);
	DetourTransactionCommit();
}