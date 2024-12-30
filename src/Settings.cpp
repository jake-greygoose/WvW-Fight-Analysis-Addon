#include "Settings.h"

#include "Shared.h"

#include <filesystem>
#include <fstream>

const char*  IS_ADDON_WIDGET_VISIBLE = "IsWidgetVisible";
const char* IS_ADDON_WINDOW_VISIBLE = "IsWindowVisible";
const char* IS_ADDON_AGG_WINDOW_VISIBLE = "isAggWindowVisible";

const char* HIDE_AGG_WINDOW_WHEN_EMPTY = "HideAggWindowWhenEmpty";

// Options
const char* IS_WINDOW_VISIBLE_IN_COMBAT = "IsWindowVisibleInCombat";
const char* CUSTOM_LOG_PATH = "CustomLogDirectoryPath";
const char* TEAM_PLAYER_THRESHOLD = "TeamPlayerThreshold";
const char*  LOG_HISTORY_SIZE = "LogHistorySize";
const char* DISABLE_CLICKING_WINDOW = "DisableClickingWindow";
const char* DISABLE_MOVING_WINDOW = "DisableMovingWindow";
const char* SHOW_NEW_PARSE_ALERT = "ShowNewParseAlert";
const char* FORCE_LINUX_COMPAT = "ForceLinuxCompat";
const char* POLL_INTERVAL_MILLISECONDS = "PollIntervalMilliseconds";

// Display
const char* SHOW_CLASS_NAMES = "ShowClassNames";
const char* USE_SHORT_CLASS_NAMES = "UseShortClassNames";
const char* SHOW_CLASS_ICONS = "ShowClassIcons";
const char* SHOW_SPEC_BARS = "ShowSpecBars";
const char* SHOW_LOG_NAME = "ShowLogName";
const char* VS_LOGGED_PLAYERS_ONLY = "VsLoggedPlayersOnly";
const char* SQUAD_PLAYERS_ONLY = "SquadPlayersOnly";

// Team Stats
const char* SHOW_TEAM_TOTAL_PLAYERS = "ShowTeamTotalPlayers";
const char* SHOW_TEAM_DEATHS = "ShowTeamDeaths";
const char* SHOW_TEAM_DOWNED = "ShowTeamDowned";
const char* SHOW_TEAM_DAMAGE = "ShowTeamDamage";
const char* SHOW_TEAM_CONDI = "ShowTeamCondiDamage";
const char* SHOW_TEAM_STRIKE = "ShowTeamCondiDamage";
const char* SHOW_TEAM_KDR = "ShowTeamKDR";
const char* SHOW_TEAM_DOWN_CONT = "ShowTeamDownCont";
const char* SHOW_TEAM_KILL_CONT = "ShowTeamKillCont";
//Specs
const char* SHOW_SPEC_DAMAGE = "ShowSpecDamage";
const char* SHOW_SPEC_DOWN_CONT = "ShowSpecDownCont";
const char* SHOW_SPEC_KILL_CONT = "ShowSpecKillCont";
const char* SORT_SPEC_DAMAGE = "SortSpecDamage";
// Window Style
const char* SHOW_SCROLL_BAR = "ShowScrollBar";
const char* USE_TABBED_VIEW = "UseTabbedView";
const char* SHOW_WINDOW_TITLE = "ShowWindowTitle";
const char* ALLOW_WINDOW_FOCUS = "AllowWindowFocus";
const char* SHOW_WINDOW_BACKGROUND = "ShowWindowBackground";
const char* SPLIT_STATS_WINDOW = "SplitStatsWindow";
const char* USE_NEXUS_ESC_CLOSE = "UseNexusEscClose";
// Widget
const char* WIDGET_STATS = "WidgetStats";
const char* WIDGET_HEIGHT = "WidgetHeight";
const char* WIDGET_WIDTH = "WidgetWidth";
const char* WIDGET_TEXT_VERTICAL_OFFSET = "WidgetTextVerticalAlignOffset";
const char* WIDGET_TEXT_HORIZONTAL_OFFSET = "WidgetTextHorizontalAlignOffset";
const char* SHOW_WIDGET_ICON = "ShowWidgetIcon";
// Sort
const char* WINDOW_SORT = "WindowSort";
const char* BAR_REPRESENTATION = "BarRepresentation";
const char* BAR_REP_INDEPENDENT = "BarRepIndependent";

const char* BAR_TEMPLATE = "BarTemplate";
const char* BAR_STATS = "BarStats";


namespace Settings
{
	std::mutex	Mutex;
	json		Settings = json::object();


