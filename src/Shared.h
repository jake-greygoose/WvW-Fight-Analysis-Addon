#pragma once
#include <Windows.h>
#include <filesystem>
#include <string>
#include <vector>
#include <unordered_map>
#include <atomic>
#include <mutex>
#include <thread>
#include "nexus/Nexus.h"
#include "mumble/Mumble.h"
#include "imgui/imgui.h"
#include <deque>

extern HMODULE hSelf;
extern AddonAPI* APIDefs;
extern HWND GameHandle;
extern Mumble::Data* MumbleLink;
extern NexusLinkData* NexusLink;
extern bool firstInstall;
extern std::filesystem::path SettingsPath;
extern std::filesystem::path AddonPath;
extern std::filesystem::path GW2Root;

extern Texture* Berserker;
extern Texture* Bladesworn;
extern Texture* Catalyst;
extern Texture* Chronomancer;
extern Texture* Daredevil;
extern Texture* Deadeye;
extern Texture* Dragonhunter;
extern Texture* Druid;
extern Texture* Elementalist;
extern Texture* Engineer;
extern Texture* Firebrand;
extern Texture* Guardian;
extern Texture* Harbinger;
extern Texture* Herald;
extern Texture* Holosmith;
extern Texture* Mechanist;
extern Texture* Mesmer;
extern Texture* Mirage;
extern Texture* Necromancer;
extern Texture* Ranger;
extern Texture* Revenant;
extern Texture* Reaper;
extern Texture* Renegade;
extern Texture* Scrapper;
extern Texture* Scourge;
extern Texture* Soulbeast;
extern Texture* Specter;
extern Texture* Spellbreaker;
extern Texture* Tempest;
extern Texture* Thief;
extern Texture* Untamed;
extern Texture* Vindicator;
extern Texture* Virtuoso;
extern Texture* Warrior;
extern Texture* Weaver;
extern Texture* Willbender;
extern Texture* Death;
extern Texture* Downed;
extern Texture* Squad;
extern Texture* Damage;
extern Texture* Condi;
extern Texture* Strike;
extern Texture* Kdr;
extern Texture* Home;


extern std::atomic<bool> initialParsingComplete;
extern std::atomic<bool> stopMonitoring;
extern std::mutex parsedLogsMutex;
extern std::mutex processedFilesMutex;
extern std::thread initialParsingThread;
extern std::thread directoryMonitorThread;
extern std::atomic<bool> isRestartInProgress;
extern int currentLogIndex;


struct TextureInfo {
    int resourceId;
    Texture** texture;
};


struct Agent {
    uint64_t address;
    uint32_t professionId;
    int32_t eliteSpecId;
    uint16_t id = 0;
    std::string name;
    std::string accountName;
    std::string subgroup;
    int subgroupNumber;
    std::string profession;
    std::string eliteSpec;
    std::string team = "Unknown";
};

struct SpecStats {
    uint32_t count = 0;
    uint32_t totalKills = 0;
    uint32_t totalKillsVsPlayers = 0;
    uint32_t totalDeaths = 0;
    uint32_t totalDowned = 0;
    uint64_t totalDamage = 0;
    uint64_t totalStrikeDamage = 0;
    uint64_t totalCondiDamage = 0;
    uint64_t totalDamageVsPlayers = 0;
    uint64_t totalStrikeDamageVsPlayers = 0;
    uint64_t totalCondiDamageVsPlayers = 0;
    uint64_t totalDownedContribution = 0;
    uint64_t totalDownedContributionVsPlayers = 0;
    uint64_t totalKillContribution = 0;
    uint64_t totalKillContributionVsPlayers = 0;
};

