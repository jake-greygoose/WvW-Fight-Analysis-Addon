#include "settings/Settings.h"
#include "shared/Shared.h"
#include "utils/Utils.h"
#include <filesystem>
#include <fstream>

// Global Settings
const char* CUSTOM_LOG_PATH = "CustomLogDirectoryPath";
const char* LOG_HISTORY_SIZE = "LogHistorySize";
const char* TEAM_PLAYER_THRESHOLD = "TeamPlayerThreshold";
const char* SHOW_NEW_PARSE_ALERT = "ShowNewParseAlert";
const char* FORCE_LINUX_COMPAT = "ForceLinuxCompat";
const char* POLL_INTERVAL_MILLISECONDS = "PollIntervalMilliseconds";
const char* USE_NEXUS_ESC_CLOSE = "UseNexusEscClose";
const char* DEBUG_STRINGS_MODE = "debugStringsMode";
const char* TEAM_IDS = "TeamIDs";

// Default team IDs
// I will keep all the old ones as I don't know the pattern of if they get reused etc
// These are always merged with user's custom team IDs
static const std::unordered_map<int, std::string> DEFAULT_TEAM_IDS = {
    // Red team IDs
    {697, "Red"}, {699, "Red"}, {705, "Red"}, {706, "Red"},
    {707, "Red"}, {882, "Red"}, {885, "Red"}, {886, "Red"}, {2520, "Red"},
    // Green team IDs
    {39, "Green"}, {2739, "Green"}, {2741, "Green"},
    {2752, "Green"}, {2763, "Green"}, {2767, "Green"},
    // Blue team IDs
    {432, "Blue"}, {433, "Blue"}, {1277, "Blue"},
    {1281, "Blue"}, {1282, "Blue"}, {1989, "Blue"}, {1996, "Blue"}
};

BaseWindowSettings::BaseWindowSettings(const json& j, const std::string& idPrefix) {
    windowId = GenerateUniqueId(idPrefix);

    if (!j.is_null()) {
        windowId = j.value("windowId", windowId);
        windowName = j.value("windowName", "");
        isEnabled = j.value("isEnabled", isEnabled);
        hideInCombat = j.value("hideInCombat", hideInCombat);
        hideOutOfCombat = j.value("hideOutOfCombat", hideOutOfCombat);
        showScrollBar = j.value("showScrollBar", showScrollBar);
        showTitle = j.value("showTitle", showTitle);
        showBackground = j.value("showBackground", showBackground);
        allowFocus = j.value("allowFocus", allowFocus);
        disableMoving = j.value("disableMoving", disableMoving);
        disableClicking = j.value("disableClicking", disableClicking);
        squadPlayersOnly = j.value("squadPlayersOnly", squadPlayersOnly);
        vsLoggedPlayersOnly = j.value("vsLoggedPlayersOnly", vsLoggedPlayersOnly);
        useNexusEscClose = j.value("useNexusEscClose", useNexusEscClose);
        useWindowStyleForTitle = j.value("useWindowStyleForTitle", useWindowStyleForTitle);

        // Exclusions
        excludeRedTeam = j.value("excludeRedTeam", excludeRedTeam);
        excludeGreenTeam = j.value("excludeGreenTeam", excludeGreenTeam);
        excludeBlueTeam = j.value("excludeBlueTeam", excludeBlueTeam);

        position.x = j.value("positionX", position.x);
        position.y = j.value("positionY", position.y);
        size.x = j.value("sizeX", size.x);
        size.y = j.value("sizeY", size.y);
    }
}

std::string BaseWindowSettings::GenerateUniqueId(const std::string& prefix) {
    static int nextId = 1;
    return prefix + std::to_string(nextId++);
}

const std::string& BaseWindowSettings::getDisplayName(const std::string& defaultName) {
    if (fullDisplayName.empty()) {
        updateDisplayName(defaultName);
    }
    return fullDisplayName;
}

void BaseWindowSettings::updateDisplayName(const std::string& defaultName) {
    std::string displayText = windowName.empty() ? defaultName : windowName;
    fullDisplayName = displayText + "###" + windowId;
}

