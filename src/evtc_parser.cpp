#define NOMINMAX
#include "evtc_parser.h"
#include "Settings.h"
#include "Shared.h"
#include "utils.h"
#include <zip.h>
#include <thread>
#include <chrono>
#include <filesystem>
#include <shlobj.h>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <cstring>
#include <algorithm>
#include <vector>
#include <mutex>

std::unordered_set<std::wstring> processedFiles;
std::filesystem::file_time_type maxProcessedTime = std::filesystem::file_time_type::min();

void updateStats(TeamStats& teamStats, Agent* agent, int32_t value, bool isDamage, bool isKill, bool vsPlayer,
	bool isStrikeDamage, bool isCondiDamage, bool isDownedContribution, bool isKillContribution) {
	auto& specStats = teamStats.eliteSpecStats[agent->eliteSpec];

	if (isDamage) {
		teamStats.totalDamage += value;
		specStats.totalDamage += value;

		if (isStrikeDamage) {
			teamStats.totalStrikeDamage += value;
			specStats.totalStrikeDamage += value;
		}
		else if (isCondiDamage) {
			teamStats.totalCondiDamage += value;
			specStats.totalCondiDamage += value;
		}

		if (vsPlayer) {
			teamStats.totalDamageVsPlayers += value;
			specStats.totalDamageVsPlayers += value;

			if (isStrikeDamage) {
				teamStats.totalStrikeDamageVsPlayers += value;
				specStats.totalStrikeDamageVsPlayers += value;
			}
			else if (isCondiDamage) {
				teamStats.totalCondiDamageVsPlayers += value;
				specStats.totalCondiDamageVsPlayers += value;
			}
		}

		if (isDownedContribution) {
			teamStats.totalDownedContribution += value;
			specStats.totalDownedContribution += value;
			if (vsPlayer) {
				teamStats.totalDownedContributionVsPlayers += value;
				specStats.totalDownedContributionVsPlayers += value;
			}
		}

		if (isKillContribution) {
			teamStats.totalKillContribution += value;
			specStats.totalKillContribution += value;
			if (vsPlayer) {
				teamStats.totalKillContributionVsPlayers += value;
				specStats.totalKillContributionVsPlayers += value;
			}
		}
	}

	if (isKill) {
		teamStats.totalKills++;
		specStats.totalKills++;
		if (vsPlayer) {
			teamStats.totalKillsVsPlayers++;
			specStats.totalKillsVsPlayers++;
		}
	}

	if (teamStats.isPOVTeam && agent->subgroupNumber > 0) {
		auto& squadStats = teamStats.squadStats;
		auto& squadSpecStats = squadStats.eliteSpecStats[agent->eliteSpec];

		if (isDamage) {
			squadStats.totalDamage += value;
			squadSpecStats.totalDamage += value;

			if (isStrikeDamage) {
				squadStats.totalStrikeDamage += value;
				squadSpecStats.totalStrikeDamage += value;
			}
			else if (isCondiDamage) {
				squadStats.totalCondiDamage += value;
				squadSpecStats.totalCondiDamage += value;
			}

			if (vsPlayer) {
				squadStats.totalDamageVsPlayers += value;
				squadSpecStats.totalDamageVsPlayers += value;

				if (isStrikeDamage) {
					squadStats.totalStrikeDamageVsPlayers += value;
					squadSpecStats.totalStrikeDamageVsPlayers += value;
				}
				else if (isCondiDamage) {
					squadStats.totalCondiDamageVsPlayers += value;
					squadSpecStats.totalCondiDamageVsPlayers += value;
				}
			}

			if (isDownedContribution) {
				squadStats.totalDownedContribution += value;
				squadSpecStats.totalDownedContribution += value;
				if (vsPlayer) {
					squadStats.totalDownedContributionVsPlayers += value;
					squadSpecStats.totalDownedContributionVsPlayers += value;
				}
			}

			if (isKillContribution) {
				squadStats.totalKillContribution += value;
				squadSpecStats.totalKillContribution += value;
				if (vsPlayer) {
					squadStats.totalKillContributionVsPlayers += value;
					squadSpecStats.totalKillContributionVsPlayers += value;
				}
			}
		}

		if (isKill) {
			squadStats.totalKills++;
			squadSpecStats.totalKills++;
			if (vsPlayer) {
				squadStats.totalKillsVsPlayers++;
				squadSpecStats.totalKillsVsPlayers++;
			}
		}
	}
}

