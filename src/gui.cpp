#define NOMINMAX
#include "gui.h"
#include "Settings.h"
#include "Shared.h"
#include "utils.h"
#include "BarTemplate.h"
#include "resource.h"
#include "nexus/Nexus.h"
#include "mumble/Mumble.h"
#include "imgui/imgui.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

uint64_t getSortValue(const std::string& sortCriteria, const SpecStats& stats, bool vsLogPlayers) {
	if (sortCriteria == "players") {
		return stats.count;
	}
	else if (sortCriteria == "damage") {
		return vsLogPlayers ? stats.totalDamageVsPlayers : stats.totalDamage;
	}
	else if (sortCriteria == "down cont") {
		return vsLogPlayers ? stats.totalDownedContributionVsPlayers : stats.totalDownedContribution;
	}
	else if (sortCriteria == "kill cont") {
		return vsLogPlayers ? stats.totalKillContributionVsPlayers : stats.totalKillContribution;
	}
	else if (sortCriteria == "deaths") {
		return stats.totalDeaths;
	}
	else if (sortCriteria == "downs") {
		return stats.totalDowned;
	}
	return 0;
}

uint64_t getBarValue(const std::string& representation, const SpecStats& stats, bool vsLogPlayers) {
	if (representation == "players") {
		return stats.count;
	}
	else if (representation == "damage") {
		return vsLogPlayers ? stats.totalDamageVsPlayers : stats.totalDamage;
	}
	else if (representation == "down cont") {
		return vsLogPlayers ? stats.totalDownedContributionVsPlayers : stats.totalDownedContribution;
	}
	else if (representation == "kill cont") {
		return vsLogPlayers ? stats.totalKillContributionVsPlayers : stats.totalKillContribution;
	}
	else if (representation == "deaths") {
		return stats.totalDeaths;
	}
	else if (representation == "downs") {
		return stats.totalDowned;
	}
	return 0;
}

std::pair<uint64_t, uint64_t> getSecondaryBarValues(
	const std::string& barRep,
	const SpecStats& stats,
	bool vsLogPlayers
) {
	if (barRep == "damage") {
		uint64_t primaryValue = vsLogPlayers ? stats.totalDamageVsPlayers : stats.totalDamage;
		uint64_t secondaryValue = vsLogPlayers ? stats.totalDownedContributionVsPlayers : stats.totalDownedContribution;
		return { primaryValue, secondaryValue };
	}
	else if (barRep == "down cont") {
		uint64_t primaryValue = vsLogPlayers ? stats.totalDownedContributionVsPlayers : stats.totalDownedContribution;
		uint64_t secondaryValue = vsLogPlayers ? stats.totalKillContributionVsPlayers : stats.totalKillContribution;
		return { primaryValue, secondaryValue };
	}
	return { 0, 0 };
}

auto getSpecSortComparator(const std::string& sortCriteria, bool vsLogPlayers) {
	return [sortCriteria, vsLogPlayers](
		const std::pair<std::string, SpecStats>& a,
		const std::pair<std::string, SpecStats>& b) {
			uint64_t valueA = getSortValue(sortCriteria, a.second, vsLogPlayers);
			uint64_t valueB = getSortValue(sortCriteria, b.second, vsLogPlayers);

			if (valueA != valueB) {
				return valueA > valueB;
			}

			uint64_t damageA = vsLogPlayers ? a.second.totalDamageVsPlayers : a.second.totalDamage;
			uint64_t damageB = vsLogPlayers ? b.second.totalDamageVsPlayers : b.second.totalDamage;

			if (damageA != damageB) {
				return damageA > damageB;
			}

			if (a.second.count != b.second.count) {
				return a.second.count > b.second.count;
			}

			return a.first < b.first;
	};
}

struct TeamRenderInfo {
	bool hasData;
	const TeamStats* stats;
	std::string name;
	ImVec4 color;
};



void DrawBar(
	const std::vector<std::pair<std::string, SpecStats>>& sortedClasses,
	size_t currentIndex,
	const std::string& barRep,
	const SpecStats& stats,
	const MainWindowSettings* settings,
	const ImVec4& primaryColor,
	const ImVec4& secondaryColor,
	const std::string& eliteSpec,
	HINSTANCE hSelf
) {
	ImVec2 cursor_pos = ImGui::GetCursorPos();
	ImVec2 screen_pos = ImGui::GetCursorScreenPos();
	float bar_height = ImGui::GetTextLineHeight() + 4.0f;

	uint64_t maxValue = 0;
	for (const auto& specPair : sortedClasses) {
		uint64_t value;
		if (settings->barRepIndependent) {
			value = getBarValue(settings->barRepresentation, specPair.second, settings->vsLoggedPlayersOnly);
		}
		else {
			value = getBarValue(settings->windowSort, specPair.second, settings->vsLoggedPlayersOnly);
		}
		maxValue = std::max(maxValue, value);
	}

	uint64_t currentValue;
	if (settings->barRepIndependent) {
		currentValue = getBarValue(settings->barRepresentation, stats, settings->vsLoggedPlayersOnly);
	}
	else {
		currentValue = getBarValue(settings->windowSort, stats, settings->vsLoggedPlayersOnly);
	}

	float frac = (maxValue > 0) ? static_cast<float>(currentValue) / static_cast<float>(maxValue) : 0.0f;
	float bar_width = ImGui::GetContentRegionAvail().x * frac;

	ImGui::GetWindowDrawList()->AddRectFilled(
		screen_pos,
		ImVec2(screen_pos.x + bar_width, screen_pos.y + bar_height),
		ImGui::ColorConvertFloat4ToU32(primaryColor)
	);

	std::string effectiveRep = settings->barRepIndependent ?
		settings->barRepresentation : settings->windowSort;

	auto secondaryStatIt = MainWindowSettings::secondaryStats.find(effectiveRep);
	if (secondaryStatIt != MainWindowSettings::secondaryStats.end() &&
		secondaryStatIt->second.enabled) {
		const auto& secondaryRelation = secondaryStatIt->second;
		uint64_t primaryValue = getBarValue(effectiveRep, stats, settings->vsLoggedPlayersOnly);
		uint64_t secondaryValue = getBarValue(secondaryRelation.secondaryStat,
			stats,
			settings->vsLoggedPlayersOnly);

		if (primaryValue > 0 && secondaryValue > 0) {
			float secondary_frac = static_cast<float>(secondaryValue) /
				static_cast<float>(primaryValue);

			secondary_frac = std::clamp(secondary_frac, 0.0f, secondaryRelation.maxRatio);
			float secondary_width = bar_width * secondary_frac;

			ImGui::GetWindowDrawList()->AddRectFilled(
				screen_pos,
				ImVec2(screen_pos.x + secondary_width, screen_pos.y + bar_height),
				ImGui::ColorConvertFloat4ToU32(secondaryColor)
			);
		}
	}

	ImGui::SetCursorPos(ImVec2(cursor_pos.x + 5.0f, cursor_pos.y + 2.0f));

	auto templateIt = settings->sortTemplates.find(settings->windowSort);
	std::string currentTemplate = (templateIt != settings->sortTemplates.end()) ?
		templateIt->second : "";

	std::string template_to_use = BarTemplateRenderer::GetTemplateForSort(
		settings->windowSort,
		currentTemplate
	);

	BarTemplateRenderer::ParseTemplate(template_to_use);
	BarTemplateRenderer::RenderTemplate(
		stats.count,
		eliteSpec,
		settings->useShortClassNames,
		stats,
		settings->vsLoggedPlayersOnly,
		settings->windowSort,
		hSelf,
		ImGui::GetFontSize(),
		settings->showSpecTooltips
	);

	ImGui::SetCursorPosY(cursor_pos.y + bar_height + 2.0f);
}