json BaseWindowSettings::toJson() const {
    return {
        {"windowName", windowName},
        {"windowId", windowId},  // Save the window ID
        {"isEnabled", isEnabled},
        {"hideInCombat", hideInCombat},
        {"hideOutOfCombat", hideOutOfCombat},
        {"showScrollBar", showScrollBar},
        {"showTitle", showTitle},
        {"showBackground", showBackground},
        {"allowFocus", allowFocus},
        {"disableMoving", disableMoving},
        {"disableClicking", disableClicking},
        {"squadPlayersOnly", squadPlayersOnly},
        {"vsLoggedPlayersOnly", vsLoggedPlayersOnly},
        {"useNexusEscClose", useNexusEscClose},
        {"useWindowStyleForTitle", useWindowStyleForTitle},
        {"excludeRedTeam", excludeRedTeam},
        {"excludeGreenTeam", excludeGreenTeam},
        {"excludeBlueTeam", excludeBlueTeam},
        {"positionX", position.x},
        {"positionY", position.y},
        {"sizeX", size.x},
        {"sizeY", size.y}
    };
}

json MainWindowSettings::toJson() const {
    json j = BaseWindowSettings::toJson();

    // Basic display settings
    j["showLogName"] = showLogName;
    j["useShortClassNames"] = useShortClassNames;
    j["showSpecBars"] = showSpecBars;
    j["showSpecTooltips"] = showSpecTooltips;
    j["showClassNames"] = showClassNames;
    j["showClassIcons"] = showClassIcons;
    j["showSpecDamage"] = showSpecDamage;
    j["useTabbedView"] = useTabbedView;
    j["vsLoggedPlayersOnly"] = vsLoggedPlayersOnly;
    j["squadPlayersOnly"] = squadPlayersOnly;

    // Team display settings
    j["showTeamTotalPlayers"] = showTeamTotalPlayers;
    j["showTeamKDR"] = showTeamKDR;
    j["showTeamDeaths"] = showTeamDeaths;
    j["showTeamDowned"] = showTeamDowned;
    j["showTeamDamage"] = showTeamDamage;
    j["showTeamDownCont"] = showTeamDownCont;
    j["showTeamKillCont"] = showTeamKillCont;
    j["showTeamStrikeDamage"] = showTeamStrikeDamage;
    j["showTeamCondiDamage"] = showTeamCondiDamage;
    j["showTeamStrips"] = showTeamStrips;

    j["barCornerRounding"] = barCornerRounding;
    j["overideTableBackgroundStyle"] = overideTableBackgroundStyle;

    // Sort and bar representation settings
    j["windowSort"] = windowSort;
    j["barRepIndependent"] = barRepIndependent;
    j["barRepresentation"] = barRepresentation;

    // Save sort templates
    json templatesJson = json::object();
    for (const auto& [sort, templ] : sortTemplates) {
        templatesJson[sort] = templ;
    }
    j["sortTemplates"] = templatesJson;

    // Save enabled stats
    j["enabledStats"] = enabledStats;

    return j;
}

json WidgetWindowSettings::toJson() const {
    json j = BaseWindowSettings::toJson();
    j["largerFont"] = largerFont;
    j["widgetWidth"] = widgetWidth;
    j["widgetHeight"] = widgetHeight;
    j["pieChartSize"] = pieChartSize;
    j["textVerticalAlignOffset"] = textVerticalAlignOffset;
    j["textHorizontalAlignOffset"] = textHorizontalAlignOffset;
    j["showWidgetIcon"] = showWidgetIcon;
    j["widgetStats"] = widgetStats;
    j["widgetBorderThickness"] = widgetBorderThickness;
    j["widgetRoundness"] = widgetRoundness;
    j["usePieChartStyle"] = usePieChartStyle;
    // Color settings
    j["colors"] = {
        {"redBackground", colors.redBackground},
        {"blueBackground", colors.blueBackground},
        {"greenBackground", colors.greenBackground},
        {"redText", colors.redText},
        {"blueText", colors.blueText},
        {"greenText", colors.greenText},
        {"redBackgroundCombat", colors.redBackgroundCombat},
        {"blueBackgroundCombat", colors.blueBackgroundCombat},
        {"greenBackgroundCombat", colors.greenBackgroundCombat},
        {"redTextCombat", colors.redTextCombat},
        {"blueTextCombat", colors.blueTextCombat},
        {"greenTextCombat", colors.greenTextCombat},
        {"widgetBorder", colors.widgetBorder},
        {"widgetBorderCombat", colors.widgetBorderCombat}

    };
    return j;
}

