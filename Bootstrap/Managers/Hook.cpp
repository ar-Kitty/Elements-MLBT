#include <Windows.h>
#include "Hook.h"


void Hook::Attach(void** target, void* detour)
{

    if (MH_Initialize() != MH_OK) {
        Debug::Msg("Failed to initialize MinHook.");
        return;
    }
    if (MH_CreateHook(target, detour, reinterpret_cast<void**>(target)) != MH_OK) {
        Debug::Msg("Failed to create hook.");
        return;
    }
    if (MH_EnableHook(target) != MH_OK) {
        Debug::Msg("Failed to enable hook.");
    }
}

void Hook::Detach(void** target, void* detour)
{
    if (MH_DisableHook(target) != MH_OK) {
        Debug::Msg("Failed to disable hook.");
    }
    if (MH_RemoveHook(target) != MH_OK) {
        Debug::Msg("Failed to remove hook.");
    }
    MH_Uninitialize();
}
