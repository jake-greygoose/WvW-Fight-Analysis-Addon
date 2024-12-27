#include "utils.h"
#include "resource.h"
#include "Shared.h"
#include "Settings.h"
#include "evtc_parser.h"
#include "utils.h"
#include <thread>
#include <chrono>
#include "utils.h"
#include <zip.h>
#include <shlobj.h>
#include <sstream>
#include <filesystem>
#include <cmath>
#include <string>
#include <cstdio>

const char* ADDON_WINDOW_VISIBLE = "KB_MISTINSIGHT_WINDOW_VISIBLE";

void initMaps() {
    // Map of profession IDs to profession names
    professions = {
        {1, "Guardian"},
        {2, "Warrior"},
        {3, "Engineer"},
        {4, "Ranger"},
        {5, "Thief"},
        {6, "Elementalist"},
        {7, "Mesmer"},
        {8, "Necromancer"},
        {9, "Revenant"}
    };

    // Map of elite specialization IDs to elite specialization names
    eliteSpecs = {
        {5,  "Druid"},
        {7,  "Daredevil"},
        {18, "Berserker"},
        {27, "Dragonhunter"},
        {34, "Reaper"},
        {40, "Chronomancer"},
        {43, "Scrapper"},
        {48, "Tempest"},
        {52, "Herald"},
        {55, "Soulbeast"},
        {56, "Weaver"},
        {57, "Holosmith"},
        {58, "Deadeye"},
        {59, "Mirage"},
        {60, "Scourge"},
        {61, "Spellbreaker"},
        {62, "Firebrand"},
        {63, "Renegade"},
        {64, "Harbinger"},
        {65, "Willbender"},
        {66, "Virtuoso"},
        {67, "Catalyst"},
        {68, "Bladesworn"},
        {69, "Vindicator"},
        {70, "Mechanist"},
        {71, "Specter"},
        {72, "Untamed"}
    };

    // Map of elite specialization short names to full names
    eliteSpecShortNames = {
    {"Core Guardian", "Gdn"},
    {"Dragonhunter", "Dgh"},
    {"Firebrand", "Fbd"},
    {"Willbender", "Wbd"},
    {"Core Warrior", "War"},
    {"Berserker", "Brs"},
    {"Spellbreaker", "Spb"},
    {"Bladesworn", "Bds"},
    {"Core Engineer", "Eng"},
    {"Scrapper", "Scr"},
    {"Holosmith", "Hls"},
    {"Mechanist", "Mec"},
    {"Core Ranger", "Rgr"},
    {"Druid", "Dru"},
    {"Soulbeast", "Slb"},
    {"Untamed", "Unt"},
    {"Core Thief", "Thf"},
    {"Daredevil", "Dar"},
    {"Deadeye", "Ded"},
    {"Specter", "Spe"},
    {"Core Elementalist", "Ele"},
    {"Tempest", "Tmp"},
    {"Weaver", "Wea"},
    {"Catalyst", "Cat"},
    {"Core Mesmer", "Mes"},
    {"Chronomancer", "Chr"},
    {"Mirage", "Mir"},
    {"Virtuoso", "Vir"},
    {"Core Necromancer", "Nec"},
    {"Reaper", "Rea"},
    {"Scourge", "Scg"},
    {"Harbinger", "Har"},
    {"Core Revenant", "Rev"},
    {"Herald", "Her"},
    {"Renegade", "Ren"},
    {"Vindicator", "Vin"}
    };


    // Map of team IDs to team names
    teamIDs = {
        {697,  "Red"}, {705,  "Red"}, {706,  "Red"}, {882,  "Red"}, {2520, "Red"},
        {39,   "Green"}, {2739, "Green"}, {2741, "Green"}, {2752, "Green"}, {2763, "Green"},
        {432,  "Blue"}, {1277, "Blue"}, {1989, "Blue"}
    };

    // Map of elite specializations to professions
    eliteSpecToProfession = {
        {"Dragonhunter",    "Guardian"},
        {"Firebrand",       "Guardian"},
        {"Willbender",      "Guardian"},
        {"Core Guardian",   "Guardian"},
        {"Berserker",       "Warrior"},
        {"Spellbreaker",    "Warrior"},
        {"Bladesworn",      "Warrior"},
        {"Core Warrior",    "Warrior"},
        {"Scrapper",        "Engineer"},
        {"Holosmith",       "Engineer"},
        {"Mechanist",       "Engineer"},
        {"Core Engineer",   "Engineer"},
        {"Druid",           "Ranger"},
        {"Soulbeast",       "Ranger"},
        {"Untamed",         "Ranger"},
        {"Core Ranger",     "Ranger"},
        {"Daredevil",       "Thief"},
        {"Deadeye",         "Thief"},
        {"Specter",         "Thief"},
        {"Core Thief",      "Thief"},
        {"Tempest",         "Elementalist"},
        {"Weaver",          "Elementalist"},
        {"Catalyst",        "Elementalist"},
        {"Core Elementalist", "Elementalist"},
        {"Chronomancer",    "Mesmer"},
        {"Mirage",          "Mesmer"},
        {"Virtuoso",        "Mesmer"},
        {"Core Mesmer",     "Mesmer"},
        {"Reaper",          "Necromancer"},
        {"Scourge",         "Necromancer"},
        {"Harbinger",       "Necromancer"},
        {"Core Necromancer","Necromancer"},
        {"Herald",          "Revenant"},
        {"Renegade",        "Revenant"},
        {"Vindicator",      "Revenant"},
        {"Core Revenant",   "Revenant"}
    };

    // Map of professions to colors (ImVec4)
    professionColors = {
        {"Guardian",     ImVec4(10 / 255.0f, 222 / 255.0f, 255 / 255.0f, 110 / 255.0f)},
        {"Warrior",      ImVec4(255 / 255.0f, 212 / 255.0f, 61 / 255.0f, 110 / 255.0f)},
        {"Engineer",     ImVec4(227 / 255.0f, 115 / 255.0f, 41 / 255.0f, 110 / 255.0f)},
        {"Ranger",       ImVec4(135 / 255.0f, 222 / 255.0f, 10 / 255.0f, 110 / 255.0f)},
        {"Thief",        ImVec4(227 / 255.0f, 94 / 255.0f, 115 / 255.0f, 115 / 255.0f)},
        {"Elementalist", ImVec4(247 / 255.0f, 56 / 255.0f, 56 / 255.0f, 110 / 255.0f)},
        {"Mesmer",       ImVec4(204 / 255.0f, 59 / 255.0f, 209 / 255.0f, 110 / 255.0f)},
        {"Necromancer",  ImVec4(5 / 255.0f, 227 / 255.0f, 125 / 255.0f, 110 / 255.0f)},
        {"Revenant",     ImVec4(161 / 255.0f, 41 / 255.0f, 41 / 255.0f, 115 / 255.0f)}
    };
}