	void Load(std::filesystem::path aPath)
	{
		if (!std::filesystem::exists(aPath)) { return; }

		Settings::Mutex.lock();
		{
			try
			{
				std::ifstream file(aPath);
				Settings = json::parse(file);
				file.close();
			}
			catch (json::parse_error& ex)
			{

			}
		}
		Settings::Mutex.unlock();
		/* Widget */
		if (!Settings[IS_ADDON_WIDGET_VISIBLE].is_null())
		{
			Settings[IS_ADDON_WIDGET_VISIBLE].get_to<bool>(IsAddonWidgetEnabled);
		}
		if (!Settings[WIDGET_STATS].is_null())
		{
			Settings[WIDGET_STATS].get_to<std::string>(widgetStats);
			strcpy_s(widgetStatsC, sizeof(widgetStatsC), widgetStats.c_str());
		}
		if (!Settings[WINDOW_SORT].is_null())
		{
			Settings[WINDOW_SORT].get_to<std::string>(windowSort);
			strcpy_s(windowSortC, sizeof(windowSortC), windowSort.c_str());
		}
		if (!Settings[BAR_REPRESENTATION].is_null())
		{
			Settings[BAR_REPRESENTATION].get_to<std::string>(barRepresentation);
			strcpy_s(barRepresentationC, sizeof(barRepresentationC), barRepresentation.c_str());
		}
		if (!Settings[SHOW_NEW_PARSE_ALERT].is_null())
		{
			Settings[SHOW_NEW_PARSE_ALERT].get_to(showNewParseAlert);
		}
		if (!Settings[WIDGET_HEIGHT].is_null())
		{
			Settings[WIDGET_HEIGHT].get_to(widgetHeight);
		}
		if (!Settings[WIDGET_WIDTH].is_null())
		{
			Settings[WIDGET_WIDTH].get_to(widgetWidth);
		}
		if (!Settings[WIDGET_TEXT_VERTICAL_OFFSET].is_null())
		{
			Settings[WIDGET_TEXT_VERTICAL_OFFSET].get_to(widgetTextVerticalAlignOffset);
		}
		if (!Settings[WIDGET_TEXT_HORIZONTAL_OFFSET].is_null())
		{
			Settings[WIDGET_TEXT_HORIZONTAL_OFFSET].get_to(widgetTextHorizontalAlignOffset);
		}
		if (!Settings[SHOW_WIDGET_ICON].is_null())
		{
			Settings[SHOW_WIDGET_ICON].get_to<bool>(showWidgetIcon);
		}
		/* Window */
		if (!Settings[IS_ADDON_WINDOW_VISIBLE].is_null())
		{
			Settings[IS_ADDON_WINDOW_VISIBLE].get_to<bool>(IsAddonWindowEnabled);
		}
		if (!Settings[IS_ADDON_AGG_WINDOW_VISIBLE].is_null())
		{
			Settings[IS_ADDON_AGG_WINDOW_VISIBLE].get_to<bool>(IsAddonAggWindowEnabled);
		}
		if (!Settings[HIDE_AGG_WINDOW_WHEN_EMPTY].is_null())
		{
			Settings[HIDE_AGG_WINDOW_WHEN_EMPTY].get_to<bool>(hideAggWhenEmpty);
		}
		if (!Settings[IS_WINDOW_VISIBLE_IN_COMBAT].is_null())
		{
			Settings[IS_WINDOW_VISIBLE_IN_COMBAT].get_to<bool>(showWindowInCombat);
		}
		if (!Settings[SHOW_LOG_NAME].is_null())
		{
			Settings[SHOW_LOG_NAME].get_to<bool>(showLogName);
		}
		if (!Settings[SHOW_CLASS_NAMES].is_null())
		{
			Settings[SHOW_CLASS_NAMES].get_to<bool>(showClassNames);
		}
		if (!Settings[USE_SHORT_CLASS_NAMES].is_null())
		{
			Settings[USE_SHORT_CLASS_NAMES].get_to<bool>(useShortClassNames);
		}
		if (!Settings[SHOW_CLASS_ICONS].is_null())
		{
			Settings[SHOW_CLASS_ICONS].get_to<bool>(showClassIcons);
		}
		if (!Settings[SHOW_SPEC_BARS].is_null())
		{
			Settings[SHOW_SPEC_BARS].get_to<bool>(showSpecBars);
		}
		if (!Settings[VS_LOGGED_PLAYERS_ONLY].is_null())
		{
			Settings[VS_LOGGED_PLAYERS_ONLY].get_to<bool>(vsLoggedPlayersOnly);
		}
		if (!Settings[BAR_REP_INDEPENDENT].is_null())
		{
			Settings[BAR_REP_INDEPENDENT].get_to<bool>(barRepIndependent);
		}
		if (!Settings[SQUAD_PLAYERS_ONLY].is_null())
		{
			Settings[SQUAD_PLAYERS_ONLY].get_to<bool>(squadPlayersOnly);
		}
		if (!Settings[TEAM_PLAYER_THRESHOLD].is_null())
		{
			Settings[TEAM_PLAYER_THRESHOLD].get_to(teamPlayerThreshold);
		}
		if (!Settings[CUSTOM_LOG_PATH].is_null())
		{
			Settings[CUSTOM_LOG_PATH].get_to<std::string>(LogDirectoryPath);
			strcpy_s(LogDirectoryPathC, sizeof(LogDirectoryPathC), LogDirectoryPath.c_str());
		}
		if (!Settings[BAR_TEMPLATE].is_null())
		{
			Settings[BAR_TEMPLATE].get_to<std::string>(barTemplate);
			strcpy_s(barTemplateC, sizeof(barTemplateC), barTemplate.c_str());
		}
		if (!Settings[LOG_HISTORY_SIZE].is_null())
		{
			Settings[LOG_HISTORY_SIZE].get_to(logHistorySize);
		}
		if (!Settings[FORCE_LINUX_COMPAT].is_null())
		{
			Settings[FORCE_LINUX_COMPAT].get_to<bool>(forceLinuxCompatibilityMode);
		}
		if (!Settings[POLL_INTERVAL_MILLISECONDS].is_null())
		{
			Settings[POLL_INTERVAL_MILLISECONDS].get_to(pollIntervalMilliseconds);
		}
		/* Team Stats */
		if (!Settings[SHOW_TEAM_TOTAL_PLAYERS].is_null())
		{
			Settings[SHOW_TEAM_TOTAL_PLAYERS].get_to<bool>(showTeamTotalPlayers);
		}
		if (!Settings[SHOW_TEAM_DEATHS].is_null())
		{
			Settings[SHOW_TEAM_DEATHS].get_to<bool>(showTeamDeaths);
		}
		if (!Settings[SHOW_TEAM_DOWNED].is_null())
		{
			Settings[SHOW_TEAM_DOWNED].get_to<bool>(showTeamDowned);
		}
		if (!Settings[SHOW_TEAM_DAMAGE].is_null())
		{
			Settings[SHOW_TEAM_DAMAGE].get_to<bool>(showTeamDamage);
		}
		if (!Settings[SHOW_TEAM_CONDI].is_null())
		{
			Settings[SHOW_TEAM_CONDI].get_to<bool>(showTeamCondiDamage);
		}
		if (!Settings[SHOW_TEAM_STRIKE].is_null())
		{
			Settings[SHOW_TEAM_STRIKE].get_to<bool>(showTeamStrikeDamage);
		}
		if (!Settings[SHOW_TEAM_KDR].is_null())
		{
			Settings[SHOW_TEAM_KDR].get_to<bool>(showTeamKDR);
		}
		if (!Settings[SHOW_TEAM_DOWN_CONT].is_null())
		{
			Settings[SHOW_TEAM_DOWN_CONT].get_to<bool>(showTeamDownCont);
		}
		if (!Settings[SHOW_TEAM_KILL_CONT].is_null())
		{
			Settings[SHOW_TEAM_KILL_CONT].get_to<bool>(showTeamKillCont);
		}
		/* Spec Stats */
		if (!Settings[SHOW_SPEC_DAMAGE].is_null())
		{
			Settings[SHOW_SPEC_DAMAGE].get_to<bool>(showSpecDamage);
		}
		if (!Settings[SHOW_SPEC_DOWN_CONT].is_null())
		{
			Settings[SHOW_SPEC_DOWN_CONT].get_to<bool>(showSpecDownCont);
		}
		if (!Settings[SHOW_SPEC_KILL_CONT].is_null())
		{
			Settings[SHOW_SPEC_KILL_CONT].get_to<bool>(showSpecKillCont);
		}
		if (!Settings[SORT_SPEC_DAMAGE].is_null())
		{
			Settings[SORT_SPEC_DAMAGE].get_to<bool>(sortSpecDamage);
		}
		/* Window Style */
		if (!Settings[SHOW_SCROLL_BAR].is_null())
		{
			Settings[SHOW_SCROLL_BAR].get_to<bool>(showScrollBar);
		}
		if (!Settings[USE_TABBED_VIEW].is_null())
		{
			Settings[USE_TABBED_VIEW].get_to<bool>(useTabbedView);
		}
		if (!Settings[SHOW_WINDOW_TITLE].is_null())
		{
			Settings[SHOW_WINDOW_TITLE].get_to<bool>(showWindowTitle);
		}
		if (!Settings[SHOW_WINDOW_BACKGROUND].is_null())
		{
			Settings[SHOW_WINDOW_BACKGROUND].get_to<bool>(showWindowBackground);
		}
		if (!Settings[ALLOW_WINDOW_FOCUS].is_null())
		{
			Settings[ALLOW_WINDOW_FOCUS].get_to<bool>(allowWindowFocus);
		}
		if (!Settings[DISABLE_MOVING_WINDOW].is_null())
		{
			Settings[DISABLE_MOVING_WINDOW].get_to<bool>(disableMovingWindow);
		}
		if (!Settings[DISABLE_CLICKING_WINDOW].is_null())
		{
			Settings[DISABLE_CLICKING_WINDOW].get_to<bool>(disableClickingWindow);
		}
		if (!Settings[SPLIT_STATS_WINDOW].is_null())
		{
			Settings[SPLIT_STATS_WINDOW].get_to<bool>(splitStatsWindow);
		}
		if (!Settings[USE_NEXUS_ESC_CLOSE].is_null())
		{
			Settings[USE_NEXUS_ESC_CLOSE].get_to<bool>(useNexusEscClose);
		}
		if (!Settings[BAR_STATS].is_null())
		{
			barStats.clear();
			for (const auto& stat : Settings[BAR_STATS])
			{
				BarStat barStat;
				if (!stat["Representation"].is_null())
				{
					barStat.representation = stat["Representation"].get<std::string>();
				}
				if (!stat["PrimaryStat"].is_null())
				{
					barStat.primaryStat = stat["PrimaryStat"].get<std::string>();
				}
				if (!stat["SecondaryStat"].is_null())
				{
					barStat.secondaryStat = stat["SecondaryStat"].get<std::string>();
				}
				barStats.push_back(barStat);
			}
		}

	}
	void Save(std::filesystem::path aPath)
	{
		Settings::Mutex.lock();
		{
			json barStatsArray = json::array();
			for (const auto& stat : barStats)
			{
				json barStat;
				barStat["Representation"] = stat.representation;
				barStat["PrimaryStat"] = stat.primaryStat;
				barStat["SecondaryStat"] = stat.secondaryStat;
				barStatsArray.push_back(barStat);
			}
			Settings[BAR_STATS] = barStatsArray;

			std::ofstream file(aPath);
			file << Settings.dump(1, '\t') << std::endl;
			file.close();
		}
		Settings::Mutex.unlock();
	}

