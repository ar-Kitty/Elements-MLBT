#include <Windows.h>
#include <VersionHelpers.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sstream>
#include <iostream>
#include "Core.h"
#include "Managers/Game.h"
#include "Managers/Mono.h"
#include "Managers/Il2Cpp.h"
#include "Managers/Hook.h"
#include "Utils/CommandLine.h"
#include "Utils/Console.h"
#include "Utils/Assertion.h"
#include "Utils/Logging/Logger.h"
#include "Utils/Debug.h"
#include "Utils/AnalyticsBlocker.h"
#include "Utils/HashCode.h"
#include "Utils/Encoding.h"
#include <random>


HINSTANCE Core::Bootstrap = NULL;
char* Core::BasePath = NULL;
char* Core::BasePathMono = NULL;
char* Core::Path = NULL;
std::string Core::Version = "0.0.1";
bool Core::Is_ALPHA_PreRelease = false;
Core::wine_get_version_t Core::wine_get_version = NULL;


std::string Core::generateRandomString(size_t length) {
	const char charset[] =
		"0123456789"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz";  // Mögliche Zeichen
	std::string result;
	result.resize(length);

	std::random_device rd;  // Zufallszahlengenerator
	std::mt19937 generator(rd());  // Mersenne-Twister-Generator
	std::uniform_int_distribution<> distribution(0, sizeof(charset) - 2);

	for (size_t i = 0; i < length; i++) {
		result[i] = charset[distribution(generator)];
	}

	return result;
}

std::string Core::GetVersionStr()
{
	std::string versionstr = std::string();
	versionstr += generateRandomString(9) + " ";
	versionstr += "Elements Melon";
	versionstr += "Loader v" + Version + " ";
	
	return versionstr;
}

std::string Core::GetVersionStrWithGameName(const char* GameName, const char* GameVersion)
{
	return (GetVersionStr()
		+ " - "
		+ GameName
		+ " "
		+ ((GameVersion == NULL)
			? ""
			: GameVersion));
}

void Core::Initialize(HINSTANCE hinstDLL)
{
	Bootstrap = hinstDLL;
	SetBasePath();
	SetupWineCheck();

	if (!OSVersionCheck() 
		|| !Game::Initialize())
		return;

	CommandLine::Read();
	if (!Console::Initialize()
		|| !Logger::Initialize()
		|| !CheckPathASCII()
		|| !HashCode::Initialize())
		return;

	WelcomeMessage();

	if (!Il2Cpp::Initialize())
		return;

	AnalyticsBlocker::Hook();
	Il2Cpp::Initialize();


	Debug::Msg("Attaching Hook to il2cpp_init...");
	Hook::Attach(&(LPVOID&)Il2Cpp::Exports::il2cpp_init, Il2Cpp::Hooks::il2cpp_init);


	if (Console::CleanUnityLogs)
		Console::NullHandles();
}

bool Core::CheckPathASCII() 
{
	if (std::string(BasePath).find('?') != std::string::npos)
	{
		Assertion::ThrowInternalFailure("The base directory path contains non-ASCII characters,\nwhich are not supported by MelonLoader.\nPlease remove them and try again.");
		return false;
	}
	if (std::string(Game::BasePath).find('?') != std::string::npos)
	{
		Assertion::ThrowInternalFailure("The game directory path contains non-ASCII characters,\nwhich are not supported by MelonLoader.\nPlease remove them and try again.");
		return false;
	}
	return true;
}

