#include "settings/Settings.h"
#include "shared/Shared.h"
#include "utils/Utils.h"
#include "resource.h"
#include "settings/Settings.h"
#include "evtc_parser.h"
#include <thread>
#include <chrono>
#include <shlobj.h>
#include <sstream>
#include <filesystem>
#include <cmath>
#include <string>
#include <cstdio>
#include <array>
#include <algorithm>


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
        {72, "Untamed"},
        {73, "Troubadour"},
        {74, "Paragon"},
        {75, "Amalgam"},
        {76, "Ritualist"},
        {77, "Antiquary"},
        {78, "Galeshot"},
        {79, "Conduit"},
        {80, "Evoker"},
        {81, "Luminary"}
    };

    // Map of elite specialization short names to full names
    eliteSpecShortNames = {
    {"Core Guardian", "Gdn"},
    {"Dragonhunter", "Dgh"},
    {"Firebrand", "Fbd"},
    {"Willbender", "Wbd"},
    {"Luminary", "Lum"},
    {"Core Warrior", "War"},
    {"Berserker", "Brs"},
    {"Spellbreaker", "Spb"},
    {"Bladesworn", "Bds"},
    {"Paragon", "Par"},
    {"Core Engineer", "Eng"},
    {"Scrapper", "Scr"},
    {"Holosmith", "Hls"},
    {"Mechanist", "Mec"},
    {"Amalgam", "Amg"},
    {"Core Ranger", "Rgr"},
    {"Druid", "Dru"},
    {"Soulbeast", "Slb"},
    {"Untamed", "Unt"},
    {"Galeshot", "Gsh"},
    {"Core Thief", "Thf"},
    {"Daredevil", "Dar"},
    {"Deadeye", "Ded"},
    {"Specter", "Spe"},
    {"Antiquary", "Ant"},
    {"Core Elementalist", "Ele"},
    {"Tempest", "Tmp"},
    {"Weaver", "Wea"},
    {"Catalyst", "Cat"},
    {"Evoker", "Evo"},
    {"Core Mesmer", "Mes"},
    {"Chronomancer", "Chr"},
    {"Mirage", "Mir"},
    {"Virtuoso", "Vir"},
    {"Troubadour", "Trb"},
    {"Core Necromancer", "Nec"},
    {"Reaper", "Rea"},
    {"Scourge", "Scg"},
    {"Harbinger", "Har"},
    {"Ritualist", "Rit"},
    {"Core Revenant", "Rev"},
    {"Herald", "Her"},
    {"Renegade", "Ren"},
    {"Vindicator", "Vin"},
    {"Conduit", "Con"}
    };

    // Map of elite specializations to professions
    eliteSpecToProfession = {
        {"Dragonhunter",    "Guardian"},
        {"Firebrand",       "Guardian"},
        {"Willbender",      "Guardian"},
        {"Luminary",        "Guardian"},
        {"Core Guardian",   "Guardian"},
        {"Berserker",       "Warrior"},
        {"Spellbreaker",    "Warrior"},
        {"Bladesworn",      "Warrior"},
        {"Paragon",         "Warrior"},
        {"Core Warrior",    "Warrior"},
        {"Scrapper",        "Engineer"},
        {"Holosmith",       "Engineer"},
        {"Mechanist",       "Engineer"},
        {"Amalgam",         "Engineer"},
        {"Core Engineer",   "Engineer"},
        {"Druid",           "Ranger"},
        {"Soulbeast",       "Ranger"},
        {"Untamed",         "Ranger"},
        {"Galeshot",        "Ranger"},
        {"Core Ranger",     "Ranger"},
        {"Daredevil",       "Thief"},
        {"Deadeye",         "Thief"},
        {"Specter",         "Thief"},
        {"Antiquary",       "Thief"},
        {"Core Thief",      "Thief"},
        {"Tempest",         "Elementalist"},
        {"Weaver",          "Elementalist"},
        {"Catalyst",        "Elementalist"},
        {"Evoker",          "Elementalist"},
        {"Core Elementalist", "Elementalist"},
        {"Chronomancer",    "Mesmer"},
        {"Mirage",          "Mesmer"},
        {"Virtuoso",        "Mesmer"},
        {"Troubadour",      "Mesmer"},
        {"Core Mesmer",     "Mesmer"},
        {"Reaper",          "Necromancer"},
        {"Scourge",         "Necromancer"},
        {"Harbinger",       "Necromancer"},
        {"Ritualist",       "Necromancer"},
        {"Core Necromancer","Necromancer"},
        {"Herald",          "Revenant"},
        {"Renegade",        "Revenant"},
        {"Vindicator",      "Revenant"},
        {"Conduit",         "Revenant"},
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


const std::vector<ProfessionColor> professionColorPair = {
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
    {"Willbender", {WILLBENDER, &Willbender}},
    {"Amalgam", {AMALGAM, &Amalgam}},
    {"Antiquary", {ANTIQUARY, &Antiquary}},
    {"Conduit", {CONDUIT, &Conduit}},
    {"Evoker", {EVOKER, &Evoker}},
    {"Galeshot", {GALESHOT, &Galeshot}},
    {"Luminary", {LUMINARY, &Luminary}},
    {"Paragon", {PARAGON, &Paragon}},
    {"Ritualist", {RITUALIST, &Ritualist}},
    {"Troubadour", {TROUBADOUR, &Troubadour}}
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

void LogMessage(ELogLevel level, const char* msg) {
    if (level == ELogLevel_DEBUG && !Settings::debugStringsMode) {
        return;
    }
    APIDefs->Log(level, ADDON_NAME, msg);
}

void LogMessage(ELogLevel level, const std::string& msg) {
    LogMessage(level, msg.c_str());
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


Texture** getTextureInfo(const std::string& eliteSpec, int* outResourceId) {
    auto it = textureMap.find(eliteSpec);
    if (it != textureMap.end()) {
        const int iconStyle = std::clamp(Settings::scrapperIconStyle, 0, 3);
        std::array<int, 4> styleIds = { it->second.resourceId, it->second.resourceId, it->second.resourceId, it->second.resourceId };
        switch (it->second.resourceId) {
        case BERSERKER: styleIds = std::array<int, 4>{ BERSERKER_SIMPLE_WHITE, BERSERKER_WHITE, BERSERKER, BERSERKER_SIMPLE }; break;
        case BLADESWORN: styleIds = std::array<int, 4>{ BLADESWORN_SIMPLE_WHITE, BLADESWORN_WHITE, BLADESWORN, BLADESWORN_SIMPLE }; break;
        case CATALYST: styleIds = std::array<int, 4>{ CATALYST_SIMPLE_WHITE, CATALYST_WHITE, CATALYST, CATALYST_SIMPLE }; break;
        case CHRONOMANCER: styleIds = std::array<int, 4>{ CHRONOMANCER_SIMPLE_WHITE, CHRONOMANCER_WHITE, CHRONOMANCER, CHRONOMANCER_SIMPLE }; break;
        case DAREDEVIL: styleIds = std::array<int, 4>{ DAREDEVIL_SIMPLE_WHITE, DAREDEVIL_WHITE, DAREDEVIL, DAREDEVIL_SIMPLE }; break;
        case DEADEYE: styleIds = std::array<int, 4>{ DEADEYE_SIMPLE_WHITE, DEADEYE_WHITE, DEADEYE, DEADEYE_SIMPLE }; break;
        case DRAGONHUNTER: styleIds = std::array<int, 4>{ DRAGONHUNTER_SIMPLE_WHITE, DRAGONHUNTER_WHITE, DRAGONHUNTER, DRAGONHUNTER_SIMPLE }; break;
        case DRUID: styleIds = std::array<int, 4>{ DRUID_SIMPLE_WHITE, DRUID_WHITE, DRUID, DRUID_SIMPLE }; break;
        case ELEMENTALIST: styleIds = std::array<int, 4>{ ELEMENTALIST_SIMPLE_WHITE, ELEMENTALIST_WHITE, ELEMENTALIST, ELEMENTALIST_SIMPLE }; break;
        case ENGINEER: styleIds = std::array<int, 4>{ ENGINEER_SIMPLE_WHITE, ENGINEER_WHITE, ENGINEER, ENGINEER_SIMPLE }; break;
        case FIREBRAND: styleIds = std::array<int, 4>{ FIREBRAND_SIMPLE_WHITE, FIREBRAND_WHITE, FIREBRAND, FIREBRAND_SIMPLE }; break;
        case GUARDIAN: styleIds = std::array<int, 4>{ GUARDIAN_SIMPLE_WHITE, GUARDIAN_WHITE, GUARDIAN, GUARDIAN_SIMPLE }; break;
        case HARBINGER: styleIds = std::array<int, 4>{ HARBINGER_SIMPLE_WHITE, HARBINGER_WHITE, HARBINGER, HARBINGER_SIMPLE }; break;
        case HERALD: styleIds = std::array<int, 4>{ HERALD_SIMPLE_WHITE, HERALD_WHITE, HERALD, HERALD_SIMPLE }; break;
        case HOLOSMITH: styleIds = std::array<int, 4>{ HOLOSMITH_SIMPLE_WHITE, HOLOSMITH_WHITE, HOLOSMITH, HOLOSMITH_SIMPLE }; break;
        case MECHANIST: styleIds = std::array<int, 4>{ MECHANIST_SIMPLE_WHITE, MECHANIST_WHITE, MECHANIST, MECHANIST_SIMPLE }; break;
        case MESMER: styleIds = std::array<int, 4>{ MESMER_SIMPLE_WHITE, MESMER_WHITE, MESMER, MESMER_SIMPLE }; break;
        case MIRAGE: styleIds = std::array<int, 4>{ MIRAGE_SIMPLE_WHITE, MIRAGE_WHITE, MIRAGE, MIRAGE_SIMPLE }; break;
        case NECROMANCER: styleIds = std::array<int, 4>{ NECROMANCER_SIMPLE_WHITE, NECROMANCER_WHITE, NECROMANCER, NECROMANCER_SIMPLE }; break;
        case RANGER: styleIds = std::array<int, 4>{ RANGER_SIMPLE_WHITE, RANGER_WHITE, RANGER, RANGER_SIMPLE }; break;
        case REAPER: styleIds = std::array<int, 4>{ REAPER_SIMPLE_WHITE, REAPER_WHITE, REAPER, REAPER_SIMPLE }; break;
        case RENEGADE: styleIds = std::array<int, 4>{ RENEGADE_SIMPLE_WHITE, RENEGADE_WHITE, RENEGADE, RENEGADE_SIMPLE }; break;
        case REVENANT: styleIds = std::array<int, 4>{ REVENANT_SIMPLE_WHITE, REVENANT_WHITE, REVENANT, REVENANT_SIMPLE }; break;
        case SCRAPPER: styleIds = std::array<int, 4>{ SCRAPPER_SIMPLE_WHITE, SCRAPPER_WHITE, SCRAPPER, SCRAPPER_SIMPLE }; break;
        case SCOURGE: styleIds = std::array<int, 4>{ SCOURGE_SIMPLE_WHITE, SCOURGE_WHITE, SCOURGE, SCOURGE_SIMPLE }; break;
        case SOULBEAST: styleIds = std::array<int, 4>{ SOULBEAST_SIMPLE_WHITE, SOULBEAST_WHITE, SOULBEAST, SOULBEAST_SIMPLE }; break;
        case SPECTER: styleIds = std::array<int, 4>{ SPECTER_SIMPLE_WHITE, SPECTER_WHITE, SPECTER, SPECTER_SIMPLE }; break;
        case SPELLBREAKER: styleIds = std::array<int, 4>{ SPELLBREAKER_SIMPLE_WHITE, SPELLBREAKER_WHITE, SPELLBREAKER, SPELLBREAKER_SIMPLE }; break;
        case TEMPEST: styleIds = std::array<int, 4>{ TEMPEST_SIMPLE_WHITE, TEMPEST_WHITE, TEMPEST, TEMPEST_SIMPLE }; break;
        case THIEF: styleIds = std::array<int, 4>{ THIEF_SIMPLE_WHITE, THIEF_WHITE, THIEF, THIEF_SIMPLE }; break;
        case UNTAMED: styleIds = std::array<int, 4>{ UNTAMED_SIMPLE_WHITE, UNTAMED_WHITE, UNTAMED, UNTAMED_SIMPLE }; break;
        case VINDICATOR: styleIds = std::array<int, 4>{ VINDICATOR_SIMPLE_WHITE, VINDICATOR_WHITE, VINDICATOR, VINDICATOR_SIMPLE }; break;
        case VIRTUOSO: styleIds = std::array<int, 4>{ VIRTUOSO_SIMPLE_WHITE, VIRTUOSO_WHITE, VIRTUOSO, VIRTUOSO_SIMPLE }; break;
        case WARRIOR: styleIds = std::array<int, 4>{ WARRIOR_SIMPLE_WHITE, WARRIOR_WHITE, WARRIOR, WARRIOR_SIMPLE }; break;
        case WEAVER: styleIds = std::array<int, 4>{ WEAVER_SIMPLE_WHITE, WEAVER_WHITE, WEAVER, WEAVER_SIMPLE }; break;
        case WILLBENDER: styleIds = std::array<int, 4>{ WILLBENDER_SIMPLE_WHITE, WILLBENDER_WHITE, WILLBENDER, WILLBENDER_SIMPLE }; break;
        case AMALGAM: styleIds = std::array<int, 4>{ AMALGAM_SIMPLE_WHITE, AMALGAM_WHITE, AMALGAM, AMALGAM_SIMPLE }; break;
        case ANTIQUARY: styleIds = std::array<int, 4>{ ANTIQUARY_SIMPLE_WHITE, ANTIQUARY_WHITE, ANTIQUARY, ANTIQUARY_SIMPLE }; break;
        case CONDUIT: styleIds = std::array<int, 4>{ CONDUIT_SIMPLE_WHITE, CONDUIT_WHITE, CONDUIT, CONDUIT_SIMPLE }; break;
        case EVOKER: styleIds = std::array<int, 4>{ EVOKER_SIMPLE_WHITE, EVOKER_WHITE, EVOKER, EVOKER_SIMPLE }; break;
        case GALESHOT: styleIds = std::array<int, 4>{ GALESHOT_SIMPLE_WHITE, GALESHOT_WHITE, GALESHOT, GALESHOT_SIMPLE }; break;
        case LUMINARY: styleIds = std::array<int, 4>{ LUMINARY_SIMPLE_WHITE, LUMINARY_WHITE, LUMINARY, LUMINARY_SIMPLE }; break;
        case PARAGON: styleIds = std::array<int, 4>{ PARAGON_SIMPLE_WHITE, PARAGON_WHITE, PARAGON, PARAGON_SIMPLE }; break;
        case RITUALIST: styleIds = std::array<int, 4>{ RITUALIST_SIMPLE_WHITE, RITUALIST_WHITE, RITUALIST, RITUALIST_SIMPLE }; break;
        case TROUBADOUR: styleIds = std::array<int, 4>{ TROUBADOUR_SIMPLE_WHITE, TROUBADOUR_WHITE, TROUBADOUR, TROUBADOUR_SIMPLE }; break;
        default: break;
        }
        *outResourceId = styleIds[iconStyle];
        return it->second.texture;
    }
    else {
        *outResourceId = 0;
        return nullptr;
    }
}

void InvalidateProfessionIconTextures() {
    for (auto& [_, textureInfo] : textureMap) {
        if (textureInfo.texture) {
            *textureInfo.texture = nullptr;
        }
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



void RegisterWindowForNexusEsc(BaseWindowSettings* window, const std::string& defaultName) {
    if (window && window->useNexusEscClose) {
        const std::string& identifier = window->getDisplayName(defaultName);
        APIDefs->UI.RegisterCloseOnEscape(identifier.c_str(), &window->isEnabled);
    }
}

void UnregisterWindowFromNexusEsc(BaseWindowSettings* window, const std::string& defaultName) {
    if (window) {
        const std::string& identifier = window->getDisplayName(defaultName);
        APIDefs->UI.DeregisterCloseOnEscape(identifier.c_str());
    }
}


uint64_t getSortValue(const std::string& sortCriteria, const SpecStats& stats, bool vsLogPlayers) {
    if (sortCriteria == "players") {
        return stats.count;
    }
    else if (sortCriteria == "damage") {
        return vsLogPlayers ? stats.totalDamageVsPlayers : stats.totalDamage;
    }
    else if (sortCriteria == "down cont") {
        return vsLogPlayers ? stats.totalDownedContributionVsPlayers : stats.totalDownedContribution;
    }
    else if (sortCriteria == "kill cont") {
        return vsLogPlayers ? stats.totalKillContributionVsPlayers : stats.totalKillContribution;
    }
    else if (sortCriteria == "deaths") {
        return stats.totalDeaths;
    }
    else if (sortCriteria == "downs") {
        return stats.totalDowned;
    }
    return 0;
}

uint64_t getBarValue(const std::string& representation, const SpecStats& stats, bool vsLogPlayers) {
    if (representation == "players") {
        return stats.count;
    }
    else if (representation == "damage") {
        return vsLogPlayers ? stats.totalDamageVsPlayers : stats.totalDamage;
    }
    else if (representation == "down cont") {
        return vsLogPlayers ? stats.totalDownedContributionVsPlayers : stats.totalDownedContribution;
    }
    else if (representation == "kill cont") {
        return vsLogPlayers ? stats.totalKillContributionVsPlayers : stats.totalKillContribution;
    }
    else if (representation == "deaths") {
        return stats.totalDeaths;
    }
    else if (representation == "downs") {
        return stats.totalDowned;
    }
    return 0;
}

std::pair<uint64_t, uint64_t> getSecondaryBarValues(
    const std::string& barRep,
    const SpecStats& stats,
    bool vsLogPlayers
) {
    if (barRep == "damage") {
        uint64_t primaryValue = vsLogPlayers ? stats.totalDamageVsPlayers : stats.totalDamage;
        uint64_t secondaryValue = vsLogPlayers ? stats.totalDownedContributionVsPlayers : stats.totalDownedContribution;
        return { primaryValue, secondaryValue };
    }
    else if (barRep == "down cont") {
        uint64_t primaryValue = vsLogPlayers ? stats.totalDownedContributionVsPlayers : stats.totalDownedContribution;
        uint64_t secondaryValue = vsLogPlayers ? stats.totalKillContributionVsPlayers : stats.totalKillContribution;
        return { primaryValue, secondaryValue };
    }
    return { 0, 0 };
}

std::function<bool(
    const std::pair<std::string, SpecStats>&,
    const std::pair<std::string, SpecStats>&
    )>
    getSpecSortComparator(const std::string& sortCriteria, bool vsLogPlayers)
{
    return [sortCriteria, vsLogPlayers](
        const std::pair<std::string, SpecStats>& a,
        const std::pair<std::string, SpecStats>& b)
        {
            uint64_t valueA = getSortValue(sortCriteria, a.second, vsLogPlayers);
            uint64_t valueB = getSortValue(sortCriteria, b.second, vsLogPlayers);

            if (valueA != valueB) {
                return valueA > valueB;
            }

            uint64_t damageA = vsLogPlayers ?
                a.second.totalDamageVsPlayers : a.second.totalDamage;
            uint64_t damageB = vsLogPlayers ?
                b.second.totalDamageVsPlayers : b.second.totalDamage;

            if (damageA != damageB) {
                return damageA > damageB;
            }

            if (a.second.count != b.second.count) {
                return a.second.count > b.second.count;
            }

            return a.first < b.first;
        };
}