void parseAgents(const std::vector<char>& bytes, size_t& offset, uint32_t agentCount,
	std::unordered_map<uint64_t, Agent>& agentsByAddress) {

	const size_t agentBlockSize = 96; // Each agent block is 96 bytes

	for (uint32_t i = 0; i < agentCount; ++i) {
		if (offset + agentBlockSize > bytes.size()) {
			APIDefs->Log(ELogLevel_WARNING, ADDON_NAME, "Insufficient data for agent block");
			break;
		}

		Agent agent;
		std::memcpy(&agent.address, bytes.data() + offset, sizeof(uint64_t));
		std::memcpy(&agent.professionId, bytes.data() + offset + 8, sizeof(uint32_t));
		std::memcpy(&agent.eliteSpecId, bytes.data() + offset + 12, sizeof(int32_t));

		// Read the full 68-byte name field
		char nameData[68];
		std::memcpy(nameData, bytes.data() + offset + 28, 68);
		nameData[67] = '\0'; // Ensure null termination

		// Split the nameData into substrings
		std::vector<std::string> splitStr;
		size_t start = 0;
		while (start < 68) {
			size_t end = start;
			while (end < 68 && nameData[end] != '\0') {
				++end;
			}
			if (start != end) {
				splitStr.emplace_back(nameData + start, nameData + end);
			}
			start = end + 1;
			if (end >= 67) {
				break;
			}
		}

		// Assign character name, account name, and subgroup
		if (!splitStr.empty()) {
			agent.name = splitStr[0]; // Character name
		}
		else {
			agent.name = "";
		}
		if (splitStr.size() > 1) {
			agent.accountName = splitStr[1]; // Account name
		}
		else {
			agent.accountName = "";
		}
		if (splitStr.size() > 2) {
			agent.subgroup = splitStr[2]; // Subgroup as string
		}
		else {
			agent.subgroup = "";
		}

		if (!agent.subgroup.empty()) {
			try {
				agent.subgroupNumber = std::stoi(agent.subgroup);
			}
			catch (const std::exception&) {
				agent.subgroupNumber = -1; // Invalid subgroup number
			}
		}
		else {
			agent.subgroupNumber = -1; // No subgroup information
		}

		if (professions.find(agent.professionId) != professions.end()) {
			agent.profession = professions[agent.professionId];
			if (agent.eliteSpecId != -1 && eliteSpecs.find(agent.eliteSpecId) != eliteSpecs.end()) {
				agent.eliteSpec = eliteSpecs[agent.eliteSpecId];
			}
			else {
				agent.eliteSpec = "Core " + agent.profession;
			}
			agentsByAddress[agent.address] = agent;
		}

		offset += agentBlockSize;
	}
}