struct ProfessionColor {
    std::string name;
    ImVec4 primaryColor;
    ImVec4 secondaryColor;
};

std::vector<ProfessionColor> professionColorPair = {
    {
        "Guardian",
        ImVec4(10 / 255.0f, 222 / 255.0f, 255 / 255.0f, 110 / 255.0f),
        ImVec4(10 / 255.0f, 222 / 255.0f, 255 / 255.0f, 54 / 255.0f) 
    },
    {
        "Warrior",
        ImVec4(255 / 255.0f, 212 / 255.0f, 61 / 255.0f, 110 / 255.0f),
        ImVec4(255 / 255.0f, 212 / 255.0f, 61 / 255.0f, 54 / 255.0f)
    },
    {
        "Engineer",
        ImVec4(227 / 255.0f, 115 / 255.0f, 41 / 255.0f, 110 / 255.0f),
        ImVec4(227 / 255.0f, 115 / 255.0f, 41 / 255.0f, 54 / 255.0f)
    },
    {
        "Ranger",
        ImVec4(135 / 255.0f, 222 / 255.0f, 10 / 255.0f, 110 / 255.0f),
        ImVec4(135 / 255.0f, 222 / 255.0f, 10 / 255.0f, 54 / 255.0f)
    },
    {
        "Thief",
        ImVec4(227 / 255.0f, 94 / 255.0f, 115 / 255.0f, 115 / 255.0f),
        ImVec4(227 / 255.0f, 94 / 255.0f, 115 / 255.0f, 71 / 255.0f)
    },
    {
        "Elementalist",
        ImVec4(247 / 255.0f, 56 / 255.0f, 56 / 255.0f, 110 / 255.0f),
        ImVec4(247 / 255.0f, 56 / 255.0f, 56 / 255.0f, 54 / 255.0f)
    },
    {
        "Mesmer",
        ImVec4(204 / 255.0f, 59 / 255.0f, 209 / 255.0f, 110 / 255.0f),
        ImVec4(204 / 255.0f, 59 / 255.0f, 209 / 255.0f, 54 / 255.0f)
    },
    {
        "Necromancer",
        ImVec4(5 / 255.0f, 227 / 255.0f, 125 / 255.0f, 110 / 255.0f),
        ImVec4(5 / 255.0f, 227 / 255.0f, 125 / 255.0f, 54 / 255.0f)
    },
    {
        "Revenant",
        ImVec4(161 / 255.0f, 41 / 255.0f, 41 / 255.0f, 115 / 255.0f),
        ImVec4(161 / 255.0f, 41 / 255.0f, 41 / 255.0f, 71 / 255.0f)
    }
};