MainWindowSettings::MainWindowSettings(const json& j) : BaseWindowSettings(j, "win_") {
    // Initialize default sort templates
    sortTemplates = {
        {"players", ""},
        {"damage", ""},
        {"down_cont", ""},
        {"kill_cont", ""},
        {"deaths", ""},
        {"downs", ""}
    };

    // Initialize default enabled stats
    enabledStats = {
        "damage",
        "down_cont",
        "kill_cont",
        "deaths",
        "downs",
        "strike_damage",
        "condi_damage"
    };

    if (!j.is_null()) {
        showLogName = j.value("showLogName", showLogName);
        useShortClassNames = j.value("useShortClassNames", useShortClassNames);
        showSpecBars = j.value("showSpecBars", showSpecBars);
        showSpecTooltips = j.value("showSpecTooltips", showSpecTooltips);
        showClassNames = j.value("showClassNames", showClassNames);
        showClassIcons = j.value("showClassIcons", showClassIcons);
        showSpecDamage = j.value("showSpecDamage", showSpecDamage);
        useTabbedView = j.value("useTabbedView", useTabbedView);
        vsLoggedPlayersOnly = j.value("vsLoggedPlayersOnly", vsLoggedPlayersOnly);
        squadPlayersOnly = j.value("squadPlayersOnly", squadPlayersOnly);

        showTeamTotalPlayers = j.value("showTeamTotalPlayers", showTeamTotalPlayers);
        showTeamKDR = j.value("showTeamKDR", showTeamKDR);
        showTeamDeaths = j.value("showTeamDeaths", showTeamDeaths);
        showTeamDowned = j.value("showTeamDowned", showTeamDowned);
        showTeamDamage = j.value("showTeamDamage", showTeamDamage);
        showTeamDownCont = j.value("showTeamDownCont", showTeamDownCont);
        showTeamKillCont = j.value("showTeamKillCont", showTeamKillCont);
        showTeamStrikeDamage = j.value("showTeamStrikeDamage", showTeamStrikeDamage);
        showTeamCondiDamage = j.value("showTeamCondiDamage", showTeamCondiDamage);
        showTeamStrips = j.value("showTeamStrips", showTeamStrips);

        barCornerRounding = j.value("barCornerRounding", barCornerRounding);
        overideTableBackgroundStyle = j.value("overideTableBackgroundStyle", overideTableBackgroundStyle);


        windowSort = j.value("windowSort", windowSort);
        barRepIndependent = j.value("barRepIndependent", barRepIndependent);
        barRepresentation = j.value("barRepresentation", barRepresentation);

        if (j.contains("sortTemplates") && j["sortTemplates"].is_object()) {
            sortTemplates.clear();
            for (const auto& [sort, templ] : j["sortTemplates"].items()) {
                sortTemplates[sort] = templ.get<std::string>();
            }
        }

        if (j.contains("enabledStats") && j["enabledStats"].is_array()) {
            enabledStats.clear();
            for (const auto& stat : j["enabledStats"]) {
                enabledStats.push_back(stat.get<std::string>());
            }
        }
    }

    const std::vector<std::string> requiredSorts = {
        "players", "damage", "down_cont", "kill_cont", "deaths", "downs"
    };
    for (const auto& sort : requiredSorts) {
        if (sortTemplates.find(sort) == sortTemplates.end()) {
            sortTemplates[sort] = "";
        }
    }
}