void RenderTeamData(const TeamStats& teamData, const MainWindowSettings* settings, HINSTANCE hSelf)
{
	ImGuiStyle& style = ImGui::GetStyle();
	float sz = ImGui::GetFontSize();

	ImGui::Spacing();

	bool useSquadStats = settings->squadPlayersOnly && teamData.isPOVTeam;

	// Total Players Display
	uint32_t totalPlayersToDisplay = useSquadStats ? teamData.squadStats.totalPlayers : teamData.totalPlayers;

	if (settings->showTeamTotalPlayers) {
		if (settings->showClassIcons) {
			if (Squad && Squad->Resource) {
				ImGui::Image(Squad->Resource, ImVec2(sz, sz));
				ImGui::SameLine(0, 5);
			}
			else {
				Squad = APIDefs->Textures.GetOrCreateFromResource("SQUAD_ICON", SQUAD, hSelf);
			}
		}
		if (settings->showClassNames) {
			ImGui::Text("Total:  %d", totalPlayersToDisplay);
		}
		else {
			ImGui::Text("%d", totalPlayersToDisplay);
		}
	}

	// K/D Ratio Display
	uint32_t totalKillsToDisplay = useSquadStats ? teamData.squadStats.totalKills : teamData.totalKills;
	uint32_t totalDeathsfromKillingBlowsToDisplay = useSquadStats ?
		teamData.squadStats.totalDeathsFromKillingBlows : teamData.totalDeathsFromKillingBlows;
	double kdRatioToDisplay = useSquadStats ? teamData.squadStats.getKillDeathRatio() : teamData.getKillDeathRatio();
	std::string kdString = (std::ostringstream() << totalKillsToDisplay << "/" <<
		totalDeathsfromKillingBlowsToDisplay << " (" << std::fixed << std::setprecision(2) <<
		kdRatioToDisplay << ")").str();

	if (settings->showTeamKDR) {
		if (settings->showClassIcons) {
			if (Kdr && Kdr->Resource) {
				ImGui::Image(Kdr->Resource, ImVec2(sz, sz));
				ImGui::SameLine(0, 5);
			}
			else {
				Kdr = APIDefs->Textures.GetOrCreateFromResource("KDR_ICON", KDR, hSelf);
			}
		}
		if (settings->showClassNames) {
			ImGui::Text("K/D: %s", kdString.c_str());
		}
		else {
			ImGui::Text("%s", kdString.c_str());
		}
	}

	// Deaths Display
	uint32_t totalDeathsToDisplay = useSquadStats ? teamData.squadStats.totalDeaths : teamData.totalDeaths;

	if (settings->showTeamDeaths) {
		if (settings->showClassIcons) {
			if (Death && Death->Resource) {
				ImGui::Image(Death->Resource, ImVec2(sz, sz));
				ImGui::SameLine(0, 5);
			}
			else {
				Death = APIDefs->Textures.GetOrCreateFromResource("DEATH_ICON", DEATH, hSelf);
			}
		}
		if (settings->showClassNames) {
			ImGui::Text("Deaths: %d", totalDeathsToDisplay);
		}
		else {
			ImGui::Text("%d", totalDeathsToDisplay);
		}
	}

	// Downs Display
	uint32_t totalDownedToDisplay = useSquadStats ? teamData.squadStats.totalDowned : teamData.totalDowned;

	if (settings->showTeamDowned) {
		if (settings->showClassIcons) {
			if (Downed && Downed->Resource) {
				ImGui::Image(Downed->Resource, ImVec2(sz, sz));
				ImGui::SameLine(0, 5);
			}
			else {
				Downed = APIDefs->Textures.GetOrCreateFromResource("DOWNED_ICON", DOWNED, hSelf);
			}
		}
		if (settings->showClassNames) {
			ImGui::Text("Downs:  %d", totalDownedToDisplay);
		}
		else {
			ImGui::Text("%d", totalDownedToDisplay);
		}
	}

	// Damage Display
	uint64_t totalDamageToDisplay = 0;
	if (settings->vsLoggedPlayersOnly) {
		totalDamageToDisplay = useSquadStats ?
			teamData.squadStats.totalDamageVsPlayers : teamData.totalDamageVsPlayers;
	}
	else {
		totalDamageToDisplay = useSquadStats ?
			teamData.squadStats.totalDamage : teamData.totalDamage;
	}

	if (settings->showTeamDamage) {
		if (settings->showClassIcons) {
			if (Damage && Damage->Resource) {
				ImGui::Image(Damage->Resource, ImVec2(sz, sz));
				ImGui::SameLine(0, 5);
			}
			else {
				Damage = APIDefs->Textures.GetOrCreateFromResource("DAMAGE_ICON", DAMAGE, hSelf);
			}
		}

		std::string formattedDamage = formatDamage(totalDamageToDisplay);

		if (settings->showClassNames) {
			ImGui::Text("Damage: %s", formattedDamage.c_str());
		}
		else {
			ImGui::Text("%s", formattedDamage.c_str());
		}
	}

	// Down Contribution Display
	uint64_t totalDownedContToDisplay = 0;
	if (settings->vsLoggedPlayersOnly) {
		totalDownedContToDisplay = useSquadStats ?
			teamData.squadStats.totalDownedContributionVsPlayers : teamData.totalDownedContributionVsPlayers;
	}
	else {
		totalDownedContToDisplay = useSquadStats ?
			teamData.squadStats.totalDownedContribution : teamData.totalDownedContribution;
	}

	if (settings->showTeamDownCont) {
		if (settings->showClassIcons) {
			if (Downcont && Downcont->Resource) {
				ImGui::Image(Downcont->Resource, ImVec2(sz, sz));
				ImGui::SameLine(0, 5);
			}
			else {
				Downcont = APIDefs->Textures.GetOrCreateFromResource("DOWNCONT_ICON", DOWNCONT, hSelf);
			}
		}
		std::string formattedDownCont = formatDamage(totalDownedContToDisplay);

		if (settings->showClassNames) {
			ImGui::Text("Down Cont: %s", formattedDownCont.c_str());
		}
		else {
			ImGui::Text("%s", formattedDownCont.c_str());
		}
	}

	// Kill Contribution Display
	uint64_t totalKillContToDisplay = 0;
	if (settings->vsLoggedPlayersOnly) {
		totalKillContToDisplay = useSquadStats ?
			teamData.squadStats.totalKillContributionVsPlayers : teamData.totalKillContributionVsPlayers;
	}
	else {
		totalKillContToDisplay = useSquadStats ?
			teamData.squadStats.totalKillContribution : teamData.totalKillContribution;
	}

	if (settings->showTeamKillCont) {
		if (Killcont && Killcont->Resource) {
			ImGui::Image(Killcont->Resource, ImVec2(sz, sz));
			ImGui::SameLine(0, 5);
		}
		else {
			Killcont = APIDefs->Textures.GetOrCreateFromResource("KILLCONT_ICON", KILLCONT, hSelf);
		}
		std::string formattedKillCont = formatDamage(totalKillContToDisplay);

		if (settings->showClassNames) {
			ImGui::Text("Kill Cont: %s", formattedKillCont.c_str());
		}
		else {
			ImGui::Text("%s", formattedKillCont.c_str());
		}
	}

	// Strike Damage Display
	uint64_t totalStrikeDamageToDisplay = 0;
	if (settings->vsLoggedPlayersOnly) {
		totalStrikeDamageToDisplay = useSquadStats ?
			teamData.squadStats.totalStrikeDamageVsPlayers : teamData.totalStrikeDamageVsPlayers;
	}
	else {
		totalStrikeDamageToDisplay = useSquadStats ?
			teamData.squadStats.totalStrikeDamage : teamData.totalStrikeDamage;
	}

	if (settings->showTeamStrikeDamage) {
		if (settings->showClassIcons) {
			if (Strike && Strike->Resource) {
				ImGui::Image(Strike->Resource, ImVec2(sz, sz));
				ImGui::SameLine(0, 5);
			}
			else {
				Strike = APIDefs->Textures.GetOrCreateFromResource("STRIKE_ICON", STRIKE, hSelf);
			}
		}

		std::string formattedStrikeDamage = formatDamage(totalStrikeDamageToDisplay);

		if (settings->showClassNames) {
			ImGui::Text("Strike: %s", formattedStrikeDamage.c_str());
		}
		else {
			ImGui::Text("%s", formattedStrikeDamage.c_str());
		}
	}

	// Condition Damage Display
	uint64_t totalCondiDamageToDisplay = 0;
	if (settings->vsLoggedPlayersOnly) {
		totalCondiDamageToDisplay = useSquadStats ?
			teamData.squadStats.totalCondiDamageVsPlayers : teamData.totalCondiDamageVsPlayers;
	}
	else {
		totalCondiDamageToDisplay = useSquadStats ?
			teamData.squadStats.totalCondiDamage : teamData.totalCondiDamage;
	}

	if (settings->showTeamCondiDamage) {
		if (settings->showClassIcons) {
			if (Condi && Condi->Resource) {
				ImGui::Image(Condi->Resource, ImVec2(sz, sz));
				ImGui::SameLine(0, 5);
			}
			else {
				Condi = APIDefs->Textures.GetOrCreateFromResource("CONDI_ICON", CONDI, hSelf);
			}
		}

		std::string formattedCondiDamage = formatDamage(totalCondiDamageToDisplay);

		if (settings->showClassNames) {
			ImGui::Text("Condi:  %s", formattedCondiDamage.c_str());
		}
		else {
			ImGui::Text("%s", formattedCondiDamage.c_str());
		}
	}

	if (settings->showSpecBars) {
		ImGui::Separator();
		RenderSpecializationBars(teamData, settings, hSelf);
	}
}