struct SquadStats {
    uint32_t totalPlayers = 0;
    uint32_t totalDeaths = 0;
    uint32_t totalDowned = 0;
    uint32_t totalKills = 0;
    uint32_t totalDeathsFromKillingBlows = 0;
    uint64_t totalDamage = 0;
    uint64_t totalStrikeDamage = 0;
    uint64_t totalCondiDamage = 0;
    uint64_t totalDamageVsPlayers = 0;
    uint64_t totalStrikeDamageVsPlayers = 0;
    uint64_t totalCondiDamageVsPlayers = 0;
    uint32_t totalKillsVsPlayers = 0;
    uint64_t totalDownedContribution = 0;
    uint64_t totalDownedContributionVsPlayers = 0;
    uint64_t totalKillContribution = 0;
    uint64_t totalKillContributionVsPlayers = 0;
    float getKillDeathRatio() const {
        if (totalDeathsFromKillingBlows == 0) {
            return static_cast<float>(totalKills);
        }
        return static_cast<float>(totalKills) / totalDeathsFromKillingBlows;
    }
    std::unordered_map<std::string, SpecStats> eliteSpecStats;
};

struct TeamStats {
    uint32_t totalPlayers = 0;
    uint32_t totalDeaths = 0;
    uint32_t totalDowned = 0;
    uint32_t totalKills = 0;
    uint32_t totalDeathsFromKillingBlows = 0;
    uint64_t totalDamage = 0;
    uint64_t totalStrikeDamage = 0;
    uint64_t totalCondiDamage = 0;
    uint64_t totalDamageVsPlayers = 0;
    uint64_t totalStrikeDamageVsPlayers = 0;
    uint64_t totalCondiDamageVsPlayers = 0;
    uint32_t totalKillsVsPlayers = 0;
    uint64_t totalDownedContribution = 0;
    uint64_t totalDownedContributionVsPlayers = 0;
    uint64_t totalKillContribution = 0;
    uint64_t totalKillContributionVsPlayers = 0;
    bool isPOVTeam = false;
    float getKillDeathRatio() const {
        if (totalDeathsFromKillingBlows == 0) {
            return static_cast<float>(totalKills);
        }
        return static_cast<float>(totalKills) / totalDeathsFromKillingBlows;
    }
    std::unordered_map<std::string, SpecStats> eliteSpecStats;
    SquadStats squadStats;
};


struct ParsedData {
    std::unordered_map<std::string, TeamStats> teamStats;
    uint64_t combatStartTime = 0;
    uint64_t combatEndTime = 0;
    uint16_t fightId = 0;
    size_t totalIdentifiedPlayers = 0;

    double getCombatDurationSeconds() const {
        if (combatEndTime > combatStartTime) {
            return (combatEndTime - combatStartTime) / 1000.0;
        }
        return 0.0; 
    }
};

struct ParsedLog {
    std::string filename;
    ParsedData data;
};

struct SpecAggregateStats {
    uint32_t totalCount = 0;
};

struct SquadAggregateStats {
    uint32_t totalPlayers = 0;
    uint32_t instanceCount = 0;
    uint32_t totalDeaths = 0;
    uint32_t totalDowned = 0;

    std::unordered_map<std::string, SpecAggregateStats> eliteSpecTotals;

    double getAveragePlayerCount() const {
        return (instanceCount > 0)
            ? static_cast<double>(totalPlayers) / instanceCount
            : 0.0;
    }

    double getAverageSpecCount(const std::string& spec) const {
        auto it = eliteSpecTotals.find(spec);
        if (it != eliteSpecTotals.end() && instanceCount > 0) {
            return static_cast<double>(it->second.totalCount) / instanceCount;
        }
        return 0.0;
    }
};

struct TeamAggregateStats {
    SquadAggregateStats teamTotals;
    bool isPOVTeam = false;
    SquadAggregateStats povSquadTotals;

    double getAverageTeamPlayerCount() const {
        return teamTotals.getAveragePlayerCount();
    }

    double getAverageTeamSpecCount(const std::string& spec) const {
        return teamTotals.getAverageSpecCount(spec);
    }

    double getAveragePOVSquadPlayerCount() const {
        if (isPOVTeam)
            return povSquadTotals.getAveragePlayerCount();
        return 0.0;
    }

    double getAveragePOVSquadSpecCount(const std::string& spec) const {
        if (isPOVTeam)
            return povSquadTotals.getAverageSpecCount(spec);
        return 0.0;
    }
};

struct GlobalAggregateStats {
    uint64_t totalCombatTime = 0;
    uint32_t combatInstanceCount = 0;
    std::unordered_map<std::string, TeamAggregateStats> teamAggregates;