bool isDamageInDownSequence(const Agent* agent, const std::vector<CombatEvent>& events, uint64_t currentTime) {
	bool currentlyDowned = false;
	for (const auto& event : events) {
		if (event.time < currentTime) {
			if (event.isStateChange == static_cast<uint8_t>(StateChange::ChangeDown) &&
				event.srcAgent == agent->address) {
				currentlyDowned = true;
			}
			else if (event.isStateChange == static_cast<uint8_t>(StateChange::ChangeUp) &&
				event.srcAgent == agent->address) {
				currentlyDowned = false;
			}
		}
	}
	if (currentlyDowned) return false;

	uint64_t nextDownTime = UINT64_MAX;
	for (const auto& event : events) {
		if (event.time > currentTime &&
			event.isStateChange == static_cast<uint8_t>(StateChange::ChangeDown) &&
			event.srcAgent == agent->address) {
			nextDownTime = event.time;
			break;
		}
	}

	if (nextDownTime == UINT64_MAX) return false;

	uint64_t lastHighHealthTime = 0;
	const uint64_t TWO_SECONDS = 2000;

	for (auto it = events.rbegin(); it != events.rend(); ++it) {
		const auto& event = *it;
		if (event.time >= currentTime) continue;

		if (event.isStateChange == static_cast<uint8_t>(StateChange::HealthUpdate) &&
			event.srcAgent == agent->address) {
			float healthPercent = (event.dstAgent * 100.0f) / event.value;
			if (healthPercent > 98.0f) {
				lastHighHealthTime = event.time;
				break;
			}
		}
	}

	if (lastHighHealthTime > 0) {
		return currentTime >= lastHighHealthTime - TWO_SECONDS &&
			currentTime <= nextDownTime;
	}

	return false;
}

bool isDamageInKillSequence(const Agent* agent, const std::vector<CombatEvent>& events, uint64_t currentTime) {
	bool currentlyDowned = false;
	for (const auto& event : events) {
		if (event.time < currentTime) {
			if (event.isStateChange == static_cast<uint8_t>(StateChange::ChangeDown) &&
				event.srcAgent == agent->address) {
				currentlyDowned = true;
			}
			else if (event.isStateChange == static_cast<uint8_t>(StateChange::ChangeUp) &&
				event.srcAgent == agent->address) {
				currentlyDowned = false;
			}
			else if (event.isStateChange == static_cast<uint8_t>(StateChange::ChangeDead) &&
				event.srcAgent == agent->address) {
				currentlyDowned = false;
			}
		}
	}
	if (!currentlyDowned) return false;

	uint64_t deathTime = UINT64_MAX;
	for (const auto& event : events) {
		if (event.time > currentTime &&
			event.isStateChange == static_cast<uint8_t>(StateChange::ChangeDead) &&
			event.srcAgent == agent->address) {
			deathTime = event.time;
			break;
		}
	}

	if (deathTime == UINT64_MAX) return false;

	uint64_t downTime = 0;
	const uint64_t TWO_SECONDS = 2000;

	for (auto it = events.rbegin(); it != events.rend(); ++it) {
		const auto& event = *it;
		if (event.time >= currentTime) continue;

		if (event.isStateChange == static_cast<uint8_t>(StateChange::ChangeDown) &&
			event.srcAgent == agent->address) {
			downTime = event.time;
			break;
		}
	}

	if (downTime > 0) {
		return currentTime >= downTime - TWO_SECONDS &&
			currentTime <= deathTime;
	}

	return false;
}