void RenderTemplateSettings(MainWindowSettings* settings) {
	if (ImGui::BeginMenu("Bar Templates")) {
		const char* sortTypes[] = {
			"players", "damage", "down cont", "kill cont", "deaths", "downs"
		};
		const char* sortLabels[] = {
			"Players", "Damage", "Down Contribution", "Kill Contribution", "Deaths", "Downs"
		};

		const char* templateLegend =
			"Template Variables:\n"
			"{1}  - Player Count\n"
			"{2}  - Class Icon\n"
			"{3}  - Class Name\n"
			"{4}  - Damage\n"
			"{5}  - Down Contribution\n"
			"{6}  - Kill Contribution\n"
			"{7}  - Deaths\n"
			"{8}  - Downs\n"
			"{9}  - Strike Damage\n"
			"{10} - Condition Damage";

		for (int i = 0; i < IM_ARRAYSIZE(sortTypes); i++) {
			if (ImGui::TreeNode(sortLabels[i])) {
				const std::string& sortType = sortTypes[i];

				std::string defaultTemplate = BarTemplateRenderer::GetTemplateForSort(sortType);

				auto& currentTemplate = settings->sortTemplates[sortType];
				if (currentTemplate.empty()) {
					currentTemplate = defaultTemplate;
				}

				char tempBuffer[256];
				strncpy(tempBuffer, currentTemplate.c_str(), sizeof(tempBuffer) - 1);

				ImGui::Text("Current Template:");
				ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.8f);

				if (ImGui::InputText(("##Template" + sortType).c_str(),
					tempBuffer, sizeof(tempBuffer)))
				{
					currentTemplate = tempBuffer;
					if (settings->windowSort == sortType) {
						BarTemplateRenderer::ParseTemplate(currentTemplate);
					}
					Settings::Save(SettingsPath);
				}

				if (ImGui::IsItemHovered()) {
					ImGui::SetTooltip("%s", templateLegend);
				}

				ImGui::Text("Default: %s", defaultTemplate.c_str());

				if (ImGui::Button(("Reset to Default##" + sortType).c_str())) {
					currentTemplate = defaultTemplate;
					if (settings->windowSort == sortType) {
						BarTemplateRenderer::ParseTemplate(currentTemplate);
					}
					Settings::Save(SettingsPath);
				}

				ImGui::TreePop();
			}
		}
		ImGui::EndMenu();
	}
}

