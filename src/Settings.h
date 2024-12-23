#ifndef SETTINGS_H
#define SETTINGS_H

#include <mutex>

#include "nlohmann/json.hpp"
using json = nlohmann::json;

extern const char* IS_ADDON_WIDGET_VISIBLE;
extern const char* IS_ADDON_WINDOW_VISIBLE;
extern const char* IS_ADDON_AGG_WINDOW_VISIBLE;

extern const char* HIDE_AGG_WINDOW_WHEN_EMPTY;

// Options
extern const char* IS_WINDOW_VISIBLE_IN_COMBAT;
extern const char* CUSTOM_LOG_PATH;
extern const char* TEAM_PLAYER_THRESHOLD;
extern const char* LOG_HISTORY_SIZE;
extern const char* DISABLE_CLICKING_WINDOW;
extern const char* DISABLE_MOVING_WINDOW;
extern const char* SHOW_NEW_PARSE_ALERT;
extern const char* FORCE_LINUX_COMPAT;
extern const char* POLL_INTERVAL_MILLISECONDS;

// Display
extern const char* USE_SHORT_CLASS_NAMES;
extern const char* SHOW_CLASS_NAMES;
extern const char* SHOW_CLASS_ICONS;
extern const char* SHOW_SPEC_BARS;
extern const char* SHOW_LOG_NAME;
extern const char* VS_LOGGED_PLAYERS_ONLY;
extern const char* SQUAD_PLAYERS_ONLY;
// Team Stats
extern const char* SHOW_TEAM_TOTAL_PLAYERS;
extern const char* SHOW_TEAM_DEATHS;
extern const char* SHOW_TEAM_DOWNED;
extern const char* SHOW_TEAM_DAMAGE;
extern const char* SHOW_TEAM_STRIKE;
extern const char* SHOW_TEAM_CONDI;
extern const char* SHOW_TEAM_KDR;
// Spec Stats
extern const char* SHOW_SPEC_DAMAGE;
extern const char* SORT_SPEC_DAMAGE;
// Window Style
extern const char* USE_TABBED_VIEW;
extern const char* SHOW_SCROLL_BAR;
extern const char* SHOW_WINDOW_TITLE;
extern const char* SHOW_WINDOW_BACKGROUND;
extern const char* ALLOW_WINDOW_FOCUS;
extern const char* SPLIT_STATS_WINDOW;
extern const char* USE_NEXUS_ESC_CLOSE;
//Widget
extern const char* WIDGET_STATS;
extern const char* SHOW_WIDGET_ICON;
extern const char* WIDGET_HEIGHT;
extern const char* WIDGET_WIDTH;
extern const char* WIDGET_TEXT_VERTICAL_OFFSET;
extern const char* WIDGET_TEXT_HORIZONTAL_OFFSET;

namespace Settings
{
	extern std::mutex	Mutex;
	extern json			Settings;

	/* Loads the settings. */
	void Load(std::filesystem::path aPath);
	/* Saves the settings. */
	void Save(std::filesystem::path aPath);

	/* Windows */
	extern bool IsAddonWidgetEnabled;
	extern bool IsAddonWindowEnabled;
	extern bool IsAddonAggWindowEnabled;

	extern bool hideAggWhenEmpty;
	
	//Options
	extern bool showWindowInCombat;
	extern int teamPlayerThreshold;
	extern std::string LogDirectoryPath;
	extern char LogDirectoryPathC[256];
	extern size_t logHistorySize;
	extern bool disableClickingWindow;
	extern bool disableMovingWindow;
	extern bool showNewParseAlert;
	extern bool forceLinuxCompatibilityMode;
	extern size_t pollIntervalMilliseconds;

	//Display
	extern bool useShortClassNames;
	extern bool showClassNames;
	extern bool showClassIcons;
	extern bool showSpecBars;
	extern bool showLogName;
	extern bool vsLoggedPlayersOnly;
	extern bool squadPlayersOnly;

	// Team Stats
	extern bool showTeamTotalPlayers;
	extern bool showTeamDeaths;
	extern bool showTeamDowned;
	extern bool showTeamDamage;
	extern bool showTeamCondiDamage;
	extern bool showTeamStrikeDamage;
	extern bool showTeamKDR;
	// Spec Stats
	extern bool showSpecDamage;
	extern bool sortSpecDamage;
	// Window Style
	extern bool showScrollBar;
	extern bool useTabbedView;
	extern bool showWindowTitle;
	extern bool showWindowBackground;
	extern bool allowWindowFocus;
	extern bool splitStatsWindow;
	extern 	bool useNexusEscClose;
	// Widget
	extern std::string widgetStats;
	extern char widgetStatsC[256];
	extern float widgetWidth;
	extern float widgetHeight;
	extern float widgetTextVerticalAlignOffset;
	extern float widgetTextHorizontalAlignOffset;
	extern bool showWidgetIcon;
}

#endif