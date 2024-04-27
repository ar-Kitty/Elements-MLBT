
#include "helper.h"
#include "Console.hpp"
#include "Game.hpp"
#include "D3D11Window.hpp"
#include "Hooking.hpp"
#include <mutex>
#include <atomic>
#include <chrono>
#include <memory>


using namespace DX11_Base;

void ClientBGThread()
{
    while (g_Running)
    {
        g_Menu->Loops();

        std::this_thread::sleep_for(1ms);
        std::this_thread::yield();
    }
}

DWORD WINAPI MainThread_Initialize(LPVOID dwModule)
{
    //  CREATE CLASS INSTANCES
    g_GameData = std::make_unique<GameData>();                  //  
    g_D3D11Window = std::make_unique<D3D11Window>();            //  
    g_Hooking = std::make_unique<Hooking2>();                    //  
    g_Menu = std::make_unique<Menu>();                          //  

    //  CREATE WINDOW AND ESTABLISH HOOKS
    g_GameData->Init();                                         //
    g_D3D11Window->HookD3D();                                   //
    g_Hooking->Hook2();                                          //

    //	INITIALIZE BACKGROUND THREAD
    std::thread WCMUpdate(ClientBGThread);	                    //  

    //  RENDER LOOP
    g_Running = true;
    static int LastTick = 0;
    while (g_Running)
    {
        if ((GetAsyncKeyState(VK_INSERT) & 1) && ((GetTickCount64() - LastTick) > 500))
        {
            g_Menu->b_ShowMenu = !g_Menu->b_ShowMenu;           //  Main Menu Window
            g_Menu->b_ShowHud = !g_Menu->b_ShowMenu;            //  Render Window
            LastTick = GetTickCount64();
        }

        if (g_KillSwitch)
        {
            g_KillSwitch = false;
            g_Hooking->Unhook2();
        }
    }

    //  EXIT
    WCMUpdate.join();						                    //	Exit Loops Thread
    FreeLibraryAndExitThread(g_hModule, EXIT_SUCCESS);          //  
    return EXIT_SUCCESS;
}