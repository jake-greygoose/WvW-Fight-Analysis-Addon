#define NOMINMAX
#include "parser/directory_monitor.h"
#include "parser/evtc_parser.h"
#include "parser/file_helpers.h"
#include "parser/statistics_helper.h"
#include "settings/Settings.h"
#include "shared/Shared.h"
#include "utils/Utils.h"
#include <thread>
#include <chrono>
#include <filesystem>
#include <shlobj.h>
#include <unordered_set>
#include <algorithm>
#include <vector>
#include <mutex>
#include <cstring>

std::unordered_set<std::wstring> processedFiles;
std::filesystem::file_time_type maxProcessedTime = std::filesystem::file_time_type::min();
static bool flatLogMode = false;

static bool shouldSkipLog(const ParsedLog& log);

bool isValidEVTCFile(const std::filesystem::path& dirPath, const std::filesystem::path& filePath)
{
	std::filesystem::path relativePath;
	try {
		relativePath = std::filesystem::relative(filePath.parent_path(), dirPath);
	}
	catch (...) {
		return false;
	}

	if (flatLogMode) {
		return relativePath == ".";
	}

	bool dirPathIsWvW = dirPath.filename().string().find("WvW") != std::string::npos;
	bool dirPathIs1 = dirPath.filename().string() == "1";

	std::vector<std::filesystem::path> components;
	if (!dirPath.has_root_path() && !dirPathIsWvW && !dirPathIs1) {
		components.push_back(dirPath.filename());
	}
	for (const auto& part : relativePath) {
		components.push_back(part);
	}

	int wvwIndex = -1;
	for (size_t i = 0; i < components.size(); ++i) {
		if (components[i].string().find("WvW") != std::string::npos) {
			wvwIndex = static_cast<int>(i);
			break;
		}
	}

	int dir1Index = -1;
	if (wvwIndex == -1) {
		for (size_t i = 0; i < components.size(); ++i) {
			if (components[i].string() == "1") {
				dir1Index = static_cast<int>(i);
				break;
			}
		}
	}

	if (dirPathIsWvW) {
		return components.size() <= 2;
	}
	if (wvwIndex != -1) {
		return (components.size() - (wvwIndex + 1)) <= 2;
	}

	if (dirPathIs1) {
		return components.size() <= 2;
	}
	if (dir1Index != -1) {
		return (components.size() - (dir1Index + 1)) <= 2;
	}

	return false;
}

void processEVTCFile(const std::filesystem::path& filePath)
{
	waitForFile(filePath);
	processNewEVTCFile(filePath);
}