void parseCombatEvents(const std::vector<char>& bytes, size_t offset, size_t eventCount,
	std::unordered_map<uint64_t, Agent>& agentsByAddress,
	std::unordered_map<uint16_t, Agent*>& playersBySrcInstid,
	ParsedData& result) {

	uint64_t logStartTime = UINT64_MAX;
	uint64_t logEndTime = 0;
	result.combatStartTime = UINT64_MAX;
	result.combatEndTime = 0;
	uint64_t earliestTime = UINT64_MAX;
	uint64_t latestTime = 0;
	uint64_t povAgentID = 0;

	const size_t eventSize = sizeof(CombatEvent);

	// Create a mapping from instance IDs to agents
	std::unordered_map<uint16_t, Agent*> agentsByInstid;

	// First collect all events
	std::vector<CombatEvent> allEvents;
	allEvents.reserve(eventCount); // Preallocate for efficiency

	for (size_t i = 0; i < eventCount; ++i) {
		size_t eventOffset = offset + (i * eventSize);
		if (eventOffset + eventSize > bytes.size()) {
			break;
		}
		CombatEvent event;
		std::memcpy(&event, bytes.data() + eventOffset, eventSize);
		allEvents.push_back(event);
	}

	// Process all events in a single pass for state changes and agent mapping
	for (const auto& event : allEvents) {
		earliestTime = std::min(earliestTime, event.time);
		latestTime = std::max(latestTime, event.time);

		switch (static_cast<StateChange>(event.isStateChange)) {
		case StateChange::LogStart:
			logStartTime = event.time;
			break;
		case StateChange::LogEnd:
			logEndTime = event.time;
			break;
		case StateChange::EnterCombat:
			result.combatStartTime = std::min(result.combatStartTime, event.time);
			break;
		case StateChange::ExitCombat:
			result.combatEndTime = std::max(result.combatEndTime, event.time);
			break;
		case StateChange::PointOfView:
			povAgentID = event.srcAgent;
			break;
		case StateChange::None:
			// Map srcInstid
			if (agentsByAddress.find(event.srcAgent) != agentsByAddress.end()) {
				Agent& agent = agentsByAddress[event.srcAgent];
				agent.id = event.srcInstid;
				agentsByInstid[event.srcInstid] = &agent;
				playersBySrcInstid[event.srcInstid] = &agent;
			}
			// Map dstInstid
			if (agentsByAddress.find(event.dstAgent) != agentsByAddress.end()) {
				Agent& agent = agentsByAddress[event.dstAgent];
				agent.id = event.dstInstid;
				agentsByInstid[event.dstInstid] = &agent;
			}
			break;
		case StateChange::TeamChange: {
			uint32_t teamID = static_cast<uint32_t>(event.value);
			if (teamID != 0 && agentsByAddress.find(event.srcAgent) != agentsByAddress.end()) {
				auto it = teamIDs.find(teamID);
				if (it != teamIDs.end()) {
					Agent& agent = agentsByAddress[event.srcAgent];
					agent.team = it->second;
				}
			}
			break;
		}
		default:
			break;
		}
	}

	// Set POV team after processing all events
	std::string povTeam;
	if (agentsByAddress.find(povAgentID) != agentsByAddress.end()) {
		Agent& povAgent = agentsByAddress[povAgentID];
		povTeam = povAgent.team;
		if (!povTeam.empty() && povTeam != "Unknown") {
			result.teamStats[povTeam].isPOVTeam = true;
			APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME,
				("POV Agent Team: " + povTeam).c_str());
		}
		else {
			APIDefs->Log(ELogLevel_WARNING, ADDON_NAME,
				("POV Agent's team is unknown - AgentID: " + std::to_string(povAgentID)).c_str());
		}
	}

	if (result.combatStartTime == UINT64_MAX) {
		result.combatStartTime = (logStartTime != UINT64_MAX) ? logStartTime : earliestTime;
	}
	if (result.combatEndTime == 0) {
		result.combatEndTime = (logEndTime != 0) ? logEndTime : latestTime;
	}

	// Process deaths and downs events
	for (const auto& event : allEvents) {
		StateChange stateChange = static_cast<StateChange>(event.isStateChange);
		if (stateChange == StateChange::ChangeDead || stateChange == StateChange::ChangeDown) {
			uint16_t srcInstid = event.srcInstid;
			auto agentIt = agentsByInstid.find(srcInstid);
			if (agentIt != agentsByInstid.end()) {
				Agent* agent = agentIt->second;
				const std::string& team = agent->team;
				if (team != "Unknown") {
					auto& teamStats = result.teamStats[team];
					if (stateChange == StateChange::ChangeDead) {
						teamStats.totalDeaths++;
						if (teamStats.isPOVTeam && agent->subgroupNumber > 0) {
							teamStats.squadStats.totalDeaths++;
						}
					}
					else {
						teamStats.totalDowned++;
						if (teamStats.isPOVTeam && agent->subgroupNumber > 0) {
							teamStats.squadStats.totalDowned++;
						}
					}
				}
			}
		}
	}

	// Process damage and kill events
	for (const auto& event : allEvents) {
		if (event.isStateChange == static_cast<uint8_t>(StateChange::None) &&
			event.isActivation == static_cast<uint8_t>(Activation::None) &&
			event.isBuffRemove == static_cast<uint8_t>(BuffRemove::None)) {

			ResultCode resultCode = static_cast<ResultCode>(event.result);

			if (resultCode == ResultCode::Normal || resultCode == ResultCode::Critical ||
				resultCode == ResultCode::Glance || resultCode == ResultCode::KillingBlow) {

				int32_t damageValue = 0;
				bool isStrikeDamage = false;
				bool isCondiDamage = false;

				if (event.buff == 0) {
					damageValue = event.value;
					isStrikeDamage = true;
				}
				else if (event.buff == 1) {
					damageValue = event.buffDmg;
					isCondiDamage = true;
				}

				if (damageValue > 0) {
					auto srcIt = playersBySrcInstid.find(event.srcInstid);
					if (srcIt != playersBySrcInstid.end()) {
						Agent* attacker = srcIt->second;
						const std::string& attackerTeam = attacker->team;

						if (attackerTeam != "Unknown") {
							bool vsPlayer = false;
							bool isDownedContribution = false;
							bool isKillContribution = false;

							auto dstIt = agentsByInstid.find(event.dstInstid);
							if (dstIt != agentsByInstid.end()) {
								Agent* target = dstIt->second;
								if (target->team != "Unknown") {
									vsPlayer = true;
									isDownedContribution = isDamageInDownSequence(target, allEvents, event.time);
									//isKillContribution = isDamageInKillSequence(target, allEvents, event.time);
								}
							}

							updateStats(result.teamStats[attackerTeam], attacker, damageValue, true, false,
								vsPlayer, isStrikeDamage, isCondiDamage, isDownedContribution, isKillContribution);
						}
					}
				}
			}

			// Process KillingBlow events
			if (resultCode == ResultCode::KillingBlow) {
				auto srcIt = playersBySrcInstid.find(event.srcInstid);
				if (srcIt != playersBySrcInstid.end()) {
					Agent* attacker = srcIt->second;
					const std::string& attackerTeam = attacker->team;

					if (attackerTeam != "Unknown") {
						auto dstIt = agentsByInstid.find(event.dstInstid);
						if (dstIt != agentsByInstid.end()) {
							Agent* target = dstIt->second;
							const std::string& targetTeam = target->team;

							if (targetTeam != "Unknown") {
								result.teamStats[targetTeam].totalDeathsFromKillingBlows++;
								if (result.teamStats[targetTeam].isPOVTeam && target->subgroupNumber > 0) {
									result.teamStats[targetTeam].squadStats.totalDeathsFromKillingBlows++;
								}
								updateStats(result.teamStats[attackerTeam], attacker, 0, false, true, true, false, false, false, false);
							}
						}
					}
				}
			}
		}
	}

	// Collect statistics
	for (const auto& [srcInstid, agent] : playersBySrcInstid) {
		if (agent->team != "Unknown") {
			auto& teamStats = result.teamStats[agent->team];
			auto& specStats = teamStats.eliteSpecStats[agent->eliteSpec];

			teamStats.totalPlayers++;
			specStats.count++;
			result.totalIdentifiedPlayers++;

			if (teamStats.isPOVTeam && agent->subgroupNumber > 0) {
				auto& squadStats = teamStats.squadStats;
				auto& squadSpecStats = squadStats.eliteSpecStats[agent->eliteSpec];

				squadStats.totalPlayers++;
				squadSpecStats.count++;
			}
		}
		else {
			APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME,
				("Agent with unknown team - InstID: " + std::to_string(srcInstid) +
					", Name: " + agent->name).c_str());
		}
	}
}

