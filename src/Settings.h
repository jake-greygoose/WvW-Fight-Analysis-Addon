#ifndef SETTINGS_H
#define SETTINGS_H

#include <mutex>
#include <memory>
#include <vector>
#include "nlohmann/json.hpp"
#include "imgui/imgui.h"

using json = nlohmann::json;

struct SecondaryStatRelation {
    std::string secondaryStat;
    float maxRatio;
    bool enabled;
};

class BaseWindowSettings {
public:
    std::string windowName;
    std::string windowId;
    std::string fullDisplayName;
    char tempWindowName[256] = { 0 };
    bool isWindowNameEditing = false;
    bool isEnabled = true;
    bool hideInCombat = false;
    bool hideOutOfCombat = false;
    bool showScrollBar = false;
    bool showTitle = true;
    bool useWindowStyleForTitle = false;
    bool showBackground = true;
    bool allowFocus = true;
    bool disableMoving = false;
    bool disableClicking = false;
    bool squadPlayersOnly = false;
    bool vsLoggedPlayersOnly = false;
    bool useNexusEscClose = false;
    ImVec2 position;
    ImVec2 size;

    BaseWindowSettings(const json& j = json::object(), const std::string& idPrefix = "win_");
    virtual json toJson() const;
    const std::string& getDisplayName(const std::string& defaultName);
    void updateDisplayName(const std::string& defaultName);

protected:
    static std::string GenerateUniqueId(const std::string& prefix);
};
struct MainWindowSettings : public BaseWindowSettings {
    // Basic display settings
    bool showLogName = true;
    bool useShortClassNames = false;
    bool showSpecBars = true;
    bool showSpecTooltips = true;
    bool showClassNames = true;
    bool showClassIcons = true;
    bool showSpecDamage = true;
    bool useTabbedView = true;
    bool vsLoggedPlayersOnly = true;
    bool squadPlayersOnly = false;

    // Team display settings
    bool showTeamTotalPlayers = true;
    bool showTeamKDR = false;
    bool showTeamDeaths = true;
    bool showTeamDowned = true;
    bool showTeamDamage = true;
    bool showTeamDownCont = false;
    bool showTeamKillCont = false;
    bool showTeamStrikeDamage = false;
    bool showTeamCondiDamage = false;

    // Sort and template settings
    std::string windowSort = "players";
    bool barRepIndependent = false;
    std::string barRepresentation = "players";
    std::unordered_map<std::string, std::string> sortTemplates;
    std::vector<std::string> enabledStats;

    // Secondary stat relationships
    static const inline std::unordered_map<std::string, SecondaryStatRelation> secondaryStats = {
        {"damage", {"down cont", 1.0f, true}},
        // Future additions would look like:
        // {"strip", {"strip cont", 1.0f, true}},
    };

    MainWindowSettings(const json& j = json::object());
    json toJson() const override;
};

struct WidgetWindowSettings : public BaseWindowSettings {
    float widgetWidth = 320.0f;
    float widgetHeight = 20.0f;
    float textVerticalAlignOffset = 0.0f;
    float textHorizontalAlignOffset = 0.0f;
    bool showWidgetIcon = true;
    std::string widgetStats = "players";

    WidgetWindowSettings(const json& j = json::object());
    json toJson() const override;
};

struct AggregateWindowSettings : public BaseWindowSettings {
    // Display settings
    bool showAvgCombatTime = true;
    bool showTotalCombatTime = true;
    bool showTeamTotalPlayers = true;
    bool showTeamDeaths = true;
    bool showTeamDowned = true;
    bool showClassNames = true;
    bool showClassIcons = true;
    bool showAvgSpecs = true;
    bool hideWhenEmpty = false;
    bool squadPlayersOnly = false;

    AggregateWindowSettings(const json& j = json::object());
    json toJson() const override;
};

struct WindowManager {
    std::vector<std::unique_ptr<MainWindowSettings>> mainWindows;
    std::vector<std::unique_ptr<WidgetWindowSettings>> widgetWindows;
    std::unique_ptr<AggregateWindowSettings> aggregateWindow;

    WindowManager() {
        aggregateWindow = std::make_unique<AggregateWindowSettings>();
    }

    void LoadFromJson(const json& j);
    json ToJson() const;
    MainWindowSettings* AddMainWindow();
    WidgetWindowSettings* AddWidgetWindow();
    void RemoveMainWindow();
    void RemoveWidgetWindow();
};

extern const char* CUSTOM_LOG_PATH;
extern const char* LOG_HISTORY_SIZE;
extern const char* TEAM_PLAYER_THRESHOLD;
extern const char* SHOW_NEW_PARSE_ALERT;
extern const char* FORCE_LINUX_COMPAT;
extern const char* POLL_INTERVAL_MILLISECONDS;
extern const char* USE_NEXUS_ESC_CLOSE;
extern const char* IS_ADDON_AGG_WINDOW_VISIBLE;
extern const char* HIDE_AGG_WINDOW_WHEN_EMPTY;

namespace Settings {
    extern std::mutex Mutex;
    extern json Settings;

    extern std::string LogDirectoryPath;
    extern char LogDirectoryPathC[256];
    extern size_t logHistorySize;
    extern int teamPlayerThreshold;
    extern bool showNewParseAlert;
    extern bool forceLinuxCompatibilityMode;
    extern size_t pollIntervalMilliseconds;
    extern bool IsAddonAggWindowEnabled;
    extern bool hideAggWhenEmpty;
    extern bool useNexusEscClose;

    extern WindowManager windowManager;

    void Load(std::filesystem::path aPath);
    void Save(std::filesystem::path aPath);
    void InitializeDefaultWindows();
}

#endif