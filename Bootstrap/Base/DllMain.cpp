#include "../Core.h"
#include <Windows.h>
#include <cstdint>
#include <chrono>
#include <functional>
#include <thread>
#include "../Addons/lazyimporter/lazyimporter.h"
#include "../kiero.h"
#include <dxgi.h>
#include <d3d11.h>
#include <d3d12sdklayers.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

typedef HRESULT(__stdcall* ResizeBuffers)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);
typedef HRESULT(__stdcall* Present) (IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
typedef LRESULT(CALLBACK* WNDPROC)(HWND, UINT, WPARAM, LPARAM);
typedef uintptr_t PTR;

HWND VRCWindow = nullptr;
HINSTANCE mhinstDLL = nullptr;
LPVOID Data = nullptr;
ResizeBuffers oResizeBuffers;
Present oPresent;
HWND window = nullptr;
WNDPROC oWndProc;
ID3D11Device* pDevice = nullptr;
ID3D11DeviceContext* pContext = nullptr;
ID3D11RenderTargetView* mainRenderTargetView;
static bool init = false;

static uintptr_t __cdecl I_beginthreadex(void* _Security, unsigned _StackSize, _beginthreadex_proc_type _StartAddress, void* _ArgList, unsigned _InitFlag, unsigned* _ThrdAddr) {
	return iat(_beginthreadex).get()(_Security, _StackSize, _StartAddress, _ArgList, _InitFlag, _ThrdAddr);
}


#define IATHookThread(mainThread, dllhandle) I_beginthreadex(0, 0, (_beginthreadex_proc_type)mainThread, dllhandle, 0, 0);

void InitImGui()
{
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags = ImGuiConfigFlags_NavNoCaptureKeyboard; // ignore keyboard capture
    ImGui_ImplWin32_Init(window);
    ImGui_ImplDX11_Init(pDevice, pContext);
}

LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (true && ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
        return true;

    //if (Menu::isGUIVisible) // uncomment this if you want to grab mouse and kb while menu is open
    //{
    //    ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);
    //    return TRUE;
    //}

    return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
}

HRESULT __stdcall hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
    if (!init)
    {
        if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&pDevice)))
        {
            pDevice->GetImmediateContext(&pContext);
            DXGI_SWAP_CHAIN_DESC sd;
            pSwapChain->GetDesc(&sd);
            window = sd.OutputWindow;
            ID3D11Texture2D* pBackBuffer;
            pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
            pDevice->CreateRenderTargetView(pBackBuffer, NULL, &mainRenderTargetView);
            pBackBuffer->Release();
            oWndProc = (WNDPROC)SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)WndProc);

            InitImGui();

            ImGuiIO& io = ImGui::GetIO(); (void)io;

            io.Fonts->Clear();
            init = true;
        }

        else
            return oPresent(pSwapChain, SyncInterval, Flags);
    }

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // Create Menu

    ImGui::Render();
    pContext->OMSetRenderTargets(1, &mainRenderTargetView, NULL);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    return oPresent(pSwapChain, SyncInterval, Flags);
}


HRESULT hkResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags) {
    if (pDevice)
    {
        if (mainRenderTargetView) {
            pContext->OMSetRenderTargets(0, 0, 0);
            mainRenderTargetView->Release();
        }

        HRESULT hr = oResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);
        ID3D11Texture2D* pBuffer;
        pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBuffer);
        pDevice->CreateRenderTargetView(pBuffer, NULL, &mainRenderTargetView);
        pBuffer->Release();
        pContext->OMSetRenderTargets(1, &mainRenderTargetView, NULL);
        D3D11_VIEWPORT vp;
        vp.Width = Width;
        vp.Height = Height;
        vp.MinDepth = 0.0f;
        vp.MaxDepth = 1.0f;
        vp.TopLeftX = 0;
        vp.TopLeftY = 0;
        pContext->RSSetViewports(1, &vp);
        return hr;
    }
    else
        return oResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);
}

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

    bool init_hook = false;
    do
    {
        if (kiero::init(kiero::RenderType::D3D11) == kiero::Status::Success)
        {
            init_hook = true;
            DisableThreadLibraryCalls(mhinstDLL);
            Core::Initialize(mhinstDLL);
        }
    } while (!init_hook);
    kiero::bind(13, (void**)&oResizeBuffers, hkResizeBuffers);
    kiero::bind(8, (void**)&oPresent, hkPresent);
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