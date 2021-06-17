#include <Windows.h>

#include <vector>

// Types

typedef void(__thiscall* onplaytest_t)(void*, void*);
typedef void(__thiscall* cctouches_t)(void*, void*, void*);

// Globals

static bool g_hasPushStacked = 0;
static onplaytest_t g_onPlaytest = nullptr;
static cctouches_t g_onTouchBegan = nullptr;
static cctouches_t g_onTouchEnded = nullptr;

// Helpers

static bool writeMemory(
	std::uintptr_t const address,
	std::vector<std::uint8_t> const& bytes)
{
	DWORD p;
	VirtualProtect(reinterpret_cast<LPVOID>(address), bytes.size(), PAGE_EXECUTE_READWRITE, &p);

	return ::WriteProcessMemory(
		::GetCurrentProcess(),
		reinterpret_cast<LPVOID>(address),
		bytes.data(),
		bytes.size(),
		NULL) == TRUE;
}

static bool writePtr(
	std::uintptr_t const address,
	std::uintptr_t const ptr)
{
	auto const p = reinterpret_cast<std::uint8_t const*>(&ptr);

	return ::writeMemory(
		address,
		std::vector<std::uint8_t>(p, p + sizeof(std::uintptr_t)));
}

// Callback

static void __fastcall onplaytestHook(void* self, void* edx, void* param)
{
	if (!g_hasPushStacked)
		g_onPlaytest(self, param);
}

static void __fastcall ontouchbeganHook(void* self, void* edx, void* param1, void* param2)
{
	g_hasPushStacked = true;
	g_onTouchBegan(self, param1, param2);
}

static void __fastcall ontouchendedHook(void* self, void* edx, void* param1, void* param2)
{
	g_hasPushStacked = false;
	g_onTouchEnded(self, param1, param2);
}

// Main

DWORD WINAPI MainThread(LPVOID)
{
	auto base = reinterpret_cast<std::uintptr_t>(GetModuleHandleA(NULL));

	g_onPlaytest = reinterpret_cast<onplaytest_t>(base + 0x87600);
	g_onTouchBegan = reinterpret_cast<cctouches_t>(base + 0x907B0);
	g_onTouchEnded = reinterpret_cast<cctouches_t>(base + 0x911A0);

	auto const onPlaytestAddr1 = base + 0x76E53;
	auto const onPlaytestAddr2 = base + 0x92109;

	writePtr(onPlaytestAddr1 + 1, reinterpret_cast<std::uintptr_t>(&onplaytestHook));
	writePtr(onPlaytestAddr2 + 1, reinterpret_cast<std::uintptr_t>(&onplaytestHook) - onPlaytestAddr2 - 5);
	writePtr(base + 0x2997D0, reinterpret_cast<std::uintptr_t>(&ontouchbeganHook));
	writePtr(base + 0x2997D8, reinterpret_cast<std::uintptr_t>(&ontouchendedHook));
}

// Entrypoint

BOOL WINAPI DllMain(HINSTANCE dll, DWORD const reason, LPVOID) 
{
	DisableThreadLibraryCalls(dll);

	if (reason == DLL_PROCESS_ATTACH)
		CreateThread(0, 0, &MainThread, 0, 0, 0);

	return TRUE;
}