void parseInitialLogs(std::unordered_set<std::wstring>& processedFiles, size_t numLogsToParse)
{
	try
	{
		std::filesystem::path dirPath;
		if (!Settings::LogDirectoryPath.empty())
		{
			dirPath = std::filesystem::path(Settings::LogDirectoryPath);
			APIDefs->Log(ELogLevel_INFO, ADDON_NAME,
				("Custom log path specified: " + dirPath.string()).c_str());
			if (!std::filesystem::exists(dirPath) || !std::filesystem::is_directory(dirPath))
			{
				APIDefs->Log(ELogLevel_WARNING, ADDON_NAME,
					("Specified log directory does not exist: " + dirPath.string()).c_str());
				return;
			}
		}
		else
		{
			char documentsPath[MAX_PATH];
			if (FAILED(SHGetFolderPath(nullptr, CSIDL_PERSONAL, nullptr, SHGFP_TYPE_CURRENT, documentsPath)))
			{
				APIDefs->Log(ELogLevel_WARNING, ADDON_NAME,
					"Failed to get path to user's Documents folder.");
				return;
			}

			dirPath = std::filesystem::path(documentsPath) / "Guild Wars 2" / "addons" /
				"arcdps" / "arcdps.cbtlogs";
		}

		std::vector<std::filesystem::path> zevtcFiles;

		// Recursively search for .zevtc files in the directory
		for (const auto& entry : std::filesystem::recursive_directory_iterator(dirPath))
		{
			if (entry.is_regular_file() && entry.path().extension() == L".zevtc")
			{
				if (isValidEVTCFile(dirPath, entry.path()))
				{
					zevtcFiles.push_back(entry.path());
				}
			}
		}

		if (zevtcFiles.empty())
		{
			APIDefs->Log(ELogLevel_WARNING, ADDON_NAME,
				("No valid .zevtc files found in directory: " + dirPath.string()).c_str());
			return;
		}

		// Sort descending order (newest first)
		std::sort(zevtcFiles.begin(), zevtcFiles.end(),
			[](const std::filesystem::path& a, const std::filesystem::path& b)
			{
				return std::filesystem::last_write_time(a) > std::filesystem::last_write_time(b);
			});

		size_t numFilesToParse = std::min<size_t>(zevtcFiles.size(), numLogsToParse);

		for (size_t i = 0; i < numFilesToParse; ++i)
		{
			const std::filesystem::path& filePath = zevtcFiles[i];
			waitForFile(filePath);

			std::wstring absolutePath = std::filesystem::absolute(filePath).wstring();

			if (processedFiles.find(absolutePath) == processedFiles.end())
			{
				ParsedLog log;
				log.filename = getUtf8Path(filePath.filename());
				log.data = parseEVTCFile(filePath);

				if (shouldSkipLog(log))
				{
					continue;
				}

				{
					std::lock_guard<std::mutex> lock(parsedLogsMutex);
					parsedLogs.push_back(log);

					while (parsedLogs.size() > Settings::logHistorySize)
					{
						parsedLogs.pop_back();
					}
					currentLogIndex = 0;
				}

				processedFiles.insert(absolutePath);

				auto fileTime = std::filesystem::last_write_time(filePath);
				if (fileTime > maxProcessedTime)
				{
					maxProcessedTime = fileTime;
				}
			}
			else
			{
				APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME,
					("File already processed during initial parsing: " + getUtf8Path(filePath)).c_str());
			}
		}

		initialParsingComplete = true;
	}
	catch (const std::exception& ex)
	{
		APIDefs->Log(ELogLevel_WARNING, ADDON_NAME,
			("Exception in initial parsing: " + std::string(ex.what())).c_str());
	}
}

void scanForNewFiles(const std::filesystem::path& dirPath, std::unordered_set<std::wstring>& processedFiles)
{
	try
	{
		std::vector<std::filesystem::path> zevtcFiles;

		for (const auto& entry : std::filesystem::recursive_directory_iterator(dirPath))
		{
			if (entry.is_regular_file() && entry.path().extension() == L".zevtc")
			{
				auto fileTime = std::filesystem::last_write_time(entry.path());

				if (fileTime <= maxProcessedTime)
					continue;

				if (isValidEVTCFile(dirPath, entry.path()))
				{
					zevtcFiles.push_back(entry.path());
				}
			}
		}

		std::sort(zevtcFiles.begin(), zevtcFiles.end(),
			[](const std::filesystem::path& a, const std::filesystem::path& b)
			{
				return std::filesystem::last_write_time(a) > std::filesystem::last_write_time(b);
			});

		for (const auto& filePath : zevtcFiles)
		{
			auto fileTime = std::filesystem::last_write_time(filePath);

			if (fileTime <= maxProcessedTime)
			{
				break;
			}

			std::wstring absolutePath = std::filesystem::absolute(filePath).wstring();

			if (processedFiles.find(absolutePath) != processedFiles.end())
			{
				continue;
			}

			processEVTCFile(filePath);

			processedFiles.insert(absolutePath);

			if (fileTime > maxProcessedTime)
			{
				maxProcessedTime = fileTime;
			}
		}
	}
	catch (const std::exception& ex)
	{
		APIDefs->Log(ELogLevel_WARNING, ADDON_NAME,
			("Exception in scanning directory: " + std::string(ex.what())).c_str());
	}
}