void RenderMainWindowSettingsPopup(MainWindowSettings* settings) {
	if (ImGui::BeginPopup("MainWindow Settings")) {
		if (!settings->isWindowNameEditing) {
			strncpy(settings->tempWindowName, settings->windowName.c_str(), sizeof(settings->tempWindowName) - 1);
			settings->isWindowNameEditing = true;
		}
		ImGui::Text("Window Name:");
		float windowWidth = ImGui::GetWindowWidth();
		ImGui::SetNextItemWidth(windowWidth * 0.5f);
		if (ImGui::InputText(" ##Window Name", settings->tempWindowName, sizeof(settings->tempWindowName))) {}

		ImGui::SameLine();
		if (ImGui::Button("Apply")) {
			UnregisterWindowFromNexusEsc(settings, "WvW Fight Analysis");
			settings->windowName = settings->tempWindowName;
			settings->updateDisplayName("WvW Fight Analysis");
			RegisterWindowForNexusEsc(settings, "WvW Fight Analysis");

			Settings::Save(SettingsPath);
		}

		if (ImGui::BeginMenu("History")) {
			for (int i = 0; i < parsedLogs.size(); ++i) {
				const auto& log = parsedLogs[i];
				std::string fnstr = log.filename.substr(0, log.filename.find_last_of('.'));
				uint64_t durationMs = log.data.combatEndTime - log.data.combatStartTime;
				auto duration = std::chrono::milliseconds(durationMs);
				int minutes = std::chrono::duration_cast<std::chrono::minutes>(duration).count();
				int seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count() % 60;
				std::string displayName = fnstr + " (" + std::to_string(minutes) + "m " + std::to_string(seconds) + "s)";

				if (ImGui::RadioButton(displayName.c_str(), &currentLogIndex, i)) {
					// Selection handled by RadioButton
				}
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Display")) {
			if (ImGui::Checkbox("log name", &settings->showLogName)) { Settings::Save(SettingsPath); }
			if (ImGui::Checkbox("short spec names", &settings->useShortClassNames)) { Settings::Save(SettingsPath); }
			if (ImGui::Checkbox("draw bars", &settings->showSpecBars)) { Settings::Save(SettingsPath); }
			if (ImGui::Checkbox("tooltips", &settings->showSpecTooltips)) { Settings::Save(SettingsPath); }
			ImGui::Separator();
			// Team statistics display options
			if (ImGui::BeginMenu("Team Stats")) {
				if (ImGui::Checkbox("player count", &settings->showTeamTotalPlayers)) { Settings::Save(SettingsPath); }
				if (ImGui::Checkbox("k/d ratio", &settings->showTeamKDR)) { Settings::Save(SettingsPath); }
				if (ImGui::Checkbox("deaths", &settings->showTeamDeaths)) { Settings::Save(SettingsPath); }
				if (ImGui::Checkbox("downs", &settings->showTeamDowned)) { Settings::Save(SettingsPath); }
				if (ImGui::Checkbox("outgoing damage", &settings->showTeamDamage)) { Settings::Save(SettingsPath); }
				if (ImGui::Checkbox("outgoing down cont", &settings->showTeamDownCont)) { Settings::Save(SettingsPath); }
				if (ImGui::Checkbox("outgoing kill cont", &settings->showTeamKillCont)) { Settings::Save(SettingsPath); }
				if (ImGui::Checkbox("outgoing strike damage", &settings->showTeamStrikeDamage)) { Settings::Save(SettingsPath); }
				if (ImGui::Checkbox("outgoing condi damage", &settings->showTeamCondiDamage)) { Settings::Save(SettingsPath); }
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Bar Configuration")) {
				if (ImGui::Checkbox("Independent of sort", &settings->barRepIndependent)) {
					Settings::Save(SettingsPath);
				}
				if (settings->barRepIndependent) {
					if (ImGui::BeginMenu("Bar Display")) {
						const char* displayOptions[] = {
							"Players", "Damage", "Down Cont", "Kill Cont", "Deaths", "Downs"
						};
						const char* displayValues[] = {
							"players", "damage", "down cont", "kill cont", "deaths", "downs"
						};

						for (int i = 0; i < IM_ARRAYSIZE(displayOptions); i++) {
							bool isSelected = settings->barRepresentation == displayValues[i];
							if (ImGui::RadioButton(displayOptions[i], isSelected)) {
								settings->barRepresentation = displayValues[i];
								Settings::Save(SettingsPath);
							}
						}

						ImGui::EndMenu();
					}
				}
				RenderTemplateSettings(settings);
				ImGui::EndMenu();
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Style")) {
			if (ImGui::Checkbox("use tabbed view", &settings->useTabbedView)) { Settings::Save(SettingsPath); }
			if (ImGui::Checkbox("show title", &settings->showTitle)) { Settings::Save(SettingsPath); }
			if (ImGui::Checkbox("use window style for title bar", &settings->useWindowStyleForTitle)) {
				Settings::Save(SettingsPath);
			}
			if (ImGui::Checkbox("scroll bar", &settings->showScrollBar)) { Settings::Save(SettingsPath); }
			if (ImGui::Checkbox("background", &settings->showBackground)) { Settings::Save(SettingsPath); }
			if (ImGui::Checkbox("allow focus", &settings->allowFocus)) { Settings::Save(SettingsPath); }
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Sort")) {
			const char* sortOptions[] = {
				"Players", "Damage", "Down Cont", "Kill Cont", "Deaths", "Downs"
			};
			const char* sortValues[] = {
				"players", "damage", "down cont", "kill cont", "deaths", "downs"
			};

			for (int i = 0; i < IM_ARRAYSIZE(sortOptions); i++) {
				bool isSelected = settings->windowSort == sortValues[i];
				if (ImGui::RadioButton(sortOptions[i], isSelected)) {
					settings->windowSort = sortValues[i];
					Settings::Save(SettingsPath);
				}
			}
			ImGui::EndMenu();
		}

		
		if (ImGui::Checkbox("damage vs logged players only", &settings->vsLoggedPlayersOnly)) {
			Settings::Save(SettingsPath);
		}

		if (ImGui::Checkbox("show squad players only", &settings->squadPlayersOnly)) {
			Settings::Save(SettingsPath);
		}

		ImGui::EndPopup();
	}
	else {
		settings->isWindowNameEditing = false;
	}
}

void RenderWidgetSettingsPopup(WidgetWindowSettings* settings) {
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5.0f, 5.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(5.0f, 5.0f));
	if (ImGui::BeginPopup("Widget Settings")) {
		if (!settings->isWindowNameEditing) {
			strncpy(settings->tempWindowName, settings->windowName.c_str(), sizeof(settings->tempWindowName) - 1);
			settings->isWindowNameEditing = true;
		}
		ImGui::Text("Widget Name:");
		float windowWidth = ImGui::GetWindowWidth();
		ImGui::SetNextItemWidth(windowWidth * 0.5f);
		if (ImGui::InputText(" ##Widget Name", settings->tempWindowName, sizeof(settings->tempWindowName))) {}
		ImGui::SameLine();
		if (ImGui::Button("Apply")) {
			UnregisterWindowFromNexusEsc(settings, "WvW Fight Analysis");
			settings->windowName = settings->tempWindowName;
			settings->updateDisplayName("WvW Fight Analysis");
			RegisterWindowForNexusEsc(settings, "WvW Fight Analysis");

			Settings::Save(SettingsPath);
		}

		if (ImGui::BeginMenu("History")) {
			for (int i = 0; i < parsedLogs.size(); ++i) {
				const auto& log = parsedLogs[i];
				std::string fnstr = log.filename.substr(0, log.filename.find_last_of('.'));
				uint64_t durationMs = log.data.combatEndTime - log.data.combatStartTime;
				auto duration = std::chrono::milliseconds(durationMs);
				int minutes = std::chrono::duration_cast<std::chrono::minutes>(duration).count();
				int seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count() % 60;
				std::string displayName = fnstr + " (" + std::to_string(minutes) + "m " + std::to_string(seconds) + "s)";

				if (ImGui::RadioButton(displayName.c_str(), &currentLogIndex, i)) {
					// Selection handled by RadioButton
				}
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Display Stats")) {
			const char* statOptions[] = {
				"Players", "K/D Ratio", "Deaths", "Downs", "Damage"
			};
			const char* statValues[] = {
				"players", "kdr", "deaths", "downs", "damage"
			};

			for (int i = 0; i < IM_ARRAYSIZE(statOptions); i++) {
				bool isSelected = settings->widgetStats == statValues[i];
				if (ImGui::RadioButton(statOptions[i], isSelected)) {
					settings->widgetStats = statValues[i];
					Settings::Settings["widgetStats"] = settings->widgetStats;
					Settings::Save(SettingsPath);
				}
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Style")) {
			if (ImGui::Checkbox("show widget icon", &settings->showWidgetIcon)) {
				Settings::Settings["ShowWidgetIcon"] = settings->showWidgetIcon;
				Settings::Save(SettingsPath);
			}

			// Widget Height
			ImGui::SetNextItemWidth(200.0f);
			if (ImGui::SliderFloat("##Widget Height", &settings->widgetHeight, 0.0f, 900.0f)) {
				settings->widgetHeight = std::clamp(settings->widgetHeight, 0.0f, 900.0f);
				Settings::Settings["WidgetHeight"] = settings->widgetHeight;
				Settings::Save(SettingsPath);
			}
			ImGui::SameLine();
			ImGui::SetNextItemWidth(60.0f);
			if (ImGui::InputFloat("Widget Height", &settings->widgetHeight, 1.0f)) {
				settings->widgetHeight = std::clamp(settings->widgetHeight, 0.0f, 900.0f);
				Settings::Settings["WidgetHeight"] = settings->widgetHeight;
				Settings::Save(SettingsPath);
			}

			// Widget Width
			ImGui::SetNextItemWidth(200.0f);
			if (ImGui::SliderFloat("##Widget Width", &settings->widgetWidth, 0.0f, 900.0f)) {
				settings->widgetWidth = std::clamp(settings->widgetWidth, 0.0f, 900.0f);
				Settings::Settings["WidgetWidth"] = settings->widgetWidth;
				Settings::Save(SettingsPath);
			}
			ImGui::SameLine();
			ImGui::SetNextItemWidth(60.0f);
			if (ImGui::InputFloat("Widget Width", &settings->widgetWidth, 1.0f)) {
				settings->widgetWidth = std::clamp(settings->widgetWidth, 0.0f, 900.0f);
				Settings::Settings["WidgetWidth"] = settings->widgetWidth;
				Settings::Save(SettingsPath);
			}

			// Text Vertical Alignment
			ImGui::SetNextItemWidth(200.0f);
			if (ImGui::SliderFloat("##Widget Text Vertical Align", &settings->textVerticalAlignOffset, -50.0f, 50.0f)) {
				settings->textVerticalAlignOffset = std::clamp(settings->textVerticalAlignOffset, -50.0f, 50.0f);
				Settings::Settings["TextVerticalAlignOffset"] = settings->textVerticalAlignOffset;
				Settings::Save(SettingsPath);
			}
			ImGui::SameLine();
			ImGui::SetNextItemWidth(60.0f);
			if (ImGui::InputFloat("Text Vertical Align", &settings->textVerticalAlignOffset, 1.0f)) {
				settings->textVerticalAlignOffset = std::clamp(settings->textVerticalAlignOffset, -50.0f, 50.0f);
				Settings::Settings["TextVerticalAlignOffset"] = settings->textVerticalAlignOffset;
				Settings::Save(SettingsPath);
			}

			// Text Horizontal Alignment
			ImGui::SetNextItemWidth(200.0f);
			if (ImGui::SliderFloat("##Widget Text Horizontal Align", &settings->textHorizontalAlignOffset, -50.0f, 50.0f)) {
				settings->textHorizontalAlignOffset = std::clamp(settings->textHorizontalAlignOffset, -50.0f, 50.0f);
				Settings::Settings["TextHorizontalAlignOffset"] = settings->textHorizontalAlignOffset;
				Settings::Save(SettingsPath);
			}
			ImGui::SameLine();
			ImGui::SetNextItemWidth(60.0f);
			if (ImGui::InputFloat("Text Horizontal Align", &settings->textHorizontalAlignOffset, 1.0f)) {
				settings->textHorizontalAlignOffset = std::clamp(settings->textHorizontalAlignOffset, -50.0f, 50.0f);
				Settings::Settings["TextHorizontalAlignOffset"] = settings->textHorizontalAlignOffset;
				Settings::Save(SettingsPath);
			}

			ImGui::EndMenu();
		}

		if (ImGui::Checkbox("Damage vs Logged Players Only", &settings->vsLoggedPlayersOnly)) {
			Settings::Settings["vsLoggedPlayersOnly"] = settings->vsLoggedPlayersOnly;
			Settings::Save(SettingsPath);
		}

		if (ImGui::Checkbox("Show Squad Players Only", &settings->squadPlayersOnly)) {
			Settings::Settings["squadPlayersOnly"] = settings->squadPlayersOnly;
			Settings::Save(SettingsPath);
		}

		ImGui::EndPopup();
	}
	else {
		settings->isWindowNameEditing = false;
	}
	ImGui::PopStyleVar(2);
}

void RenderSpecializationBars(const TeamStats& teamData, const MainWindowSettings* settings, HINSTANCE hSelf) {
	bool useSquadStats = settings->squadPlayersOnly && teamData.isPOVTeam;
	bool vsLogPlayers = settings->vsLoggedPlayersOnly;

	const std::unordered_map<std::string, SpecStats>& eliteSpecStatsToDisplay =
		useSquadStats ? teamData.squadStats.eliteSpecStats : teamData.eliteSpecStats;

	std::vector<std::pair<std::string, SpecStats>> sortedClasses;
	sortedClasses.reserve(eliteSpecStatsToDisplay.size());

	for (const auto& [eliteSpec, stats] : eliteSpecStatsToDisplay) {
		sortedClasses.emplace_back(eliteSpec, stats);
	}

	std::sort(sortedClasses.begin(), sortedClasses.end(),
		getSpecSortComparator(settings->windowSort, vsLogPlayers));

	for (size_t i = 0; i < sortedClasses.size(); ++i) {
		const auto& specPair = sortedClasses[i];
		const std::string& eliteSpec = specPair.first;
		const SpecStats& stats = specPair.second;

		std::string professionName = "Unknown";
		auto it = eliteSpecToProfession.find(eliteSpec);
		if (it != eliteSpecToProfession.end()) {
			professionName = it->second;
		}

		ImVec4 primaryColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
		ImVec4 secondaryColor = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

		auto colorIt = std::find_if(professionColorPair.begin(), professionColorPair.end(),
			[&](const ProfessionColor& pc) { return pc.name == professionName; });

		if (colorIt != professionColorPair.end()) {
			primaryColor = colorIt->primaryColor;
			secondaryColor = colorIt->secondaryColor;
		}

		DrawBar(
			sortedClasses,
			i,
			settings->windowSort,
			stats,
			settings,
			primaryColor,
			secondaryColor,
			eliteSpec,
			hSelf
		);
	}
}

void RenderMainWindow(MainWindowSettings* settings, HINSTANCE hSelf) {
	if (!settings->isEnabled) return;

	static bool templateInitialized = false;
	if (!templateInitialized) {
		std::string currentTemplate = settings->sortTemplates[settings->windowSort];
		std::string template_to_use = BarTemplateRenderer::GetTemplateForSort(
			settings->windowSort,
			currentTemplate
		);
		BarTemplateRenderer::ParseTemplate(template_to_use);
		templateInitialized = true;
	}

	std::string windowName = settings->getDisplayName("WvW Fight Analysis");

	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoCollapse;
	if (!settings->showScrollBar) window_flags |= ImGuiWindowFlags_NoScrollbar;
	if (!settings->showTitle) window_flags |= ImGuiWindowFlags_NoTitleBar;
	if (!settings->allowFocus) {
		window_flags |= ImGuiWindowFlags_NoFocusOnAppearing;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
	}
	if (!settings->showBackground) window_flags |= ImGuiWindowFlags_NoBackground;
	if (settings->disableMoving) window_flags |= ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
	if (settings->disableClicking) window_flags |= ImGuiWindowFlags_NoInputs;

	ImGui::SetNextWindowPos(settings->position, ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(settings->size, ImGuiCond_FirstUseEver);

	if (settings->useWindowStyleForTitle) {
		ImGuiStyle& style = ImGui::GetStyle();
		ImVec4 prevTitleBg = style.Colors[ImGuiCol_TitleBg];
		ImVec4 prevTitleBgActive = style.Colors[ImGuiCol_TitleBgActive];
		ImVec4 prevTitleBgCollapsed = style.Colors[ImGuiCol_TitleBgCollapsed];

		if (settings->showBackground) {
			style.Colors[ImGuiCol_TitleBg] = style.Colors[ImGuiCol_WindowBg];
			style.Colors[ImGuiCol_TitleBgActive] = style.Colors[ImGuiCol_WindowBg];
			style.Colors[ImGuiCol_TitleBgCollapsed] = style.Colors[ImGuiCol_WindowBg];
		}
		else {
			ImVec4 transparentColor = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
			style.Colors[ImGuiCol_TitleBg] = transparentColor;
			style.Colors[ImGuiCol_TitleBgActive] = transparentColor;
			style.Colors[ImGuiCol_TitleBgCollapsed] = transparentColor;
		}

		if (ImGui::Begin(windowName.c_str(), &settings->isEnabled, window_flags)) {
			if (parsedLogs.empty()) {
				ImGui::Text(initialParsingComplete ? "No logs parsed yet." : "Parsing logs...");
				ImGui::End();

				style.Colors[ImGuiCol_TitleBg] = prevTitleBg;
				style.Colors[ImGuiCol_TitleBgActive] = prevTitleBgActive;
				style.Colors[ImGuiCol_TitleBgCollapsed] = prevTitleBgCollapsed;
				return;
			}

			const auto& currentLog = parsedLogs[currentLogIndex];
			const auto& currentLogData = currentLog.data;

			if (settings->showLogName) {
				std::string displayName = generateLogDisplayName(currentLog.filename,
					currentLog.data.combatStartTime, currentLog.data.combatEndTime);
				ImGui::Text("%s", displayName.c_str());
			}

			const char* team_names[] = { "Red", "Blue", "Green" };
			const ImVec4 team_colors[] = {
				ImGui::ColorConvertU32ToFloat4(IM_COL32(0xFF, 0x44, 0x44, 0xFF)),
				ImGui::ColorConvertU32ToFloat4(IM_COL32(0x33, 0xB5, 0xE5, 0xFF)),
				ImGui::ColorConvertU32ToFloat4(IM_COL32(0x99, 0xCC, 0x00, 0xFF))
			};

			std::array<TeamRenderInfo, 3> teams;
			int teamsWithData = 0;

			for (int i = 0; i < 3; ++i) {
				auto teamIt = currentLogData.teamStats.find(team_names[i]);
				teams[i].hasData = teamIt != currentLogData.teamStats.end() &&
					teamIt->second.totalPlayers >= Settings::teamPlayerThreshold;
				if (teams[i].hasData) {
					teamsWithData++;
					teams[i].stats = &teamIt->second;
					teams[i].name = team_names[i];
					teams[i].color = team_colors[i];
				}
			}

			if (teamsWithData == 0) {
				ImGui::Text("No team data available meeting the player threshold.");
			}
			else if (settings->useTabbedView) {
				if (ImGui::BeginTabBar("TeamTabBar", ImGuiTabBarFlags_None)) {
					for (int i = 0; i < 3; ++i) {
						if (teams[i].hasData) {
							ImGui::PushStyleColor(ImGuiCol_Text, teams[i].color);
							std::string tabName = teams[i].stats->isPOVTeam ?
								"* " + teams[i].name : teams[i].name;

							if (ImGui::BeginTabItem(tabName.c_str())) {
								ImGui::PopStyleColor();
								RenderTeamData(*teams[i].stats, settings, hSelf);
								ImGui::EndTabItem();
							}
							else {
								ImGui::PopStyleColor();
							}
						}
					}
					ImGui::EndTabBar();
				}
			}
			else {
				if (ImGui::BeginTable("TeamTable", teamsWithData, ImGuiTableFlags_BordersInner)) {
					for (const auto& team : teams) {
						if (team.hasData) {
							ImGui::TableSetupColumn(team.name.c_str(), ImGuiTableColumnFlags_WidthStretch);
						}
					}

					ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
					int columnIndex = 0;
					for (int i = 0; i < 3; ++i) {
						if (teams[i].hasData) {
							ImGui::TableSetColumnIndex(columnIndex++);
							ImGui::PushStyleColor(ImGuiCol_Text, teams[i].color);
							if (teams[i].stats->isPOVTeam) {
								if (Home && Home->Resource) {
									float sz = ImGui::GetFontSize() - 1;
									ImGui::Image(Home->Resource, ImVec2(sz, sz));
									ImGui::SameLine(0, 5);
								}
								else {
									Home = APIDefs->Textures.GetOrCreateFromResource("HOME_ICON", HOME, hSelf);
								}
							}
							ImGui::Text("%s Team", teams[i].name.c_str());
							ImGui::PopStyleColor();
						}
					}

					ImGui::TableNextRow();
					columnIndex = 0;
					for (int i = 0; i < 3; ++i) {
						if (teams[i].hasData) {
							ImGui::TableSetColumnIndex(columnIndex++);
							RenderTeamData(*teams[i].stats, settings, hSelf);
						}
					}

					ImGui::EndTable();
				}
			}

			if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup |
				ImGuiHoveredFlags_ChildWindows) && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
				ImGui::OpenPopup("MainWindow Settings");
			}

			RenderMainWindowSettingsPopup(settings);

			settings->position = ImGui::GetWindowPos();
			settings->size = ImGui::GetWindowSize();

			ImGui::End();
		}

		style.Colors[ImGuiCol_TitleBg] = prevTitleBg;
		style.Colors[ImGuiCol_TitleBgActive] = prevTitleBgActive;
		style.Colors[ImGuiCol_TitleBgCollapsed] = prevTitleBgCollapsed;
	}
	else {
		if (ImGui::Begin(windowName.c_str(), &settings->isEnabled, window_flags)) {
			if (parsedLogs.empty()) {
				ImGui::Text(initialParsingComplete ? "No logs parsed yet." : "Parsing logs...");
				ImGui::End();
				return;
			}

			const auto& currentLog = parsedLogs[currentLogIndex];
			const auto& currentLogData = currentLog.data;

			if (settings->showLogName) {
				std::string displayName = generateLogDisplayName(currentLog.filename,
					currentLog.data.combatStartTime, currentLog.data.combatEndTime);
				ImGui::Text("%s", displayName.c_str());
			}

			const char* team_names[] = { "Red", "Blue", "Green" };
			const ImVec4 team_colors[] = {
				ImGui::ColorConvertU32ToFloat4(IM_COL32(0xFF, 0x44, 0x44, 0xFF)),
				ImGui::ColorConvertU32ToFloat4(IM_COL32(0x33, 0xB5, 0xE5, 0xFF)),
				ImGui::ColorConvertU32ToFloat4(IM_COL32(0x99, 0xCC, 0x00, 0xFF))
			};

			std::array<TeamRenderInfo, 3> teams;
			int teamsWithData = 0;

			for (int i = 0; i < 3; ++i) {
				auto teamIt = currentLogData.teamStats.find(team_names[i]);
				teams[i].hasData = teamIt != currentLogData.teamStats.end() &&
					teamIt->second.totalPlayers >= Settings::teamPlayerThreshold;
				if (teams[i].hasData) {
					teamsWithData++;
					teams[i].stats = &teamIt->second;
					teams[i].name = team_names[i];
					teams[i].color = team_colors[i];
				}
			}

			if (teamsWithData == 0) {
				ImGui::Text("No team data available meeting the player threshold.");
			}
			else if (settings->useTabbedView) {
				if (ImGui::BeginTabBar("TeamTabBar", ImGuiTabBarFlags_None)) {
					for (int i = 0; i < 3; ++i) {
						if (teams[i].hasData) {
							ImGui::PushStyleColor(ImGuiCol_Text, teams[i].color);
							std::string tabName = teams[i].stats->isPOVTeam ?
								"* " + teams[i].name : teams[i].name;

							if (ImGui::BeginTabItem(tabName.c_str())) {
								ImGui::PopStyleColor();
								RenderTeamData(*teams[i].stats, settings, hSelf);
								ImGui::EndTabItem();
							}
							else {
								ImGui::PopStyleColor();
							}
						}
					}
					ImGui::EndTabBar();
				}
			}
			else {
				if (ImGui::BeginTable("TeamTable", teamsWithData, ImGuiTableFlags_BordersInner)) {
					for (const auto& team : teams) {
						if (team.hasData) {
							ImGui::TableSetupColumn(team.name.c_str(), ImGuiTableColumnFlags_WidthStretch);
						}
					}

					ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
					int columnIndex = 0;
					for (int i = 0; i < 3; ++i) {
						if (teams[i].hasData) {
							ImGui::TableSetColumnIndex(columnIndex++);
							ImGui::PushStyleColor(ImGuiCol_Text, teams[i].color);
							if (teams[i].stats->isPOVTeam) {
								if (Home && Home->Resource) {
									float sz = ImGui::GetFontSize() - 1;
									ImGui::Image(Home->Resource, ImVec2(sz, sz));
									ImGui::SameLine(0, 5);
								}
								else {
									Home = APIDefs->Textures.GetOrCreateFromResource("HOME_ICON", HOME, hSelf);
								}
							}
							ImGui::Text("%s Team", teams[i].name.c_str());
							ImGui::PopStyleColor();
						}
					}

					ImGui::TableNextRow();
					columnIndex = 0;
					for (int i = 0; i < 3; ++i) {
						if (teams[i].hasData) {
							ImGui::TableSetColumnIndex(columnIndex++);
							RenderTeamData(*teams[i].stats, settings, hSelf);
						}
					}

					ImGui::EndTable();
				}
			}

			if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup |
				ImGuiHoveredFlags_ChildWindows) && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
				ImGui::OpenPopup("MainWindow Settings");
			}

			RenderMainWindowSettingsPopup(settings);

			settings->position = ImGui::GetWindowPos();
			settings->size = ImGui::GetWindowSize();

			ImGui::End();
		}
	}
}