ParsedData parseEVTCFile(const std::string& filePath) {
	ParsedData result;
	std::vector<char> bytes = extractZipFile(filePath);
	if (bytes.size() < 16) {
		APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME, "EVTC file is too small");
		return result;
	}

	size_t offset = 0;

	// Read header (12 bytes)
	char header[13] = { 0 };
	std::memcpy(header, bytes.data() + offset, 12);
	offset += 12;

	// Read revision (1 byte)
	uint8_t revision;
	std::memcpy(&revision, bytes.data() + offset, sizeof(uint8_t));
	offset += sizeof(uint8_t);

	// Read fight instance ID (uint16_t)
	uint16_t fightId;
	std::memcpy(&fightId, bytes.data() + offset, sizeof(uint16_t));
	offset += sizeof(uint16_t);

	// skip (1 byte)
	uint8_t skipByte;
	std::memcpy(&skipByte, bytes.data() + offset, sizeof(uint8_t));
	offset += sizeof(uint8_t);

	std::string headerStr(header);

	if (headerStr.rfind("EVTC", 0) != 0 ||
		!std::all_of(headerStr.begin() + 4, headerStr.end(), ::isdigit)) {
		APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME, ("Not EVTC " + headerStr).c_str());
		return ParsedData(); // Return an empty result
	}

	APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME, ("Header: " + headerStr + ", Revision: " + std::to_string(revision) + ", Fight Instance ID: " + std::to_string(fightId)).c_str());
	int evtcVersion = std::stoi(headerStr.substr(4));

	if (evtcVersion < 20240612) {
		APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME, ("Cannot parse EVTC Version is pre TeamChangeOnDespawn / 20240612"));
		return ParsedData(); // Return an empty result
	}

	// Check if fightId is 1 (WvW)
	if (fightId != 1) {
		APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME, ("Skipping non-WvW log. FightInstanceID: " + std::to_string(fightId)).c_str());
		return ParsedData(); // Return an empty result
	}
	

	result.fightId = fightId;

	// Read agent count (uint32_t)
	if (offset + sizeof(uint32_t) > bytes.size()) {
		APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME, "Incomplete EVTC file: Missing agent count");
		return result;
	}
	uint32_t agentCount;
	std::memcpy(&agentCount, bytes.data() + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	std::unordered_map<uint64_t, Agent> agentsByAddress;
	parseAgents(bytes, offset, agentCount, agentsByAddress);

	// Read skill count (uint32_t)
	if (offset + sizeof(uint32_t) > bytes.size()) {
		APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME, "Incomplete EVTC file: Missing skill count");
		return result;
	}
	uint32_t skillCount;
	std::memcpy(&skillCount, bytes.data() + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	// Skip skills (68 bytes per skill)
	size_t skillsSize = 68 * skillCount;
	if (offset + skillsSize > bytes.size()) {
		APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME, "Incomplete EVTC file: Skills data missing");
		return result;
	}
	offset += skillsSize;

	// Parse combat events
	const size_t eventSize = sizeof(CombatEvent);
	size_t remainingBytes = bytes.size() - offset;
	size_t eventCount = remainingBytes / eventSize;

	std::unordered_map<uint16_t, Agent*> playersBySrcInstid;
	parseCombatEvents(bytes, offset, eventCount, agentsByAddress, playersBySrcInstid, result);


	return result;
}