void monitorDirectory(size_t numLogsToParse, size_t pollIntervalMilliseconds)
{
	try
	{
		std::filesystem::path dirPath;

		flatLogMode = (getBossEncounterNpcDirs() == 0);
		if (flatLogMode)
		{
			APIDefs->Log(ELogLevel_WARNING, ADDON_NAME,
				"ArcDPS 'boss_encounter_npc_dirs' is disabled: logs are stored flat in arcdps.cbtlogs. "
				"Parsing all logs and filtering WvW fights by fight ID. For better performance, enable "
				"boss encounter directory sorting in ArcDPS.");
		}

		if (firstInstall) {

			std::filesystem::path arcPath = getArcPath();
			Settings::LogDirectoryPath = arcPath.string();

			strncpy(Settings::LogDirectoryPathC, Settings::LogDirectoryPath.c_str(), sizeof(Settings::LogDirectoryPathC) - 1);
			Settings::LogDirectoryPathC[sizeof(Settings::LogDirectoryPathC) - 1] = '\0';

			Settings::Settings[CUSTOM_LOG_PATH] = Settings::LogDirectoryPath;
			Settings::Save(SettingsPath);
		}


		if (!Settings::LogDirectoryPath.empty())
		{
			dirPath = std::filesystem::path(Settings::LogDirectoryPath);

			if (!std::filesystem::exists(dirPath) || !std::filesystem::is_directory(dirPath))
			{
				APIDefs->Log(ELogLevel_WARNING, ADDON_NAME,
					("Specified log directory does not exist: " + dirPath.string()).c_str());
				return;
			}
		}
		else
		{
			char documentsPath[MAX_PATH];
			if (FAILED(SHGetFolderPath(nullptr, CSIDL_PERSONAL, nullptr, SHGFP_TYPE_CURRENT, documentsPath)))
			{
				APIDefs->Log(ELogLevel_WARNING, ADDON_NAME,
					"Failed to get path to user's Documents folder.");
				return;
			}

			dirPath = std::filesystem::path(documentsPath) / "Guild Wars 2" / "addons" /
				"arcdps" / "arcdps.cbtlogs";
		}

		parseInitialLogs(processedFiles, numLogsToParse);

		// Manual pure-polling fallback (e.g. change notifications misbehaving).
		if (Settings::forceLinuxCompatibilityMode)
		{
			APIDefs->Log(ELogLevel_INFO, ADDON_NAME,
				"Compatibility mode enabled: using polling for directory monitoring.");

			while (!stopMonitoring)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(pollIntervalMilliseconds));

				if (stopMonitoring)
				{
					break;
				}

				scanForNewFiles(dirPath, processedFiles);
			}
			return;
		}

		APIDefs->Log(ELogLevel_INFO, ADDON_NAME,
			"Using FindFirstChangeNotification for directory monitoring.");

		HANDLE hChange = FindFirstChangeNotificationW(
			dirPath.wstring().c_str(),
			TRUE,
			FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_SIZE |
			FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_CREATION);

		if (hChange == INVALID_HANDLE_VALUE)
		{
			DWORD err = GetLastError();
			APIDefs->Log(ELogLevel_WARNING, ADDON_NAME,
				("FindFirstChangeNotificationW failed (error " + std::to_string(err) +
					"); falling back to polling.").c_str());

			while (!stopMonitoring)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(pollIntervalMilliseconds));

				if (stopMonitoring)
				{
					break;
				}

				scanForNewFiles(dirPath, processedFiles);
			}
			return;
		}

		const DWORD waitTimeoutMs = 500;

		while (!stopMonitoring)
		{
			DWORD waitStatus = WaitForSingleObject(hChange, waitTimeoutMs);

			if (stopMonitoring)
			{
				break;
			}

			if (waitStatus == WAIT_OBJECT_0)
			{
				scanForNewFiles(dirPath, processedFiles);

				if (!FindNextChangeNotification(hChange))
				{
					DWORD err = GetLastError();
					APIDefs->Log(ELogLevel_WARNING, ADDON_NAME,
						("FindNextChangeNotification failed (error " + std::to_string(err) + ").").c_str());
					break;
				}
			}
			else if (waitStatus == WAIT_TIMEOUT)
			{
				continue;
			}
			else
			{
				DWORD err = GetLastError();
				APIDefs->Log(ELogLevel_WARNING, ADDON_NAME,
					("WaitForSingleObject failed (error " + std::to_string(err) + ").").c_str());
				break;
			}
		}

		FindCloseChangeNotification(hChange);
	}
	catch (const std::exception& ex)
	{
		APIDefs->Log(ELogLevel_WARNING, ADDON_NAME,
			("Exception in directory monitoring thread: " + std::string(ex.what())).c_str());
	}
}

