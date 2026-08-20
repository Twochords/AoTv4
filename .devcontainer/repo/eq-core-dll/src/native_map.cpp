// Standalone MQ2 map overlay extracted from the mq-interface native DLL.
// This file intentionally keeps only map, spawn, command, INI, and dinput8 proxy behavior.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define DIRECTINPUT_VERSION 0x0800

#include <windows.h>
#include <dinput.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unordered_set>
#include <vector>

#if !defined(_M_IX86)
#error This DLL is intended for the 32-bit EQ client and must be built as Win32.
#endif

namespace nativeinterface {

// AoTv4: this dll builds as C++14 (stdcpp14), where clamp (C++17) is unavailable. Local replacement
// (this is a self-contained translation unit, so it affects nothing else). Used for map sizes/colors/etc.
template <typename T>
static inline T clamp(T v, T lo, T hi) { return v < lo ? lo : (v > hi ? hi : v); }

constexpr uintptr_t kEqImageBase = 0x400000;
constexpr uintptr_t kPInstSpawnManager = 0xE641D0;
constexpr uintptr_t kPInstCMapViewWnd = 0xD1FC54;
constexpr uintptr_t kPInstCharData = 0xDD261C;
constexpr uintptr_t kPInstCharSpawn = 0xDD2644;
constexpr uintptr_t kPInstTarget = 0xDD2648;
constexpr uintptr_t kPInstCEverQuest = 0xE67CCC;

constexpr uintptr_t kPInstCTargetWnd = 0xD1FC60;
constexpr uintptr_t kPInstCPlayerWnd = 0xD1FC68;
constexpr uintptr_t kPInstEQItemListCandidates[] = {
    0xDCD9A8,
    0xDCF8E0,
    0xDCFD18,
};

constexpr uintptr_t kCEverQuestDspChat = 0x51F1A0;
constexpr uintptr_t kCEverQuestInterpretCmd = 0x51FCE0;
constexpr uintptr_t kEQCharacterGetConLevel = 0x577CB0;
constexpr size_t kSpawnNext = 0x008;
constexpr size_t kSpawnY = 0x064;
constexpr size_t kSpawnX = 0x068;
constexpr size_t kSpawnZ = 0x06c;
constexpr size_t kSpawnSpeedY = 0x070;
constexpr size_t kSpawnSpeedX = 0x074;
constexpr size_t kSpawnSpeedZ = 0x078;
constexpr size_t kSpawnHeading = 0x080;
constexpr size_t kSpawnName = 0x0a4;
constexpr size_t kSpawnDisplayedName = 0x0e4;
constexpr size_t kSpawnType = 0x125;
constexpr size_t kSpawnID = 0x148;
constexpr size_t kSpawnLevel = 0x250;
constexpr size_t kSpawnGM = 0x25c;

constexpr size_t kSpawnManagerFirstSpawn = 0x008;

constexpr size_t kMapViewVTable = 0x358;
constexpr size_t kMapLines = 0x5a8;
constexpr size_t kMapLabels = 0x5ac;
constexpr size_t kMapVTableBytes = 0x17c;
constexpr size_t kMapPostDraw2Index = 4;
constexpr uint32_t kMapTargetColor = 0xFFFF4040;
constexpr uint32_t kMapXTargetColor = 0xFFFF80FF;
constexpr uint32_t kMapGroundColor = 0xFFC0C0C0;
constexpr uint32_t kMapLocationColor = 0xFF00FF80;
constexpr uint32_t kMapRadiusColor = 0xFF808080;
constexpr float kPi = 3.14159265358979323846f;

constexpr size_t kGroundItemNext = 0x04;
constexpr size_t kGroundItemDropID = 0x0c;
constexpr size_t kGroundItemName = 0x1c;
constexpr size_t kGroundItemY = 0x70;
constexpr size_t kGroundItemX = 0x74;
constexpr size_t kGroundItemZ = 0x78;

constexpr size_t kCharDataXTargetMgr = 0x2460;
constexpr size_t kXTargetMgrSlots = 0x04;
constexpr size_t kXTargetMgrArray = 0x08;
constexpr size_t kXTargetSlotSize = 0x4c;
constexpr size_t kXTargetType = 0x00;
constexpr size_t kXTargetSpawnID = 0x08;
constexpr size_t kXTargetName = 0x0c;
constexpr size_t kMaxXTargets = 20;

enum SpawnType : uint8_t {
    kSpawnPlayer = 0,
    kSpawnNPC = 1,
    kSpawnCorpse = 2
};

struct Config {
    bool map_enabled = true;
    bool map_show_npcs = true;
    bool map_show_players = false;
    bool map_show_corpses = false;
    bool map_chain_eq_labels = true;
    bool map_use_con_color = true;
    bool map_show_target = true;
    bool map_target_line = true;
    bool map_show_ground = false;
    bool map_show_vectors = false;
    bool map_named_only = false;
    bool map_show_xtargets = true;
    bool map_xtarget_labels = true;
    float map_target_radius = 0.0f;
    float map_cast_radius = 0.0f;
    float map_spell_radius = 0.0f;
    std::string map_highlight_filter;
    uint32_t map_highlight_color = 0xFFFF00FF;
    int map_highlight_size = 24;
    int map_max_labels = 0;
    int map_refresh_ms = 1000;
    std::string map_name_filter;
    std::string map_hide_filter;
    bool xtar_enabled = true;
};

#pragma pack(push, 1)
struct Point3 {
    float x;
    float y;
    float z;
};

struct MapLabelNative {
    uint32_t unknown0;
    MapLabelNative* next;
    MapLabelNative* prev;
    Point3 location;
    uint32_t color;
    uint32_t size;
    char* label;
    uint32_t layer;
    uint32_t width;
    uint32_t height;
    uint32_t unknown30;
    uint32_t unknown34;
};

struct MapLineNative {
    MapLineNative* next;
    MapLineNative* prev;
    Point3 start;
    Point3 end;
    uint32_t color;
    uint32_t layer;
};
#pragma pack(pop)

static_assert(sizeof(MapLabelNative) == 0x38, "MapLabelNative must match EQ MAPLABEL size");
static_assert(sizeof(MapLineNative) == 0x28, "MapLineNative must match EQ MAPLINE size");

struct ManagedMapLabel {
    MapLabelNative native{};
    std::string text;
};

struct MapLocation {
    std::string label;
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct XTargetInfo {
    int slot = 0;
    uint32_t type = 0;
    uint32_t spawn_id = 0;
    std::string name;
    void* spawn = nullptr;
    float distance = 0.0f;
};


struct InlineHook {
    void* target = nullptr;
    void* detour = nullptr;
    BYTE original_bytes[16]{};
    size_t length = 0;
    BYTE* gateway = nullptr;
    bool installed = false;

    template <typename T>
    T original() const {
        return reinterpret_cast<T>(gateway);
    }
};

using MapPostDraw2Proc = int(__thiscall*)(void*);
using InterpretCmdProc = void(__thiscall*)(void*, void*, char*);
using DspChatProc = void(__thiscall*)(void*, const char*, DWORD, bool, bool);
using GetConLevelProc = int(__thiscall*)(void*, void*);

HMODULE g_module = nullptr;
HMODULE g_real_dinput = nullptr;
HANDLE g_worker = nullptr;
volatile bool g_shutdown = false;
Config g_config;
InlineHook g_command_hook;

void* g_hooked_map_window = nullptr;
void** g_old_map_vtable = nullptr;
void** g_new_map_vtable = nullptr;
MapPostDraw2Proc g_old_map_post_draw2 = nullptr;
std::vector<ManagedMapLabel> g_map_labels;
std::vector<MapLineNative> g_map_lines;
std::vector<MapLocation> g_map_locations;
DWORD g_last_map_refresh = 0;
__declspec(thread) bool g_in_map_post_draw = false;
int g_map_draw_log_count = 0;

uintptr_t Rebase(uintptr_t address) {
    auto base = reinterpret_cast<uintptr_t>(GetModuleHandleA(nullptr));
    return base + (address - kEqImageBase);
}

std::string GetClientDirectory() {
    char path[MAX_PATH]{};
    if (!GetModuleFileNameA(nullptr, path, MAX_PATH)) {
        return ".";
    }

    char* slash = strrchr(path, '\\');
    if (slash) {
        *slash = '\0';
    }

    return path;
}

std::string JoinClientPath(const char* file_name) {
    std::string path = GetClientDirectory();
    if (!path.empty() && path.back() != '\\') {
        path.push_back('\\');
    }
    path += file_name;
    return path;
}

void Log(const char* fmt, ...) {
    char line[2048]{};
    va_list args;
    va_start(args, fmt);
    _vsnprintf_s(line, sizeof(line), _TRUNCATE, fmt, args);
    va_end(args);

    std::string path = JoinClientPath("mq2_map.log");
    FILE* fp = nullptr;
    if (fopen_s(&fp, path.c_str(), "a") == 0 && fp) {
        SYSTEMTIME st{};
        GetLocalTime(&st);
        fprintf(fp, "[%04u-%02u-%02u %02u:%02u:%02u] %s\n",
            st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, line);
        fclose(fp);
    }
}

bool IsReadableMemory(const void* ptr, size_t bytes) {
    if (!ptr || bytes == 0) {
        return false;
    }

    MEMORY_BASIC_INFORMATION mbi{};
    if (!VirtualQuery(ptr, &mbi, sizeof(mbi))) {
        return false;
    }

    if (mbi.State != MEM_COMMIT || (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD))) {
        return false;
    }