std::wstring getCanonicalPath(const std::filesystem::path& path)
{
	return std::filesystem::weakly_canonical(path).wstring();
}

bool isValidEVTCFile(const std::filesystem::path& dirPath, const std::filesystem::path& filePath)
{
	std::filesystem::path relativePath;
	try {
		relativePath = std::filesystem::relative(filePath.parent_path(), dirPath);
	}
	catch (...) {
		return false;
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
	waitForFile(filePath.string());
	processNewEVTCFile(filePath.string());
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

		// Sort escending order (newest first)
		std::sort(zevtcFiles.begin(), zevtcFiles.end(),
			[](const std::filesystem::path& a, const std::filesystem::path& b)
			{
				return std::filesystem::last_write_time(a) > std::filesystem::last_write_time(b);
			});

		size_t numFilesToParse = std::min<size_t>(zevtcFiles.size(), numLogsToParse);

		for (size_t i = 0; i < numFilesToParse; ++i)
		{
			const std::filesystem::path& filePath = zevtcFiles[i];
			waitForFile(filePath.string());

			std::wstring absolutePath = std::filesystem::absolute(filePath).wstring();

			if (processedFiles.find(absolutePath) == processedFiles.end())
			{
				ParsedLog log;
				log.filename = filePath.filename().string();
				log.data = parseEVTCFile(filePath.string());

				// fightId 1 = WvW
				if (log.data.fightId != 1)
				{
					APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME,
						("Skipping non-WvW log during initial parsing: " + log.filename).c_str());
					continue;
				}

				if (log.data.totalIdentifiedPlayers == 0)
				{
					APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME,
						("Skipping log with no identified players during initial parsing: " + log.filename).c_str());
					continue;
				}

				parsedLogs.push_back(log);

				while (parsedLogs.size() > Settings::logHistorySize)
				{
					parsedLogs.pop_front();
				}
				currentLogIndex = 0;

				processedFiles.insert(absolutePath);

				// In the Wine implementation we will only parse logs with newer time stamps.
				// Not quite the same behaviour but good enough
				auto fileTime = std::filesystem::last_write_time(filePath);
				if (fileTime > maxProcessedTime)
				{
					maxProcessedTime = fileTime;
				}
			}
			else
			{
				APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME,
					("File already processed during initial parsing: " + filePath.string()).c_str());
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


			processEVTCFile(filePath.string());

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

// ReadDirectoryChangesW doesn't seem to work under Wine, we'll keep using this version on Windows though
void directoryMonitor(const std::filesystem::path& dirPath, size_t numLogsToParse)
{
	try
	{
		HANDLE hDir = CreateFileW(
			dirPath.wstring().c_str(),
			FILE_LIST_DIRECTORY,
			FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			nullptr,
			OPEN_EXISTING,
			FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
			nullptr);

		if (hDir == INVALID_HANDLE_VALUE)
		{
			APIDefs->Log(ELogLevel_WARNING, ADDON_NAME,
				("Failed to open directory for monitoring: " + dirPath.string()).c_str());
			return;
		}

		char buffer[4096];
		OVERLAPPED overlapped = { 0 };
		overlapped.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);

		parseInitialLogs(processedFiles, numLogsToParse);

		BOOL success = ReadDirectoryChangesW(
			hDir,
			buffer,
			sizeof(buffer),
			TRUE,
			FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_SIZE |
			FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_CREATION,
			nullptr,
			&overlapped,
			nullptr);

		if (!success)
		{
			APIDefs->Log(ELogLevel_WARNING, ADDON_NAME, "Failed to start directory monitoring.");
			CloseHandle(overlapped.hEvent);
			CloseHandle(hDir);
			return;
		}

		while (!stopMonitoring)
		{
			DWORD waitStatus = WaitForSingleObject(overlapped.hEvent, 500);

			if (stopMonitoring)
			{
				CancelIoEx(hDir, &overlapped);
				break;
			}

			if (waitStatus == WAIT_OBJECT_0)
			{
				DWORD bytesTransferred = 0;
				if (GetOverlappedResult(hDir, &overlapped, &bytesTransferred, FALSE))
				{
					BYTE* pBuffer = reinterpret_cast<BYTE*>(buffer);
					FILE_NOTIFY_INFORMATION* fni = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(pBuffer);

					do
					{
						std::wstring filenameW(fni->FileName, fni->FileNameLength / sizeof(WCHAR));
						std::filesystem::path fullPath = dirPath / filenameW;

						// Filter for .zevtc files
						if ((fni->Action == FILE_ACTION_ADDED ||
							fni->Action == FILE_ACTION_MODIFIED ||
							fni->Action == FILE_ACTION_RENAMED_NEW_NAME) &&
							fullPath.extension() == L".zevtc")
						{
							if (isValidEVTCFile(dirPath, fullPath))
							{
								std::wstring fullPathStr = std::filesystem::absolute(fullPath).wstring();

								{
									std::lock_guard<std::mutex> lock(processedFilesMutex);

									if (processedFiles.find(fullPathStr) == processedFiles.end())
									{
										processEVTCFile(fullPath);

										processedFiles.insert(fullPathStr);
									}
								}
							}
						}

						if (fni->NextEntryOffset == 0)
							break;

						fni = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(
							reinterpret_cast<BYTE*>(fni) + fni->NextEntryOffset);
					} while (true);

					ResetEvent(overlapped.hEvent);

					success = ReadDirectoryChangesW(
						hDir,
						buffer,
						sizeof(buffer),
						TRUE,
						FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_SIZE |
						FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_CREATION,
						nullptr,
						&overlapped,
						nullptr);

					if (!success)
					{
						APIDefs->Log(ELogLevel_WARNING, ADDON_NAME,
							"Failed to reissue directory monitoring.");
						break;
					}
				}
				else
				{
					DWORD errorCode = GetLastError();
					APIDefs->Log(ELogLevel_WARNING, ADDON_NAME,
						("GetOverlappedResult failed with error code: " + std::to_string(errorCode)).c_str());
					break;
				}
			}
			else if (waitStatus == WAIT_TIMEOUT)
			{
				continue;
			}
			else
			{
				DWORD errorCode = GetLastError();
				APIDefs->Log(ELogLevel_WARNING, ADDON_NAME,
					("WaitForSingleObject failed with error code: " + std::to_string(errorCode)).c_str());
				break;
			}
		}

		CloseHandle(overlapped.hEvent);
		CloseHandle(hDir);
	}
	catch (const std::exception& ex)
	{
		APIDefs->Log(ELogLevel_WARNING, ADDON_NAME,
			("Exception in directory monitoring thread: " + std::string(ex.what())).c_str());
	}
}