static bool shouldSkipLog(const ParsedLog& log)
{
	if (log.data.fightId != 1)
	{
		APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME, ("Skipping non-WvW log: " + log.filename).c_str());
		return true;
	}

	if (log.data.totalIdentifiedPlayers == 0)
	{
		if (Settings::debugStringsMode) {
			size_t teamCount = log.data.teamStats.size();
			std::string teamInfo = "Teams found: " + std::to_string(teamCount);
			for (const auto& [teamName, stats] : log.data.teamStats) {
				teamInfo += " [" + teamName + ": " + std::to_string(stats.totalPlayers) + " players]";
			}
			APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME,
				("Skipping log with no identified players: " + log.filename + " - " + teamInfo).c_str());
		} else {
			APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME, ("Skipping log with no identified players: " + log.filename).c_str());
		}
		return true;
	}

	if (Settings::minTotalPlayers > 0 && log.data.totalIdentifiedPlayers < (size_t)Settings::minTotalPlayers)
	{
		APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME,
			("Skipping log below min total players (" + std::to_string(Settings::minTotalPlayers) + "): " + log.filename + " (" + std::to_string(log.data.totalIdentifiedPlayers) + " players)").c_str());
		return true;
	}

	if (Settings::minTotalDeaths > 0 || Settings::minTotalDowns > 0)
	{
		uint32_t totalDeaths = 0;
		uint32_t totalDowns = 0;
		for (const auto& [teamName, stats] : log.data.teamStats)
		{
			totalDeaths += stats.totalDeaths;
			totalDowns += stats.totalDowned;
		}

		if (Settings::minTotalDeaths > 0 && totalDeaths < (uint32_t)Settings::minTotalDeaths)
		{
			APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME,
				("Skipping log below min total deaths (" + std::to_string(Settings::minTotalDeaths) + "): " + log.filename + " (" + std::to_string(totalDeaths) + " deaths)").c_str());
			return true;
		}

		if (Settings::minTotalDowns > 0 && totalDowns < (uint32_t)Settings::minTotalDowns)
		{
			APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME,
				("Skipping log below min total downs (" + std::to_string(Settings::minTotalDowns) + "): " + log.filename + " (" + std::to_string(totalDowns) + " downs)").c_str());
			return true;
		}
	}

	if (Settings::minCombatDuration > 0)
	{
		double durationSec = log.data.getCombatDurationSeconds();
		if (durationSec < Settings::minCombatDuration)
		{
			APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME,
				("Skipping log below min combat duration (" + std::to_string(Settings::minCombatDuration) + "s): " + log.filename + " (" + std::to_string(durationSec) + "s)").c_str());
			return true;
		}
	}

	return false;
}