WidgetWindowSettings::WidgetWindowSettings(const json& j) : BaseWindowSettings(j, "widget_") {
    if (!j.is_null()) {
        largerFont = j.value("largerFont", largerFont);
        widgetWidth = j.value("widgetWidth", widgetWidth);
        widgetHeight = j.value("widgetHeight", widgetHeight);
        pieChartSize = j.value("pieChartSize", pieChartSize);
        textVerticalAlignOffset = j.value("textVerticalAlignOffset", textVerticalAlignOffset);
        textHorizontalAlignOffset = j.value("textHorizontalAlignOffset", textHorizontalAlignOffset);
        showWidgetIcon = j.value("showWidgetIcon", showWidgetIcon);
        widgetStats = j.value("widgetStats", widgetStats);
        widgetRoundness = j.value("widgetRoundness", widgetRoundness);
        widgetBorderThickness = j.value("widgetBorderThickness", widgetBorderThickness);
        usePieChartStyle = j.value("usePieChartStyle", usePieChartStyle);

        if (j.contains("colors")) {
            const auto& colorsJson = j["colors"];
            colors.redBackground = colorsJson.value("redBackground", colors.redBackground);
            colors.blueBackground = colorsJson.value("blueBackground", colors.blueBackground);
            colors.greenBackground = colorsJson.value("greenBackground", colors.greenBackground);
            colors.redText = colorsJson.value("redText", colors.redText);
            colors.blueText = colorsJson.value("blueText", colors.blueText);
            colors.greenText = colorsJson.value("greenText", colors.greenText);
            colors.redBackgroundCombat = colorsJson.value("redBackgroundCombat", colors.redBackgroundCombat);
            colors.blueBackgroundCombat = colorsJson.value("blueBackgroundCombat", colors.blueBackgroundCombat);
            colors.greenBackgroundCombat = colorsJson.value("greenBackgroundCombat", colors.greenBackgroundCombat);
            colors.redTextCombat = colorsJson.value("redTextCombat", colors.redTextCombat);
            colors.blueTextCombat = colorsJson.value("blueTextCombat", colors.blueTextCombat);
            colors.greenTextCombat = colorsJson.value("greenTextCombat", colors.greenTextCombat);
            colors.widgetBorder = colorsJson.value("widgetBorder", colors.widgetBorder);
            colors.widgetBorderCombat = colorsJson.value("widgetBorderCombat", colors.widgetBorderCombat);
        }

    }
}

AggregateWindowSettings::AggregateWindowSettings(const json& j) : BaseWindowSettings(j, "agg_") {
    if (!j.is_null()) {
        showAvgCombatTime = j.value("showAvgCombatTime", showAvgCombatTime);
        showTotalCombatTime = j.value("showTotalCombatTime", showTotalCombatTime);
        showTeamTotalPlayers = j.value("showTeamTotalPlayers", showTeamTotalPlayers);
        showTeamDeaths = j.value("showTeamDeaths", showTeamDeaths);
        showTeamDowned = j.value("showTeamDowned", showTeamDowned);
        showClassNames = j.value("showClassNames", showClassNames);
        showClassIcons = j.value("showClassIcons", showClassIcons);
        showAvgSpecs = j.value("showAvgSpecs", showAvgSpecs);
        hideWhenEmpty = j.value("hideWhenEmpty", hideWhenEmpty);
        squadPlayersOnly = j.value("squadPlayersOnly", squadPlayersOnly);
    }
}

json AggregateWindowSettings::toJson() const {
    json j = BaseWindowSettings::toJson();

    j["showAvgCombatTime"] = showAvgCombatTime;
    j["showTotalCombatTime"] = showTotalCombatTime;
    j["showTeamTotalPlayers"] = showTeamTotalPlayers;
    j["showTeamDeaths"] = showTeamDeaths;
    j["showTeamDowned"] = showTeamDowned;
    j["showClassNames"] = showClassNames;
    j["showClassIcons"] = showClassIcons;
    j["showAvgSpecs"] = showAvgSpecs;
    j["hideWhenEmpty"] = hideWhenEmpty;
    j["squadPlayersOnly"] = squadPlayersOnly;

    return j;
}

MainWindowSettings* WindowManager::AddMainWindow() {
    mainWindows.push_back(std::make_unique<MainWindowSettings>());
    auto* window = mainWindows.back().get();
    RegisterWindowForNexusEsc(window, "WvW Fight Analysis");
    return window;
}