void Core::WelcomeMessage()
{
	if (Debug::Enabled)
		Logger::WriteSpacer();
	Logger::QuickLog("------------------------------");
	Logger::QuickLog(GetVersionStr().c_str());
	Logger::QuickLog((std::string("OS: ") + GetOSVersion()).c_str());
	Logger::QuickLog(("Hash Code: " + HashCode::Hash).c_str());
	Logger::QuickLog("------------------------------");
	Logger::QuickLog(("Game Type: " + std::string((Game::IsIl2Cpp ? "Il2Cpp" : (Mono::IsOldMono ? "Mono" : "MonoBleedingEdge")))).c_str());
	Logger::QuickLog(
#ifdef _WIN64
		"Game Arch: x64"
#else
		"Game Arch: x86"
#endif
	);
	Logger::QuickLog("------------------------------");
	if (Debug::Enabled)
		Logger::WriteSpacer();
	Logger::QuickLog(("Core::BasePath = " + std::string(BasePath)).c_str());
	Logger::QuickLog(("Game::BasePath = " + std::string(Game::BasePath)).c_str());
	Logger::QuickLog(("Game::DataPath = " + std::string(Game::DataPath)).c_str());
	Logger::QuickLog(("Game::ApplicationPath = " + std::string(Game::ApplicationPath)).c_str());
}

bool Core::OSVersionCheck()
{
	if (IsRunningInWine() || IsWindows7OrGreater())
		return true;
	int result = MessageBoxA(NULL, "You are running on an Older Operating System.\nWe can not offer support if there are any issues.\nContinue?", "MelonLoader", MB_ICONWARNING | MB_YESNO);
	if (result == IDYES)
		return true;
	return false;
}

void Core::KillCurrentProcess()
{
	HANDLE current_process = GetCurrentProcess();
	TerminateProcess(current_process, NULL);
	CloseHandle(current_process);
}

const char* Core::GetFileInfoProductName(const char* path)
{
	DWORD handle;
	DWORD size = GetFileVersionInfoSizeA(path, &handle);
	if (size == NULL)
		return NULL;
	LPSTR buffer = new char[size];
	if (!GetFileVersionInfoA(path, handle, size, buffer))
		return NULL;
	UINT size2;
	WORD* buffer2;
	if (!VerQueryValueA(buffer, "\\VarFileInfo\\Translation", (LPVOID*)&buffer2, &size2) || (size2 <= 0))
		return NULL;
	std::stringstream productverpath;
	productverpath << "\\StringFileInfo\\" << std::setw(4) << std::setfill('0') << std::hex << std::uppercase << buffer2[0] << std::setw(4) << std::setfill('0') << std::hex << std::uppercase << buffer2[1] << "\\ProductName";
	if (!VerQueryValueA(buffer, productverpath.str().c_str(), (LPVOID*)&buffer2, &size2) || (size2 <= 0))
		return NULL;
	return (LPCSTR)buffer2;
}

const char* Core::GetFileInfoProductVersion(const char* path)
{
	DWORD handle;
	DWORD size = GetFileVersionInfoSizeA(path, &handle);
	if (size == NULL)
		return NULL;
	LPSTR buffer = new char[size];
	if (!GetFileVersionInfoA(path, handle, size, buffer))
		return NULL;
	UINT size2;
	WORD* buffer2;
	if (!VerQueryValueA(buffer, "\\VarFileInfo\\Translation", (LPVOID*)&buffer2, &size2) || (size2 <= 0))
		return NULL;
	std::stringstream productverpath;
	productverpath << "\\StringFileInfo\\" << std::setw(4) << std::setfill('0') << std::hex << std::uppercase << buffer2[0] << std::setw(4) << std::setfill('0') << std::hex << std::uppercase << buffer2[1] << "\\FileVersion";
	if (!VerQueryValueA(buffer, productverpath.str().c_str(), (LPVOID*)&buffer2, &size2) || (size2 <= 0))
		return NULL;
	return (LPCSTR)buffer2;
}

VERSIONHELPERAPI IsWindows11OrGreater()
{
	OSVERSIONINFOEXW osinfo = { sizeof(osinfo), HIBYTE(_WIN32_WINNT_WIN10), LOBYTE(_WIN32_WINNT_WIN10), 22000, 0, { 0 }, 0, 0 };

	DWORDLONG const mask = VerSetConditionMask(
		VerSetConditionMask(
			VerSetConditionMask(
				0, VER_MAJORVERSION, VER_GREATER_EQUAL),
			VER_MINORVERSION, VER_GREATER_EQUAL),
		VER_BUILDNUMBER, VER_GREATER_EQUAL);

	return VerifyVersionInfoW(&osinfo, VER_MAJORVERSION | VER_MINORVERSION | VER_BUILDNUMBER, mask) != FALSE;
}