	/* Windows */

	bool IsAddonWidgetEnabled = true;
	bool IsAddonWindowEnabled = true;
	bool IsAddonAggWindowEnabled = false;

	bool hideAggWhenEmpty = false;
	
	/* Options */

	bool showWindowInCombat = true;
	int teamPlayerThreshold = 1;
	bool disableMovingWindow = false;
	bool disableClickingWindow = false;
	bool showNewParseAlert = true;
	bool forceLinuxCompatibilityMode = false;
	size_t pollIntervalMilliseconds = 3000;

	std::string LogDirectoryPath;
	char LogDirectoryPathC[256] = "";
	size_t logHistorySize = 10;

	// Display
	bool showClassNames = true;
	bool useShortClassNames = false;
	bool showClassIcons = true;
	bool showSpecBars = true;
	bool showLogName = true;
	bool vsLoggedPlayersOnly = true;
	bool squadPlayersOnly = false;
	// Team Stats
	bool showTeamTotalPlayers = true;
	bool showTeamDeaths = true;
	bool showTeamDowned = true;
	bool showTeamDamage = true;
	bool showTeamCondiDamage = false;
	bool showTeamStrikeDamage = false;
	bool showTeamKDR = false;
	bool showTeamDownCont = false;
	bool showTeamKillCont = false;
	// Spec Stats
	bool showSpecDamage = true;
	bool sortSpecDamage = false;
	bool showSpecDownCont = false;
	bool showSpecKillCont = false;
	// Window Style
	bool showScrollBar = true;
	bool useTabbedView = true;
	bool showWindowTitle = true;
	bool showWindowBackground = true;
	bool allowWindowFocus = true;
	bool splitStatsWindow = false;
	bool useNexusEscClose = false;
	// Widget
	std::string widgetStats;
	char widgetStatsC[256] = "players";
	float widgetWidth = 320.0f;
	float widgetHeight = 20.0f;
	float widgetTextVerticalAlignOffset = 0.0f;
	float widgetTextHorizontalAlignOffset = 0.0f;
	bool showWidgetIcon = true;
	// sort
	std::string windowSort;
	char windowSortC[256] = "players";
	std::string barRepresentation = "players";
	char barRepresentationC[32] = "players";
	bool barRepIndependent = false;

	std::string barTemplate;
	char barTemplateC[256] = "@1 @2 @3 (@4)";

	std::vector<BarStat> barStats = {
	BarStat("players", "damage", "strips"),
	BarStat("damage", "damage", "down cont")
	};
}