WidgetWindowSettings* WindowManager::AddWidgetWindow() {
    widgetWindows.push_back(std::make_unique<WidgetWindowSettings>());
    return widgetWindows.back().get();
}

void WindowManager::RemoveMainWindow() {
    if (mainWindows.size() > 1) {
        UnregisterWindowFromNexusEsc(mainWindows.back().get(), "WvW Fight Analysis");
        mainWindows.pop_back();
    }
}

void WindowManager::RemoveWidgetWindow() {
    if (widgetWindows.size() > 1) {
        widgetWindows.pop_back();
    }
}

void WindowManager::LoadFromJson(const json& j) {
    if (j.contains("mainWindows") && j["mainWindows"].is_array()) {
        mainWindows.clear();
        for (const auto& windowJson : j["mainWindows"]) {
            mainWindows.push_back(std::make_unique<MainWindowSettings>(windowJson));
            RegisterWindowForNexusEsc(mainWindows.back().get(), "WvW Fight Analysis");
        }
    }

    if (j.contains("widgetWindows") && j["widgetWindows"].is_array()) {
        widgetWindows.clear();
        for (const auto& windowJson : j["widgetWindows"]) {
            widgetWindows.push_back(std::make_unique<WidgetWindowSettings>(windowJson));
        }
    }

    if (j.contains("aggregateWindow")) {
        aggregateWindow = std::make_unique<AggregateWindowSettings>(j["aggregateWindow"]);
    }
    else {
        aggregateWindow = std::make_unique<AggregateWindowSettings>();
    }
}

json WindowManager::ToJson() const {
    json j;

    json mainWindowsJson = json::array();
    for (const auto& window : mainWindows) {
        if (window) {
            mainWindowsJson.push_back(window->toJson());
        }
    }
    j["mainWindows"] = mainWindowsJson;

    json widgetWindowsJson = json::array();
    for (const auto& window : widgetWindows) {
        if (window) {
            widgetWindowsJson.push_back(window->toJson());
        }
    }
    j["widgetWindows"] = widgetWindowsJson;

    if (aggregateWindow) {
        j["aggregateWindow"] = aggregateWindow->toJson();
    }

    return j;
}

namespace Settings {
    std::mutex Mutex;
    json Settings = json::object();
    WindowManager windowManager;

    std::string LogDirectoryPath;
    char LogDirectoryPathC[256] = "";
    size_t logHistorySize = 10;
    int teamPlayerThreshold = 1;
    bool showNewParseAlert = true;
    bool forceLinuxCompatibilityMode = false;
    size_t pollIntervalMilliseconds = 3000;
    bool hideAggWhenEmpty = false;
    bool useNexusEscClose = false;
    bool debugStringsMode = false;
    std::unordered_map<int, std::string> teamIDs;