void RenderSimpleRatioBar(
	const std::vector<float>& counts,
	const std::vector<ImVec4>& colors,
	const ImVec2& size,
	const std::vector<const char*>& texts,
	ImTextureID statIcon,
	const WidgetWindowSettings* settings
) {
	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	const ImVec2 p = ImGui::GetCursorScreenPos();

	size_t numTeams = counts.size();
	float total = 0.0f;
	for (float count : counts) {
		total += count;
	}

	std::vector<float> fractions(numTeams);
	if (total == 0.0f) {
		float equalFrac = 1.0f / numTeams;
		for (size_t i = 0; i < numTeams; ++i) {
			fractions[i] = equalFrac;
		}
	}
	else {
		for (size_t i = 0; i < numTeams; ++i) {
			fractions[i] = counts[i] / total;
		}
	}

	std::vector<ImU32> colorsU32(numTeams);
	for (size_t i = 0; i < numTeams; ++i) {
		colorsU32[i] = ImGui::ColorConvertFloat4ToU32(colors[i]);
	}

	float sz = ImGui::GetFontSize();
	float iconWidth = sz;
	float iconHeight = sz;

	float x = p.x;
	float y = p.y;
	float width = size.x;
	float height = size.y;

	float leftPadding = 10.0f;
	float rightPadding = 5.0f;
	float interPadding = 5.0f;

	float iconAreaWidth = 0.0f;
	float barX = x;

	if (settings->showWidgetIcon && statIcon) {
		iconAreaWidth = leftPadding + iconWidth + rightPadding;
		float iconX = x + leftPadding;
		float iconY = y + (height - iconHeight) * 0.5f;

		draw_list->AddImage(
			statIcon,
			ImVec2(iconX, iconY),
			ImVec2(iconX + iconWidth, iconY + iconHeight)
		);

		barX = x + iconAreaWidth + interPadding;
	}

	float barWidth = width - (iconAreaWidth + (settings->showWidgetIcon ? interPadding : 0));

	float x_start = barX;
	for (size_t i = 0; i < numTeams; ++i) {
		float section_width = barWidth * fractions[i];
		float x_end = x_start + section_width;

		draw_list->AddRectFilled(
			ImVec2(x_start, y),
			ImVec2(x_end, y + height),
			colorsU32[i]
		);

		ImVec2 textSize = ImGui::CalcTextSize(texts[i]);
		float text_center_x = x_start + (section_width - textSize.x) * 0.5f + settings->textHorizontalAlignOffset;
		float center_y = y + (height - textSize.y) * 0.5f + settings->textVerticalAlignOffset;

		if (section_width >= textSize.x) {
			draw_list->AddText(
				ImVec2(text_center_x, center_y),
				IM_COL32_WHITE,
				texts[i]
			);
		}

		x_start = x_end;
	}

	draw_list->AddRect(
		ImVec2(x, y),
		ImVec2(x + width, y + height),
		IM_COL32_WHITE
	);

	ImGui::Dummy(size);
}

