#define NOMINMAX
#include "parser/evtc_parser.h"
#include "parser/file_helpers.h"
#include "settings/Settings.h"
#include "shared/Shared.h"
#include "utils/Utils.h"
#include "parser/statistics_helper.h"
#include <thread>
#include <chrono>
#include <filesystem>
#include <shlobj.h>
#include <unordered_map>
#include <unordered_set>
#include <cstring>
#include <algorithm>
#include <vector>
#include <mutex>

std::unordered_map<uint64_t, AgentState> preProcessAgentStates(const std::vector<CombatEvent>& events) {
	std::unordered_map<uint64_t, AgentState> agentStates;

	for (const auto& event : events) {
		auto& state = agentStates[event.srcAgent];

		// Store all state change events for precise sequencing
		if (event.isStateChange == static_cast<uint8_t>(StateChange::ChangeDown) ||
			event.isStateChange == static_cast<uint8_t>(StateChange::ChangeUp) ||
			event.isStateChange == static_cast<uint8_t>(StateChange::ChangeDead) ||
			event.isStateChange == static_cast<uint8_t>(StateChange::HealthUpdate)) {
			state.relevantEvents.push_back(event);
		}

		// Also maintain interval structures for quick filtering
		if (event.isStateChange == static_cast<uint8_t>(StateChange::ChangeDown)) {
			state.downIntervals.emplace_back(event.time, UINT64_MAX);
			state.currentlyDowned = true;
		}
		else if (event.isStateChange == static_cast<uint8_t>(StateChange::ChangeUp)) {
			if (state.currentlyDowned && !state.downIntervals.empty()) {
				state.downIntervals.back().second = event.time;
				state.currentlyDowned = false;
			}
		}
		else if (event.isStateChange == static_cast<uint8_t>(StateChange::ChangeDead)) {
			if (state.currentlyDowned && !state.downIntervals.empty()) {
				state.downIntervals.back().second = event.time;
				state.currentlyDowned = false;
			}
			state.deathIntervals.emplace_back(event.time, UINT64_MAX);
		}
		else if (event.isStateChange == static_cast<uint8_t>(StateChange::HealthUpdate)) {
			float healthPercent = (event.dstAgent * 100.0f) / event.value;
			state.healthUpdates.emplace_back(event.time, healthPercent);
		}
	}

	// Sort all intervals and events
	for (auto& [_, state] : agentStates) {
		std::sort(state.healthUpdates.begin(), state.healthUpdates.end());
		std::sort(state.downIntervals.begin(), state.downIntervals.end());
		std::sort(state.deathIntervals.begin(), state.deathIntervals.end());

		// Use explicit comparison function for CombatEvents
		std::sort(state.relevantEvents.begin(), state.relevantEvents.end(),
			[](const CombatEvent& a, const CombatEvent& b) -> bool {
				return a.time < b.time;
			});
	}

	return agentStates;
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
static constexpr uint8_t SC_ID_TO_GUID = 46;

static std::string guidToHex(uint64_t first8, uint64_t last8) {
	static const char hex[] = "0123456789ABCDEF";
	std::string s;
	s.reserve(32);
	for (int i = 0; i < 8; ++i) {
		uint8_t b = (first8 >> (i * 8)) & 0xFF;
		s += hex[b >> 4]; s += hex[b & 0xF];
	}
	for (int i = 0; i < 8; ++i) {
		uint8_t b = (last8 >> (i * 8)) & 0xFF;
		s += hex[b >> 4]; s += hex[b & 0xF];
	}
	return s;
}

static const std::unordered_map<std::string, std::string> WVW_TEAM_COLOR_GUIDS = {
	{"BC8AEAEF73DC8C43B041CEDFEA4D5020", "Green"},
	{"5D22513B9498EB48944E94EC7A8DD657", "Red"},
	{"CF6F7C254FCB184CBCCE4738EADD8388", "Blue"},
};

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
	std::unordered_map<uint16_t, Agent*> agentsByInstid;
	std::unordered_map<uint64_t, uint16_t> ptr_to_instid;
	std::unordered_set<uint64_t> active_ptrs;

	std::vector<CombatEvent> allEvents;
	allEvents.reserve(eventCount);

	// First pass: collect all events
	for (size_t i = 0; i < eventCount; ++i) {
		size_t eventOffset = offset + (i * eventSize);
		if (eventOffset + eventSize > bytes.size()) {
			break;
		}
		CombatEvent event;
		std::memcpy(&event, bytes.data() + eventOffset, eventSize);
		allEvents.push_back(event);
	}

	auto agentStates = preProcessAgentStates(allEvents);

	// Pre-scan IDToGUID events to auto-detect WvW team colors from stable GUIDs.
	std::unordered_map<uint32_t, std::string> teamIdToColor;
	for (const auto& ev : allEvents) {
		if (ev.isStateChange != SC_ID_TO_GUID || ev.skillId == 0) continue;
		auto it = WVW_TEAM_COLOR_GUIDS.find(guidToHex(ev.srcAgent, ev.dstAgent));
		if (it != WVW_TEAM_COLOR_GUIDS.end())
			teamIdToColor[ev.skillId] = it->second;
	}

	// Process all events
	for (const auto& event : allEvents) {
		earliestTime = std::min(earliestTime, event.time);
		latestTime = std::max(latestTime, event.time);

		if (event.srcAgent != 0 && event.srcInstid != 0 &&
			agentsByAddress.count(event.srcAgent)) {
			ptr_to_instid.emplace(event.srcAgent, event.srcInstid);
		}
		if (event.dstAgent != 0 && event.dstInstid != 0 &&
			agentsByAddress.count(event.dstAgent)) {
			ptr_to_instid.emplace(event.dstAgent, event.dstInstid);
		}

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
			if (agentsByAddress.find(event.srcAgent) != agentsByAddress.end()) {
				active_ptrs.insert(event.srcAgent);
				Agent& agent = agentsByAddress[event.srcAgent];
				agent.id = event.srcInstid;
				agentsByInstid[event.srcInstid] = &agent;
				playersBySrcInstid[event.srcInstid] = &agent;
			}
			if (agentsByAddress.find(event.dstAgent) != agentsByAddress.end()) {
				Agent& agent = agentsByAddress[event.dstAgent];
				agent.id = event.dstInstid;
				agentsByInstid[event.dstInstid] = &agent;
			}
			break;
		case StateChange::TeamChange: {
			uint32_t teamID = static_cast<uint32_t>(event.value);
			if (teamID != 0 && agentsByAddress.find(event.srcAgent) != agentsByAddress.end()) {
				Agent& agent = agentsByAddress[event.srcAgent];

				std::string teamName;
				auto gitc = teamIdToColor.find(teamID);
				if (gitc != teamIdToColor.end()) {
					teamName = gitc->second;
				} else {
					auto it = Settings::teamIDs.find(teamID);
					if (it != Settings::teamIDs.end())
						teamName = it->second;
				}

				if (Settings::debugStringsMode) {
					std::string agentInfo = agent.name.empty() ? agent.accountName : agent.name;
					if (agentInfo.empty()) agentInfo = "Unknown Agent";
					if (!teamName.empty()) {
						APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME,
							("TeamChange: Agent '" + agentInfo + "' assigned to team ID " +
							std::to_string(teamID) + " (" + teamName + ")").c_str());
					} else {
						APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME,
							("TeamChange: Agent '" + agentInfo + "' has UNKNOWN team ID " +
							std::to_string(teamID) + " (not in GUID map or settings)").c_str());
					}
				}

				if (!teamName.empty()) {
					agent.team = teamName;
				}
			}
			break;
		}
		default:
			break;
		}
	}

	// Set POV team
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

	// Set combat times
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
						teamStats.eliteSpecStats[agent->eliteSpec].totalDeaths++;
						if (teamStats.isPOVTeam && agent->subgroupNumber > 0) {
							teamStats.squadStats.totalDeaths++;
							teamStats.squadStats.eliteSpecStats[agent->eliteSpec].totalDeaths++;
						}
					}
					else {
						teamStats.totalDowned++;
						teamStats.eliteSpecStats[agent->eliteSpec].totalDowned++;
						if (teamStats.isPOVTeam && agent->subgroupNumber > 0) {
							teamStats.squadStats.totalDowned++;
							teamStats.squadStats.eliteSpecStats[agent->eliteSpec].totalDowned++;
						}
					}
				}
			}
		}
	}

	// Process damage, kills, and strips events
	for (const auto& event : allEvents) {
		if (event.isStateChange == static_cast<uint8_t>(StateChange::None)) {
			if (event.isActivation == static_cast<uint8_t>(Activation::None)) {
				// Handle buff removals (strips)
				if (event.isBuffRemove != static_cast<uint8_t>(BuffRemove::None)) {
					// Skip if destination agent is 0 (unknown) - since dst is now our stripper
					if (event.dstAgent == 0) {
						continue;
					}
					// Skip if it's a self-strip
					if (event.srcInstid == event.dstInstid) {
						continue;
					}
					// Skip if overstackValue (RemovedDuration) is 0
					if (event.value == 0) {
						continue;
					}
					// Only count "All" removals of specific boons
					if (event.isBuffRemove == static_cast<uint8_t>(BuffRemove::All) &&
						isTrackedBoon(event.skillId)) {

						// Look up the destination agent (stripper) instead of source
						auto dstIt = playersBySrcInstid.find(event.dstInstid);
						if (dstIt != playersBySrcInstid.end()) {
							Agent* stripper = dstIt->second;
							const std::string& stripperTeam = stripper->team;
							if (stripperTeam != "Unknown") {
								bool vsPlayer = false;
								// Now check the source agent (target) instead of destination
								auto srcIt = agentsByInstid.find(event.srcInstid);
								if (srcIt != agentsByInstid.end()) {
									Agent* target = srcIt->second;
									if (target->team != "Unknown") {
										vsPlayer = true;
									}
								}
								updateStats(result.teamStats[stripperTeam], stripper, 0, false, false,
									vsPlayer, false, false, false, false, true);
							}
						}
					}
				}
				// Handle damage and kills
				else if (event.isBuffRemove == static_cast<uint8_t>(BuffRemove::None)) {
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
											auto stateIt = agentStates.find(target->address);
											if (stateIt != agentStates.end()) {
												isDownedContribution = isDamageInDownSequence(target, stateIt->second, event.time);
												isKillContribution = isDamageInKillSequence(target, stateIt->second, event.time);
											}
										}
									}

									updateStats(result.teamStats[attackerTeam], attacker, damageValue, true, false,
										vsPlayer, isStrikeDamage, isCondiDamage, isDownedContribution, isKillContribution, false);
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
											updateStats(result.teamStats[attackerTeam], attacker, 0, false, true,
												true, false, false, false, false, false);
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	if (Settings::debugStringsMode) {
		APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME,
			("playersBySrcInstid total entries: " + std::to_string(playersBySrcInstid.size())).c_str());
		for (const auto& [instid, agent] : playersBySrcInstid) {
			APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME,
				("  instid=" + std::to_string(instid) +
				 " addr=" + std::to_string(agent->address) +
				 " team=" + agent->team +
				 " spec=" + agent->eliteSpec +
				 " acct=" + agent->accountName).c_str());
		}
	}

	std::unordered_map<std::string, std::unordered_set<std::string>> countedAccounts;
	std::unordered_set<uint16_t> countedNonSquadInstids;

	for (const auto& [addr, agent] : agentsByAddress) {
		if (agent.team.empty() || agent.team == "Unknown") continue;

		bool isSquad = agent.subgroupNumber > 0;

		if (isSquad) {
			if (!active_ptrs.count(addr)) continue;
			if (!agent.accountName.empty() && agent.accountName[0] == ':') {
				if (!countedAccounts[agent.team].insert(agent.accountName).second)
					continue;
			}
		} else {
			auto iit = ptr_to_instid.find(addr);
			if (iit == ptr_to_instid.end()) continue;
			if (!countedNonSquadInstids.insert(iit->second).second)
				continue;
		}

		auto& teamStats = result.teamStats[agent.team];
		auto& specStats = teamStats.eliteSpecStats[agent.eliteSpec];

		teamStats.totalPlayers++;
		specStats.count++;
		result.totalIdentifiedPlayers++;

		if (Settings::debugStringsMode) {
			APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME,
				("COUNT: addr=" + std::to_string(addr) +
				 " squad=" + std::to_string(isSquad) +
				 " team=" + agent.team +
				 " spec=" + agent.eliteSpec +
				 " total=" + std::to_string(teamStats.totalPlayers)).c_str());
		}

		if (teamStats.isPOVTeam && isSquad) {
			auto& squadStats = teamStats.squadStats;
			squadStats.totalPlayers++;
			squadStats.eliteSpecStats[agent.eliteSpec].count++;
		}
	}

	if (Settings::debugStringsMode) {
		for (const auto& [teamName, stats] : result.teamStats) {
			APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME,
				("FINAL: team=" + teamName + " totalPlayers=" + std::to_string(stats.totalPlayers)).c_str());
		}
	}
}

ParsedData parseEVTCFile(const std::filesystem::path& filePath) {
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

	// Extract combat events
	const size_t eventSize = sizeof(CombatEvent);
	size_t remainingBytes = bytes.size() - offset;
	size_t eventCount = remainingBytes / eventSize;

	// Process combat events
	std::unordered_map<uint16_t, Agent*> playersBySrcInstid;
	parseCombatEvents(bytes, offset, eventCount, agentsByAddress, playersBySrcInstid, result);

	return result;
}

std::wstring getCanonicalPath(const std::filesystem::path& path)
{
	return std::filesystem::weakly_canonical(path).wstring();
}