std::unordered_map<std::string, TextureInfo> textureMap = {
    {"Berserker", {BERSERKER, &Berserker}},
    {"Bladesworn", {BLADESWORN, &Bladesworn}},
    {"Catalyst", {CATALYST, &Catalyst}},
    {"Chronomancer", {CHRONOMANCER, &Chronomancer}},
    {"Daredevil", {DAREDEVIL, &Daredevil}},
    {"Deadeye", {DEADEYE, &Deadeye}},
    {"Dragonhunter", {DRAGONHUNTER, &Dragonhunter}},
    {"Druid", {DRUID, &Druid}},
    {"Core Elementalist", {ELEMENTALIST, &Elementalist}},
    {"Core Engineer", {ENGINEER, &Engineer}},
    {"Firebrand", {FIREBRAND, &Firebrand}},
    {"Core Guardian", {GUARDIAN, &Guardian}},
    {"Harbinger", {HARBINGER, &Harbinger}},
    {"Herald", {HERALD, &Herald}},
    {"Holosmith", {HOLOSMITH, &Holosmith}},
    {"Mechanist", {MECHANIST, &Mechanist}},
    {"Core Mesmer", {MESMER, &Mesmer}},
    {"Mirage", {MIRAGE, &Mirage}},
    {"Core Necromancer", {NECROMANCER, &Necromancer}},
    {"Core Ranger", {RANGER, &Ranger}},
    {"Reaper", {REAPER, &Reaper}},
    {"Renegade", {RENEGADE, &Renegade}},
    {"Core Revenant", {REVENANT, &Revenant}},
    {"Scrapper", {SCRAPPER, &Scrapper}},
    {"Scourge", {SCOURGE, &Scourge}},
    {"Soulbeast", {SOULBEAST, &Soulbeast}},
    {"Specter", {SPECTER, &Specter}},
    {"Spellbreaker", {SPELLBREAKER, &Spellbreaker}},
    {"Tempest", {TEMPEST, &Tempest}},
    {"Core Thief", {THIEF, &Thief}},
    {"Untamed", {UNTAMED, &Untamed}},
    {"Vindicator", {VINDICATOR, &Vindicator}},
    {"Virtuoso", {VIRTUOSO, &Virtuoso}},
    {"Core Warrior", {WARRIOR, &Warrior}},
    {"Weaver", {WEAVER, &Weaver}},
    {"Willbender", {WILLBENDER, &Willbender}}
};

ImVec4 GetTeamColor(const std::string& teamName)
{
    if (teamName == "Red")
        return ImGui::ColorConvertU32ToFloat4(IM_COL32(0xff, 0x44, 0x44, 0xff)); // Red
    else if (teamName == "Blue")
        return ImGui::ColorConvertU32ToFloat4(IM_COL32(0x33, 0xb5, 0xe5, 0xff)); // Blue
    else if (teamName == "Green")
        return ImGui::ColorConvertU32ToFloat4(IM_COL32(0x99, 0xcc, 0x00, 0xff)); // Green
    else
        return ImGui::GetStyleColorVec4(ImGuiCol_Text); // Default text color
}

std::string generateLogDisplayName(const std::string& filename, uint64_t combatStartMs, uint64_t combatEndMs) 
{

    size_t lastDotPosition = filename.find_last_of('.');
    std::string filenameWithoutExt = (lastDotPosition != std::string::npos)
        ? filename.substr(0, lastDotPosition)
        : filename;

    uint64_t durationMs = (combatEndMs >= combatStartMs) ? (combatEndMs - combatStartMs) : 0;
    std::chrono::milliseconds duration(durationMs);
    int minutes = std::chrono::duration_cast<std::chrono::minutes>(duration).count();
    int seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count() % 60;

    std::string displayName = filenameWithoutExt + " ("
        + std::to_string(minutes) + "m "
        + std::to_string(seconds) + "s)";

    return displayName;
}

std::string formatDuration(uint64_t milliseconds) {
    std::chrono::milliseconds duration(milliseconds);
    int minutes = std::chrono::duration_cast<std::chrono::minutes>(duration).count();
    int seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count() % 60;

    return std::to_string(minutes) + "m " + std::to_string(seconds) + "s";
}