    uintptr_t start = reinterpret_cast<uintptr_t>(ptr);
    uintptr_t end = start + bytes;
    uintptr_t region_end = reinterpret_cast<uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;
    return end <= region_end;
}

template <typename T>
bool SafeRead(const void* address, T& value) {
    if (!IsReadableMemory(address, sizeof(T))) {
        return false;
    }

    __try {
        value = *reinterpret_cast<const T*>(address);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

template <typename T>
T ReadValue(const void* address, T fallback = T{}) {
    T value{};
    if (SafeRead(address, value)) {
        return value;
    }
    return fallback;
}

void* ReadPtr(const void* address) {
    void* value = nullptr;
    SafeRead(address, value);
    return value;
}

void* ReadPtrOffset(const void* base, size_t offset) {
    if (!base) {
        return nullptr;
    }
    return ReadPtr(static_cast<const BYTE*>(base) + offset);
}

void* ReadGlobalPtr(uintptr_t rebased_global_ptr) {
    return ReadPtr(reinterpret_cast<const void*>(Rebase(rebased_global_ptr)));
}

bool WriteGlobalPtr(uintptr_t rebased_global_ptr, void* value) {
    void** target = reinterpret_cast<void**>(Rebase(rebased_global_ptr));
    if (!IsReadableMemory(target, sizeof(void*))) {
        return false;
    }

    __try {
        *target = value;
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool CopyFixedStringRaw(const char* address, size_t max_len, char* buffer, size_t buffer_len) {
    if (!address || !buffer || buffer_len == 0 || max_len == 0) {
        return false;
    }

    buffer[0] = '\0';
    size_t limit = max_len;
    if (limit > buffer_len - 1) {
        limit = buffer_len - 1;
    }

    __try {
        for (size_t i = 0; i < limit; ++i) {
            char c = *(address + i);
            if (c == '\0') {
                break;
            }
            buffer[i] = c;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        buffer[0] = '\0';
        return false;
    }

    return true;
}

std::string ReadFixedString(const void* address, size_t max_len) {
    if (!address || max_len == 0 || !IsReadableMemory(address, 1)) {
        return {};
    }

    char buffer[256]{};
    if (!CopyFixedStringRaw(static_cast<const char*>(address), max_len, buffer, sizeof(buffer))) {
        return {};
    }

    return std::string(buffer);
}

void LoadConfig() {
    std::string path = JoinClientPath("mq2_map.ini");
    g_config.map_enabled = GetPrivateProfileIntA("Map", "Enabled", 1, path.c_str()) != 0;
    g_config.map_show_npcs = GetPrivateProfileIntA("Map", "ShowNPCs", 1, path.c_str()) != 0;
    g_config.map_show_players = GetPrivateProfileIntA("Map", "ShowPlayers", 0, path.c_str()) != 0;
    g_config.map_show_corpses = GetPrivateProfileIntA("Map", "ShowCorpses", 0, path.c_str()) != 0;
    g_config.map_chain_eq_labels = GetPrivateProfileIntA("Map", "ChainEQLabels", 1, path.c_str()) != 0;
    g_config.map_use_con_color = GetPrivateProfileIntA("Map", "UseConColor", 1, path.c_str()) != 0;
    g_config.map_show_target = GetPrivateProfileIntA("Map", "ShowTarget", 1, path.c_str()) != 0;
    g_config.map_target_line = GetPrivateProfileIntA("Map", "TargetLine", 1, path.c_str()) != 0;
    g_config.map_show_ground = GetPrivateProfileIntA("Map", "Ground", 0, path.c_str()) != 0;
    g_config.map_show_vectors = GetPrivateProfileIntA("Map", "Vector", 0, path.c_str()) != 0;
    g_config.map_named_only = GetPrivateProfileIntA("Map", "Named", 0, path.c_str()) != 0;
    g_config.map_show_xtargets = GetPrivateProfileIntA("Map", "XTargets", 1, path.c_str()) != 0;
    g_config.map_xtarget_labels = GetPrivateProfileIntA("Map", "XTargetLabels", 1, path.c_str()) != 0;
    g_config.map_highlight_color = static_cast<uint32_t>(GetPrivateProfileIntA("Map", "HighlightColor", 0x00FF00FF, path.c_str())) | 0xFF000000;
    g_config.map_highlight_size = clamp(static_cast<int>(GetPrivateProfileIntA("Map", "HighlightSize", 24, path.c_str())), 4, 200);
    int max_labels = static_cast<int>(GetPrivateProfileIntA("Map", "MaxLabels", 0, path.c_str()));
    g_config.map_max_labels = max_labels <= 0 ? 0 : clamp(max_labels, 1, 2048);
    int refresh_ms = static_cast<int>(GetPrivateProfileIntA("Map", "RefreshMs", 1000, path.c_str()));
    g_config.map_refresh_ms = clamp(refresh_ms, 250, 5000);
    char float_value[64]{};
    GetPrivateProfileStringA("Map", "TargetRadius", "0", float_value, sizeof(float_value), path.c_str());
    g_config.map_target_radius = std::max(0.0f, static_cast<float>(atof(float_value)));
    GetPrivateProfileStringA("Map", "CastRadius", "0", float_value, sizeof(float_value), path.c_str());
    g_config.map_cast_radius = std::max(0.0f, static_cast<float>(atof(float_value)));
    GetPrivateProfileStringA("Map", "SpellRadius", "0", float_value, sizeof(float_value), path.c_str());
    g_config.map_spell_radius = std::max(0.0f, static_cast<float>(atof(float_value)));
    char name_filter[256]{};
    GetPrivateProfileStringA("Map", "NameFilter", "", name_filter, sizeof(name_filter), path.c_str());
    g_config.map_name_filter = name_filter;
    char hide_filter[256]{};
    GetPrivateProfileStringA("Map", "HideFilter", "", hide_filter, sizeof(hide_filter), path.c_str());
    g_config.map_hide_filter = hide_filter;
    char highlight_filter[256]{};
    GetPrivateProfileStringA("Map", "HighlightFilter", "", highlight_filter, sizeof(highlight_filter), path.c_str());
    g_config.map_highlight_filter = highlight_filter;
    g_config.xtar_enabled = GetPrivateProfileIntA("XTarget", "Enabled", 1, path.c_str()) != 0;
}

void SaveConfigBool(const char* section, const char* key, bool value) {
    std::string path = JoinClientPath("mq2_map.ini");
    WritePrivateProfileStringA(section, key, value ? "1" : "0", path.c_str());
}

void SaveConfigString(const char* section, const char* key, const std::string& value) {
    std::string path = JoinClientPath("mq2_map.ini");
    WritePrivateProfileStringA(section, key, value.c_str(), path.c_str());
}

void SaveConfigInt(const char* section, const char* key, int value) {
    char buffer[32]{};
    _snprintf_s(buffer, sizeof(buffer), _TRUNCATE, "%d", value);
    std::string path = JoinClientPath("mq2_map.ini");
    WritePrivateProfileStringA(section, key, buffer, path.c_str());
}

void SaveConfigFloat(const char* section, const char* key, float value) {
    char buffer[32]{};
    _snprintf_s(buffer, sizeof(buffer), _TRUNCATE, "%.2f", value);
    std::string path = JoinClientPath("mq2_map.ini");
    WritePrivateProfileStringA(section, key, buffer, path.c_str());
}

void Chat(const char* fmt, ...) {
    char line[1024]{};
    va_list args;
    va_start(args, fmt);
    _vsnprintf_s(line, sizeof(line), _TRUNCATE, fmt, args);
    va_end(args);

    Log("%s", line);

    void* everquest = ReadGlobalPtr(kPInstCEverQuest);
    if (!everquest) {
        return;
    }

    auto dsp_chat = reinterpret_cast<DspChatProc>(Rebase(kCEverQuestDspChat));
    dsp_chat(everquest, line, 15, false, true);
}

bool CommandMatch(const char* line, const char* command, const char** arguments) {
    if (!line || !command) {
        return false;
    }

    while (*line == ' ' || *line == '\t') {
        ++line;
    }

    size_t command_len = strlen(command);
    if (_strnicmp(line, command, command_len) != 0) {
        return false;
    }

    char next = line[command_len];
    if (next != '\0' && next != ' ' && next != '\t') {
        return false;
    }

    if (arguments) {
        const char* current = line + command_len;
        while (*current == ' ' || *current == '\t') {
            ++current;
        }
        *arguments = current;
    }

    return true;
}

std::string TrimCopy(const char* text) {
    if (!text) {
        return {};
    }

    while (*text == ' ' || *text == '\t') {
        ++text;
    }

    const char* end = text + strlen(text);
    while (end > text && (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\r' || end[-1] == '\n')) {
        --end;
    }

    return std::string(text, end);
}

std::string LowerCopy(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

bool StartsWith(const char* value, const char* prefix) {
    return value && prefix && strncmp(value, prefix, strlen(prefix)) == 0;
}

bool ContainsInsensitive(const std::string& value, const std::string& needle) {
    if (needle.empty()) {
        return true;
    }
    return LowerCopy(value).find(LowerCopy(needle)) != std::string::npos;
}

bool ParseOnOff(const char* args, bool current_value, bool default_if_missing) {
    if (!args || !args[0]) {
        return default_if_missing;
    }

    if (CommandMatch(args, "on", nullptr) || CommandMatch(args, "1", nullptr) ||
        CommandMatch(args, "true", nullptr)) {
        return true;
    }

    if (CommandMatch(args, "off", nullptr) || CommandMatch(args, "0", nullptr) ||
        CommandMatch(args, "false", nullptr)) {
        return false;
    }

    return current_value;
}

std::vector<std::string> SplitWords(const char* text) {
    std::vector<std::string> words;
    if (!text) {
        return words;
    }

    while (*text) {
        while (*text == ' ' || *text == '\t') {
            ++text;
        }
        if (!*text) {
            break;
        }

        const char* start = text;
        while (*text && *text != ' ' && *text != '\t') {
            ++text;
        }
        words.emplace_back(start, text);
    }

    return words;
}

bool TryParseInt(const std::string& value, int& out) {
    if (value.empty()) {
        return false;
    }

    char* end = nullptr;
    long parsed = strtol(value.c_str(), &end, 10);
    if (!end || *end != '\0') {
        return false;
    }

    out = static_cast<int>(parsed);
    return true;
}

bool TryParseFloat(const std::string& value, float& out) {
    if (value.empty()) {
        return false;
    }

    char* end = nullptr;
    float parsed = strtof(value.c_str(), &end);
    if (!end || *end != '\0') {
        return false;
    }

    out = parsed;
    return true;
}

bool TryParseColorToken(const std::string& value, uint32_t& out) {
    if (value.empty()) {
        return false;
    }

    std::string token = value;
    if (token[0] == '#') {
        token.erase(token.begin());
    } else if (token.size() > 2 && token[0] == '0' && (token[1] == 'x' || token[1] == 'X')) {
        token.erase(0, 2);
    }

    if (token.empty() || token.size() > 8) {
        return false;
    }

    char* end = nullptr;
    unsigned long parsed = strtoul(token.c_str(), &end, 16);
    if (!end || *end != '\0') {
        return false;
    }

    uint32_t color = static_cast<uint32_t>(parsed);
    if (token.size() <= 6) {
        color |= 0xFF000000;
    }
    out = color;
    return true;
}
bool WriteJump(BYTE* target, void* detour) {
    DWORD old_protect = 0;
    if (!VirtualProtect(target, 5, PAGE_EXECUTE_READWRITE, &old_protect)) {
        return false;
    }

    target[0] = 0xE9;
    *reinterpret_cast<int32_t*>(target + 1) =
        static_cast<int32_t>(reinterpret_cast<BYTE*>(detour) - target - 5);

    DWORD unused = 0;
    VirtualProtect(target, 5, old_protect, &unused);
    FlushInstructionCache(GetCurrentProcess(), target, 5);
    return true;
}

bool InstallInlineHook(InlineHook& hook, void* target, void* detour, size_t length) {
    if (hook.installed) {
        return true;
    }

    if (!target || !detour || length < 5 || length > sizeof(hook.original_bytes)) {
        return false;
    }

    hook.gateway = static_cast<BYTE*>(VirtualAlloc(nullptr, length + 5, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
    if (!hook.gateway) {
        return false;
    }

    hook.target = target;
    hook.detour = detour;
    hook.length = length;
    memcpy(hook.original_bytes, target, length);
    memcpy(hook.gateway, target, length);
    WriteJump(hook.gateway + length, static_cast<BYTE*>(target) + length);

    DWORD old_protect = 0;
    if (!VirtualProtect(target, length, PAGE_EXECUTE_READWRITE, &old_protect)) {
        VirtualFree(hook.gateway, 0, MEM_RELEASE);
        hook.gateway = nullptr;
        return false;
    }

    memset(target, 0x90, length);
    WriteJump(static_cast<BYTE*>(target), detour);

    DWORD unused = 0;
    VirtualProtect(target, length, old_protect, &unused);
    FlushInstructionCache(GetCurrentProcess(), target, length);

    hook.installed = true;
    return true;
}

void RemoveInlineHook(InlineHook& hook) {
    if (!hook.installed || !hook.target) {
        return;
    }

    DWORD old_protect = 0;
    if (VirtualProtect(hook.target, hook.length, PAGE_EXECUTE_READWRITE, &old_protect)) {
        memcpy(hook.target, hook.original_bytes, hook.length);
        DWORD unused = 0;
        VirtualProtect(hook.target, hook.length, old_protect, &unused);
        FlushInstructionCache(GetCurrentProcess(), hook.target, hook.length);
    }

    if (hook.gateway) {
        VirtualFree(hook.gateway, 0, MEM_RELEASE);
    }

    hook = InlineHook{};
}

std::string CleanSpawnName(std::string name) {
    if (name.empty()) {
        return name;
    }

    std::replace(name.begin(), name.end(), '_', ' ');
    while (!name.empty() && (name.back() == ' ' || name.back() == '\t')) {
        name.pop_back();
    }
    return name;
}

bool ShouldShowSpawn(uint8_t type, uint8_t gm, bool is_target) {
    if (is_target && g_config.map_show_target) {
        return true;
    }

    if (gm) {
        return g_config.map_show_players || g_config.map_show_npcs;
    }

    switch (type) {
    case kSpawnNPC:
        return g_config.map_show_npcs;
    case kSpawnPlayer:
        return g_config.map_show_players;
    case kSpawnCorpse:
        return g_config.map_show_corpses;
    default:
        return false;
    }
}

bool NamePassesFilters(const std::string& name, bool is_target) {
    if (is_target && g_config.map_show_target) {
        return true;
    }

    if (!g_config.map_name_filter.empty() && !ContainsInsensitive(name, g_config.map_name_filter)) {
        return false;
    }

    if (!g_config.map_hide_filter.empty() && ContainsInsensitive(name, g_config.map_hide_filter)) {
        return false;
    }

    return true;
}

bool TryParseUInt32(const std::string& value, uint32_t& out) {
    if (value.empty()) {
        return false;
    }

    char* end = nullptr;
    unsigned long parsed = strtoul(value.c_str(), &end, 10);
    if (!end || *end != '\0') {
        return false;
    }

    out = static_cast<uint32_t>(parsed);
    return true;
}

double SpawnDistanceSquared(void* local_spawn, void* spawn) {
    if (!local_spawn || !spawn) {
        return 0.0;
    }

    double dx = static_cast<double>(ReadValue<float>(static_cast<BYTE*>(spawn) + kSpawnX, 0.0f)) -
        static_cast<double>(ReadValue<float>(static_cast<BYTE*>(local_spawn) + kSpawnX, 0.0f));
    double dy = static_cast<double>(ReadValue<float>(static_cast<BYTE*>(spawn) + kSpawnY, 0.0f)) -
        static_cast<double>(ReadValue<float>(static_cast<BYTE*>(local_spawn) + kSpawnY, 0.0f));
    double dz = static_cast<double>(ReadValue<float>(static_cast<BYTE*>(spawn) + kSpawnZ, 0.0f)) -
        static_cast<double>(ReadValue<float>(static_cast<BYTE*>(local_spawn) + kSpawnZ, 0.0f));
    return dx * dx + dy * dy + dz * dz;
}

float SpawnDistance(void* local_spawn, void* spawn) {
    return static_cast<float>(sqrt(SpawnDistanceSquared(local_spawn, spawn)));
}

bool IsFiniteWorldCoord(float value) {
    return std::isfinite(value) && value > -100000.0f && value < 100000.0f;
}

std::string GetSpawnCleanName(void* spawn) {
    if (!spawn) {
        return {};
    }

    std::string name = CleanSpawnName(ReadFixedString(static_cast<BYTE*>(spawn) + kSpawnDisplayedName, 0x40));
    if (name.empty()) {
        name = CleanSpawnName(ReadFixedString(static_cast<BYTE*>(spawn) + kSpawnName, 0x40));
    }
    return name;
}

void* FindSpawnByID(uint32_t spawn_id) {
    if (spawn_id == 0) {
        return nullptr;
    }

    void* spawn_mgr = ReadGlobalPtr(kPInstSpawnManager);
    void* spawn = ReadPtrOffset(spawn_mgr, kSpawnManagerFirstSpawn);
    std::unordered_set<void*> visited;
    while (spawn && visited.insert(spawn).second) {
        void* next = ReadPtrOffset(spawn, kSpawnNext);
        uint32_t current_id = ReadValue<uint32_t>(static_cast<BYTE*>(spawn) + kSpawnID, 0);
        if (current_id == spawn_id) {
            return spawn;
        }
        spawn = next;
    }

    return nullptr;
}

bool LooksLikeXTargetMgr(void* manager) {
    if (!manager || !IsReadableMemory(manager, kXTargetMgrArray + sizeof(void*))) {
        return false;
    }

    uint32_t slots = ReadValue<uint32_t>(static_cast<BYTE*>(manager) + kXTargetMgrSlots, 0xFFFFFFFF);
    if (slots > kMaxXTargets) {
        return false;
    }

    void* array = ReadPtrOffset(manager, kXTargetMgrArray);
    if (slots == 0) {
        return true;
    }

    return array && IsReadableMemory(array, static_cast<size_t>(slots) * kXTargetSlotSize);
}

std::vector<XTargetInfo> ReadXTargets() {
    std::vector<XTargetInfo> targets;
    void* char_data = ReadGlobalPtr(kPInstCharData);
    if (!char_data) {
        return targets;
    }

    void* manager = ReadPtrOffset(char_data, kCharDataXTargetMgr);
    if (!LooksLikeXTargetMgr(manager)) {
        void* inline_manager = static_cast<BYTE*>(char_data) + kCharDataXTargetMgr;
        manager = LooksLikeXTargetMgr(inline_manager) ? inline_manager : nullptr;
    }
    if (!manager) {
        return targets;
    }

    uint32_t slot_count = ReadValue<uint32_t>(static_cast<BYTE*>(manager) + kXTargetMgrSlots, 0);
    slot_count = std::min<uint32_t>(slot_count, kMaxXTargets);
    void* array = ReadPtrOffset(manager, kXTargetMgrArray);
    if (!array || !IsReadableMemory(array, static_cast<size_t>(slot_count) * kXTargetSlotSize)) {
        return targets;
    }

    void* local_spawn = ReadGlobalPtr(kPInstCharSpawn);
    targets.reserve(slot_count);
    for (uint32_t i = 0; i < slot_count; ++i) {
        BYTE* slot = static_cast<BYTE*>(array) + static_cast<size_t>(i) * kXTargetSlotSize;
        uint32_t spawn_id = ReadValue<uint32_t>(slot + kXTargetSpawnID, 0);
        std::string name = CleanSpawnName(ReadFixedString(slot + kXTargetName, 0x40));
        if (spawn_id == 0 && name.empty()) {
            continue;
        }

        XTargetInfo info{};
        info.slot = static_cast<int>(i + 1);
        info.type = ReadValue<uint32_t>(slot + kXTargetType, 0);
        info.spawn_id = spawn_id;
        info.name = name;
        info.spawn = FindSpawnByID(spawn_id);
        if (info.spawn && local_spawn) {
            info.distance = SpawnDistance(local_spawn, info.spawn);
            if (info.name.empty()) {
                info.name = GetSpawnCleanName(info.spawn);
            }
        }
        targets.push_back(std::move(info));
    }

    return targets;
}

const XTargetInfo* FindXTargetForSpawn(const std::vector<XTargetInfo>& targets, void* spawn, uint32_t spawn_id) {
    if (!spawn && spawn_id == 0) {
        return nullptr;
    }

    for (const auto& target : targets) {
        if ((spawn && target.spawn == spawn) || (spawn_id != 0 && target.spawn_id == spawn_id)) {
            return &target;
        }
    }
    return nullptr;
}

bool LooksLikeGroundItem(void* item) {
    if (!item || !IsReadableMemory(item, 0x80)) {
        return false;
    }

    std::string name = CleanSpawnName(ReadFixedString(static_cast<BYTE*>(item) + kGroundItemName, 0x20));
    if (name.empty()) {
        return false;
    }

    float x = ReadValue<float>(static_cast<BYTE*>(item) + kGroundItemX, 0.0f);
    float y = ReadValue<float>(static_cast<BYTE*>(item) + kGroundItemY, 0.0f);
    float z = ReadValue<float>(static_cast<BYTE*>(item) + kGroundItemZ, 0.0f);
    return IsFiniteWorldCoord(x) && IsFiniteWorldCoord(y) && IsFiniteWorldCoord(z);
}

void* FindGroundItemList() {
    for (uintptr_t candidate_address : kPInstEQItemListCandidates) {
        void* head = ReadGlobalPtr(candidate_address);
        if (LooksLikeGroundItem(head)) {
            return head;
        }

        void* nested_head = ReadPtr(head);
        if (LooksLikeGroundItem(nested_head)) {
            return nested_head;
        }
    }

    return nullptr;
}

bool CanAddMapLabel() {
    return g_config.map_max_labels <= 0 || static_cast<int>(g_map_labels.size()) < g_config.map_max_labels;
}

void AddMapLabelText(const std::string& text, float x, float y, float z, uint32_t color, uint32_t size = 3, uint32_t layer = 2) {
    if (text.empty() || !CanAddMapLabel() || !IsFiniteWorldCoord(x) || !IsFiniteWorldCoord(y) || !IsFiniteWorldCoord(z)) {
        return;
    }

    ManagedMapLabel managed{};
    managed.text = text;
    managed.native.location.x = -x;
    managed.native.location.y = -y;
    managed.native.location.z = z;
    managed.native.color = color;
    managed.native.size = size;
    managed.native.layer = layer;
    managed.native.width = 20;
    managed.native.height = 14;
    g_map_labels.push_back(std::move(managed));
}

void AddWorldMapLine(float start_x, float start_y, float start_z, float end_x, float end_y, float end_z,
    uint32_t color, uint32_t layer = 2) {
    if (!IsFiniteWorldCoord(start_x) || !IsFiniteWorldCoord(start_y) || !IsFiniteWorldCoord(start_z) ||
        !IsFiniteWorldCoord(end_x) || !IsFiniteWorldCoord(end_y) || !IsFiniteWorldCoord(end_z)) {
        return;
    }

    MapLineNative line{};
    line.start.x = -start_x;
    line.start.y = -start_y;
    line.start.z = start_z;
    line.end.x = -end_x;
    line.end.y = -end_y;
    line.end.z = end_z;
    line.color = color;
    line.layer = layer;
    g_map_lines.push_back(line);
}

void AddMapCircle(float center_x, float center_y, float center_z, float radius, uint32_t color, uint32_t segments = 48) {
    if (radius <= 0.0f || !IsFiniteWorldCoord(center_x) || !IsFiniteWorldCoord(center_y) || !IsFiniteWorldCoord(center_z)) {
        return;
    }

    segments = clamp<uint32_t>(segments, 12, 96);
    for (uint32_t i = 0; i < segments; ++i) {
        float a1 = (static_cast<float>(i) / static_cast<float>(segments)) * (2.0f * kPi);
        float a2 = (static_cast<float>(i + 1) / static_cast<float>(segments)) * (2.0f * kPi);
        AddWorldMapLine(center_x + cosf(a1) * radius, center_y + sinf(a1) * radius, center_z,
            center_x + cosf(a2) * radius, center_y + sinf(a2) * radius, center_z, color, 2);
    }
}

void AddMapBoxMarker(float center_x, float center_y, float center_z, float radius, uint32_t color) {
    if (radius <= 0.0f) {
        return;
    }

    AddWorldMapLine(center_x - radius, center_y - radius, center_z, center_x + radius, center_y - radius, center_z, color, 2);
    AddWorldMapLine(center_x + radius, center_y - radius, center_z, center_x + radius, center_y + radius, center_z, color, 2);
    AddWorldMapLine(center_x + radius, center_y + radius, center_z, center_x - radius, center_y + radius, center_z, color, 2);
    AddWorldMapLine(center_x - radius, center_y + radius, center_z, center_x - radius, center_y - radius, center_z, color, 2);
}

bool LooksNamedSpawn(uint8_t type, const std::string& name) {
    if (type != kSpawnNPC || name.empty()) {
        return false;
    }

    std::string lowered = LowerCopy(name);
    if (lowered.find("corpse") != std::string::npos) {
        return false;
    }

    return lowered.rfind("a ", 0) != 0 && lowered.rfind("an ", 0) != 0 && lowered.rfind("the ", 0) != 0;
}

bool NameIsHighlighted(const std::string& name) {
    return !g_config.map_highlight_filter.empty() && ContainsInsensitive(name, g_config.map_highlight_filter);
}

void* FindSpawnByNameOrID(const std::string& query, std::string& found_name) {
    found_name.clear();
    if (query.empty()) {
        return nullptr;
    }

    uint32_t query_id = 0;
    bool match_id = TryParseUInt32(query, query_id);
    void* spawn_mgr = ReadGlobalPtr(kPInstSpawnManager);
    void* local_spawn = ReadGlobalPtr(kPInstCharSpawn);
    void* spawn = ReadPtrOffset(spawn_mgr, kSpawnManagerFirstSpawn);
    void* best_spawn = nullptr;
    double best_distance = 1.0e30;

    std::unordered_set<void*> visited;
    while (spawn && visited.insert(spawn).second) {
        void* next = ReadPtrOffset(spawn, kSpawnNext);
        if (spawn == local_spawn) {
            spawn = next;
            continue;
        }

        std::string name = CleanSpawnName(ReadFixedString(static_cast<BYTE*>(spawn) + kSpawnDisplayedName, 0x40));
        if (name.empty()) {
            name = CleanSpawnName(ReadFixedString(static_cast<BYTE*>(spawn) + kSpawnName, 0x40));
        }

        uint32_t spawn_id = ReadValue<uint32_t>(static_cast<BYTE*>(spawn) + kSpawnID, 0);
        bool matches = (match_id && spawn_id == query_id) || (!name.empty() && ContainsInsensitive(name, query));
        if (matches) {
            double distance = SpawnDistanceSquared(local_spawn, spawn);
            if (!best_spawn || distance < best_distance) {
                best_spawn = spawn;
                best_distance = distance;
                found_name = name;
            }
        }

        spawn = next;
    }

    return best_spawn;
}

uint32_t ConColorToARGB(int con_color) {
    switch (con_color) {
    case 0:
    case 1:
        return 0xFF505050;
    case 2:
        return 0xFF00FF00;
    case 3:
        return 0xFF00FFFF;
    case 4:
        return 0xFF0000FF;
    case 5:
        return 0xFFFFFFFF;
    case 6:
        return 0xFFFFFF00;
    case 7:
    default:
        return 0xFFFF0000;
    }
}

int ApproximateConLevel(int player_level, int spawn_level) {
    if (player_level <= 0 || spawn_level <= 0) {
        return 5;
    }

    int diff = spawn_level - player_level;
    if (diff >= 3) {
        return 7;
    }
    if (diff >= 1) {
        return 6;
    }
    if (diff == 0) {
        return 5;
    }
    if (diff >= -2) {
        return 4;
    }
    if (diff >= -4) {
        return 3;
    }
    if (diff >= -7) {
        return 2;
    }
    return 1;
}

uint32_t GetSpawnConColor(void* spawn, int spawn_level) {
    void* char_data = ReadGlobalPtr(kPInstCharData);
    if (char_data) {
        auto get_con_level = reinterpret_cast<GetConLevelProc>(Rebase(kEQCharacterGetConLevel));
        __try {
            int con_level = get_con_level(char_data, spawn);
            if (con_level >= 0 && con_level <= 7) {
                return ConColorToARGB(con_level);
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
        }
    }

    void* local_spawn = ReadGlobalPtr(kPInstCharSpawn);
    int player_level = ReadValue<int32_t>(static_cast<BYTE*>(local_spawn) + kSpawnLevel, 0);
    return ConColorToARGB(ApproximateConLevel(player_level, spawn_level));
}

uint32_t SpawnColor(void* spawn, uint8_t type, uint8_t gm, int spawn_level) {
    if (g_config.map_use_con_color && spawn && (type == kSpawnNPC || type == kSpawnPlayer)) {
        return GetSpawnConColor(spawn, spawn_level);
    }

    if (gm) {
        return 0xFFFF66FF;
    }

    switch (type) {
    case kSpawnNPC:
        return 0xFF00E5FF;
    case kSpawnPlayer:
        return 0xFF66CCFF;
    case kSpawnCorpse:
        return 0xFFBBBBBB;
    default:
        return 0xFFFFFFFF;
    }
}

void UpdateMapLabels() {
    g_map_labels.clear();
    g_map_lines.clear();

    if (!g_config.map_enabled) {
        return;
    }

    void* spawn_mgr = ReadGlobalPtr(kPInstSpawnManager);
    void* local_spawn = ReadGlobalPtr(kPInstCharSpawn);
    void* target_spawn = ReadGlobalPtr(kPInstTarget);
    void* spawn = ReadPtrOffset(spawn_mgr, kSpawnManagerFirstSpawn);
    if (!spawn) {
        return;
    }

    std::vector<XTargetInfo> xtargets = (g_config.xtar_enabled || g_config.map_show_xtargets) ? ReadXTargets() : std::vector<XTargetInfo>{};
    g_map_labels.reserve(384);
    g_map_lines.reserve(128);
    std::unordered_set<void*> visited;
    bool target_in_spawn_list = false;
    while (spawn && visited.insert(spawn).second) {

        void* next = ReadPtrOffset(spawn, kSpawnNext);
        if (spawn == local_spawn) {
            spawn = next;
            continue;
        }

        uint8_t type = ReadValue<uint8_t>(static_cast<BYTE*>(spawn) + kSpawnType, 0xFF);
        uint8_t gm = ReadValue<uint8_t>(static_cast<BYTE*>(spawn) + kSpawnGM, 0);
        bool is_target = spawn == target_spawn;
        if (is_target) {
            target_in_spawn_list = true;
        }

        uint32_t spawn_id = ReadValue<uint32_t>(static_cast<BYTE*>(spawn) + kSpawnID, 0);
        const XTargetInfo* xtarget = FindXTargetForSpawn(xtargets, spawn, spawn_id);
        bool is_xtarget = xtarget != nullptr;
        if (!ShouldShowSpawn(type, gm, is_target) && !(g_config.map_show_xtargets && is_xtarget)) {
            spawn = next;
            continue;
        }

        std::string name = GetSpawnCleanName(spawn);
        if (name.empty()) {
            spawn = next;
            continue;
        }
        if (!NamePassesFilters(name, is_target)) {
            spawn = next;
            continue;
        }
        if (g_config.map_named_only && !is_target && !is_xtarget && !LooksNamedSpawn(type, name)) {
            spawn = next;
            continue;
        }

        int level = ReadValue<int32_t>(static_cast<BYTE*>(spawn) + kSpawnLevel, 0);
        float x = ReadValue<float>(static_cast<BYTE*>(spawn) + kSpawnX, 0.0f);
        float y = ReadValue<float>(static_cast<BYTE*>(spawn) + kSpawnY, 0.0f);
        float z = ReadValue<float>(static_cast<BYTE*>(spawn) + kSpawnZ, 0.0f);

        uint32_t color = (is_target && g_config.map_show_target) ? kMapTargetColor : SpawnColor(spawn, type, gm, level);
        uint32_t label_size = 3;
        if (is_xtarget && !is_target) {
            color = kMapXTargetColor;
        }
        if (NameIsHighlighted(name)) {
            color = g_config.map_highlight_color;
            label_size = static_cast<uint32_t>(clamp(g_config.map_highlight_size, 4, 200));
            AddMapBoxMarker(x, y, z, 12.0f, g_config.map_highlight_color);
        }

        std::string label = name;
        if (is_xtarget && g_config.map_xtarget_labels && xtarget) {
            char suffix[64]{};
            if (xtarget->distance > 0.0f) {
                _snprintf_s(suffix, sizeof(suffix), _TRUNCATE, " [XT%d %.0f]", xtarget->slot, xtarget->distance);
            } else {
                _snprintf_s(suffix, sizeof(suffix), _TRUNCATE, " [XT%d]", xtarget->slot);
            }
            label += suffix;
        }
        AddMapLabelText(label, x, y, z, color, label_size);

        if (g_config.map_show_vectors) {
            float speed_x = ReadValue<float>(static_cast<BYTE*>(spawn) + kSpawnSpeedX, 0.0f);
            float speed_y = ReadValue<float>(static_cast<BYTE*>(spawn) + kSpawnSpeedY, 0.0f);
            float heading = ReadValue<float>(static_cast<BYTE*>(spawn) + kSpawnHeading, 0.0f);
            float vector_x = speed_x;
            float vector_y = speed_y;
            if (fabsf(vector_x) < 0.01f && fabsf(vector_y) < 0.01f) {
                float angle = heading * (2.0f * kPi / 512.0f);
                vector_x = sinf(angle) * 12.0f;
                vector_y = cosf(angle) * 12.0f;
            } else {
                vector_x *= 8.0f;
                vector_y *= 8.0f;
            }
            AddWorldMapLine(x, y, z, x + vector_x, y + vector_y, z, color, 2);
        }

        spawn = next;
    }

    if (g_config.map_show_ground) {
        void* ground = FindGroundItemList();
        std::unordered_set<void*> ground_visited;
        unsigned ground_count = 0;
        while (ground && ground_visited.insert(ground).second && ground_count < 512) {
            void* next_ground = ReadPtrOffset(ground, kGroundItemNext);
            if (!LooksLikeGroundItem(ground)) {
                break;
            }

            std::string name = CleanSpawnName(ReadFixedString(static_cast<BYTE*>(ground) + kGroundItemName, 0x20));
            float x = ReadValue<float>(static_cast<BYTE*>(ground) + kGroundItemX, 0.0f);
            float y = ReadValue<float>(static_cast<BYTE*>(ground) + kGroundItemY, 0.0f);
            float z = ReadValue<float>(static_cast<BYTE*>(ground) + kGroundItemZ, 0.0f);
            AddMapLabelText(std::string("G: ") + name, x, y, z, kMapGroundColor, 3);
            AddMapBoxMarker(x, y, z, 4.0f, kMapGroundColor);
            ++ground_count;
            ground = next_ground;
        }
    }

    for (const auto& location : g_map_locations) {
        AddMapLabelText(location.label, location.x, location.y, location.z, kMapLocationColor, 4);
        AddMapBoxMarker(location.x, location.y, location.z, 8.0f, kMapLocationColor);
    }

    for (size_t i = 0; i < g_map_labels.size(); ++i) {
        g_map_labels[i].native.next = (i + 1 < g_map_labels.size()) ? &g_map_labels[i + 1].native : nullptr;
        g_map_labels[i].native.prev = (i > 0) ? &g_map_labels[i - 1].native : nullptr;
        g_map_labels[i].native.label = const_cast<char*>(g_map_labels[i].text.c_str());
    }

    if (target_in_spawn_list && local_spawn && target_spawn && local_spawn != target_spawn) {
        float local_x = ReadValue<float>(static_cast<BYTE*>(local_spawn) + kSpawnX, 0.0f);
        float local_y = ReadValue<float>(static_cast<BYTE*>(local_spawn) + kSpawnY, 0.0f);
        float local_z = ReadValue<float>(static_cast<BYTE*>(local_spawn) + kSpawnZ, 0.0f);
        float target_x = ReadValue<float>(static_cast<BYTE*>(target_spawn) + kSpawnX, 0.0f);
        float target_y = ReadValue<float>(static_cast<BYTE*>(target_spawn) + kSpawnY, 0.0f);
        float target_z = ReadValue<float>(static_cast<BYTE*>(target_spawn) + kSpawnZ, 0.0f);

        if (g_config.map_target_line) {
            AddWorldMapLine(local_x, local_y, local_z, target_x, target_y, target_z, kMapTargetColor, 2);
        }
        if (g_config.map_target_radius > 0.0f) {
            AddMapCircle(target_x, target_y, target_z, g_config.map_target_radius, kMapTargetColor);
        }
    }

    if (local_spawn) {
        float local_x = ReadValue<float>(static_cast<BYTE*>(local_spawn) + kSpawnX, 0.0f);
        float local_y = ReadValue<float>(static_cast<BYTE*>(local_spawn) + kSpawnY, 0.0f);
        float local_z = ReadValue<float>(static_cast<BYTE*>(local_spawn) + kSpawnZ, 0.0f);
        AddMapCircle(local_x, local_y, local_z, g_config.map_cast_radius, kMapRadiusColor);
        AddMapCircle(local_x, local_y, local_z, g_config.map_spell_radius, 0xFF80C0FF);
    }

    for (size_t i = 0; i < g_map_lines.size(); ++i) {
        g_map_lines[i].next = (i + 1 < g_map_lines.size()) ? &g_map_lines[i + 1] : nullptr;
        g_map_lines[i].prev = (i > 0) ? &g_map_lines[i - 1] : nullptr;
    }
}

void AttachMapLabels(void* map_window, MapLabelNative*& original_labels) {
    original_labels = nullptr;
    if (!map_window || g_map_labels.empty()) {
        return;
    }

    auto labels_ptr = reinterpret_cast<MapLabelNative**>(static_cast<BYTE*>(map_window) + kMapLabels);
    original_labels = ReadValue<MapLabelNative*>(labels_ptr, nullptr);

    MapLabelNative* head = &g_map_labels.front().native;
    MapLabelNative* tail = &g_map_labels.back().native;
    if (g_config.map_chain_eq_labels && original_labels) {
        tail->next = original_labels;
    }

    *labels_ptr = head;
}

void DetachMapLabels(void* map_window, MapLabelNative* original_labels) {
    if (!map_window || g_map_labels.empty()) {
        return;
    }

    auto labels_ptr = reinterpret_cast<MapLabelNative**>(static_cast<BYTE*>(map_window) + kMapLabels);
    MapLabelNative* tail = &g_map_labels.back().native;
    if (g_config.map_chain_eq_labels && original_labels) {
        tail->next = nullptr;
    }

    *labels_ptr = original_labels;
}

void AttachMapLines(void* map_window, MapLineNative*& original_lines) {
    original_lines = nullptr;
    if (!map_window || g_map_lines.empty()) {
        return;
    }

    auto lines_ptr = reinterpret_cast<MapLineNative**>(static_cast<BYTE*>(map_window) + kMapLines);
    original_lines = ReadValue<MapLineNative*>(lines_ptr, nullptr);

    MapLineNative* head = &g_map_lines.front();
    MapLineNative* tail = &g_map_lines.back();
    tail->next = original_lines;
    *lines_ptr = head;
}

void DetachMapLines(void* map_window, MapLineNative* original_lines) {
    if (!map_window || g_map_lines.empty()) {
        return;
    }

    auto lines_ptr = reinterpret_cast<MapLineNative**>(static_cast<BYTE*>(map_window) + kMapLines);
    MapLineNative* tail = &g_map_lines.back();
    tail->next = nullptr;
    *lines_ptr = original_lines;
}

int __fastcall MapPostDraw2Detour(void* self, void*) {
    if (g_in_map_post_draw) {
        return g_old_map_post_draw2 ? g_old_map_post_draw2(self) : 0;
    }

    g_in_map_post_draw = true;

    DWORD now = GetTickCount();
    DWORD update_start = now;
    if (now - g_last_map_refresh > static_cast<DWORD>(g_config.map_refresh_ms)) {
        UpdateMapLabels();
        g_last_map_refresh = now;
    }
    DWORD update_elapsed = GetTickCount() - update_start;

    if (g_map_draw_log_count < 8) {
        ++g_map_draw_log_count;
        Log("Map draw pass labels=%u lines=%u update_ms=%lu parent=%p draw_self=%p",
            static_cast<unsigned>(g_map_labels.size()), static_cast<unsigned>(g_map_lines.size()),
            update_elapsed, g_hooked_map_window, self);
    }

    MapLabelNative* original_labels = nullptr;
    MapLineNative* original_lines = nullptr;
    void* map_window = g_hooked_map_window ? g_hooked_map_window : ReadGlobalPtr(kPInstCMapViewWnd);
    AttachMapLabels(map_window, original_labels);
    AttachMapLines(map_window, original_lines);

    int result = 0;
    if (g_old_map_post_draw2) {
        result = g_old_map_post_draw2(self);
    }

    DetachMapLines(map_window, original_lines);
    DetachMapLabels(map_window, original_labels);
    g_in_map_post_draw = false;
    return result;
}

void RestoreMapHook() {
    if (!g_hooked_map_window || !g_new_map_vtable || !g_old_map_vtable) {
        return;
    }

    auto vtable_field = reinterpret_cast<void***>(static_cast<BYTE*>(g_hooked_map_window) + kMapViewVTable);
    void** current = ReadValue<void**>(vtable_field, nullptr);
    if (current == g_new_map_vtable) {
        DWORD old_protect = 0;
        if (VirtualProtect(vtable_field, sizeof(void*), PAGE_EXECUTE_READWRITE, &old_protect)) {
            *vtable_field = g_old_map_vtable;
            DWORD unused = 0;
            VirtualProtect(vtable_field, sizeof(void*), old_protect, &unused);
        }
    }

    VirtualFree(g_new_map_vtable, 0, MEM_RELEASE);
    g_hooked_map_window = nullptr;
    g_old_map_vtable = nullptr;
    g_new_map_vtable = nullptr;
    g_old_map_post_draw2 = nullptr;
}

bool InstallMapHook() {
    if (!g_config.map_enabled) {
        return false;
    }

    void* map_window = ReadGlobalPtr(kPInstCMapViewWnd);
    if (!map_window) {
        return false;
    }

    if (map_window == g_hooked_map_window && g_new_map_vtable) {
        return true;
    }

    RestoreMapHook();

    auto vtable_field = reinterpret_cast<void***>(static_cast<BYTE*>(map_window) + kMapViewVTable);
    void** old_vtable = ReadValue<void**>(vtable_field, nullptr);
    if (!old_vtable || !IsReadableMemory(old_vtable, kMapVTableBytes)) {
        return false;
    }

    void** new_vtable = static_cast<void**>(VirtualAlloc(nullptr, kMapVTableBytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
    if (!new_vtable) {
        return false;
    }

    memcpy(new_vtable, old_vtable, kMapVTableBytes);
    g_old_map_post_draw2 = reinterpret_cast<MapPostDraw2Proc>(new_vtable[kMapPostDraw2Index]);
    if (new_vtable[kMapPostDraw2Index] == reinterpret_cast<void*>(&MapPostDraw2Detour)) {
        VirtualFree(new_vtable, 0, MEM_RELEASE);
        g_old_map_post_draw2 = nullptr;
        return false;
    }
    new_vtable[kMapPostDraw2Index] = reinterpret_cast<void*>(&MapPostDraw2Detour);

    DWORD old_protect = 0;
    if (!VirtualProtect(vtable_field, sizeof(void*), PAGE_EXECUTE_READWRITE, &old_protect)) {
        VirtualFree(new_vtable, 0, MEM_RELEASE);
        return false;
    }

    *vtable_field = new_vtable;
    DWORD unused = 0;
    VirtualProtect(vtable_field, sizeof(void*), old_protect, &unused);

    g_hooked_map_window = map_window;
    g_old_map_vtable = old_vtable;
    g_new_map_vtable = new_vtable;
    Log("Installed map PostDraw2 hook at map window %p", map_window);
    return true;
}

void PrintMapStatus() {
    void* map_window = ReadGlobalPtr(kPInstCMapViewWnd);
    void* spawn_mgr = ReadGlobalPtr(kPInstSpawnManager);
    void* first_spawn = ReadPtrOffset(spawn_mgr, kSpawnManagerFirstSpawn);
    void* target_spawn = ReadGlobalPtr(kPInstTarget);
    char max_labels[32]{};
    if (g_config.map_max_labels <= 0) {
        strcpy_s(max_labels, "unlimited");
    } else {
        _snprintf_s(max_labels, sizeof(max_labels), _TRUNCATE, "%d", g_config.map_max_labels);
    }
    Chat("NativeMap: %s, hook=%s, labels=%u, map=%p, hooked=%p, first_spawn=%p",
        g_config.map_enabled ? "on" : "off",
        g_new_map_vtable ? "installed" : "not installed",
        static_cast<unsigned>(g_map_labels.size()),
        map_window,
        g_hooked_map_window,
        first_spawn);
    Chat("NativeMap filters: npcs=%d players=%d corpses=%d con_color=%d target=%d target_line=%d normal_labels=%d max=%s refresh_ms=%d",
        g_config.map_show_npcs ? 1 : 0,
        g_config.map_show_players ? 1 : 0,
        g_config.map_show_corpses ? 1 : 0,
        g_config.map_use_con_color ? 1 : 0,
        g_config.map_show_target ? 1 : 0,
        g_config.map_target_line ? 1 : 0,
        g_config.map_chain_eq_labels ? 1 : 0,
        max_labels,
        g_config.map_refresh_ms);
    Chat("NativeMap extra: ground=%d vector=%d named=%d xtargets=%d xtlabels=%d radii target=%.1f cast=%.1f spell=%.1f",
        g_config.map_show_ground ? 1 : 0,
        g_config.map_show_vectors ? 1 : 0,
        g_config.map_named_only ? 1 : 0,
        g_config.map_show_xtargets ? 1 : 0,
        g_config.map_xtarget_labels ? 1 : 0,
        g_config.map_target_radius,
        g_config.map_cast_radius,
        g_config.map_spell_radius);
    Chat("NativeMap search: filter='%s' hide='%s' highlight='%s' current_target=%p lines=%u",
        g_config.map_name_filter.empty() ? "" : g_config.map_name_filter.c_str(),
        g_config.map_hide_filter.empty() ? "" : g_config.map_hide_filter.c_str(),
        g_config.map_highlight_filter.empty() ? "" : g_config.map_highlight_filter.c_str(),
        target_spawn,
        static_cast<unsigned>(g_map_lines.size()));
}

void RefreshMapOverlay() {
    g_last_map_refresh = 0;
    UpdateMapLabels();
}

void SetMapBool(bool& setting, const char* ini_key, bool enabled, bool refresh) {
    setting = enabled;
    SaveConfigBool("Map", ini_key, enabled);
    if (refresh) {
        RefreshMapOverlay();
    }
}

void SetMapFloat(float& setting, const char* ini_key, float value) {
    setting = std::max(0.0f, value);
    SaveConfigFloat("Map", ini_key, setting);
    RefreshMapOverlay();
}

bool HandleTextFilterCommand(const char* args, std::string& setting, const char* ini_key, const char* label) {
    std::string value = TrimCopy(args);
    if (value.empty()) {
        Chat("NativeMap %s filter is '%s'.", label, setting.empty() ? "" : setting.c_str());
        return true;
    }

    std::string lowered = LowerCopy(value);
    if (lowered == "clear" || lowered == "off" || lowered == "none") {
        setting.clear();
        SaveConfigString("Map", ini_key, setting);
        RefreshMapOverlay();
        Chat("NativeMap %s filter cleared.", label);
        return true;
    }

    setting = value;
    SaveConfigString("Map", ini_key, setting);
    RefreshMapOverlay();
    Chat("NativeMap %s filter set to '%s'.", label, setting.c_str());
    return true;
}

bool HandleTargetCommand(const char* args) {
    std::string query = TrimCopy(args);
    if (query.empty()) {
        Chat("NativeMap target usage: /nimap target <name-or-id>|clear");
        return true;
    }

    std::string lowered = LowerCopy(query);
    if (lowered == "clear" || lowered == "off") {
        if (WriteGlobalPtr(kPInstTarget, nullptr)) {
            RefreshMapOverlay();
            Chat("NativeMap target cleared.");
        } else {
            Chat("NativeMap target clear failed.");
        }
        return true;
    }

    std::string found_name;
    void* spawn = FindSpawnByNameOrID(query, found_name);
    if (!spawn) {
        Chat("NativeMap found no spawn matching '%s'.", query.c_str());
        return true;
    }

    if (WriteGlobalPtr(kPInstTarget, spawn)) {
        RefreshMapOverlay();
        Chat("NativeMap target selected: %s.", found_name.empty() ? query.c_str() : found_name.c_str());
    } else {
        Chat("NativeMap target select failed.");
    }
    return true;
}

bool HandleMapFilterCommand(const char* args) {
    if (!args || !args[0] || CommandMatch(args, "status", nullptr)) {
        RefreshMapOverlay();
        PrintMapStatus();
        return true;
    }

    if (CommandMatch(args, "help", nullptr)) {
        Chat("MapFilter: NPC|PC|Corpse|Target|TargetLine|NormalLabels|NPCConColor|Ground|Vector|Named|XTargets|XTargetLabels on/off");
        Chat("MapFilter: Custom <text>, Hide <text>, TargetRadius|CastRadius|SpellRadius <distance>");
        return true;
    }

    const char* sub_args = nullptr;
    if (CommandMatch(args, "NPC", &sub_args)) {
        bool enabled = ParseOnOff(sub_args, g_config.map_show_npcs, !g_config.map_show_npcs);
        SetMapBool(g_config.map_show_npcs, "ShowNPCs", enabled, true);
        Chat("MapFilter NPC is now %s.", enabled ? "on" : "off");
        return true;
    }

    if (CommandMatch(args, "PC", &sub_args) || CommandMatch(args, "Player", &sub_args) ||
        CommandMatch(args, "Players", &sub_args)) {
        bool enabled = ParseOnOff(sub_args, g_config.map_show_players, !g_config.map_show_players);
        SetMapBool(g_config.map_show_players, "ShowPlayers", enabled, true);
        Chat("MapFilter PC is now %s.", enabled ? "on" : "off");
        return true;
    }

    if (CommandMatch(args, "Corpse", &sub_args) || CommandMatch(args, "Corpses", &sub_args)) {
        bool enabled = ParseOnOff(sub_args, g_config.map_show_corpses, !g_config.map_show_corpses);
        SetMapBool(g_config.map_show_corpses, "ShowCorpses", enabled, true);
        Chat("MapFilter Corpse is now %s.", enabled ? "on" : "off");
        return true;
    }

    if (CommandMatch(args, "Target", &sub_args)) {
        bool enabled = ParseOnOff(sub_args, g_config.map_show_target, !g_config.map_show_target);
        SetMapBool(g_config.map_show_target, "ShowTarget", enabled, true);
        Chat("MapFilter Target is now %s.", enabled ? "on" : "off");
        return true;
    }

    if (CommandMatch(args, "TargetLine", &sub_args)) {
        bool enabled = ParseOnOff(sub_args, g_config.map_target_line, !g_config.map_target_line);
        SetMapBool(g_config.map_target_line, "TargetLine", enabled, true);
        Chat("MapFilter TargetLine is now %s.", enabled ? "on" : "off");
        return true;
    }

    if (CommandMatch(args, "NormalLabels", &sub_args) || CommandMatch(args, "Labels", &sub_args)) {
        bool enabled = ParseOnOff(sub_args, g_config.map_chain_eq_labels, !g_config.map_chain_eq_labels);
        SetMapBool(g_config.map_chain_eq_labels, "ChainEQLabels", enabled, true);
        Chat("MapFilter NormalLabels is now %s.", enabled ? "on" : "off");
        return true;
    }

    if (CommandMatch(args, "NPCConColor", &sub_args) || CommandMatch(args, "ConColor", &sub_args)) {
        bool enabled = ParseOnOff(sub_args, g_config.map_use_con_color, !g_config.map_use_con_color);
        SetMapBool(g_config.map_use_con_color, "UseConColor", enabled, true);
        Chat("MapFilter NPCConColor is now %s.", enabled ? "on" : "off");
        return true;
    }

    if (CommandMatch(args, "Ground", &sub_args) || CommandMatch(args, "GroundItems", &sub_args)) {
        bool enabled = ParseOnOff(sub_args, g_config.map_show_ground, !g_config.map_show_ground);
        SetMapBool(g_config.map_show_ground, "Ground", enabled, true);
        Chat("MapFilter Ground is now %s.", enabled ? "on" : "off");
        return true;
    }

    if (CommandMatch(args, "Vector", &sub_args) || CommandMatch(args, "Vectors", &sub_args)) {
        bool enabled = ParseOnOff(sub_args, g_config.map_show_vectors, !g_config.map_show_vectors);
        SetMapBool(g_config.map_show_vectors, "Vector", enabled, true);
        Chat("MapFilter Vector is now %s.", enabled ? "on" : "off");
        return true;
    }

    if (CommandMatch(args, "Named", &sub_args)) {
        bool enabled = ParseOnOff(sub_args, g_config.map_named_only, !g_config.map_named_only);
        SetMapBool(g_config.map_named_only, "Named", enabled, true);
        Chat("MapFilter Named is now %s.", enabled ? "on" : "off");
        return true;
    }

    if (CommandMatch(args, "XTargets", &sub_args) || CommandMatch(args, "XTarget", &sub_args)) {
        bool enabled = ParseOnOff(sub_args, g_config.map_show_xtargets, !g_config.map_show_xtargets);
        SetMapBool(g_config.map_show_xtargets, "XTargets", enabled, true);
        Chat("MapFilter XTargets is now %s.", enabled ? "on" : "off");
        return true;
    }

    if (CommandMatch(args, "XTargetLabels", &sub_args) || CommandMatch(args, "XTLabels", &sub_args)) {
        bool enabled = ParseOnOff(sub_args, g_config.map_xtarget_labels, !g_config.map_xtarget_labels);
        SetMapBool(g_config.map_xtarget_labels, "XTargetLabels", enabled, true);
        Chat("MapFilter XTargetLabels is now %s.", enabled ? "on" : "off");
        return true;
    }

    if (CommandMatch(args, "TargetRadius", &sub_args)) {
        float value = 0.0f;
        if (!TryParseFloat(TrimCopy(sub_args), value)) {
            Chat("MapFilter TargetRadius is %.1f. Usage: /mapfilter TargetRadius <distance>", g_config.map_target_radius);
            return true;
        }
        SetMapFloat(g_config.map_target_radius, "TargetRadius", value);
        Chat("MapFilter TargetRadius set to %.1f.", g_config.map_target_radius);
        return true;
    }

    if (CommandMatch(args, "CastRadius", &sub_args)) {
        float value = 0.0f;
        if (!TryParseFloat(TrimCopy(sub_args), value)) {
            Chat("MapFilter CastRadius is %.1f. Usage: /mapfilter CastRadius <distance>", g_config.map_cast_radius);
            return true;
        }
        SetMapFloat(g_config.map_cast_radius, "CastRadius", value);
        Chat("MapFilter CastRadius set to %.1f.", g_config.map_cast_radius);
        return true;
    }

    if (CommandMatch(args, "SpellRadius", &sub_args)) {
        float value = 0.0f;
        if (!TryParseFloat(TrimCopy(sub_args), value)) {
            Chat("MapFilter SpellRadius is %.1f. Usage: /mapfilter SpellRadius <distance>", g_config.map_spell_radius);
            return true;
        }
        SetMapFloat(g_config.map_spell_radius, "SpellRadius", value);
        Chat("MapFilter SpellRadius set to %.1f.", g_config.map_spell_radius);
        return true;
    }

    if (CommandMatch(args, "Custom", &sub_args) || CommandMatch(args, "Filter", &sub_args)) {
        return HandleTextFilterCommand(sub_args, g_config.map_name_filter, "NameFilter", "name");
    }

    if (CommandMatch(args, "Hide", &sub_args)) {
        return HandleTextFilterCommand(sub_args, g_config.map_hide_filter, "HideFilter", "hide");
    }

    Chat("MapFilter supported: NPC, PC, Corpse, Target, TargetLine, NormalLabels, NPCConColor, Ground, Vector, Named, XTargets, Custom, Hide.");
    return true;
}

bool HandleNativeMapCommand(const char* line) {
    const char* args = nullptr;
    if (CommandMatch(line, "/mapfilter", &args)) {
        return HandleMapFilterCommand(args);
    }

    if (!CommandMatch(line, "/nimap", &args) &&
        !CommandMatch(line, "/nativeinterfacemap", &args)) {
        return false;
    }

    if (!args || !args[0] || CommandMatch(args, "status", nullptr)) {
        RefreshMapOverlay();
        PrintMapStatus();
        return true;
    }

    if (CommandMatch(args, "help", nullptr)) {
        Chat("NativeMap: on|off|status|reload|npcs|players|corpses|con|labels|showtarget|targetline|ground|vector|named|target");
        Chat("NativeMap commands: /nimap or /nativeinterfacemap.");
        Chat("Map tools: /mapfilter help, /mapshow <text>, /maphide <text>, /highlight <text>, /maploc add <label>, /xtarinfo");
        return true;
    }

    if (CommandMatch(args, "on", nullptr)) {
        g_config.map_enabled = true;
        SaveConfigBool("Map", "Enabled", true);
        g_last_map_refresh = 0;
        InstallMapHook();
        UpdateMapLabels();
        Chat("NativeMap is now on.");
        PrintMapStatus();
        return true;
    }

    if (CommandMatch(args, "off", nullptr)) {
        g_config.map_enabled = false;
        SaveConfigBool("Map", "Enabled", false);
        RestoreMapHook();
        g_map_labels.clear();
        g_map_lines.clear();
        Chat("NativeMap is now off.");
        return true;
    }

    if (CommandMatch(args, "reload", nullptr)) {
        LoadConfig();
        g_last_map_refresh = 0;
        UpdateMapLabels();
        InstallMapHook();
        Chat("NativeMap config reloaded.");
        PrintMapStatus();
        return true;
    }

    const char* sub_args = nullptr;
    if (CommandMatch(args, "npcs", &sub_args) || CommandMatch(args, "npc", &sub_args)) {
        bool enabled = ParseOnOff(sub_args, g_config.map_show_npcs, true);
        SetMapBool(g_config.map_show_npcs, "ShowNPCs", enabled, true);
        Chat("NativeMap NPC labels %s.", enabled ? "on" : "off");
        return true;
    }

    if (CommandMatch(args, "players", &sub_args) || CommandMatch(args, "player", &sub_args)) {
        bool enabled = ParseOnOff(sub_args, g_config.map_show_players, true);
        SetMapBool(g_config.map_show_players, "ShowPlayers", enabled, true);
        Chat("NativeMap player labels %s.", enabled ? "on" : "off");
        return true;
    }

    if (CommandMatch(args, "corpses", &sub_args) || CommandMatch(args, "corpse", &sub_args)) {
        bool enabled = ParseOnOff(sub_args, g_config.map_show_corpses, true);
        SetMapBool(g_config.map_show_corpses, "ShowCorpses", enabled, true);
        Chat("NativeMap corpse labels %s.", enabled ? "on" : "off");
        return true;
    }

    if (CommandMatch(args, "labels", &sub_args) || CommandMatch(args, "normallabels", &sub_args)) {
        bool enabled = ParseOnOff(sub_args, g_config.map_chain_eq_labels, true);
        SetMapBool(g_config.map_chain_eq_labels, "ChainEQLabels", enabled, true);
        Chat("NativeMap normal map labels %s.", enabled ? "on" : "off");
        return true;
    }

    if (CommandMatch(args, "con", &sub_args) || CommandMatch(args, "concolor", &sub_args)) {
        bool enabled = ParseOnOff(sub_args, g_config.map_use_con_color, true);
        SetMapBool(g_config.map_use_con_color, "UseConColor", enabled, true);
        Chat("NativeMap con colors %s.", enabled ? "on" : "off");
        return true;
    }

    if (CommandMatch(args, "showtarget", &sub_args) || CommandMatch(args, "targetlabel", &sub_args) ||
        CommandMatch(args, "highlight", &sub_args)) {
        bool enabled = ParseOnOff(sub_args, g_config.map_show_target, true);
        SetMapBool(g_config.map_show_target, "ShowTarget", enabled, true);
        Chat("NativeMap target highlight %s.", enabled ? "on" : "off");
        return true;
    }

    if (CommandMatch(args, "targetline", &sub_args) || CommandMatch(args, "line", &sub_args)) {
        bool enabled = ParseOnOff(sub_args, g_config.map_target_line, true);
        SetMapBool(g_config.map_target_line, "TargetLine", enabled, true);
        Chat("NativeMap target line %s.", enabled ? "on" : "off");
        return true;
    }

    if (CommandMatch(args, "ground", &sub_args) || CommandMatch(args, "grounditems", &sub_args)) {
        bool enabled = ParseOnOff(sub_args, g_config.map_show_ground, true);
        SetMapBool(g_config.map_show_ground, "Ground", enabled, true);
        Chat("NativeMap ground labels %s.", enabled ? "on" : "off");
        return true;
    }

    if (CommandMatch(args, "vector", &sub_args) || CommandMatch(args, "vectors", &sub_args)) {
        bool enabled = ParseOnOff(sub_args, g_config.map_show_vectors, true);
        SetMapBool(g_config.map_show_vectors, "Vector", enabled, true);
        Chat("NativeMap movement vectors %s.", enabled ? "on" : "off");
        return true;
    }

    if (CommandMatch(args, "named", &sub_args)) {
        bool enabled = ParseOnOff(sub_args, g_config.map_named_only, true);
        SetMapBool(g_config.map_named_only, "Named", enabled, true);
        Chat("NativeMap named-only filter %s.", enabled ? "on" : "off");
        return true;
    }

    if (CommandMatch(args, "filter", &sub_args) || CommandMatch(args, "name", &sub_args)) {
        return HandleTextFilterCommand(sub_args, g_config.map_name_filter, "NameFilter", "name");
    }

    if (CommandMatch(args, "hide", &sub_args)) {
        return HandleTextFilterCommand(sub_args, g_config.map_hide_filter, "HideFilter", "hide");
    }

    if (CommandMatch(args, "target", &sub_args)) {
        return HandleTargetCommand(sub_args);
    }

    Chat("NativeMap commands: /nimap help, or /mapfilter help.");
    return true;
}

void PrintXTargets(bool list_entries) {
    std::vector<XTargetInfo> targets = ReadXTargets();
    Chat("XTarInfo: %s, slots=%u, map=%d, labels=%d",
        g_config.xtar_enabled ? "on" : "off",
        static_cast<unsigned>(targets.size()),
        g_config.map_show_xtargets ? 1 : 0,
        g_config.map_xtarget_labels ? 1 : 0);

    if (!list_entries) {
        return;
    }

    if (targets.empty()) {
        Chat("XTarInfo: no extended targets found.");
        return;
    }

    for (const auto& target : targets) {
        Chat("XT%d: id=%u type=%u dist=%.1f name='%s'",
            target.slot,
            target.spawn_id,
            target.type,
            target.distance,
            target.name.empty() ? "" : target.name.c_str());
    }
}

bool HandleXTarInfoCommand(const char* line) {
    const char* args = nullptr;
    if (!CommandMatch(line, "/xtarinfo", &args) && !CommandMatch(line, "/xtar", &args)) {
        return false;
    }

    if (!args || !args[0] || CommandMatch(args, "status", nullptr)) {
        PrintXTargets(false);
        return true;
    }

    if (CommandMatch(args, "help", nullptr)) {
        Chat("XTarInfo: on|off|status|list|map on/off|labels on/off");
        return true;
    }

    if (CommandMatch(args, "list", nullptr)) {
        PrintXTargets(true);
        return true;
    }

    if (CommandMatch(args, "on", nullptr) || CommandMatch(args, "off", nullptr)) {
        bool enabled = ParseOnOff(args, g_config.xtar_enabled, !g_config.xtar_enabled);
        g_config.xtar_enabled = enabled;
        SaveConfigBool("XTarget", "Enabled", enabled);
        RefreshMapOverlay();
        Chat("XTarInfo is now %s.", enabled ? "on" : "off");
        return true;
    }

    const char* sub_args = nullptr;
    if (CommandMatch(args, "map", &sub_args)) {
        bool enabled = ParseOnOff(sub_args, g_config.map_show_xtargets, !g_config.map_show_xtargets);
        SetMapBool(g_config.map_show_xtargets, "XTargets", enabled, true);
        Chat("XTarInfo map labels are now %s.", enabled ? "on" : "off");
        return true;
    }

    if (CommandMatch(args, "labels", &sub_args) || CommandMatch(args, "label", &sub_args)) {
        bool enabled = ParseOnOff(sub_args, g_config.map_xtarget_labels, !g_config.map_xtarget_labels);
        SetMapBool(g_config.map_xtarget_labels, "XTargetLabels", enabled, true);
        Chat("XTarInfo label suffixes are now %s.", enabled ? "on" : "off");
        return true;
    }

    Chat("XTarInfo commands: /xtarinfo help.");
    return true;
}

bool HandleHighlightCommand(const char* line) {
    const char* args = nullptr;
    if (!CommandMatch(line, "/highlight", &args)) {
        return false;
    }

    std::string value = TrimCopy(args);
    if (value.empty() || CommandMatch(value.c_str(), "status", nullptr)) {
        Chat("Highlight: filter='%s' color=#%06X size=%d",
            g_config.map_highlight_filter.empty() ? "" : g_config.map_highlight_filter.c_str(),
            g_config.map_highlight_color & 0x00FFFFFF,
            g_config.map_highlight_size);
        return true;
    }

    std::vector<std::string> words = SplitWords(value.c_str());
    std::string first = words.empty() ? "" : LowerCopy(words[0]);
    if (first == "clear" || first == "off" || first == "none") {
        g_config.map_highlight_filter.clear();
        SaveConfigString("Map", "HighlightFilter", g_config.map_highlight_filter);
        RefreshMapOverlay();
        Chat("Highlight filter cleared.");
        return true;
    }

    if (first == "color") {
        uint32_t color = 0;
        if (words.size() == 2 && TryParseColorToken(words[1], color)) {
            g_config.map_highlight_color = color | 0xFF000000;
            SaveConfigInt("Map", "HighlightColor", static_cast<int>(g_config.map_highlight_color & 0x00FFFFFF));
            RefreshMapOverlay();
            Chat("Highlight color set to #%06X.", g_config.map_highlight_color & 0x00FFFFFF);
            return true;
        }

        int r = 0;
        int g = 0;
        int b = 0;
        if (words.size() >= 4 && TryParseInt(words[1], r) && TryParseInt(words[2], g) && TryParseInt(words[3], b)) {
            r = clamp(r, 0, 255);
            g = clamp(g, 0, 255);
            b = clamp(b, 0, 255);
            g_config.map_highlight_color = 0xFF000000 | (static_cast<uint32_t>(r) << 16) |
                (static_cast<uint32_t>(g) << 8) | static_cast<uint32_t>(b);
            SaveConfigInt("Map", "HighlightColor", static_cast<int>(g_config.map_highlight_color & 0x00FFFFFF));
            RefreshMapOverlay();
            Chat("Highlight color set to #%06X.", g_config.map_highlight_color & 0x00FFFFFF);
            return true;
        }

        Chat("Highlight color usage: /highlight color #RRGGBB or /highlight color <r> <g> <b>");
        return true;
    }

    if (first == "size") {
        int size = 0;
        if (words.size() < 2 || !TryParseInt(words[1], size)) {
            Chat("Highlight size is %d. Usage: /highlight size <4-200>", g_config.map_highlight_size);
            return true;
        }

        g_config.map_highlight_size = clamp(size, 4, 200);
        SaveConfigInt("Map", "HighlightSize", g_config.map_highlight_size);
        RefreshMapOverlay();
        Chat("Highlight size set to %d.", g_config.map_highlight_size);
        return true;
    }

    g_config.map_highlight_filter = value;
    SaveConfigString("Map", "HighlightFilter", g_config.map_highlight_filter);
    RefreshMapOverlay();
    Chat("Highlight filter set to '%s'.", g_config.map_highlight_filter.c_str());
    return true;
}

bool HandleMapLocCommand(const char* line) {
    const char* args = nullptr;
    if (!CommandMatch(line, "/maploc", &args)) {
        return false;
    }

    std::string value = TrimCopy(args);
    if (value.empty() || CommandMatch(value.c_str(), "help", nullptr)) {
        Chat("MapLoc: add <label>|clear|list. Markers are runtime-only.");
        return true;
    }

    if (CommandMatch(value.c_str(), "clear", nullptr)) {
        g_map_locations.clear();
        RefreshMapOverlay();
        Chat("MapLoc markers cleared.");
        return true;
    }

    if (CommandMatch(value.c_str(), "list", nullptr)) {
        Chat("MapLoc markers: %u", static_cast<unsigned>(g_map_locations.size()));
        for (size_t i = 0; i < g_map_locations.size() && i < 20; ++i) {
            const auto& loc = g_map_locations[i];
            Chat("%u: %s (%.1f, %.1f, %.1f)", static_cast<unsigned>(i + 1), loc.label.c_str(), loc.x, loc.y, loc.z);
        }
        return true;
    }

    const char* label_args = nullptr;
    std::string label;
    if (CommandMatch(value.c_str(), "add", &label_args)) {
        label = TrimCopy(label_args);
    } else {
        label = value;
    }
    if (label.empty()) {
        char fallback[32]{};
        _snprintf_s(fallback, sizeof(fallback), _TRUNCATE, "Loc %u", static_cast<unsigned>(g_map_locations.size() + 1));
        label = fallback;
    }

    void* local_spawn = ReadGlobalPtr(kPInstCharSpawn);
    if (!local_spawn) {
        Chat("MapLoc failed: local spawn is unavailable.");
        return true;
    }

    MapLocation location{};
    location.label = label;
    location.x = ReadValue<float>(static_cast<BYTE*>(local_spawn) + kSpawnX, 0.0f);
    location.y = ReadValue<float>(static_cast<BYTE*>(local_spawn) + kSpawnY, 0.0f);
    location.z = ReadValue<float>(static_cast<BYTE*>(local_spawn) + kSpawnZ, 0.0f);
    g_map_locations.push_back(std::move(location));
    RefreshMapOverlay();
    Chat("MapLoc added '%s'.", label.c_str());
    return true;
}

bool HandleMapShowHideCommand(const char* line) {
    const char* args = nullptr;
    if (CommandMatch(line, "/mapshow", &args)) {
        return HandleTextFilterCommand(args, g_config.map_name_filter, "NameFilter", "name");
    }

    if (CommandMatch(line, "/maphide", &args)) {
        return HandleTextFilterCommand(args, g_config.map_hide_filter, "HideFilter", "hide");
    }

    return false;
}

void PrintGroundProbe() {
    for (uintptr_t candidate_address : kPInstEQItemListCandidates) {
        void* head = ReadGlobalPtr(candidate_address);
        void* nested = ReadPtr(head);
        std::string head_name = LooksLikeGroundItem(head)
            ? CleanSpawnName(ReadFixedString(static_cast<BYTE*>(head) + kGroundItemName, 0x20))
            : "";
        std::string nested_name = LooksLikeGroundItem(nested)
            ? CleanSpawnName(ReadFixedString(static_cast<BYTE*>(nested) + kGroundItemName, 0x20))
            : "";
        Chat("NativeMap ground pinst=%p head=%p '%s' nested=%p '%s'",
            reinterpret_cast<void*>(Rebase(candidate_address)),
            head,
            head_name.c_str(),
            nested,
            nested_name.c_str());
    }
}

bool HandleNativeInterfaceCommand(const char* line) {
    const char* args = nullptr;
    if (!CommandMatch(line, "/ni", &args) &&
        !CommandMatch(line, "/nativeinterface", &args)) {
        return false;
    }

    if (!args || !args[0] || CommandMatch(args, "help", nullptr)) {
        Chat("NativeMap: status|map|xtar|ground|windows|spawn <name-or-id>");
        Chat("NativeMap commands: /ni or /nativeinterface.");
        return true;
    }

    if (CommandMatch(args, "status", nullptr)) {
        Chat("NativeMap status: map_hook=%d cmd_hook=%d labels=%u lines=%u locs=%u",
            g_new_map_vtable ? 1 : 0,
            g_command_hook.installed ? 1 : 0,
            static_cast<unsigned>(g_map_labels.size()),
            static_cast<unsigned>(g_map_lines.size()),
            static_cast<unsigned>(g_map_locations.size()));
        Chat("NativeMap globals: char=%p self=%p target=%p map=%p spawn_mgr=%p",
            ReadGlobalPtr(kPInstCharData),
            ReadGlobalPtr(kPInstCharSpawn),
            ReadGlobalPtr(kPInstTarget),
            ReadGlobalPtr(kPInstCMapViewWnd),
            ReadGlobalPtr(kPInstSpawnManager));
        return true;
    }

    if (CommandMatch(args, "map", nullptr)) {
        RefreshMapOverlay();
        PrintMapStatus();
        return true;
    }

    if (CommandMatch(args, "xtar", nullptr) || CommandMatch(args, "xtarget", nullptr)) {
        PrintXTargets(true);
        return true;
    }

    if (CommandMatch(args, "ground", nullptr)) {
        PrintGroundProbe();
        return true;
    }

    if (CommandMatch(args, "windows", nullptr)) {
        Chat("NativeMap windows: target=%p player=%p map=%p",
            ReadGlobalPtr(kPInstCTargetWnd),
            ReadGlobalPtr(kPInstCPlayerWnd),
            ReadGlobalPtr(kPInstCMapViewWnd));
        return true;
    }

    const char* sub_args = nullptr;
    if (CommandMatch(args, "spawn", &sub_args)) {
        std::string query = TrimCopy(sub_args);
        if (query.empty()) {
            Chat("NativeMap spawn usage: /ni spawn <name-or-id>");
            return true;
        }

        std::string found_name;
        void* spawn = FindSpawnByNameOrID(query, found_name);
        if (!spawn) {
            Chat("NativeMap spawn: no match for '%s'.", query.c_str());
            return true;
        }

        void* local_spawn = ReadGlobalPtr(kPInstCharSpawn);
        uint32_t spawn_id = ReadValue<uint32_t>(static_cast<BYTE*>(spawn) + kSpawnID, 0);
        uint8_t type = ReadValue<uint8_t>(static_cast<BYTE*>(spawn) + kSpawnType, 0xFF);
        int level = ReadValue<int32_t>(static_cast<BYTE*>(spawn) + kSpawnLevel, 0);
        float x = ReadValue<float>(static_cast<BYTE*>(spawn) + kSpawnX, 0.0f);
        float y = ReadValue<float>(static_cast<BYTE*>(spawn) + kSpawnY, 0.0f);
        float z = ReadValue<float>(static_cast<BYTE*>(spawn) + kSpawnZ, 0.0f);
        float speed_x = ReadValue<float>(static_cast<BYTE*>(spawn) + kSpawnSpeedX, 0.0f);
        float speed_y = ReadValue<float>(static_cast<BYTE*>(spawn) + kSpawnSpeedY, 0.0f);
        float heading = ReadValue<float>(static_cast<BYTE*>(spawn) + kSpawnHeading, 0.0f);
        Chat("NativeMap spawn '%s': ptr=%p id=%u type=%u level=%d dist=%.1f loc=(%.1f,%.1f,%.1f) speed=(%.2f,%.2f) heading=%.2f",
            found_name.empty() ? query.c_str() : found_name.c_str(),
            spawn,
            spawn_id,
            type,
            level,
            SpawnDistance(local_spawn, spawn),
            x,
            y,
            z,
            speed_x,
            speed_y,
            heading);
        return true;
    }

    Chat("NativeMap commands: /ni help.");
    return true;
}

void __fastcall InterpretCmdDetour(void* self, void*, void* player, char* line) {
    if (HandleNativeMapCommand(line) ||
        HandleXTarInfoCommand(line) ||
        HandleHighlightCommand(line) ||
        HandleMapLocCommand(line) ||
        HandleMapShowHideCommand(line) ||
        HandleNativeInterfaceCommand(line)) {
        return;
    }

    InterpretCmdProc original = g_command_hook.original<InterpretCmdProc>();
    if (original) {
        original(self, player, line);
    }
}

void InstallCommandHook() {
    void* target = reinterpret_cast<void*>(Rebase(kCEverQuestInterpretCmd));
    if (InstallInlineHook(g_command_hook, target, reinterpret_cast<void*>(&InterpretCmdDetour), 7)) {
        Log("Installed InterpretCmd hook at %p", target);
    } else {
        Log("Failed to install InterpretCmd hook at %p", target);
    }
}

HMODULE LoadRealDInput() {
    if (g_real_dinput) {
        return g_real_dinput;
    }

    char system_dir[MAX_PATH]{};
    if (!GetSystemDirectoryA(system_dir, MAX_PATH)) {
        return nullptr;
    }

    std::string path = system_dir;
    if (!path.empty() && path.back() != '\\') {
        path.push_back('\\');
    }
    path += "dinput8.dll";

    g_real_dinput = LoadLibraryA(path.c_str());
    return g_real_dinput;
}

DWORD WINAPI WorkerThread(LPVOID) {
    Sleep(1200);
    LoadConfig();
    Log("MQ2 Map DLL starting. Map=%d", g_config.map_enabled ? 1 : 0);

    // AoTv4: command hook NOT installed here -- the host eq-core-dll already detours InterpretCmd and
    // routes /nimap etc. to nativeinterface::TryHandleCommand(). Only the map draw hook is ours.

    DWORD last_config_reload = GetTickCount();
    while (!g_shutdown) {
        DWORD now = GetTickCount();
        if (now - last_config_reload > 5000) {
            LoadConfig();
            last_config_reload = now;
        }

        if (g_config.map_enabled) {
            InstallMapHook();
        } else {
            RestoreMapHook();
        }

        Sleep(500);
    }

    return 0;
}

void Shutdown() {
    g_shutdown = true;
    RemoveInlineHook(g_command_hook);
    RestoreMapHook();
}

// AoTv4: host entry points. The eq-core-dll owns the dinput8 proxy + DllMain, so this file provides no
// exports. Start() spawns the map worker (call it once from the host's per-frame ProcessGameEvents, NOT
// from DllMain -- thread creation under loader lock is unsafe). TryHandleCommand() is called from the
// host's existing InterpretCmd detour so we don't double-hook InterpretCmd.
void Start() {
    if (g_worker) {
        return;
    }
    g_worker = CreateThread(nullptr, 0, WorkerThread, nullptr, 0, nullptr);
}

bool TryHandleCommand(const char* line) {
    return HandleNativeMapCommand(line) ||
           HandleXTarInfoCommand(line) ||
           HandleHighlightCommand(line) ||
           HandleMapLocCommand(line) ||
           HandleMapShowHideCommand(line) ||
           HandleNativeInterfaceCommand(line);
}

} // namespace nativeinterface