void monitorDirectory(size_t numLogsToParse, size_t pollIntervalMilliseconds)
{
	try
	{
		std::filesystem::path dirPath;

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

		// Determine if we're running under Wine
		bool runningUnderWine = isRunningUnderWine();
		if (runningUnderWine)
		{
			APIDefs->Log(ELogLevel_INFO, ADDON_NAME, "Running under Wine/Proton. Using polling method.");

			parseInitialLogs(processedFiles, numLogsToParse);

			while (!stopMonitoring)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(pollIntervalMilliseconds));

				if (stopMonitoring)
				{
					break;
				}

				scanForNewFiles(dirPath, processedFiles);
			}
		}
		else
		{
			APIDefs->Log(ELogLevel_INFO, ADDON_NAME, "Running under Windows. Using ReadDirectoryChangesW method.");

			directoryMonitor(dirPath, numLogsToParse);
		}
	}
	catch (const std::exception& ex)
	{
		APIDefs->Log(ELogLevel_WARNING, ADDON_NAME,
			("Exception in directory monitoring thread: " + std::string(ex.what())).c_str());
	}
}

void processNewEVTCFile(const std::string& filePath)
{
	std::filesystem::path path(filePath);
	std::string filename = path.filename().string();

	ParsedLog log;
	log.filename = filename;
	log.data = parseEVTCFile(filePath);

	// Validate WvW log
	if (log.data.fightId != 1)
	{
		APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME, ("Skipping non-WvW log: " + filename).c_str());
		return;
	}

	if (log.data.totalIdentifiedPlayers == 0)
	{
		APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME, ("Skipping log with no identified players: " + filename).c_str());
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
}







