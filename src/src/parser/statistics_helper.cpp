#include "parser/statistics_helper.h"
#include "parser/evtc_parser.h"

bool isTrackedBoon(uint32_t skillId) {
    switch (skillId) {
    case static_cast<uint32_t>(BoonIds::Protection):
    case static_cast<uint32_t>(BoonIds::Regeneration):
    case static_cast<uint32_t>(BoonIds::Swiftness):
    case static_cast<uint32_t>(BoonIds::Fury):
    case static_cast<uint32_t>(BoonIds::Vigor):
    case static_cast<uint32_t>(BoonIds::Might):
    case static_cast<uint32_t>(BoonIds::Aegis):
    case static_cast<uint32_t>(BoonIds::Retaliation):
    case static_cast<uint32_t>(BoonIds::Stability):
    case static_cast<uint32_t>(BoonIds::Quickness):
    case static_cast<uint32_t>(BoonIds::Resistance):
        return true;
    default:
        return false;
    }
}

void updateStats(TeamStats& teamStats, Agent* agent, int32_t value, bool isDamage, bool isKill, bool vsPlayer,
    bool isStrikeDamage, bool isCondiDamage, bool isDownedContribution, bool isKillContribution, bool isStrip) {
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

    if (isStrip) {
        teamStats.totalStrips++;
        specStats.totalStrips++;

        if (vsPlayer) {
            teamStats.totalStripsVsPlayers++;
            specStats.totalStripsVsPlayers++;
        }
    }

    // Handle squad stats if this is the POV team and agent is in a subgroup
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

        if (isStrip) {
            squadStats.totalStrips++;
            squadSpecStats.totalStrips++;

            if (vsPlayer) {
                squadStats.totalStripsVsPlayers++;
                squadSpecStats.totalStripsVsPlayers++;
            }
        }
    }
}

bool isDamageInDownSequence(const Agent* agent, const AgentState& state, uint64_t currentTime) {
    const uint64_t TWO_SECONDS = 2000;

    // Quick check using intervals
    bool currentlyDowned = std::any_of(state.downIntervals.begin(), state.downIntervals.end(),
        [currentTime](const auto& interval) {
            return currentTime >= interval.first &&
                (interval.second == UINT64_MAX || currentTime <= interval.second);
        });

    if (currentlyDowned) return false;

    // Find next down using original sequence
    uint64_t nextDownTime = UINT64_MAX;
    for (const auto& event : state.relevantEvents) {
        if (event.time > currentTime &&
            event.isStateChange == static_cast<uint8_t>(StateChange::ChangeDown)) {
            nextDownTime = event.time;
            break;
        }
    }

    if (nextDownTime == UINT64_MAX) return false;

    // Find last high health using original sequence
    uint64_t lastHighHealthTime = 0;
    for (auto it = state.relevantEvents.rbegin(); it != state.relevantEvents.rend(); ++it) {
        const auto& event = *it;
        if (event.time >= currentTime) continue;

        if (event.isStateChange == static_cast<uint8_t>(StateChange::HealthUpdate)) {
            if (event.value > 0 && (event.dstAgent * 100.0f) / event.value > 98.0f) {
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

bool isDamageInKillSequence(const Agent* agent, const AgentState& state, uint64_t currentTime) {
    const uint64_t TWO_SECONDS = 2000;

    // Use original sequence to determine current state
    bool currentlyDowned = false;
    uint64_t downTime = 0;

    for (const auto& event : state.relevantEvents) {
        if (event.time >= currentTime) break;

        if (event.isStateChange == static_cast<uint8_t>(StateChange::ChangeDown)) {
            currentlyDowned = true;
            downTime = event.time;
        }
        else if (event.isStateChange == static_cast<uint8_t>(StateChange::ChangeUp) ||
            event.isStateChange == static_cast<uint8_t>(StateChange::ChangeDead)) {
            currentlyDowned = false;
        }
    }

    if (!currentlyDowned) return false;

    // Find next death after current down
    uint64_t deathTime = UINT64_MAX;
    for (const auto& event : state.relevantEvents) {
        if (event.time > currentTime &&
            event.isStateChange == static_cast<uint8_t>(StateChange::ChangeDead)) {
            deathTime = event.time;
            break;
        }
    }

    if (deathTime == UINT64_MAX) return false;

    return currentTime >= downTime - TWO_SECONDS &&
        currentTime <= deathTime;
}
