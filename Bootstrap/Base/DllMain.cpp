#include "../Core.h"
#include <Windows.h>
#include <cstdint>
#include <chrono>
#include <functional>
#include <thread>
#include "../Addons/lazyimporter/lazyimporter.h"
#include ""

HWND VRCWindow = nullptr;
HINSTANCE mhinstDLL = nullptr;
LPVOID Data = nullptr;


static uintptr_t __cdecl I_beginthreadex(void* _Security, unsigned _StackSize, _beginthreadex_proc_type _StartAddress, void* _ArgList, unsigned _InitFlag, unsigned* _ThrdAddr) {
	return iat(_beginthreadex).get()(_Security, _StackSize, _StartAddress, _ArgList, _InitFlag, _ThrdAddr);
}

extern DWORD WINAPI MainThread_Initialize(LPVOID dwModule);

#define IATHookThread(mainThread, dllhandle) I_beginthreadex(0, 0, (_beginthreadex_proc_type)mainThread, dllhandle, 0, 0);


bool isCurrentProcessVRChat() {
    char path[MAX_PATH];
    if (GetModuleFileNameA(NULL, path, MAX_PATH) == 0) {
        return false;
    }

    std::string pathStr = path;
    // Suche nach "VRChat.exe" im Pfad
    return pathStr.find("VRChat.exe") != std::string::npos;
}

void Checkprocess() {

    do
    {
        VRCWindow = FindWindowW(nullptr, L"VRChat");
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    } while (!VRCWindow);

    DisableThreadLibraryCalls(mhinstDLL);
    Core::Initialize(mhinstDLL);
    MainThread_Initialize(Data);

}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
   
    if (fdwReason == 1) { // Process Attach
        if (isCurrentProcessVRChat()) {
            mhinstDLL = hinstDLL;
            Data = lpvReserved;
            DisableThreadLibraryCalls(hinstDLL);
            IATHookThread(Checkprocess, hinstDLL); //If not VRChat he does Nothing at all :)
        }
       
    }
	return TRUE;
}