void RenderWidgetWindow(WidgetWindowSettings* settings, HINSTANCE hSelf) {
	if (!settings->isEnabled) return;

	std::string windowName = settings->getDisplayName("Team Ratio Bar");

	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_AlwaysAutoResize;

	if (!settings->allowFocus) {
		window_flags |= ImGuiWindowFlags_NoFocusOnAppearing;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
	}
	if (!settings->showBackground) window_flags |= ImGuiWindowFlags_NoBackground;
	if (settings->disableMoving) window_flags |= ImGuiWindowFlags_NoMove;
	if (settings->disableClicking) window_flags |= ImGuiWindowFlags_NoInputs;

	// Set window style
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

	ImVec2 widgetSize(settings->widgetWidth, settings->widgetHeight);
	ImGui::SetNextWindowPos(settings->position, ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(widgetSize, ImGuiCond_FirstUseEver);

	if (ImGui::Begin(windowName.c_str(), &settings->isEnabled, window_flags)) {
		if (parsedLogs.empty()) {
			ImGui::Text(initialParsingComplete ? "No logs parsed yet." : "Parsing logs...");
		}
		else {
			const auto& currentLogData = parsedLogs[currentLogIndex].data;

			const std::vector<std::string> team_names = { "Red", "Blue", "Green" };
			const std::vector<ImVec4> team_colors = {
				ImGui::ColorConvertU32ToFloat4(IM_COL32(0xff, 0x44, 0x44, 0xff)),
				ImGui::ColorConvertU32ToFloat4(IM_COL32(0x33, 0xb5, 0xe5, 0xff)),
				ImGui::ColorConvertU32ToFloat4(IM_COL32(0x99, 0xcc, 0x00, 0xff))
			};

			struct TeamDisplayData {
				float count;
				ImVec4 color;
				std::string text;
			};

			std::vector<TeamDisplayData> teamsToDisplay;

			for (size_t i = 0; i < team_names.size(); ++i) {
				auto teamIt = currentLogData.teamStats.find(team_names[i]);
				if (teamIt != currentLogData.teamStats.end()) {
					bool useSquadStats = settings->squadPlayersOnly && teamIt->second.isPOVTeam;

					float teamCountValue = 0.0f;
					if (settings->widgetStats == "players") {
						teamCountValue = static_cast<float>(useSquadStats ?
							teamIt->second.squadStats.totalPlayers : teamIt->second.totalPlayers);
					}
					else if (settings->widgetStats == "deaths") {
						teamCountValue = static_cast<float>(useSquadStats ?
							teamIt->second.squadStats.totalDeaths : teamIt->second.totalDeaths);
					}
					else if (settings->widgetStats == "downs") {
						teamCountValue = static_cast<float>(useSquadStats ?
							teamIt->second.squadStats.totalDowned : teamIt->second.totalDowned);
					}
					else if (settings->widgetStats == "damage") {
						float totalDamageToDisplay;
						if (settings->vsLoggedPlayersOnly) {
							totalDamageToDisplay = static_cast<float>(useSquadStats ?
								teamIt->second.squadStats.totalDamageVsPlayers :
								teamIt->second.totalDamageVsPlayers);
						}
						else {
							totalDamageToDisplay = static_cast<float>(useSquadStats ?
								teamIt->second.squadStats.totalDamage :
								teamIt->second.totalDamage);
						}
						teamCountValue = totalDamageToDisplay;
					}
					else if (settings->widgetStats == "kdr") {
						float kdRatioToDisplay = useSquadStats ?
							teamIt->second.squadStats.getKillDeathRatio() :
							teamIt->second.getKillDeathRatio();
						teamCountValue = kdRatioToDisplay;
					}

					char buf[64];
					if (settings->widgetStats == "damage") {
						std::string formattedDamage = formatDamage(static_cast<uint64_t>(teamCountValue));
						snprintf(buf, sizeof(buf), "%s", formattedDamage.c_str());
					}
					else if (settings->widgetStats == "kdr") {
						snprintf(buf, sizeof(buf), "%.2f", teamCountValue);
					}
					else {
						snprintf(buf, sizeof(buf), "%.0f", teamCountValue);
					}

					teamsToDisplay.push_back(TeamDisplayData{
						teamCountValue,
						team_colors[i],
						buf
						});
				}
			}

			if (teamsToDisplay.empty()) {
				ImGui::Text("No team data available.");
			}
			else {
				ImTextureID currentStatIcon = nullptr;
				if (settings->showWidgetIcon) {
					if (settings->widgetStats == "players") {
						if (!Squad || !Squad->Resource) {
							Squad = APIDefs->Textures.GetOrCreateFromResource("SQUAD_ICON", SQUAD, hSelf);
						}
						currentStatIcon = Squad ? Squad->Resource : nullptr;
					}
					else if (settings->widgetStats == "deaths") {
						if (!Death || !Death->Resource) {
							Death = APIDefs->Textures.GetOrCreateFromResource("DEATH_ICON", DEATH, hSelf);
						}
						currentStatIcon = Death ? Death->Resource : nullptr;
					}
					else if (settings->widgetStats == "downs") {
						if (!Downed || !Downed->Resource) {
							Downed = APIDefs->Textures.GetOrCreateFromResource("DOWNED_ICON", DOWNED, hSelf);
						}
						currentStatIcon = Downed ? Downed->Resource : nullptr;
					}
					else if (settings->widgetStats == "damage") {
						if (!Damage || !Damage->Resource) {
							Damage = APIDefs->Textures.GetOrCreateFromResource("DAMAGE_ICON", DAMAGE, hSelf);
						}
						currentStatIcon = Damage ? Damage->Resource : nullptr;
					}
					else if (settings->widgetStats == "kdr") {
						if (!Kdr || !Kdr->Resource) {
							Kdr = APIDefs->Textures.GetOrCreateFromResource("KDR_ICON", KDR, hSelf);
						}
						currentStatIcon = Kdr ? Kdr->Resource : nullptr;
					}
				}

				std::vector<float> counts;
				std::vector<ImVec4> colors;
				std::vector<const char*> texts;

				for (const auto& teamData : teamsToDisplay) {
					counts.push_back(teamData.count);
					colors.push_back(teamData.color);
					texts.push_back(teamData.text.c_str());
				}

				RenderSimpleRatioBar(
					counts,
					colors,
					ImVec2(widgetSize.x, widgetSize.y),
					texts,
					currentStatIcon,
					settings
				);
			}
		}

		// Handle right-click menu
		if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup |
			ImGuiHoveredFlags_ChildWindows) && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
			ImGui::OpenPopup("Widget Settings");
		}

		RenderWidgetSettingsPopup(settings);

		settings->position = ImGui::GetWindowPos();
	}
	ImGui::End();

	ImGui::PopStyleVar(4);
}