void processNewEVTCFile(const std::filesystem::path& filePath)
{
	std::string filename = getUtf8Path(filePath.filename());

	// Trigger "new log detected" animation
	newLogDetectedTime.store(static_cast<float>(ImGui::GetTime()));

	ParsedLog log;
	log.filename = filename;
	log.data = parseEVTCFile(filePath);

	if (shouldSkipLog(log))
	{
		return;
	}

	{
		std::lock_guard<std::mutex> lock(parsedLogsMutex);
		parsedLogs.push_front(log);

		while (parsedLogs.size() > Settings::logHistorySize)
		{
			parsedLogs.pop_back();
		}

		currentLogIndex = 0;
	}

	{
		std::lock_guard<std::mutex> lock(aggregateStatsMutex);

		// Update total combat time
		uint64_t fightDuration = (log.data.combatEndTime - log.data.combatStartTime);
		globalAggregateStats.totalCombatTime += fightDuration;
		globalAggregateStats.combatInstanceCount++;

		// Aggregate team data
		for (const auto& [teamName, stats] : log.data.teamStats)
		{
			auto& teamAgg = globalAggregateStats.teamAggregates[teamName];

			// Update full team totals
			teamAgg.teamTotals.totalPlayers += stats.totalPlayers;
			teamAgg.teamTotals.totalDeaths += stats.totalDeaths;
			teamAgg.teamTotals.totalDowned += stats.totalDowned;
			teamAgg.teamTotals.instanceCount++;

			for (const auto& [eliteSpec, specStats] : stats.eliteSpecStats)
			{
				teamAgg.teamTotals.eliteSpecTotals[eliteSpec].totalCount += specStats.count;
			}

			// POV team updates
			if (stats.isPOVTeam) {
				teamAgg.isPOVTeam = true;
				teamAgg.povSquadTotals.totalPlayers += stats.squadStats.totalPlayers;
				teamAgg.povSquadTotals.totalDeaths += stats.squadStats.totalDeaths;
				teamAgg.povSquadTotals.totalDowned += stats.squadStats.totalDowned;
				teamAgg.povSquadTotals.instanceCount++;

				for (const auto& [eliteSpec, specStats] : stats.squadStats.eliteSpecStats)
				{
					teamAgg.povSquadTotals.eliteSpecTotals[eliteSpec].totalCount += specStats.count;
				}
			}
		}

		// Compute and cache averages
		cachedAverages.averageCombatTime = globalAggregateStats.getAverageCombatTime();
		cachedAverages.averageTeamPlayerCounts.clear();
		cachedAverages.averageTeamSpecCounts.clear();
		cachedAverages.averagePOVSquadPlayerCounts.clear();
		cachedAverages.averagePOVSquadSpecCounts.clear();

		for (const auto& [teamName, teamAgg] : globalAggregateStats.teamAggregates)
		{
			// Full team averages
			cachedAverages.averageTeamPlayerCounts[teamName] = teamAgg.getAverageTeamPlayerCount();

			for (const auto& [specName, _] : teamAgg.teamTotals.eliteSpecTotals)
			{
				cachedAverages.averageTeamSpecCounts[teamName][specName] = teamAgg.getAverageTeamSpecCount(specName);
			}

			// POV squad averages if POV
			if (teamAgg.isPOVTeam) {
				cachedAverages.averagePOVSquadPlayerCounts[teamName] = teamAgg.getAveragePOVSquadPlayerCount();
				for (const auto& [specName, _] : teamAgg.povSquadTotals.eliteSpecTotals)
				{
					cachedAverages.averagePOVSquadSpecCounts[teamName][specName] = teamAgg.getAveragePOVSquadSpecCount(specName);
				}
			}
		}
	}

	if (Settings::showNewParseAlert) {
		std::string displayName = generateLogDisplayName(log.filename, log.data.combatStartTime, log.data.combatEndTime);
		APIDefs->UI.SendAlert(("Parsed New Log: " + displayName).c_str());
	}

	// Trigger "parsing complete" animation
	parseCompleteTime.store(static_cast<float>(ImGui::GetTime()));

	LogParsedEventArgs args{log.filename.c_str(), &log.data};
	APIDefs->Events.Raise(EV_LOG_PARSED, &args);
}