bool isRunningUnderWine()
{
    if (Settings::forceLinuxCompatibilityMode) {
        return true;
    }
    HKEY hKey;
    LONG result = RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Wine", 0, KEY_READ, &hKey);
    if (result == ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        return true;
    }
    else
    {
        result = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "Software\\Wine", 0, KEY_READ, &hKey);
        if (result == ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            return true;
        }
    }

    HMODULE hNtDll = GetModuleHandleA("ntdll.dll");
    if (hNtDll)
    {
        FARPROC wine_get_version = GetProcAddress(hNtDll, "wine_get_version");
        if (wine_get_version)
        {
            return true;
        }
    }

    return false;
}

Texture** getTextureInfo(const std::string& eliteSpec, int* outResourceId) {
    auto it = textureMap.find(eliteSpec);
    if (it != textureMap.end()) {
        *outResourceId = it->second.resourceId;
        return it->second.texture;
    }
    else {
        *outResourceId = 0;
        return nullptr;
    }
}

std::string formatDamage(double damage) {
    if (damage >= 1'000'000.0) {
        if (std::fmod(damage, 1'000'000.0) == 0.0) {
            return std::to_string(static_cast<int>(damage / 1'000'000.0)) + "M";
        }
        else {
            char buffer[10];
            std::snprintf(buffer, sizeof(buffer), "%.1fM", damage / 1'000'000.0);
            if (buffer[strlen(buffer) - 3] == '.' && buffer[strlen(buffer) - 2] == '0') {
                buffer[strlen(buffer) - 3] = 'M';  // Overwrite '.' with 'M'
                buffer[strlen(buffer) - 2] = '\0'; // Terminate string after 'M'
            }
            return std::string(buffer);
        }
    }
    else if (damage >= 1'000.0) {
        if (std::fmod(damage, 1'000.0) == 0.0) {
            return std::to_string(static_cast<int>(damage / 1'000.0)) + "k";
        }
        else {
            char buffer[10];
            std::snprintf(buffer, sizeof(buffer), "%.1fk", damage / 1'000.0);
            if (buffer[strlen(buffer) - 3] == '.' && buffer[strlen(buffer) - 2] == '0') {
                buffer[strlen(buffer) - 3] = 'k';  // Overwrite '.' with 'k'
                buffer[strlen(buffer) - 2] = '\0'; // Terminate string after 'k'
            }
            return std::string(buffer);
        }
    }
    else {
        return std::to_string(static_cast<int>(damage));
    }
}


void waitForFile(const std::string& filePath)
{
    HANDLE hFile = INVALID_HANDLE_VALUE;
    DWORD previousSize = 0;
    //APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME,
    //    ("Waiting for file: " + filePath).c_str());
    while (true)
    {
        hFile = CreateFile(
            filePath.c_str(),
            GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            nullptr,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            nullptr);

        if (hFile == INVALID_HANDLE_VALUE)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        DWORD currentSize = GetFileSize(hFile, nullptr);

        if (currentSize == previousSize && currentSize != INVALID_FILE_SIZE)
        {
            CloseHandle(hFile);
            break;
        }

        previousSize = currentSize;
        CloseHandle(hFile);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

std::vector<char> extractZipFile(const std::string& filePath) {
    int err = 0;
    zip* z = zip_open(filePath.c_str(), 0, &err);
    if (!z) {
        APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME, ("Failed to open zip file. Error code: " + std::to_string(err)).c_str());
        return std::vector<char>();
    }

    zip_stat_t zstat;
    zip_stat_init(&zstat);
    zip_stat_index(z, 0, 0, &zstat);

    std::vector<char> buffer(zstat.size);
    zip_file* f = zip_fopen_index(z, 0, 0);
    if (!f) {
        zip_close(z);
        APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME, "Failed to open file in zip");
        return std::vector<char>();
    }

    zip_fread(f, buffer.data(), zstat.size);
    zip_fclose(f);
    zip_close(z);

    return buffer;
}

std::filesystem::path getArcPath()
{
    std::filesystem::path filename = APIDefs->Paths.GetAddonDirectory("arcdps\\arcdps.ini");

    char buffer[256] = { 0 };

    GetPrivateProfileStringA(
        "session",
        "boss_encounter_path",
        "",
        buffer,
        sizeof(buffer),
        filename.string().c_str()
    );

    std::filesystem::path boss_encounter_path = buffer;

    return boss_encounter_path;
}