void RenderAggregateStatsWindow(AggregateWindowSettings* settings, HINSTANCE hSelf) {
	if (!settings->isEnabled) return;

	bool hasData = false;
	{
		std::lock_guard<std::mutex> lock(aggregateStatsMutex);
		hasData = !globalAggregateStats.teamAggregates.empty();
	}

	if (!hasData && settings->hideWhenEmpty) return;

	std::string windowName = settings->getDisplayName("Aggregate Stats");

	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoCollapse;
	if (!settings->showScrollBar) window_flags |= ImGuiWindowFlags_NoScrollbar;
	if (!settings->showTitle) window_flags |= ImGuiWindowFlags_NoTitleBar;
	if (!settings->allowFocus) {
		window_flags |= ImGuiWindowFlags_NoFocusOnAppearing;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
	}
	if (!settings->showBackground) window_flags |= ImGuiWindowFlags_NoBackground;
	if (settings->disableMoving) window_flags |= ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
	if (settings->disableClicking) window_flags |= ImGuiWindowFlags_NoInputs;

	ImGui::SetNextWindowPos(settings->position, ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(settings->size, ImGuiCond_FirstUseEver);

	if (settings->useWindowStyleForTitle) {
		ImGuiStyle& style = ImGui::GetStyle();
		ImVec4 prevTitleBg = style.Colors[ImGuiCol_TitleBg];
		ImVec4 prevTitleBgActive = style.Colors[ImGuiCol_TitleBgActive];
		ImVec4 prevTitleBgCollapsed = style.Colors[ImGuiCol_TitleBgCollapsed];

		if (settings->showBackground) {
			style.Colors[ImGuiCol_TitleBg] = style.Colors[ImGuiCol_WindowBg];
			style.Colors[ImGuiCol_TitleBgActive] = style.Colors[ImGuiCol_WindowBg];
			style.Colors[ImGuiCol_TitleBgCollapsed] = style.Colors[ImGuiCol_WindowBg];
		}
		else {
			ImVec4 transparentColor = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
			style.Colors[ImGuiCol_TitleBg] = transparentColor;
			style.Colors[ImGuiCol_TitleBgActive] = transparentColor;
			style.Colors[ImGuiCol_TitleBgCollapsed] = transparentColor;
		}

		if (ImGui::Begin(windowName.c_str(), &settings->isEnabled, window_flags)) {
			float sz = ImGui::GetFontSize();

			if (ImGui::Button("Reset")) {
				std::lock_guard<std::mutex> lock(aggregateStatsMutex);
				globalAggregateStats = GlobalAggregateStats();
				cachedAverages = CachedAverages();
			}

			{
				std::lock_guard<std::mutex> lock(aggregateStatsMutex);

				if (settings->showTotalCombatTime) {
					ImGui::Text("Total Combat Time: %s", formatDuration(globalAggregateStats.totalCombatTime));
				}
				if (settings->showAvgCombatTime) {
					ImGui::Text("Avg Combat Time: %s", formatDuration(cachedAverages.averageCombatTime));
				}

				ImGui::Separator();

				for (const auto& [teamName, teamAgg] : globalAggregateStats.teamAggregates) {
					ImGui::Spacing();

					ImVec4 teamColor = GetTeamColor(teamName);
					ImGui::PushStyleColor(ImGuiCol_Text, teamColor);
					ImGui::Text("%s", teamName.c_str());
					ImGui::PopStyleColor();

					bool useSquadStats = (settings->squadPlayersOnly && teamAgg.isPOVTeam);
					const SquadAggregateStats& displayStats = useSquadStats ?
						teamAgg.povSquadTotals : teamAgg.teamTotals;

					if (settings->showTeamTotalPlayers) {
						double avgPlayerCount = useSquadStats ?
							cachedAverages.averagePOVSquadPlayerCounts[teamName] :
							cachedAverages.averageTeamPlayerCounts[teamName];

						if (settings->showClassIcons) {
							if (Squad && Squad->Resource) {
								ImGui::Image(Squad->Resource, ImVec2(sz, sz));
								ImGui::SameLine(0, 5);
							}
							else {
								Squad = APIDefs->Textures.GetOrCreateFromResource("SQUAD_ICON", SQUAD, hSelf);
							}
						}
						if (settings->showClassNames) {
							ImGui::Text("Avg Players: %d", (int)std::round(avgPlayerCount));
						}
						else {
							ImGui::Text("%d", (int)std::round(avgPlayerCount));
						}
					}

					if (settings->showTeamDeaths) {
						if (settings->showClassIcons) {
							if (Death && Death->Resource) {
								ImGui::Image(Death->Resource, ImVec2(sz, sz));
								ImGui::SameLine(0, 5);
							}
							else {
								Death = APIDefs->Textures.GetOrCreateFromResource("DEATH_ICON", DEATH, hSelf);
							}
						}
						if (settings->showClassNames) {
							ImGui::Text("Deaths: %d", displayStats.totalDeaths);
						}
						else {
							ImGui::Text("%d", displayStats.totalDeaths);
						}
					}

					if (settings->showTeamDowned) {
						if (settings->showClassIcons) {
							if (Downed && Downed->Resource) {
								ImGui::Image(Downed->Resource, ImVec2(sz, sz));
								ImGui::SameLine(0, 5);
							}
							else {
								Downed = APIDefs->Textures.GetOrCreateFromResource("DOWNED_ICON", DOWNED, hSelf);
							}
						}
						if (settings->showClassNames) {
							ImGui::Text("Downs: %d", displayStats.totalDowned);
						}
						else {
							ImGui::Text("%d", displayStats.totalDowned);
						}
					}

					if (settings->showAvgSpecs) {
						std::string label = "Specs##" + teamName;
						if (ImGui::TreeNode(label.c_str())) {
							if (useSquadStats) {
								auto teamIt = cachedAverages.averagePOVSquadSpecCounts.find(teamName);
								if (teamIt != cachedAverages.averagePOVSquadSpecCounts.end()) {
									for (const auto& [specName, avgCount] : teamIt->second) {
										ImGui::Text("- %s: %d", specName.c_str(), (int)std::round(avgCount));
									}
								}
							}
							else {
								auto teamIt = cachedAverages.averageTeamSpecCounts.find(teamName);
								if (teamIt != cachedAverages.averageTeamSpecCounts.end()) {
									for (const auto& [specName, avgCount] : teamIt->second) {
										ImGui::Text("- %s: %d", specName.c_str(), (int)std::round(avgCount));
									}
								}
							}
							ImGui::TreePop();
						}
					}

					ImGui::Separator();
				}
			}

			if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup |
				ImGuiHoveredFlags_ChildWindows) && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
				ImGui::OpenPopup("Aggregate Settings");
			}

			RenderAggregateSettingsPopup(settings);

			settings->position = ImGui::GetWindowPos();
			settings->size = ImGui::GetWindowSize();

			ImGui::End();
		}

		style.Colors[ImGuiCol_TitleBg] = prevTitleBg;
		style.Colors[ImGuiCol_TitleBgActive] = prevTitleBgActive;
		style.Colors[ImGuiCol_TitleBgCollapsed] = prevTitleBgCollapsed;
	}
	else {
		if (ImGui::Begin(windowName.c_str(), &settings->isEnabled, window_flags)) {
			float sz = ImGui::GetFontSize();

			if (ImGui::Button("Reset")) {
				std::lock_guard<std::mutex> lock(aggregateStatsMutex);
				globalAggregateStats = GlobalAggregateStats();
				cachedAverages = CachedAverages();
			}

			{
				std::lock_guard<std::mutex> lock(aggregateStatsMutex);

				if (settings->showTotalCombatTime) {
					ImGui::Text("Total Combat Time: %s", formatDuration(globalAggregateStats.totalCombatTime));
				}
				if (settings->showAvgCombatTime) {
					ImGui::Text("Avg Combat Time: %s", formatDuration(cachedAverages.averageCombatTime));
				}

				ImGui::Separator();

				for (const auto& [teamName, teamAgg] : globalAggregateStats.teamAggregates) {
					ImGui::Spacing();

					ImVec4 teamColor = GetTeamColor(teamName);
					ImGui::PushStyleColor(ImGuiCol_Text, teamColor);
					ImGui::Text("%s", teamName.c_str());
					ImGui::PopStyleColor();

					bool useSquadStats = (settings->squadPlayersOnly && teamAgg.isPOVTeam);
					const SquadAggregateStats& displayStats = useSquadStats ?
						teamAgg.povSquadTotals : teamAgg.teamTotals;

					if (settings->showTeamTotalPlayers) {
						double avgPlayerCount = useSquadStats ?
							cachedAverages.averagePOVSquadPlayerCounts[teamName] :
							cachedAverages.averageTeamPlayerCounts[teamName];

						if (settings->showClassIcons) {
							if (Squad && Squad->Resource) {
								ImGui::Image(Squad->Resource, ImVec2(sz, sz));
								ImGui::SameLine(0, 5);
							}
							else {
								Squad = APIDefs->Textures.GetOrCreateFromResource("SQUAD_ICON", SQUAD, hSelf);
							}
						}
						if (settings->showClassNames) {
							ImGui::Text("Avg Players: %d", (int)std::round(avgPlayerCount));
						}
						else {
							ImGui::Text("%d", (int)std::round(avgPlayerCount));
						}
					}

					if (settings->showTeamDeaths) {
						if (settings->showClassIcons) {
							if (Death && Death->Resource) {
								ImGui::Image(Death->Resource, ImVec2(sz, sz));
								ImGui::SameLine(0, 5);
							}
							else {
								Death = APIDefs->Textures.GetOrCreateFromResource("DEATH_ICON", DEATH, hSelf);
							}
						}
						if (settings->showClassNames) {
							ImGui::Text("Deaths: %d", displayStats.totalDeaths);
						}
						else {
							ImGui::Text("%d", displayStats.totalDeaths);
						}
					}

					if (settings->showTeamDowned) {
						if (settings->showClassIcons) {
							if (Downed && Downed->Resource) {
								ImGui::Image(Downed->Resource, ImVec2(sz, sz));
								ImGui::SameLine(0, 5);
							}
							else {
								Downed = APIDefs->Textures.GetOrCreateFromResource("DOWNED_ICON", DOWNED, hSelf);
							}
						}
						if (settings->showClassNames) {
							ImGui::Text("Downs: %d", displayStats.totalDowned);
						}
						else {
							ImGui::Text("%d", displayStats.totalDowned);
						}
					}

					if (settings->showAvgSpecs) {
						std::string label = "Specs##" + teamName;
						if (ImGui::TreeNode(label.c_str())) {
							if (useSquadStats) {
								auto teamIt = cachedAverages.averagePOVSquadSpecCounts.find(teamName);
								if (teamIt != cachedAverages.averagePOVSquadSpecCounts.end()) {
									for (const auto& [specName, avgCount] : teamIt->second) {
										ImGui::Text("- %s: %d", specName.c_str(), (int)std::round(avgCount));
									}
								}
							}
							else {
								auto teamIt = cachedAverages.averageTeamSpecCounts.find(teamName);
								if (teamIt != cachedAverages.averageTeamSpecCounts.end()) {
									for (const auto& [specName, avgCount] : teamIt->second) {
										ImGui::Text("- %s: %d", specName.c_str(), (int)std::round(avgCount));
									}
								}
							}
							ImGui::TreePop();
						}
					}

					ImGui::Separator();
				}
			}

			if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup |
				ImGuiHoveredFlags_ChildWindows) && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
				ImGui::OpenPopup("Aggregate Settings");
			}

			RenderAggregateSettingsPopup(settings);

			settings->position = ImGui::GetWindowPos();
			settings->size = ImGui::GetWindowSize();

			ImGui::End();
		}
	}
}

