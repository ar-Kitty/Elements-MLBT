#include <Windows.h>
#include "Hook.h"


void Hook::Attach(void** target, void* detour)
{
    HOOK_TRACE_INFO hHook = { NULL }; 
    NTSTATUS result = LhInstallHook(
        target,                
        detour,                
        nullptr,               
        &hHook               
    );

    if (FAILED(result)) {
        Debug::Msg("Failed to install hook.");
        return;
    }

    result = LhSetExclusiveACL(new ULONG[1]{ 0 }, 1, &hHook);
    if (FAILED(result)) {
        Debug::Msg("Failed to enable hook.");
    }
}

void Hook::Detach(void** target)
{
    HOOK_TRACE_INFO hHook = { NULL }; 
    NTSTATUS result = LhUninstallHook(&hHook);
    if (FAILED(result)) {
        Debug::Msg("Failed to uninstall hook.");
    }
    LhWaitForPendingRemovals();
}