const char* Core::GetOSVersion()
{
	if (IsRunningInWine())
		return (std::string("Wine ") + wine_get_version()).c_str();
	else if (IsWindows11OrGreater())
		return "Windows 11";
	else if (IsWindows10OrGreater())
		return "Windows 10";
	else if (IsWindows8Point1OrGreater())
		return "Windows 8.1";
	else if (IsWindows8OrGreater())
		return "Windows 8";
	else if (IsWindows7SP1OrGreater())
		return "Windows 7 SP1";
	else if (IsWindows7OrGreater())
		return "Windows 7";
	else if (IsWindowsVistaSP2OrGreater())
		return "Windows Vista SP2";
	else if (IsWindowsVistaSP1OrGreater())
		return "Windows Vista SP1";
	else if (IsWindowsVistaOrGreater())
		return "Windows Vista";
	else if (IsWindowsXPSP3OrGreater())
		return "Windows XP SP3";
	else if (IsWindowsXPSP2OrGreater())
		return "Windows XP SP2";
	else if (IsWindowsXPSP1OrGreater())
		return "Windows XP SP1";
	else if (IsWindowsXPOrGreater())
		return "Windows XP";
	return "UNKNOWN";
}

void Core::SetupWineCheck()
{
	HMODULE ntdll = LoadLibraryA("ntdll.dll");
	if (ntdll == NULL)
		return;
	FARPROC wine_get_version_proc = GetProcAddress(ntdll, "wine_get_version");
	if (wine_get_version_proc == NULL)
		return;
	wine_get_version = (wine_get_version_t)wine_get_version_proc;
}

void Core::SetBasePath()
{
	// Pfad zur Registry
	HKEY hKey;
	const char* subkey = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Steam App 438100";
	LONG lRes = RegOpenKeyExA(HKEY_LOCAL_MACHINE, subkey, 0, KEY_READ, &hKey);
	if (lRes != ERROR_SUCCESS) {
		std::cerr << "Fehler beim Öffnen des Registry-Schlüssels." << std::endl;
		return;
	}

	char vrChatPath[MAX_PATH];
	DWORD pathLen = sizeof(vrChatPath);
	DWORD type = REG_SZ;  // Registrierungseintrag vom Typ "String"

	LONG lResult = RegQueryValueExA(hKey, "InstallLocation", NULL, &type, (LPBYTE)vrChatPath, &pathLen);
	if (lResult != ERROR_SUCCESS) {
		std::cerr << "Fehler beim Abrufen des Installationspfads." << std::endl;
		RegCloseKey(hKey);
		return;
	}

	RegCloseKey(hKey);

	// Sicherstellen, dass genügend Speicher für BasePath reserviert wird
	BasePath = new char[pathLen];
	strcpy_s(BasePath, pathLen, vrChatPath);  // Verwendung von strcpy_s für erhöhte Sicherheit

	// Umwandlung in UTF-8, Annahme Encoding::OsToUtf8 ist verfügbar
	BasePathMono = Encoding::OsToUtf8(BasePath);
}

bool Core::DirectoryExists(const char* path) { struct stat Stat; return ((stat(path, &Stat) == 0) && (Stat.st_mode & S_IFDIR)); }
bool Core::FileExists(const char* path) { WIN32_FIND_DATAA data; return (FindFirstFileA(path, &data) != INVALID_HANDLE_VALUE); }
void Core::GetLocalTime(std::chrono::system_clock::time_point* now, std::chrono::milliseconds* ms, std::tm* bt) { *now = std::chrono::system_clock::now(); *ms = std::chrono::duration_cast<std::chrono::milliseconds>((*now).time_since_epoch()) % 1000; time_t timer = std::chrono::system_clock::to_time_t(*now); localtime_s(bt, &timer); }