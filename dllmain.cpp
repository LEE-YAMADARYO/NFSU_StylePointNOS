// StylePointNOS - converts style point changes into NOS for NFS Underground v1.4.0
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License or any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include <windows.h>
#include <MinHook.h>
#include <cstdlib>

// Game addresses relative to the default image base.
static constexpr DWORD kDefaultImageBase      = 0x400000;
static constexpr DWORD kOffStyleMomentsUpdate = 0x436940; // sub_436940
static constexpr DWORD kOffPlayersByNumber    = 0x7361B4; // Player::pPlayersByNumber
static constexpr DWORD kOffWorldTimer         = 0x73AD34; // WorldTimer

// Field offsets.
static constexpr int kOffPlayerCar      = 0x004; // Player -> Car*
static constexpr int kOffPlayerScore    = 0x7C;  // records -> style score total (Player+0x9DC)
static constexpr int kCarNosCapacity    = 0x3F0; // Car -> NOS capacity (ticks)
static constexpr int kCarNosRemaining   = 0x3F8; // Car -> NOS remaining / expiry (ticks)
static constexpr int kCarNosBoostAnchor = 0x3FC; // Car -> boost anchor (ticks), 0 = idle

typedef int* (__stdcall* StyleMomentsUpdate_t)(int records, int player, float dt);
static StyleMomentsUpdate_t RealStyleMomentsUpdate = nullptr;

static void** g_playersByNumber = nullptr;
static DWORD* g_worldTimer      = nullptr;
static float  g_nosPerPoint     = 10.0f;

// Previous frame score per local player (split screen supports up to 2).
struct PlayerScore {
    void* player;
    float total;
    bool  valid;
};
static PlayerScore g_scores[2];

// Reads NOSPerPoint from the .ini next to the plugin.
static void LoadConfig()
{
    HMODULE self = nullptr;
    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                           GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       reinterpret_cast<LPCSTR>(&LoadConfig), &self);
    char path[MAX_PATH];
    GetModuleFileNameA(self, path, MAX_PATH);
    if (char* slash = strrchr(path, '\\'))
        slash[1] = '\0';
    strcat_s(path, "StylePointNOS.ini");

    char buf[64];
    GetPrivateProfileStringA("StylePointNOS", "NOSPerPoint", "10.0", buf, sizeof(buf), path);
    g_nosPerPoint = static_cast<float>(atof(buf));
    if (g_nosPerPoint < 0.0f)
        g_nosPerPoint = 10.0f;
}

// Applies a score delta to the car's NOS, clamped to [0, capacity].
static void AdjustNOS(void* player, float points)
{
    DWORD car = *reinterpret_cast<DWORD*>(static_cast<char*>(player) + kOffPlayerCar);
    if (!car)
        return;

    int capacity = *reinterpret_cast<int*>(car + kCarNosCapacity);
    if (capacity == 0 || capacity == 0x7FFFFFFF)
        return; // no NOS installed

    int ticks = static_cast<int>(points * g_nosPerPoint + (points > 0.0f ? 0.5f : -0.5f));
    if (ticks == 0)
        return;

    __try {
        int anchor = *reinterpret_cast<int*>(car + kCarNosBoostAnchor);
        if (anchor != 0 && anchor != 0x7FFFFFFF) {
            // boosting: Car+0x3F8 stores the expiry time (see sub_449F80)
            int now = *g_worldTimer;
            int remaining = *reinterpret_cast<int*>(car + kCarNosRemaining) - now + anchor;
            remaining += ticks;
            if (remaining > capacity) remaining = capacity;
            if (remaining < 0) remaining = 0;
            *reinterpret_cast<int*>(car + kCarNosRemaining) = remaining + now - anchor;
        } else {
            // idle: Car+0x3F8 is the remaining tick count
            int remaining = *reinterpret_cast<int*>(car + kCarNosRemaining);
            if (remaining == 0x7FFFFFFF) remaining = 0;
            remaining += ticks;
            if (remaining > capacity) remaining = capacity;
            if (remaining < 0) remaining = 0;
            *reinterpret_cast<int*>(car + kCarNosRemaining) = remaining;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
    }
}

// Diffs the style score total of one player and applies the delta as NOS.
static void ProcessScore(int* records, void* player)
{
    int index = -1;
    for (int i = 0; i < 2; ++i) {
        if (g_playersByNumber[i] == player) {
            index = i;
            break;
        }
    }
    if (index < 0)
        return; // AI player or unrecognized table

    float total;
    __try {
        total = *reinterpret_cast<float*>(reinterpret_cast<char*>(records) + kOffPlayerScore);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return;
    }

    PlayerScore& ps = g_scores[index];
    if (!ps.valid || ps.player != player) {
        ps.player = player;
        ps.total = total;
        ps.valid = true;
        return;
    }

    float delta = total - ps.total;
    ps.total = total;
    if (delta != 0.0f)
        AdjustNOS(player, delta);
}

// Per-frame hook: lets the game update the score first, then applies the delta.
static int* __stdcall StyleMomentsUpdateHook(int records, int player, float dt)
{
    int* result = RealStyleMomentsUpdate(records, player, dt);
    ProcessScore(reinterpret_cast<int*>(records), reinterpret_cast<void*>(player));
    return result;
}

// Installs the style-moments hook and resolves game globals.
static void InstallHook()
{
    if (MH_Initialize() != MH_OK)
        return;

    DWORD base = reinterpret_cast<DWORD>(GetModuleHandleA(nullptr)) - kDefaultImageBase;
    void* target = reinterpret_cast<void*>(base + kOffStyleMomentsUpdate);
    g_playersByNumber = reinterpret_cast<void**>(base + kOffPlayersByNumber);
    g_worldTimer = reinterpret_cast<DWORD*>(base + kOffWorldTimer);

    if (MH_CreateHook(target, reinterpret_cast<LPVOID>(&StyleMomentsUpdateHook),
                      reinterpret_cast<LPVOID*>(&RealStyleMomentsUpdate)) != MH_OK)
        return;
    MH_EnableHook(target);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        LoadConfig();
        InstallHook();
    } else if (reason == DLL_PROCESS_DETACH) {
        MH_Uninitialize();
    }
    return TRUE;
}