    double getAverageCombatTime() const {
        return (combatInstanceCount > 0)
            ? static_cast<double>(totalCombatTime) / combatInstanceCount
            : 0.0;
    }
};

struct CachedAverages {
    double averageCombatTime = 0.0;

    std::unordered_map<std::string, double> averageTeamPlayerCounts;
    std::unordered_map<std::string, std::unordered_map<std::string, double>> averageTeamSpecCounts;

    std::unordered_map<std::string, double> averagePOVSquadPlayerCounts;
    std::unordered_map<std::string, std::unordered_map<std::string, double>> averagePOVSquadSpecCounts;
};

extern std::deque<ParsedLog> parsedLogs;

// Combat Event Structure
#pragma pack(push, 1)  // Disable padding
struct CombatEvent {
    uint64_t time;
    uint64_t srcAgent;
    uint64_t dstAgent;
    int32_t value;
    int32_t buffDmg;
    uint32_t overstackValue;
    uint32_t skillId;
    uint16_t srcInstid;
    uint16_t dstInstid;
    uint16_t srcMasterInstid;
    uint16_t dstMasterInstid;
    uint8_t iff;
    uint8_t buff;
    uint8_t result;
    uint8_t isActivation;
    uint8_t isBuffRemove;
    uint8_t isNinety;
    uint8_t isFifty;
    uint8_t isMoving;
    uint8_t isStateChange;
    uint8_t isFlanking;
    uint8_t isShields;
    uint8_t isOffCycle;
    uint32_t pad;
};
#pragma pack(pop)

struct AgentState {
    std::vector<std::pair<uint64_t, uint64_t>> downIntervals;
    std::vector<std::pair<uint64_t, uint64_t>> deathIntervals;
    std::vector<std::pair<uint64_t, float>> healthUpdates;
    std::vector<CombatEvent> relevantEvents;  // Store original events for precise sequencing
    bool currentlyDowned = false;
};

// enum

enum class StateChange : uint8_t {
    None = 0,
    EnterCombat = 1,
    ExitCombat = 2,
    ChangeUp = 3,
    ChangeDead = 4,
    ChangeDown = 5,
    Spawn = 6,
    Despawn = 7,
    HealthUpdate = 8,
    LogStart = 9,
    LogEnd = 10,
    WeaponSwap = 11,
    MaxHealthUpdate = 12,
    PointOfView = 13,
    Language = 14,
    GWBuild = 15,
    ShardId = 16,
    Reward = 17,
    BuffInitial = 18,
    Position = 19,
    Velocity = 20,
    Facing = 21,
    TeamChange = 22,
    AttackTarget = 23,
    Targetable = 24,
    MapID = 29,
    ReplInfo = 25,
    StackActive = 26,
    StackReset = 27,
    Guild = 28,
    Error = 0xFF
};

enum class ResultCode : uint8_t {
    Normal = 0,
    Critical = 1,
    Glance = 2,
    Block = 3,
    Evade = 4,
    Interrupt = 5,
    Absorb = 6,
    Blind = 7,
    KillingBlow = 8,
    Downed = 9
};

enum class Activation : uint8_t {
    None = 0,
    Normal = 1,
    Quickness = 2,
    CancelFire = 3,
    CancelCancel = 4,
    Reset = 5
};

enum class BuffRemove : uint8_t {
    None = 0,
    All = 1,
    Single = 2,
    Manual = 3
};



// Maps
extern std::unordered_map<int, std::string> professions;
extern std::unordered_map<int, std::string> eliteSpecs;
extern std::unordered_map<int, std::string> teamIDs;
extern std::unordered_map<std::string, std::string> eliteSpecToProfession;
extern std::unordered_map<std::string, std::string> eliteSpecShortNames;
extern std::unordered_map<std::string, ImVec4> professionColors;
extern GlobalAggregateStats globalAggregateStats;
extern CachedAverages cachedAverages;
extern std::mutex aggregateStatsMutex;

// Constants
extern const char* const ADDON_NAME;
extern const char* KB_WINDOW_TOGGLEVISIBLE;