void RenderAggregateSettingsPopup(AggregateWindowSettings* settings) {
	if (ImGui::BeginPopup("Aggregate Settings")) {
		if (!settings->isWindowNameEditing) {
			strncpy(settings->tempWindowName, settings->windowName.c_str(), sizeof(settings->tempWindowName) - 1);
			settings->isWindowNameEditing = true;
		}

		ImGui::Text("Window Name:");
		float windowWidth = ImGui::GetWindowWidth();
		ImGui::SetNextItemWidth(windowWidth * 0.5f);
		if (ImGui::InputText("##Window Name", settings->tempWindowName, sizeof(settings->tempWindowName))) {}

		ImGui::SameLine();
		if (ImGui::Button("Apply")) {
			settings->windowName = settings->tempWindowName;
			Settings::Save(SettingsPath);
		}

		if (ImGui::BeginMenu("Display")) {

			if (ImGui::Checkbox("average combat time", &settings->showAvgCombatTime)) {
				Settings::Save(SettingsPath);
			}
			if (ImGui::Checkbox("show total tombat time", &settings->showTotalCombatTime)) {
				Settings::Save(SettingsPath);
			}
			ImGui::Separator();

			if (ImGui::Checkbox("average players", &settings->showTeamTotalPlayers)) {
				Settings::Save(SettingsPath);
			}
			if (ImGui::Checkbox("deaths", &settings->showTeamDeaths)) {
				Settings::Save(SettingsPath);
			}
			if (ImGui::Checkbox("downs", &settings->showTeamDowned)) {
				Settings::Save(SettingsPath);
			}
			ImGui::Separator();

			if (ImGui::Checkbox("label names", &settings->showClassNames)) {
				Settings::Save(SettingsPath);
			}
			if (ImGui::Checkbox("icons", &settings->showClassIcons)) {
				Settings::Save(SettingsPath);
			}
			if (ImGui::Checkbox("average specs", &settings->showAvgSpecs)) {
				Settings::Save(SettingsPath);
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Style")) {
			if (ImGui::Checkbox("show title", &settings->showTitle)) { Settings::Save(SettingsPath); }
			if (ImGui::Checkbox("use window style for title bar", &settings->useWindowStyleForTitle)) {
				Settings::Save(SettingsPath);
			}
			if (ImGui::Checkbox("scroll bar", &settings->showScrollBar)) { Settings::Save(SettingsPath); }
			if (ImGui::Checkbox("background", &settings->showBackground)) { Settings::Save(SettingsPath); }
			if (ImGui::Checkbox("allow focus", &settings->allowFocus)) { Settings::Save(SettingsPath); }
			ImGui::EndMenu();
		}

		if (ImGui::Checkbox("Squad Players Only", &settings->squadPlayersOnly)) {
			Settings::Save(SettingsPath);
		}

		ImGui::EndPopup();
	}
	else {
		settings->isWindowNameEditing = false;
	}
}

void RenderAllWindows(HINSTANCE hSelf) {
	if (!NexusLink || !NexusLink->IsGameplay || !MumbleLink || MumbleLink->Context.IsMapOpen) {
		return;
	}

	if (
		MumbleLink->Context.MapType != Mumble::EMapType::WvW_EternalBattlegrounds &&
		MumbleLink->Context.MapType != Mumble::EMapType::WvW_BlueBorderlands &&
		MumbleLink->Context.MapType != Mumble::EMapType::WvW_GreenBorderlands &&
		MumbleLink->Context.MapType != Mumble::EMapType::WvW_RedBorderlands &&
		MumbleLink->Context.MapType != Mumble::EMapType::WvW_ObsidianSanctum &&
		MumbleLink->Context.MapType != Mumble::EMapType::WvW_EdgeOfTheMists &&
		MumbleLink->Context.MapType != Mumble::EMapType::WvW_Lounge
		) {
		return;
	}

	bool inCombat = MumbleLink->Context.IsInCombat;

	for (auto& mainWindow : Settings::windowManager.mainWindows) {
		if (!mainWindow || !mainWindow->isEnabled) continue;
		if (inCombat && mainWindow->hideInCombat) continue;
		if (!inCombat && mainWindow->hideOutOfCombat) continue;

		std::string windowId = mainWindow->windowId;
		if (windowId.empty()) continue;
		ImGui::SetNextWindowSizeConstraints(ImVec2(100, 100), ImVec2(FLT_MAX, FLT_MAX));

		RenderMainWindow(mainWindow.get(), hSelf);
	}

	for (auto& widgetWindow : Settings::windowManager.widgetWindows) {
		if (!widgetWindow || !widgetWindow->isEnabled) continue;
		if (inCombat && widgetWindow->hideInCombat) continue;
		if (!inCombat && widgetWindow->hideOutOfCombat) continue;

		std::string windowId = widgetWindow->windowId;
		if (windowId.empty()) continue;

		RenderWidgetWindow(widgetWindow.get(), hSelf);
	}

	if (Settings::windowManager.aggregateWindow &&
		Settings::windowManager.aggregateWindow->isEnabled) {

		bool shouldHide = false;

		if (inCombat && Settings::windowManager.aggregateWindow->hideInCombat) {
			shouldHide = true;
		}
		if (!inCombat && Settings::windowManager.aggregateWindow->hideOutOfCombat) {
			shouldHide = true;
		}
		if (!shouldHide) {
			ImGui::SetNextWindowSizeConstraints(ImVec2(100, 100), ImVec2(FLT_MAX, FLT_MAX));
			RenderAggregateStatsWindow(Settings::windowManager.aggregateWindow.get(), hSelf);
		}
	}
}