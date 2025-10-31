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