    void Settings::Load(std::filesystem::path aPath) {
        Settings::Mutex.lock();
        {
            try {
                if (!std::filesystem::exists(aPath)) {
                    // File doesn't exist - initialize with defaults
                    Settings = json::object();
                }
                else {
                    // Load existing settings
                    std::ifstream file(aPath);
                    Settings = json::parse(file);
                    file.close();
                }

                // Set any missing settings to defaults
                if (!Settings.contains(CUSTOM_LOG_PATH)) {
                    Settings[CUSTOM_LOG_PATH] = "";
                }
                if (!Settings.contains(LOG_HISTORY_SIZE)) {
                    Settings[LOG_HISTORY_SIZE] = 10;
                }
                if (!Settings.contains(TEAM_PLAYER_THRESHOLD)) {
                    Settings[TEAM_PLAYER_THRESHOLD] = 1;
                }
                if (!Settings.contains(SHOW_NEW_PARSE_ALERT)) {
                    Settings[SHOW_NEW_PARSE_ALERT] = true;
                }
                if (!Settings.contains(FORCE_LINUX_COMPAT)) {
                    Settings[FORCE_LINUX_COMPAT] = false;
                }
                if (!Settings.contains(POLL_INTERVAL_MILLISECONDS)) {
                    Settings[POLL_INTERVAL_MILLISECONDS] = 3000;
                }
                if (!Settings.contains(USE_NEXUS_ESC_CLOSE)) {
                    Settings[USE_NEXUS_ESC_CLOSE] = false;
                }
                if (!Settings.contains(DEBUG_STRINGS_MODE)) {
                    Settings[DEBUG_STRINGS_MODE] = false;
                }

                // Initialize TeamIDs with defaults if key doesn't exist
                if (!Settings.contains(TEAM_IDS)) {
                    Settings[TEAM_IDS] = json::object();
                }

                // Safely load values into variables with try/catch for each
                try {
                    LogDirectoryPath = Settings[CUSTOM_LOG_PATH].get<std::string>();
                }
                catch (...) {
                    LogDirectoryPath = "";
                    Settings[CUSTOM_LOG_PATH] = "";
                }
                strcpy_s(LogDirectoryPathC, sizeof(LogDirectoryPathC), LogDirectoryPath.c_str());

                try {
                    logHistorySize = Settings[LOG_HISTORY_SIZE].get<size_t>();
                }
                catch (...) {
                    logHistorySize = 10;
                    Settings[LOG_HISTORY_SIZE] = 10;
                }

                try {
                    teamPlayerThreshold = Settings[TEAM_PLAYER_THRESHOLD].get<int>();
                }
                catch (...) {
                    teamPlayerThreshold = 1;
                    Settings[TEAM_PLAYER_THRESHOLD] = 1;
                }

                try {
                    showNewParseAlert = Settings[SHOW_NEW_PARSE_ALERT].get<bool>();
                }
                catch (...) {
                    showNewParseAlert = true;
                    Settings[SHOW_NEW_PARSE_ALERT] = true;
                }

                try {
                    forceLinuxCompatibilityMode = Settings[FORCE_LINUX_COMPAT].get<bool>();
                }
                catch (...) {
                    forceLinuxCompatibilityMode = false;
                    Settings[FORCE_LINUX_COMPAT] = false;
                }

                try {
                    pollIntervalMilliseconds = Settings[POLL_INTERVAL_MILLISECONDS].get<size_t>();
                }
                catch (...) {
                    pollIntervalMilliseconds = 3000;
                    Settings[POLL_INTERVAL_MILLISECONDS] = 3000;
                }

                try {
                    useNexusEscClose = Settings[USE_NEXUS_ESC_CLOSE].get<bool>();
                }
                catch (...) {
                    useNexusEscClose = false;
                    Settings[USE_NEXUS_ESC_CLOSE] = false;
                }

                try {
                    debugStringsMode = Settings[DEBUG_STRINGS_MODE].get<bool>();
                }
                catch (...) {
                    debugStringsMode = false;
                    Settings[DEBUG_STRINGS_MODE] = false;
                }

                // Load team IDs - merge defaults with user's custom IDs
                try {
                    // Start with defaults (always include addon's known team IDs)
                    teamIDs = DEFAULT_TEAM_IDS;

                    // Load user's custom IDs from settings and merge/override
                    if (Settings[TEAM_IDS].is_object()) {
                        for (auto& [key, value] : Settings[TEAM_IDS].items()) {
                            try {
                                int teamId = std::stoi(key);
                                std::string teamName = value.get<std::string>();
                                // User's custom IDs can override defaults or add new ones
                                teamIDs[teamId] = teamName;
                            }
                            catch (...) {
                                // Skip invalid entries
                            }
                        }
                    }

                    // Save the merged result back to settings (defaults + user custom)
                    json mergedTeamIDs = json::object();
                    for (const auto& [id, name] : teamIDs) {
                        mergedTeamIDs[std::to_string(id)] = name;
                    }
                    Settings[TEAM_IDS] = mergedTeamIDs;
                }
                catch (...) {
                    // On error, use defaults only
                    teamIDs = DEFAULT_TEAM_IDS;
                    json defaultTeamIDsJson = json::object();
                    for (const auto& [id, name] : DEFAULT_TEAM_IDS) {
                        defaultTeamIDsJson[std::to_string(id)] = name;
                    }
                    Settings[TEAM_IDS] = defaultTeamIDsJson;
                }

                // Handle windows
                try {
                    if (Settings.contains("windows")) {
                        windowManager.LoadFromJson(Settings["windows"]);
                    }
                    else {
                        InitializeDefaultWindows();
                        Settings["windows"] = windowManager.ToJson();
                    }
                }
                catch (...) {
                    InitializeDefaultWindows();
                    Settings["windows"] = windowManager.ToJson();
                }

                // Save to ensure all defaults are written, including merged team IDs
                std::ofstream file(aPath);
                file << Settings.dump(1, '\t') << std::endl;
                file.close();

            }
            catch (...) {
                // If anything goes wrong, reset to complete defaults
                Settings = json::object();

                // Reset all variables to defaults
                LogDirectoryPath = "";
                strcpy_s(LogDirectoryPathC, sizeof(LogDirectoryPathC), "");
                logHistorySize = 10;
                teamPlayerThreshold = 1;
                showNewParseAlert = true;
                forceLinuxCompatibilityMode = false;
                pollIntervalMilliseconds = 3000;
                hideAggWhenEmpty = false;
                useNexusEscClose = false;
                debugStringsMode = false;
                teamIDs = DEFAULT_TEAM_IDS;

                // Set all default settings
                Settings[CUSTOM_LOG_PATH] = LogDirectoryPath;
                Settings[LOG_HISTORY_SIZE] = logHistorySize;
                Settings[TEAM_PLAYER_THRESHOLD] = teamPlayerThreshold;
                Settings[SHOW_NEW_PARSE_ALERT] = showNewParseAlert;
                Settings[FORCE_LINUX_COMPAT] = forceLinuxCompatibilityMode;
                Settings[POLL_INTERVAL_MILLISECONDS] = pollIntervalMilliseconds;
                Settings[USE_NEXUS_ESC_CLOSE] = useNexusEscClose;
                Settings[DEBUG_STRINGS_MODE] = debugStringsMode;

                // Use the same defaults defined at file scope
                json defaultTeamIDsJson = json::object();
                for (const auto& [id, name] : DEFAULT_TEAM_IDS) {
                    defaultTeamIDsJson[std::to_string(id)] = name;
                }
                Settings[TEAM_IDS] = defaultTeamIDsJson;

                InitializeDefaultWindows();
                Settings["windows"] = windowManager.ToJson();
            }
        }
        Settings::Mutex.unlock();
    }

