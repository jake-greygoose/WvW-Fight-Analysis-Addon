#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <Windows.h>
#include "imgui/imgui.h"


class Texture;
class BaseWindowSettings;
struct SpecStats;

// Function declarations
ImVec4 GetTeamColor(const std::string& teamName);

void initMaps();
void waitForFile(const std::string& filePath);
std::filesystem::path getArcPath();

Texture** getTextureInfo(const std::string& eliteSpec, int* outResourceId);
std::vector<char> extractZipFile(const std::string& filePath);
std::string CP1252_to_UTF8(const std::string& byte_array);
std::string formatDamage(double damage);
std::string generateLogDisplayName(const std::string& filename, uint64_t combatStartMs, uint64_t combatEndMs);
std::string formatDuration(uint64_t milliseconds);
bool isRunningUnderWine();

struct ProfessionColor {
    std::string name;
    ImVec4 primaryColor;
    ImVec4 secondaryColor;
};
extern const std::vector<ProfessionColor> professionColorPair;

void RegisterWindowForNexusEsc(BaseWindowSettings* window, const std::string& defaultName);
void UnregisterWindowFromNexusEsc(BaseWindowSettings* window, const std::string& defaultName);

uint64_t getSortValue(const std::string& sortCriteria, const SpecStats& stats, bool vsLogPlayers);
uint64_t getBarValue(const std::string& representation, const SpecStats& stats, bool vsLogPlayers);
std::pair<uint64_t, uint64_t> getSecondaryBarValues(
    const std::string& barRep,
    const SpecStats& stats,
    bool vsLogPlayers
);

std::function<bool(
    const std::pair<std::string, SpecStats>&,
    const std::pair<std::string, SpecStats>&
    )>
    getSpecSortComparator(const std::string& sortCriteria, bool vsLogPlayers);

void LogMessage(ELogLevel level, const char* msg);
void LogMessage(ELogLevel level, const std::string& msg);