#pragma 
#include <cstdint>
#include "../Minhook/MinHook.h"

namespace HookManager {
    template<typename PVOID> void HookFunction(PVOID address, PVOID detour, void** original) {

        MH_CreateHook(reinterpret_cast<LPVOID>(address), reinterpret_cast<LPVOID>(detour), reinterpret_cast<void**>(&original));



        MH_EnableHook(reinterpret_cast<LPVOID*>(address));

    }
   
}