    void Settings::Save(std::filesystem::path aPath) {
        Settings::Mutex.lock();
        {
            // Update windows section
            Settings["windows"] = windowManager.ToJson();

            // Update team IDs section
            json teamIDsJson = json::object();
            for (const auto& [id, name] : teamIDs) {
                teamIDsJson[std::to_string(id)] = name;
            }
            Settings[TEAM_IDS] = teamIDsJson;

            // Write to file
            std::ofstream file(aPath);
            file << Settings.dump(1, '\t') << std::endl;
            file.close();
        }
        Settings::Mutex.unlock();
    }

    void Settings::InitializeDefaultWindows() {
        if (windowManager.mainWindows.empty()) {
            auto mainWindow = windowManager.AddMainWindow();

            // Window position and basic settings
            mainWindow->position = ImVec2(1030, 300);
            mainWindow->size = ImVec2(250, 350);
            mainWindow->isEnabled = true;
            mainWindow->showScrollBar = false;
            mainWindow->showTitle = true;
            mainWindow->showBackground = true;
            mainWindow->allowFocus = true;
            mainWindow->disableMoving = false;
            mainWindow->disableClicking = false;
            mainWindow->hideInCombat = false;
            mainWindow->hideOutOfCombat = false;
            mainWindow->vsLoggedPlayersOnly = true;
            mainWindow->squadPlayersOnly = false;
            mainWindow->useNexusEscClose = false;

            // Display settings
            mainWindow->showLogName = true;
            mainWindow->useShortClassNames = false;
            mainWindow->showSpecBars = true;
            mainWindow->showSpecTooltips = true;
            mainWindow->showClassNames = true;
            mainWindow->showClassIcons = true;
            mainWindow->showSpecDamage = true;
            mainWindow->useTabbedView = true;

            // Team display settings
            mainWindow->showTeamTotalPlayers = true;
            mainWindow->showTeamKDR = false;
            mainWindow->showTeamDeaths = true;
            mainWindow->showTeamDowned = true;
            mainWindow->showTeamDamage = true;
            mainWindow->showTeamDownCont = false;
            mainWindow->showTeamKillCont = false;
            mainWindow->showTeamStrikeDamage = false;
            mainWindow->showTeamCondiDamage = false;
            mainWindow->showTeamStrips = false;

            // Exclusions
            mainWindow->excludeRedTeam = false;
            mainWindow->excludeGreenTeam = false;
            mainWindow->excludeBlueTeam = false;

            mainWindow->barCornerRounding = 0;
            mainWindow->overideTableBackgroundStyle = false;

            mainWindow->windowSort = "players";

            mainWindow->sortTemplates = {
                {"players", ""},
                {"damage", ""},
                {"down_cont", ""},
                {"kill_cont", ""},
                {"deaths", ""},
                {"downs", ""}
            };

            mainWindow->enabledStats = {
                "damage",
                "down_cont",
                "kill_cont",
                "deaths",
                "downs",
                "strike_damage",
                "condi_damage"
            };
        }
        if (windowManager.widgetWindows.empty()) {
            auto widgetWindow = windowManager.AddWidgetWindow();

            widgetWindow->largerFont = false;
            widgetWindow->position = ImVec2(750, 350);
            widgetWindow->size = ImVec2(320, 20);
            widgetWindow->isEnabled = true;
            widgetWindow->showScrollBar = false;
            widgetWindow->showTitle = false;
            widgetWindow->showBackground = true;
            widgetWindow->allowFocus = true;
            widgetWindow->disableMoving = false;
            widgetWindow->disableClicking = false;
            widgetWindow->hideInCombat = false;
            widgetWindow->hideOutOfCombat = false;
            widgetWindow->vsLoggedPlayersOnly = true;
            widgetWindow->squadPlayersOnly = false;
            widgetWindow->useNexusEscClose = false;

            widgetWindow->widgetWidth = 320.0f;
            widgetWindow->widgetHeight = 20.0f;
            widgetWindow->textVerticalAlignOffset = 0.0f;
            widgetWindow->textHorizontalAlignOffset = 0.0f;
            widgetWindow->showWidgetIcon = true;
            widgetWindow->widgetStats = "players";
            widgetWindow->widgetBorderThickness = 1.0f;
            widgetWindow->widgetRoundness = 0.0f;
            
            widgetWindow->colors.redBackground = ImVec4(1.0f, 0.266f, 0.266f, 1.0f);
            widgetWindow->colors.blueBackground = ImVec4(0.2f, 0.71f, 0.898f, 1.0f);
            widgetWindow->colors.greenBackground = ImVec4(0.6f, 0.8f, 0.0f, 1.0f);
            widgetWindow->colors.redText = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
            widgetWindow->colors.blueText = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
            widgetWindow->colors.greenText = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
            widgetWindow->colors.widgetBorder = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
            widgetWindow->colors.widgetBorderCombat = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

            // in combat style
            widgetWindow->colors.redBackgroundCombat = ImVec4(0.498f, 0.498f, 0.498f, 0.8f);
            widgetWindow->colors.blueBackgroundCombat = ImVec4(0.565f, 0.565f, 0.565f, 0.8f);
            widgetWindow->colors.greenBackgroundCombat = ImVec4(0.675f, 0.675f, 0.675f, 0.8f);
            widgetWindow->colors.redTextCombat = ImVec4(1.0f, 0.266f, 0.266f, 1.0f);
            widgetWindow->colors.blueTextCombat = ImVec4(0.2f, 0.71f, 0.898f, 1.0f);
            widgetWindow->colors.greenTextCombat = ImVec4(0.6f, 0.8f, 0.0f, 1.0f);
        }
    }
}
