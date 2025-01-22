#include "Shared.h"
#include <deque>
#include <filesystem>


// Existing definitions
HMODULE hSelf = nullptr;
AddonAPI* APIDefs = nullptr;
HWND GameHandle = nullptr;
Mumble::Data* MumbleLink = nullptr;
NexusLinkData* NexusLink = nullptr;
bool firstInstall = false;
std::atomic<bool> isRestartInProgress(false);
std::filesystem::path SettingsPath;
std::filesystem::path AddonPath;
std::filesystem::path GW2Root;


// Texture definitions
Texture* Berserker = nullptr;
Texture* Bladesworn = nullptr;
Texture* Catalyst = nullptr;
Texture* Chronomancer = nullptr;
Texture* Daredevil = nullptr;
Texture* Deadeye = nullptr;
Texture* Dragonhunter = nullptr;
Texture* Druid = nullptr;
Texture* Elementalist = nullptr;
Texture* Engineer = nullptr;
Texture* Firebrand = nullptr;
Texture* Guardian = nullptr;
Texture* Harbinger = nullptr;
Texture* Herald = nullptr;
Texture* Holosmith = nullptr;
Texture* Mechanist = nullptr;
Texture* Mesmer = nullptr;
Texture* Mirage = nullptr;
Texture* Necromancer = nullptr;
Texture* Ranger = nullptr;
Texture* Reaper = nullptr;
Texture* Renegade = nullptr;
Texture* Revenant = nullptr;
Texture* Scrapper = nullptr;
Texture* Scourge = nullptr;
Texture* Soulbeast = nullptr;
Texture* Specter = nullptr;
Texture* Spellbreaker = nullptr;
Texture* Tempest = nullptr;
Texture* Thief = nullptr;
Texture* Untamed = nullptr;
Texture* Vindicator = nullptr;
Texture* Virtuoso = nullptr;
Texture* Warrior = nullptr;
Texture* Weaver = nullptr;
Texture* Willbender = nullptr;
Texture* Death = nullptr;
Texture* Downed = nullptr;
Texture* Squad = nullptr;
Texture* Damage = nullptr;
Texture* Condi = nullptr;
Texture* Strike = nullptr;
Texture* Kdr = nullptr;
Texture* Home = nullptr;
Texture* Downcont = nullptr;
Texture* Killcont = nullptr;

// New definitions
std::atomic<bool> initialParsingComplete{ false };
std::atomic<bool> stopMonitoring{ false };
std::mutex parsedLogsMutex;
std::mutex processedFilesMutex;
std::thread initialParsingThread;
std::thread directoryMonitorThread;
int currentLogIndex = 0;

std::deque<ParsedLog> parsedLogs;

// Maps
std::unordered_map<int, std::string> professions;
std::unordered_map<int, std::string> eliteSpecs;
std::unordered_map<int, std::string> teamIDs;
std::unordered_map<std::string, std::string> eliteSpecToProfession;
std::unordered_map<std::string, std::string> eliteSpecShortNames;
std::unordered_map<std::string, ImVec4> professionColors;
GlobalAggregateStats globalAggregateStats;
CachedAverages cachedAverages;
std::mutex aggregateStatsMutex;

// Constants
const char* const ADDON_NAME = "WvW Fight Analysis";
const char* KB_WINDOW_TOGGLEVISIBLE = "KB_WINDOW_TOGGLEVISIBLE";