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
	return 0; // Default to 0 if criteria not found
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
            // First try primary sort criteria
            uint64_t valueA = getSortValue(sortCriteria, a.second, vsLogPlayers);
            uint64_t valueB = getSortValue(sortCriteria, b.second, vsLogPlayers);
            
            if (valueA != valueB) {
                return valueA > valueB;
            }
            
            // If primary values are equal, sort by damage
            uint64_t damageA = vsLogPlayers ? a.second.totalDamageVsPlayers : a.second.totalDamage;
            uint64_t damageB = vsLogPlayers ? b.second.totalDamageVsPlayers : b.second.totalDamage;
            
            if (damageA != damageB) {
                return damageA > damageB;
            }
            
            // If damage is equal, sort by player count
            if (a.second.count != b.second.count) {
                return a.second.count > b.second.count;
            }
            
            // If everything else is equal, sort alphabetically
            return a.first < b.first;
    };
}


void DrawBar(
	float frac,
	int count,
	const std::string& barRep,
	const SpecStats& stats,
	bool vsLogPlayers,
	const ImVec4& primaryColor,
	const ImVec4& secondaryColor,
	const std::string& eliteSpec,
	bool showDamage,
	HINSTANCE hSelf
) {
	// Get current cursor positions
	ImVec2 cursor_pos = ImGui::GetCursorPos();
	ImVec2 screen_pos = ImGui::GetCursorScreenPos();

	// Calculate bar dimensions
	float bar_width = ImGui::GetContentRegionAvail().x * frac;
	float bar_height = ImGui::GetTextLineHeight() + 4.0f; // Slight padding

	// Draw the primary bar
	ImGui::GetWindowDrawList()->AddRectFilled(
		screen_pos,
		ImVec2(screen_pos.x + bar_width, screen_pos.y + bar_height),
		ImGui::ColorConvertFloat4ToU32(primaryColor)
	);

	// Get values for primary and secondary bars
	auto [primaryValue, secondaryValue] = getSecondaryBarValues(barRep, stats, vsLogPlayers);

	// Draw the secondary bar if applicable
	if (primaryValue > 0 && secondaryValue > 0 && secondaryValue <= primaryValue) {
		float secondary_frac = static_cast<float>(secondaryValue) / static_cast<float>(primaryValue);
		secondary_frac = std::clamp(secondary_frac, 0.0f, 1.0f);
		float secondary_width = bar_width * secondary_frac;

		ImGui::GetWindowDrawList()->AddRectFilled(
			screen_pos,
			ImVec2(screen_pos.x + secondary_width, screen_pos.y + bar_height),
			ImGui::ColorConvertFloat4ToU32(secondaryColor)
		);
	}

	// Set cursor position for text
	ImGui::SetCursorPos(ImVec2(cursor_pos.x + 5.0f, cursor_pos.y + 2.0f));

	// Render the template
	BarTemplateRenderer::RenderTemplate(
		count,
		eliteSpec,
		Settings::useShortClassNames,
		primaryValue,
		secondaryValue,
		hSelf,
		ImGui::GetFontSize()
	);

	// Add tooltip for more detailed information
	if (ImGui::IsItemHovered())
	{
		std::string tooltip;
		if (barRep == "damage") {
			tooltip = "Total Damage: " + std::to_string(primaryValue);
			if (secondaryValue > 0) {
				tooltip += "\nDowned Contribution: " + std::to_string(secondaryValue) +
					" (" + std::to_string(static_cast<int>((static_cast<float>(secondaryValue) /
						static_cast<float>(primaryValue)) * 100.0f)) + "%)";
			}
		}
		ImGui::SetTooltip("%s", tooltip.c_str());
	}

	// Move cursor below the bar for the next item
	ImGui::SetCursorPosY(cursor_pos.y + bar_height + 2.0f);
}

void RenderSpecializationBars(const TeamStats& teamData, int teamIndex, HINSTANCE hSelf) {
	// Determine if squad-specific stats should be used
	bool useSquadStats = Settings::squadPlayersOnly && teamData.isPOVTeam;
	bool vsLogPlayers = Settings::vsLoggedPlayersOnly;
	bool showDamage = Settings::showSpecDamage;

	// Select the appropriate set of elite specialization stats
	const std::unordered_map<std::string, SpecStats>& eliteSpecStatsToDisplay =
		useSquadStats ? teamData.squadStats.eliteSpecStats : teamData.eliteSpecStats;

	// Copy the elite specs into a sortable vector
	std::vector<std::pair<std::string, SpecStats>> sortedClasses;
	sortedClasses.reserve(eliteSpecStatsToDisplay.size());

	for (const auto& [eliteSpec, stats] : eliteSpecStatsToDisplay) {
		sortedClasses.emplace_back(eliteSpec, stats);
	}

	// Sort based on the selected criteria and secondary criteria
	std::sort(sortedClasses.begin(), sortedClasses.end(),
		getSpecSortComparator(Settings::windowSort, vsLogPlayers));

	// Get the representation to use for bars
	const std::string& barRep = Settings::barRepIndependent ? Settings::barRepresentation : Settings::windowSort;

	// Find the maximum value for bar scaling based on selected representation
	uint64_t maxValue = 0;
	if (!sortedClasses.empty()) {
		maxValue = getBarValue(barRep, sortedClasses.front().second, vsLogPlayers);
		for (const auto& specPair : sortedClasses) {
			uint64_t value = getBarValue(barRep, specPair.second, vsLogPlayers);
			maxValue = std::max(maxValue, value);
		}
	}

	// Iterate through each specialization and render the bars
	for (const auto& specPair : sortedClasses) {
		const std::string& eliteSpec = specPair.first;
		const SpecStats& stats = specPair.second;
		int count = stats.count;

		// Calculate the fraction for bar size based on selected representation
		uint64_t barValue = getBarValue(barRep, stats, vsLogPlayers);
		float frac = (maxValue > 0) ?
			static_cast<float>(barValue) / static_cast<float>(maxValue) : 0.0f;

		// Get total damage and downed contribution for display
		uint64_t totalDamage = vsLogPlayers ? stats.totalDamageVsPlayers : stats.totalDamage;
		uint64_t totalDownedContribution = vsLogPlayers ?
			stats.totalDownedContributionVsPlayers : stats.totalDownedContribution;

		// Get profession colors
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
			frac,
			count,
			barRep,
			stats,
			vsLogPlayers,
			primaryColor,
			secondaryColor,
			eliteSpec,
			showDamage,
			hSelf
		);
	}
}


void RenderTeamData(int teamIndex, const TeamStats& teamData, HINSTANCE hSelf)
{
	ImGuiStyle& style = ImGui::GetStyle();
	float sz = ImGui::GetFontSize();

	ImGui::Spacing();

	bool useSquadStats = Settings::squadPlayersOnly && teamData.isPOVTeam;

	uint32_t totalPlayersToDisplay = useSquadStats ? teamData.squadStats.totalPlayers : teamData.totalPlayers;

	if (Settings::showTeamTotalPlayers) {
		if (Settings::showClassIcons) {
			if (Squad && Squad->Resource) {
				ImGui::Image(Squad->Resource, ImVec2(sz, sz));
				ImGui::SameLine(0, 5);
			}
			else {
				Squad = APIDefs->Textures.GetOrCreateFromResource("SQUAD_ICON", SQUAD, hSelf);
			}
		}
		if (Settings::showClassNames) {
			ImGui::Text("Total:  %d", totalPlayersToDisplay);
		}
		else {
			ImGui::Text("%d", totalPlayersToDisplay);
		}
	}

	uint32_t totalKillsToDisplay = useSquadStats ? teamData.squadStats.totalKills : teamData.totalKills;
	uint32_t totalDeathsfromKillingBlowsToDisplay = useSquadStats ? teamData.squadStats.totalDeathsFromKillingBlows : teamData.totalDeathsFromKillingBlows;
	double kdRatioToDisplay = useSquadStats ? teamData.squadStats.getKillDeathRatio() : teamData.getKillDeathRatio();
	std::string kdString = (std::ostringstream() << totalKillsToDisplay << "/" << totalDeathsfromKillingBlowsToDisplay << " (" << std::fixed << std::setprecision(2) << kdRatioToDisplay << ")").str();

	if (Settings::showTeamKDR) {
		if (Settings::showClassIcons) {
			if (Kdr && Kdr->Resource) {
				ImGui::Image(Kdr->Resource, ImVec2(sz, sz));
				ImGui::SameLine(0, 5);
			}
			else {
				Kdr = APIDefs->Textures.GetOrCreateFromResource("KDR_ICON", KDR, hSelf);
			}
		}
		if (Settings::showClassNames) {
			ImGui::Text("K/D: %s", kdString.c_str());
		}
		else {
			ImGui::Text("%s", kdString.c_str());
		}
	}

	// Total Deaths
	uint32_t totalDeathsToDisplay = useSquadStats ? teamData.squadStats.totalDeaths : teamData.totalDeaths;

	if (Settings::showTeamDeaths) {
		if (Settings::showClassIcons) {
			if (Death && Death->Resource) {
				ImGui::Image(Death->Resource, ImVec2(sz, sz));
				ImGui::SameLine(0, 5);
			}
			else {
				Death = APIDefs->Textures.GetOrCreateFromResource("DEATH_ICON", DEATH, hSelf);
			}
		}
		if (Settings::showClassNames) {
			ImGui::Text("Deaths: %d", totalDeathsToDisplay);
		}
		else {
			ImGui::Text("%d", totalDeathsToDisplay);
		}
	}

	// Total Downed
	uint32_t totalDownedToDisplay = useSquadStats ? teamData.squadStats.totalDowned : teamData.totalDowned;

	if (Settings::showTeamDowned) {
		if (Settings::showClassIcons) {
			if (Downed && Downed->Resource) {
				ImGui::Image(Downed->Resource, ImVec2(sz, sz));
				ImGui::SameLine(0, 5);
			}
			else {
				Downed = APIDefs->Textures.GetOrCreateFromResource("DOWNED_ICON", DOWNED, hSelf);
			}
		}
		if (Settings::showClassNames) {
			ImGui::Text("Downs:  %d", totalDownedToDisplay);
		}
		else {
			ImGui::Text("%d", totalDownedToDisplay);
		}
	}

	// Total Damage
	uint64_t totalDamageToDisplay = 0;
	if (Settings::vsLoggedPlayersOnly) {
		totalDamageToDisplay = useSquadStats ? teamData.squadStats.totalDamageVsPlayers : teamData.totalDamageVsPlayers;
	}
	else {
		totalDamageToDisplay = useSquadStats ? teamData.squadStats.totalDamage : teamData.totalDamage;
	}

	if (Settings::showTeamDamage) {
		if (Settings::showClassIcons) {
			if (Damage && Damage->Resource) {
				ImGui::Image(Damage->Resource, ImVec2(sz, sz));
				ImGui::SameLine(0, 5);
			}
			else {
				Damage = APIDefs->Textures.GetOrCreateFromResource("DAMAGE_ICON", DAMAGE, hSelf);
			}
		}

		std::string formattedDamage = formatDamage(totalDamageToDisplay);

		if (Settings::showClassNames) {
			ImGui::Text("Damage: %s", formattedDamage.c_str());
		}
		else {
			ImGui::Text("%s", formattedDamage.c_str());
		}
	}

	// Total Downed Contribution
	uint64_t totalDownedContToDisplay = 0;
	if (Settings::vsLoggedPlayersOnly) {
		totalDownedContToDisplay = useSquadStats ? teamData.squadStats.totalDownedContributionVsPlayers : teamData.totalDownedContributionVsPlayers;
	}
	else {
		totalDownedContToDisplay = useSquadStats ? teamData.squadStats.totalDownedContribution : teamData.totalDownedContribution;
	}

	if (Settings::showTeamDownCont) {
		if (Settings::showClassIcons) {
			// Add icon handling here if needed
		}

		std::string formattedDamage = formatDamage(totalDownedContToDisplay);

		if (Settings::showClassNames) {
			ImGui::Text("Down Cont: %s", formattedDamage.c_str());
		}
		else {
			ImGui::Text("%s", formattedDamage.c_str());
		}
	}

	// Total Kill Contribution
	uint64_t totalKillContToDisplay = 0;
	if (Settings::vsLoggedPlayersOnly) {
		totalKillContToDisplay = useSquadStats ? teamData.squadStats.totalKillContributionVsPlayers : teamData.totalKillContributionVsPlayers;
	}
	else {
		totalKillContToDisplay = useSquadStats ? teamData.squadStats.totalKillContribution : teamData.totalKillContribution;
	}

	if (Settings::showTeamKillCont) {
		if (Settings::showClassIcons) {
			// Add icon handling here if needed
		}

		std::string formattedDamage = formatDamage(totalKillContToDisplay);

		if (Settings::showClassNames) {
			ImGui::Text("Kill Cont: %s", formattedDamage.c_str());
		}
		else {
			ImGui::Text("%s", formattedDamage.c_str());
		}
	}

	// Total Strike Damage
	uint64_t totalStrikeDamageToDisplay = 0;
	if (Settings::vsLoggedPlayersOnly) {
		totalStrikeDamageToDisplay = useSquadStats ? teamData.squadStats.totalStrikeDamageVsPlayers : teamData.totalStrikeDamageVsPlayers;
	}
	else {
		totalStrikeDamageToDisplay = useSquadStats ? teamData.squadStats.totalStrikeDamage : teamData.totalStrikeDamage;
	}

	if (Settings::showTeamStrikeDamage) {
		if (Settings::showClassIcons) {
			if (Strike && Strike->Resource) {
				ImGui::Image(Strike->Resource, ImVec2(sz, sz));
				ImGui::SameLine(0, 5);
			}
			else {
				Strike = APIDefs->Textures.GetOrCreateFromResource("STRIKE_ICON", STRIKE, hSelf);
			}
		}

		std::string formattedDamage = formatDamage(totalStrikeDamageToDisplay);

		if (Settings::showClassNames) {
			ImGui::Text("Strike: %s", formattedDamage.c_str());
		}
		else {
			ImGui::Text("%s", formattedDamage.c_str());
		}
	}

	// Total Condition Damage
	uint64_t totalCondiDamageToDisplay = 0;
	if (Settings::vsLoggedPlayersOnly) {
		totalCondiDamageToDisplay = useSquadStats ? teamData.squadStats.totalCondiDamageVsPlayers : teamData.totalCondiDamageVsPlayers;
	}
	else {
		totalCondiDamageToDisplay = useSquadStats ? teamData.squadStats.totalCondiDamage : teamData.totalCondiDamage;
	}

	if (Settings::showTeamCondiDamage) {
		if (Settings::showClassIcons) {
			if (Condi && Condi->Resource) {
				ImGui::Image(Condi->Resource, ImVec2(sz, sz));
				ImGui::SameLine(0, 5);
			}
			else {
				Condi = APIDefs->Textures.GetOrCreateFromResource("CONDI_ICON", CONDI, hSelf);
			}
		}

		std::string formattedDamage = formatDamage(totalCondiDamageToDisplay);

		if (Settings::showClassNames) {
			ImGui::Text("Condi:  %s", formattedDamage.c_str());
		}
		else {
			ImGui::Text("%s", formattedDamage.c_str());
		}
	}

	// Render Specialization Bars
	if (Settings::showSpecBars && !Settings::splitStatsWindow) {
		ImGui::Separator();

		bool vsLogPlayers = Settings::vsLoggedPlayersOnly;
		bool showDamage = Settings::showSpecDamage;

		const std::unordered_map<std::string, SpecStats>& eliteSpecStatsToDisplay = useSquadStats ?
			teamData.squadStats.eliteSpecStats : teamData.eliteSpecStats;

		std::vector<std::pair<std::string, SpecStats>> sortedClasses;
		sortedClasses.reserve(eliteSpecStatsToDisplay.size());

		for (const auto& [eliteSpec, stats] : eliteSpecStatsToDisplay) {
			sortedClasses.emplace_back(eliteSpec, stats);
		}

		// Sort based on primary and secondary criteria
		std::sort(sortedClasses.begin(), sortedClasses.end(),
			getSpecSortComparator(Settings::windowSort, vsLogPlayers));

		// Get the representation to use for bars
		const std::string& barRep = Settings::barRepIndependent ? Settings::barRepresentation : Settings::windowSort;

		// Find the maximum value for bar scaling based on selected representation
		uint64_t maxValue = 0;
		if (!sortedClasses.empty()) {
			maxValue = getBarValue(barRep, sortedClasses.front().second, vsLogPlayers);
			for (const auto& specPair : sortedClasses) {
				uint64_t value = getBarValue(barRep, specPair.second, vsLogPlayers);
				maxValue = std::max(maxValue, value);
			}
		}

		// Render each class bar
		for (const auto& specPair : sortedClasses) {
			const std::string& eliteSpec = specPair.first;
			const SpecStats& stats = specPair.second;
			int count = stats.count;

			// Calculate the fraction for bar size based on selected representation
			uint64_t barValue = getBarValue(barRep, stats, vsLogPlayers);
			float frac = (maxValue > 0) ?
				static_cast<float>(barValue) / static_cast<float>(maxValue) : 0.0f;

			// Get total damage for display
			uint64_t totalDamage = vsLogPlayers ? stats.totalDamageVsPlayers : stats.totalDamage;
			uint64_t totalDownedContribution = vsLogPlayers ?
				stats.totalDownedContributionVsPlayers : stats.totalDownedContribution;

			// Get profession colors
			std::string professionName = "Unknown";
			auto it = eliteSpecToProfession.find(eliteSpec);
			if (it != eliteSpecToProfession.end()) {
				professionName = it->second;
			}

			ImVec4 primaryColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
			ImVec4 secondaryColor = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

			auto colorIt = std::find_if(professionColorPair.begin(), professionColorPair.end(),
				[professionName](const ProfessionColor& pc) { return pc.name == professionName; });

			if (colorIt != professionColorPair.end()) {
				primaryColor = colorIt->primaryColor;
				secondaryColor = colorIt->secondaryColor;
			}

			DrawBar(
				frac,
				count,
				barRep,
				stats,
				vsLogPlayers,
				primaryColor,
				secondaryColor,
				eliteSpec,
				showDamage,
				hSelf
			);
		}
	}
}


void RenderSimpleRatioBar(
	const std::vector<float>& counts,
	const std::vector<ImVec4>& colors,
	const ImVec2& size,
	const std::vector<const char*>& texts,
	ImTextureID statIcon)
{
	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	const ImVec2 p = ImGui::GetCursorScreenPos();

	size_t numTeams = counts.size();

	float total = 0.0f;
	for (float count : counts)
		total += count;

	std::vector<float> fractions(numTeams);
	if (total == 0.0f)
	{
		float equalFrac = 1.0f / numTeams;
		for (size_t i = 0; i < numTeams; ++i)
			fractions[i] = equalFrac;
	}
	else
	{
		for (size_t i = 0; i < numTeams; ++i)
			fractions[i] = counts[i] / total;
	}

	std::vector<ImU32> colorsU32(numTeams);
	for (size_t i = 0; i < numTeams; ++i)
		colorsU32[i] = ImGui::ColorConvertFloat4ToU32(colors[i]);

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

	if (Settings::showWidgetIcon && statIcon)
	{
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

	float barWidth = width - (iconAreaWidth + (Settings::showWidgetIcon ? interPadding : 0));

	float x_start = barX;
	for (size_t i = 0; i < numTeams; ++i)
	{
		float section_width = barWidth * fractions[i];
		float x_end = x_start + section_width;

		draw_list->AddRectFilled(
			ImVec2(x_start, y),
			ImVec2(x_end, y + height),
			colorsU32[i]
		);

		ImVec2 textSize = ImGui::CalcTextSize(texts[i]);
		float text_center_x = x_start + (section_width - textSize.x) * 0.5f + Settings::widgetTextHorizontalAlignOffset;
		float center_y = y + (height - textSize.y) * 0.5f + Settings::widgetTextVerticalAlignOffset;

		if (section_width >= textSize.x)
		{
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

void ratioBarSetup(HINSTANCE hSelf)
{
	if (!Settings::IsAddonWidgetEnabled)
		return;

	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize;
	if (Settings::disableMovingWindow)
	{
		window_flags |= ImGuiWindowFlags_NoMove;
	}
	if (Settings::disableClickingWindow)
	{
		window_flags |= ImGuiWindowFlags_NoInputs;
	}
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

	float sz = ImGui::GetFontSize();
	float padding = 5.0f;
	float minBarWidth = 50.0f;
	if (Settings::widgetWidth < sz + minBarWidth + padding)
	{
		Settings::widgetWidth = sz + minBarWidth + padding;
	}

	ImVec2 barSize = ImVec2(Settings::widgetWidth, Settings::widgetHeight);
	ImGui::SetNextWindowSize(barSize);
	if (ImGui::Begin("Team Ratio Bar", nullptr, window_flags))
	{
		if (parsedLogs.empty())
		{
			ImGui::Text(initialParsingComplete ? "No logs parsed yet." : "Parsing logs...");
			ImGui::End();
			ImGui::PopStyleVar(4);
			return;
		}

		const auto& currentLogData = parsedLogs[currentLogIndex].data;

		const std::vector<std::string> team_names = { "Red", "Blue", "Green" };
		const std::vector<ImVec4> team_colors = {
			ImGui::ColorConvertU32ToFloat4(IM_COL32(0xff, 0x44, 0x44, 0xff)),
			ImGui::ColorConvertU32ToFloat4(IM_COL32(0x33, 0xb5, 0xe5, 0xff)),
			ImGui::ColorConvertU32ToFloat4(IM_COL32(0x99, 0xcc, 0x00, 0xff))
		};

		struct TeamDisplayData
		{
			float count;
			ImVec4 color;
			std::string text;
		};

		std::vector<TeamDisplayData> teamsToDisplay;

		for (size_t i = 0; i < team_names.size(); ++i)
		{
			auto teamIt = currentLogData.teamStats.find(team_names[i]);
			if (teamIt != currentLogData.teamStats.end())
			{
				bool useSquadStats = Settings::squadPlayersOnly && teamIt->second.isPOVTeam;
				Settings::widgetStats = Settings::widgetStatsC;

				float teamCountValue = 0.0f;

				if (Settings::widgetStats == "players") {
					teamCountValue = static_cast<float>(useSquadStats ? teamIt->second.squadStats.totalPlayers : teamIt->second.totalPlayers);
				}
				else if (Settings::widgetStats == "deaths") {
					teamCountValue = static_cast<float>(useSquadStats ? teamIt->second.squadStats.totalDeaths : teamIt->second.totalDeaths);
				}
				else if (Settings::widgetStats == "downs") {
					teamCountValue = static_cast<float>(useSquadStats ? teamIt->second.squadStats.totalDowned : teamIt->second.totalDowned);
				}
				else if (Settings::widgetStats == "damage") {
					float totalDamageToDisplay;
					if (Settings::vsLoggedPlayersOnly) {
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
				else if (Settings::widgetStats == "kdr") {
					float kdRatioToDisplay = useSquadStats ? teamIt->second.squadStats.getKillDeathRatio() : teamIt->second.getKillDeathRatio();
					teamCountValue = kdRatioToDisplay;
				}

				char buf[64];
				if (Settings::widgetStats == "damage") {
					std::string formattedDamage = formatDamage(static_cast<uint64_t>(teamCountValue));
					snprintf(buf, sizeof(buf), "%s", formattedDamage.c_str());
				}
				else if (Settings::widgetStats == "kdr") {
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

		if (teamsToDisplay.empty())
		{
			ImGui::Text("No team data available.");
			ImGui::End();
			ImGui::PopStyleVar(4);
			return;
		}

		std::string statType = Settings::widgetStats;
		ImTextureID currentStatIcon = nullptr;

		if (Settings::showClassIcons)
		{
			if (statType == "players")
			{
				if (Squad && Squad->Resource)
				{
					currentStatIcon = Squad->Resource;
				}
				else
				{
					Squad = APIDefs->Textures.GetOrCreateFromResource("SQUAD_ICON", SQUAD, hSelf);
					currentStatIcon = Squad ? Squad->Resource : nullptr;
				}
			}
			else if (statType == "deaths")
			{
				if (Death && Death->Resource)
				{
					currentStatIcon = Death->Resource;
				}
				else
				{
					Death = APIDefs->Textures.GetOrCreateFromResource("DEATH_ICON", DEATH, hSelf);
					currentStatIcon = Death ? Death->Resource : nullptr;
				}
			}
			else if (statType == "downs")
			{
				if (Downed && Downed->Resource)
				{
					currentStatIcon = Downed->Resource;
				}
				else
				{
					Downed = APIDefs->Textures.GetOrCreateFromResource("DOWNED_ICON", DOWNED, hSelf);
					currentStatIcon = Downed ? Downed->Resource : nullptr;
				}
			}
			else if (statType == "damage")
			{
				if (Damage && Damage->Resource)
				{
					currentStatIcon = Damage->Resource;
				}
				else
				{
					Damage = APIDefs->Textures.GetOrCreateFromResource("DAMAGE_ICON", DAMAGE, hSelf);
					currentStatIcon = Damage ? Damage->Resource : nullptr;
				}
			}
			else if (statType == "kdr")
			{
				if (Kdr && Kdr->Resource)
				{
					currentStatIcon = Kdr->Resource;
				}
				else
				{
					Kdr = APIDefs->Textures.GetOrCreateFromResource("KDR_ICON", KDR, hSelf);
					currentStatIcon = Kdr ? Kdr->Resource : nullptr;
				}
			}
		}

		std::vector<float> counts;
		std::vector<ImVec4> colors;
		std::vector<const char*> texts;

		for (const auto& teamData : teamsToDisplay)
		{
			counts.push_back(teamData.count);
			colors.push_back(teamData.color);
			texts.push_back(teamData.text.c_str());
		}

		RenderSimpleRatioBar(
			counts,
			colors,
			ImVec2(barSize.x, barSize.y),
			texts,
			currentStatIcon
		);
	}
	ImGui::PopStyleVar(4);

	if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup | ImGuiHoveredFlags_ChildWindows) && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
	{
		ImGui::OpenPopup("Widget Menu");
	}
	if (ImGui::BeginPopup("Widget Menu"))
	{
		if (ImGui::BeginMenu("History"))
		{
			for (int i = 0; i < parsedLogs.size(); ++i)
			{
				const auto& log = parsedLogs[i];
				std::string fnstr = log.filename.substr(0, log.filename.find_last_of('.'));
				uint64_t durationMs = log.data.combatEndTime - log.data.combatStartTime;
				auto duration = std::chrono::milliseconds(durationMs);
				int minutes = std::chrono::duration_cast<std::chrono::minutes>(duration).count();
				int seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count() % 60;
				std::string displayName = fnstr + " (" + std::to_string(minutes) + "m " + std::to_string(seconds) + "s)";
				if (ImGui::RadioButton(displayName.c_str(), &currentLogIndex, i))
				{
				}
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Display Stats"))
		{
			Settings::widgetStats = Settings::widgetStatsC;
			bool isPlayers = Settings::widgetStats == "players";
			bool isDeaths = Settings::widgetStats == "deaths";
			bool isDowns = Settings::widgetStats == "downs";
			bool isDamage = Settings::widgetStats == "damage";
			bool isKDR = Settings::widgetStats == "kdr";

			if (ImGui::RadioButton("Players", isPlayers))
			{
				strcpy_s(Settings::widgetStatsC, sizeof(Settings::widgetStatsC), "players");
				Settings::widgetStats = "players";
				Settings::Settings[WIDGET_STATS] = Settings::widgetStatsC;
				Settings::Save(APIDefs->Paths.GetAddonDirectory("WvWFightAnalysis/settings.json"));
			}
			if (ImGui::RadioButton("K/D Ratio", isKDR))
			{
				strcpy_s(Settings::widgetStatsC, sizeof(Settings::widgetStatsC), "kdr");
				Settings::widgetStats = "kdr";
				Settings::Settings[WIDGET_STATS] = Settings::widgetStatsC;
				Settings::Save(APIDefs->Paths.GetAddonDirectory("WvWFightAnalysis/settings.json"));
			}
			if (ImGui::RadioButton("Deaths", isDeaths))
			{
				strcpy_s(Settings::widgetStatsC, sizeof(Settings::widgetStatsC), "deaths");
				Settings::widgetStats = "deaths";
				Settings::Settings[WIDGET_STATS] = Settings::widgetStatsC;
				Settings::Save(APIDefs->Paths.GetAddonDirectory("WvWFightAnalysis/settings.json"));
			}
			if (ImGui::RadioButton("Downs", isDowns))
			{
				strcpy_s(Settings::widgetStatsC, sizeof(Settings::widgetStatsC), "downs");
				Settings::widgetStats = "downs";
				Settings::Settings[WIDGET_STATS] = Settings::widgetStatsC;
				Settings::Save(APIDefs->Paths.GetAddonDirectory("WvWFightAnalysis/settings.json"));
			}
			if (ImGui::RadioButton("Damage", isDamage))
			{
				strcpy_s(Settings::widgetStatsC, sizeof(Settings::widgetStatsC), "damage");
				Settings::widgetStats = "damage";
				Settings::Settings[WIDGET_STATS] = Settings::widgetStatsC;
				Settings::Save(APIDefs->Paths.GetAddonDirectory("WvWFightAnalysis/settings.json"));
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Style"))
		{
			if (ImGui::Checkbox("Show Widget Icon", &Settings::showWidgetIcon))
			{
				Settings::Settings[SHOW_WIDGET_ICON] = Settings::showWidgetIcon;
				Settings::Save(APIDefs->Paths.GetAddonDirectory("WvWFightAnalysis/settings.json"));
			}
			ImGui::SetNextItemWidth(200.0f);
			if (ImGui::SliderFloat("##Widget Height", &Settings::widgetHeight, 0.0f, 900.0f))
			{
				Settings::widgetHeight = std::clamp(Settings::widgetHeight, 0.0f, 900.0f);
				Settings::Settings[WIDGET_HEIGHT] = Settings::widgetHeight;
				Settings::Save(APIDefs->Paths.GetAddonDirectory("WvWFightAnalysis/settings.json"));
			}
			ImGui::SameLine();
			ImGui::SetNextItemWidth(60.0f);
			if (ImGui::InputFloat("Widget Height", &Settings::widgetHeight, 1.0f))
			{
				Settings::widgetHeight = std::clamp(Settings::widgetHeight, 0.0f, 900.0f);
				Settings::Settings[WIDGET_HEIGHT] = Settings::widgetHeight;
				Settings::Save(APIDefs->Paths.GetAddonDirectory("WvWFightAnalysis/settings.json"));
			}

			ImGui::SetNextItemWidth(200.0f);
			if (ImGui::SliderFloat("##Widget Width", &Settings::widgetWidth, 0.0f, 900.0f))
			{
				Settings::widgetWidth = std::clamp(Settings::widgetWidth, 0.0f, 900.0f);
				Settings::Settings[WIDGET_WIDTH] = Settings::widgetWidth;
				Settings::Save(APIDefs->Paths.GetAddonDirectory("WvWFightAnalysis/settings.json"));
			}
			ImGui::SameLine();
			ImGui::SetNextItemWidth(60.0f);
			if (ImGui::InputFloat("Widget Width", &Settings::widgetWidth, 1.0f))
			{
				Settings::widgetWidth = std::clamp(Settings::widgetWidth, 0.0f, 900.0f);
				Settings::Settings[WIDGET_WIDTH] = Settings::widgetWidth;
				Settings::Save(APIDefs->Paths.GetAddonDirectory("WvWFightAnalysis/settings.json"));
			}

			ImGui::SetNextItemWidth(200.0f);
			if (ImGui::SliderFloat("##Widget Text Vertical Align", &Settings::widgetTextVerticalAlignOffset, -50.0f, 50.0f))
			{
				Settings::widgetTextVerticalAlignOffset = std::clamp(Settings::widgetTextVerticalAlignOffset, -50.0f, 50.0f);
				Settings::Settings[WIDGET_TEXT_VERTICAL_OFFSET] = Settings::widgetTextVerticalAlignOffset;
				Settings::Save(APIDefs->Paths.GetAddonDirectory("WvWFightAnalysis/settings.json"));
			}
			ImGui::SameLine();
			ImGui::SetNextItemWidth(60.0f);
			if (ImGui::InputFloat("Widget Text Vertical Align", &Settings::widgetTextVerticalAlignOffset, 1.0f))
			{
				Settings::widgetTextVerticalAlignOffset = std::clamp(Settings::widgetTextVerticalAlignOffset, -50.0f, 50.0f);
				Settings::Settings[WIDGET_TEXT_VERTICAL_OFFSET] = Settings::widgetTextVerticalAlignOffset;
				Settings::Save(APIDefs->Paths.GetAddonDirectory("WvWFightAnalysis/settings.json"));
			}

			ImGui::SetNextItemWidth(200.0f);
			if (ImGui::SliderFloat("##Widget Text Horizontal Align", &Settings::widgetTextHorizontalAlignOffset, -50.0f, 50.0f))
			{
				Settings::widgetTextHorizontalAlignOffset = std::clamp(Settings::widgetTextHorizontalAlignOffset, -50.0f, 50.0f);
				Settings::Settings[WIDGET_TEXT_HORIZONTAL_OFFSET] = Settings::widgetTextHorizontalAlignOffset;
				Settings::Save(APIDefs->Paths.GetAddonDirectory("WvWFightAnalysis/settings.json"));
			}
			ImGui::SameLine();
			ImGui::SetNextItemWidth(60.0f);
			if (ImGui::InputFloat("Widget Text Horizontal Align", &Settings::widgetTextHorizontalAlignOffset, 1.0f))
			{
				Settings::widgetTextHorizontalAlignOffset = std::clamp(Settings::widgetTextHorizontalAlignOffset, -50.0f, 50.0f);
				Settings::Settings[WIDGET_TEXT_HORIZONTAL_OFFSET] = Settings::widgetTextHorizontalAlignOffset;
				Settings::Save(APIDefs->Paths.GetAddonDirectory("WvWFightAnalysis/settings.json"));
			}
			ImGui::EndMenu();
		}
		if (ImGui::Checkbox("Damage vs Logged Players Only", &Settings::vsLoggedPlayersOnly))
		{
			Settings::Settings[VS_LOGGED_PLAYERS_ONLY] = Settings::vsLoggedPlayersOnly;
			Settings::Save(APIDefs->Paths.GetAddonDirectory("WvWFightAnalysis/settings.json"));
		}
		if (ImGui::Checkbox("Show Squad Players Only", &Settings::squadPlayersOnly))
		{
			Settings::Settings[SQUAD_PLAYERS_ONLY] = Settings::squadPlayersOnly;
			Settings::Save(APIDefs->Paths.GetAddonDirectory("WvWFightAnalysis/settings.json"));
		}

	}
	ImGui::End();
}

void DrawAggregateStatsWindow(HINSTANCE hSelf)
{
	if (!Settings::IsAddonAggWindowEnabled)
		return;

	bool hasData = false;
	{
		std::lock_guard<std::mutex> lock(aggregateStatsMutex);
		hasData = !globalAggregateStats.teamAggregates.empty();
	}

	if (!hasData && Settings::hideAggWhenEmpty)
		return;

	ImGui::SetNextWindowPos(ImVec2(750, 350), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(200, 300), ImGuiCond_FirstUseEver);
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoCollapse;

	if (!Settings::showScrollBar) window_flags |= ImGuiWindowFlags_NoScrollbar;
	if (!Settings::showWindowTitle) window_flags |= ImGuiWindowFlags_NoTitleBar;
	if (Settings::disableMovingWindow) window_flags |= ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
	if (Settings::disableClickingWindow) window_flags |= ImGuiWindowFlags_NoInputs;

	if (ImGui::Begin("Aggregate Stats", &Settings::IsAddonAggWindowEnabled, window_flags))
	{
		float sz = ImGui::GetFontSize();

		if (ImGui::Button("Reset"))
		{
			std::lock_guard<std::mutex> lock(aggregateStatsMutex);
			globalAggregateStats = GlobalAggregateStats();
			cachedAverages = CachedAverages();
		}

		{
			std::lock_guard<std::mutex> lock(aggregateStatsMutex);

			ImGui::Text("Avg Combat Time: %s", formatDuration(cachedAverages.averageCombatTime));
			ImGui::Text("Total Combat Time: %s", formatDuration(globalAggregateStats.totalCombatTime));

			ImGui::Separator();

			for (const auto& [teamName, teamAgg] : globalAggregateStats.teamAggregates)
			{
				ImGui::Spacing();

				ImVec4 teamColor = GetTeamColor(teamName);
				ImGui::PushStyleColor(ImGuiCol_Text, teamColor);
				ImGui::Text("%s", teamName.c_str());
				ImGui::PopStyleColor();

				bool useSquadStats = (Settings::squadPlayersOnly && teamAgg.isPOVTeam);
				const SquadAggregateStats& displayStats = useSquadStats
					? teamAgg.povSquadTotals
					: teamAgg.teamTotals;

				if (Settings::showTeamTotalPlayers) {
					double avgPlayerCount = 0.0;
					if (useSquadStats) {
						auto it = cachedAverages.averagePOVSquadPlayerCounts.find(teamName);
						if (it != cachedAverages.averagePOVSquadPlayerCounts.end())
							avgPlayerCount = it->second;
					}
					else
					{
						auto it = cachedAverages.averageTeamPlayerCounts.find(teamName);
						if (it != cachedAverages.averageTeamPlayerCounts.end())
							avgPlayerCount = it->second;
					}
					if (Settings::showClassIcons) {
						if (Squad && Squad->Resource)
						{
							ImGui::Image(Squad->Resource, ImVec2(sz, sz));
							ImGui::SameLine(0, 5);
						}
						else
						{
							Squad = APIDefs->Textures.GetOrCreateFromResource("SQUAD_ICON", SQUAD, hSelf);
						}
					}
					if (Settings::showClassNames)
					{
						ImGui::Text("Avg Players: %d", (int)std::round(avgPlayerCount));

					}
					else
					{
						ImGui::Text("%d", (int)std::round(avgPlayerCount));

					}
				}

				if (Settings::showTeamDeaths)
				{
					if (Settings::showClassIcons)
					{
						if (Death && Death->Resource)
						{
							ImGui::Image(Death->Resource, ImVec2(sz, sz));
							ImGui::SameLine(0, 5);
						}
						else
						{
							Death = APIDefs->Textures.GetOrCreateFromResource("DEATH_ICON", DEATH, hSelf);
						}
					}
					if (Settings::showClassNames)
					{
						ImGui::Text("Deaths: %d", displayStats.totalDeaths);
					}
					else
					{
						ImGui::Text("%d", displayStats.totalDeaths);
					}
				}

				if (Settings::showTeamDowned)
				{
					if (Settings::showClassIcons)
					{
						if (Downed && Downed->Resource)
						{
							ImGui::Image(Downed->Resource, ImVec2(sz, sz));
							ImGui::SameLine(0, 5);
						}
						else
						{
							Downed = APIDefs->Textures.GetOrCreateFromResource("DOWNED_ICON", DOWNED, hSelf);
						}
					}
					if (Settings::showClassNames)
					{
						ImGui::Text("Downs:  %d", displayStats.totalDowned);
					}
					else
					{
						ImGui::Text("%d", displayStats.totalDowned);
					}
				}

				// Show spec averages
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

				ImGui::Separator();
			}
		}
	}
	ImGui::End();
}

void SortSpecMenu() {
	Settings::windowSort = Settings::windowSortC;

	struct SortOption {
		const char* label;
		const char* value;
	};

	const SortOption sortOptions[] = {
		{"Players", "players"},
		{"Damage", "damage"},
		{"Down Cont", "down cont"},
		{"Kill Cont", "kill cont"},
		{"Deaths", "deaths"},
		{"Downs", "downs"}
	};

	for (const auto& option : sortOptions) {
		bool isSelected = Settings::windowSort == option.value;
		if (ImGui::RadioButton(option.label, isSelected)) {
			strcpy_s(Settings::windowSortC, sizeof(Settings::windowSortC), option.value);
			Settings::windowSort = option.value;
			Settings::Settings[WINDOW_SORT] = Settings::windowSortC;
			Settings::Save(SettingsPath);
		}
	}
}

void BarRepMenu() {
	if (Settings::barRepIndependent) {
		if (ImGui::BeginMenu("Bar Display")) {
			Settings::barRepresentation = Settings::barRepresentationC;
			struct DisplayOption {
				const char* label;
				const char* value;
			};

			const DisplayOption displayOptions[] = {
				{"Players", "players"},
				{"Damage", "damage"},
				{"Down Cont", "down cont"},
				{"Kill Cont", "kill cont"},
				{"Deaths", "deaths"},
				{"Downs", "downs"}
			};

			for (const auto& option : displayOptions) {
				bool isSelected = Settings::barRepresentation == option.value;
				if (ImGui::RadioButton(option.label, isSelected)) {
					strcpy_s(Settings::barRepresentationC, sizeof(Settings::barRepresentationC), option.value);
					Settings::barRepresentation = option.value;
					Settings::Settings[BAR_REPRESENTATION] = Settings::barRepresentationC;
					Settings::Save(SettingsPath);
				}
			}

			if (ImGui::InputText("Bar Template", Settings::barTemplateC, sizeof(Settings::barTemplateC))) {
				Settings::barTemplate = Settings::barTemplateC;
				Settings::Settings[BAR_TEMPLATE] = Settings::barTemplate;
				Settings::Save(SettingsPath);
				BarTemplateRenderer::ParseTemplate(Settings::barTemplate);
			}

			ImGui::SameLine();
			if (ImGui::Button("Reset")) {
				const char* defaultTemplate = "@1 @2 @3 (@4)";
				strcpy_s(Settings::barTemplateC, sizeof(Settings::barTemplateC), defaultTemplate);
				Settings::barTemplate = Settings::barTemplateC;
				Settings::Settings[BAR_TEMPLATE] = Settings::barTemplate;
				Settings::Save(SettingsPath);
				BarTemplateRenderer::ParseTemplate(Settings::barTemplate);
			}

			ImGui::Text("Variables:");
			ImGui::Text("  @1 = Player Count");
			ImGui::Text("  @2 = Class Icon");
			ImGui::Text("  @3 = Class Name");
			ImGui::Text("  @4 = Primary Stat");
			ImGui::Text("  @5 = Secondary Stat");

			ImGui::EndMenu();
		}
	}
}


void RenderSpecializationsSettingsPopup()
{
	if (ImGui::BeginPopup("Specializations Settings"))
	{
		if (ImGui::BeginMenu("Display"))
		{
			if (ImGui::Checkbox("Show Class Window", &Settings::showSpecBars))
			{
				Settings::Settings[SHOW_SPEC_BARS] = Settings::showSpecBars;
				Settings::Save(SettingsPath);
			}
			if (ImGui::Checkbox("Short Class Names", &Settings::useShortClassNames))
			{
				Settings::Settings[USE_SHORT_CLASS_NAMES] = Settings::useShortClassNames;
				Settings::Save(SettingsPath);
			}
			if (ImGui::Checkbox("Class Names", &Settings::showClassNames))
			{
				Settings::Settings[SHOW_CLASS_NAMES] = Settings::showClassNames;
				Settings::Save(SettingsPath);
			}
			if (ImGui::Checkbox("Class Icons", &Settings::showClassIcons))
			{
				Settings::Settings[SHOW_CLASS_ICONS] = Settings::showClassIcons;
				Settings::Save(SettingsPath);
			}
			if (ImGui::Checkbox("Class Outgoing Damage", &Settings::showSpecDamage))
			{
				Settings::Settings[SHOW_SPEC_DAMAGE] = Settings::showSpecDamage;
				Settings::Save(SettingsPath);
			}
			if (ImGui::Checkbox("Class Down Cont", &Settings::showSpecDamage))
			{
				Settings::Settings[SHOW_SPEC_DAMAGE] = Settings::showSpecDamage;
				Settings::Save(SettingsPath);
			}
			if (ImGui::Checkbox("Class Kill Cont", &Settings::showSpecDamage))
			{
				Settings::Settings[SHOW_SPEC_DAMAGE] = Settings::showSpecDamage;
				Settings::Save(SettingsPath);
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Style"))
		{
			if (ImGui::Checkbox("Use Tabbed View", &Settings::useTabbedView))
			{
				Settings::Settings[USE_TABBED_VIEW] = Settings::useTabbedView;
				Settings::Save(SettingsPath);
			}
			if (ImGui::Checkbox("Split Stats Window", &Settings::splitStatsWindow))
			{
				Settings::Settings[SPLIT_STATS_WINDOW] = Settings::splitStatsWindow;
				Settings::Save(SettingsPath);
			}
			if (ImGui::Checkbox("Show Scroll Bar", &Settings::showScrollBar))
			{
				Settings::Settings[SHOW_SCROLL_BAR] = Settings::showScrollBar;
				Settings::Save(SettingsPath);
			}
			if (ImGui::Checkbox("Show Title", &Settings::showWindowTitle))
			{
				Settings::Settings[SHOW_WINDOW_TITLE] = Settings::showWindowTitle;
				Settings::Save(SettingsPath);
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Sort"))
		{
			SortSpecMenu();
			ImGui::EndMenu();
		}
		BarRepMenu();
		if (ImGui::Checkbox("Bars Independent of Sort", &Settings::barRepIndependent)) {
			Settings::Settings[BAR_REP_INDEPENDENT] = Settings::barRepIndependent;
			Settings::Save(SettingsPath);
		}
		
		if (ImGui::Checkbox("Class Damage", &Settings::sortSpecDamage))
		{
			Settings::Settings[SORT_SPEC_DAMAGE] = Settings::sortSpecDamage;
			Settings::Save(SettingsPath);
		}

		if (ImGui::Checkbox("Damage vs Logged Players Only", &Settings::vsLoggedPlayersOnly))
		{
			Settings::Settings[VS_LOGGED_PLAYERS_ONLY] = Settings::vsLoggedPlayersOnly;
			Settings::Save(SettingsPath);
		}
		if (ImGui::Checkbox("Show Squad Players Only", &Settings::squadPlayersOnly))
		{
			Settings::Settings[SQUAD_PLAYERS_ONLY] = Settings::squadPlayersOnly;
			Settings::Save(SettingsPath);
		}

		ImGui::EndPopup();
	}
}

void RenderTeamStatsDisplayOptions()
{
	if (Settings::splitStatsWindow) {
		if (ImGui::Checkbox("Show Log Name", &Settings::showLogName))
		{
			Settings::Settings[SHOW_LOG_NAME] = Settings::showLogName;
			Settings::Save(SettingsPath);
		}
	}
	if (ImGui::Checkbox("Team Player Count", &Settings::showTeamTotalPlayers))
	{
		Settings::Settings[SHOW_TEAM_TOTAL_PLAYERS] = Settings::showTeamTotalPlayers;
		Settings::Save(SettingsPath);
	}
	if (ImGui::Checkbox("Team K/D Ratio", &Settings::showTeamKDR))
	{
		Settings::Settings[SHOW_TEAM_KDR] = Settings::showTeamKDR;
		Settings::Save(SettingsPath);
	}
	if (ImGui::Checkbox("Team Incoming Deaths", &Settings::showTeamDeaths))
	{
		Settings::Settings[SHOW_TEAM_DEATHS] = Settings::showTeamDeaths;
		Settings::Save(SettingsPath);
	}
	if (ImGui::Checkbox("Team Incoming Downs", &Settings::showTeamDowned))
	{
		Settings::Settings[SHOW_TEAM_DOWNED] = Settings::showTeamDowned;
		Settings::Save(SettingsPath);
	}
	if (ImGui::Checkbox("Team Outgoing Damage", &Settings::showTeamDamage))
	{
		Settings::Settings[SHOW_TEAM_DAMAGE] = Settings::showTeamDamage;
		Settings::Save(SettingsPath);
	}
	if (ImGui::Checkbox("Team Outgoing Down Cont", &Settings::showTeamDownCont))
	{
		Settings::Settings[SHOW_TEAM_DOWN_CONT] = Settings::showTeamDownCont;
		Settings::Save(SettingsPath);
	}
	if (ImGui::Checkbox("Team Outgoing Kill Cont", &Settings::showTeamKillCont))
	{
		Settings::Settings[SHOW_TEAM_KILL_CONT] = Settings::showTeamKillCont;
		Settings::Save(SettingsPath);
	}
	if (ImGui::Checkbox("Team Outgoing Strike Damage", &Settings::showTeamStrikeDamage))
	{
		Settings::Settings[SHOW_TEAM_STRIKE] = Settings::showTeamStrikeDamage;
		Settings::Save(SettingsPath);
	}
	if (ImGui::Checkbox("Team Outgoing Condi Damage", &Settings::showTeamCondiDamage))
	{
		Settings::Settings[SHOW_TEAM_CONDI] = Settings::showTeamCondiDamage;
		Settings::Save(SettingsPath);
	}
}

void RenderLogSelectionPopup(const std::deque<ParsedLog>& parsedLogs, int& currentLogIndex)
{
	if (ImGui::BeginPopup("Log Selection"))
	{
		if (ImGui::BeginMenu("History"))
		{
			for (int i = 0; i < parsedLogs.size(); ++i)
			{
				const auto& log = parsedLogs[i];
				std::string fnstr = log.filename.substr(0, log.filename.find_last_of('.'));
				uint64_t durationMs = log.data.combatEndTime - log.data.combatStartTime;
				auto duration = std::chrono::milliseconds(durationMs);
				int minutes = std::chrono::duration_cast<std::chrono::minutes>(duration).count();
				int seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count() % 60;
				std::string displayName = fnstr + " (" + std::to_string(minutes) + "m " + std::to_string(seconds) + "s)";

				if (ImGui::RadioButton(displayName.c_str(), &currentLogIndex, i))
				{
					// Selection handled by RadioButton
				}
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Display"))
		{
			if (!Settings::splitStatsWindow) {
				if (ImGui::Checkbox("Log Name", &Settings::showLogName))
				{
					Settings::Settings[SHOW_LOG_NAME] = Settings::showLogName;
					Settings::Save(SettingsPath);
				}
				if (ImGui::Checkbox("Short Names", &Settings::useShortClassNames))
				{
					Settings::Settings[USE_SHORT_CLASS_NAMES] = Settings::useShortClassNames;
					Settings::Save(SettingsPath);
				}
				if (ImGui::Checkbox("Class Bars", &Settings::showSpecBars))
				{
					Settings::Settings[SHOW_SPEC_BARS] = Settings::showSpecBars;
					Settings::Save(SettingsPath);
				}
				if (ImGui::Checkbox("Class Names", &Settings::showClassNames))
				{
					Settings::Settings[SHOW_CLASS_NAMES] = Settings::showClassNames;
					Settings::Save(SettingsPath);
				}
				if (ImGui::Checkbox("Class Icons", &Settings::showClassIcons))
				{
					Settings::Settings[SHOW_CLASS_ICONS] = Settings::showClassIcons;
					Settings::Save(SettingsPath);
				}
				if (ImGui::Checkbox("Class Outgoing Damage", &Settings::showSpecDamage))
				{
					Settings::Settings[SHOW_SPEC_DAMAGE] = Settings::showSpecDamage;
					Settings::Save(SettingsPath);
				}
				if (ImGui::Checkbox("Class Down Cont", &Settings::showSpecDamage))
				{
					Settings::Settings[SHOW_SPEC_DAMAGE] = Settings::showSpecDamage;
					Settings::Save(SettingsPath);
				}
				if (ImGui::Checkbox("Class Kill Cont", &Settings::showSpecDamage))
				{
					Settings::Settings[SHOW_SPEC_DAMAGE] = Settings::showSpecDamage;
					Settings::Save(SettingsPath);
				}
				ImGui::Separator();
			}
			else {
				if (ImGui::Checkbox("Split Class Window", &Settings::showSpecBars))
				{
					Settings::Settings[SHOW_SPEC_BARS] = Settings::showSpecBars;
					Settings::Save(SettingsPath);
				}
			}

			// Team statistics display options
			RenderTeamStatsDisplayOptions();

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Style"))
		{
			if (ImGui::Checkbox("Use Tabbed View", &Settings::useTabbedView))
			{
				Settings::Settings[USE_TABBED_VIEW] = Settings::useTabbedView;
				Settings::Save(SettingsPath);
			}
			if (ImGui::Checkbox("Split Stats Window", &Settings::splitStatsWindow))
			{
				Settings::Settings[SPLIT_STATS_WINDOW] = Settings::splitStatsWindow;
				Settings::Save(SettingsPath);
			}
			if (ImGui::Checkbox("Show Scroll Bar", &Settings::showScrollBar))
			{
				Settings::Settings[SHOW_SCROLL_BAR] = Settings::showScrollBar;
				Settings::Save(SettingsPath);
			}
			if (ImGui::Checkbox("Show Title", &Settings::showWindowTitle))
			{
				Settings::Settings[SHOW_WINDOW_TITLE] = Settings::showWindowTitle;
				Settings::Save(SettingsPath);
			}
			if (ImGui::Checkbox("Show Background", &Settings::showWindowBackground))
			{
				Settings::Settings[SHOW_WINDOW_BACKGROUND] = Settings::showWindowBackground;
				Settings::Save(SettingsPath);
			}
			if (ImGui::Checkbox("Allow Focus", &Settings::allowWindowFocus))
			{
				Settings::Settings[ALLOW_WINDOW_FOCUS] = Settings::allowWindowFocus;
				Settings::Save(SettingsPath);
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Sort"))
		{
			SortSpecMenu();
			ImGui::EndMenu();
		}
		BarRepMenu();
		if (ImGui::Checkbox("Bars Independent of Sort", &Settings::barRepIndependent)) {
			Settings::Settings[BAR_REP_INDEPENDENT] = Settings::barRepIndependent;
			Settings::Save(SettingsPath);
		}
		
		if (ImGui::Checkbox("Damage vs Logged Players Only", &Settings::vsLoggedPlayersOnly))
		{
			Settings::Settings[VS_LOGGED_PLAYERS_ONLY] = Settings::vsLoggedPlayersOnly;
			Settings::Save(SettingsPath);
		}
		if (ImGui::Checkbox("Show Squad Players Only", &Settings::squadPlayersOnly))
		{
			Settings::Settings[SQUAD_PLAYERS_ONLY] = Settings::squadPlayersOnly;
			Settings::Save(SettingsPath);
		}

		ImGui::EndPopup();
	}
}

void RenderTeamHeader(const std::string& teamName, const TeamStats& teamData, const ImVec4& teamColor, HINSTANCE hSelf) {
	ImGui::PushStyleColor(ImGuiCol_Text, teamColor);

	if (teamData.isPOVTeam) {
		if (Home && Home->Resource) {
			float sz = ImGui::GetFontSize() - 1;
			ImGui::Image(Home->Resource, ImVec2(sz, sz));
			ImGui::SameLine(0, 5);
		}
		else {
			Home = APIDefs->Textures.GetOrCreateFromResource("HOME_ICON", HOME, hSelf);
		}
	}

	ImGui::Text("%s Team", teamName.c_str());
	ImGui::PopStyleColor();
}

struct TeamRenderInfo {
	bool hasData;
	const TeamStats* stats;
	std::string name;
	ImVec4 color;
};

void RenderMainWindow(HINSTANCE hSelf)
{
	if (!Settings::IsAddonWindowEnabled)
	{
		if (Settings::Settings[IS_ADDON_WINDOW_VISIBLE] != Settings::IsAddonWindowEnabled)
		{
			Settings::Settings[IS_ADDON_WINDOW_VISIBLE] = Settings::IsAddonWindowEnabled;
			Settings::Save(SettingsPath);
		}
		return;
	}

	if (!Settings::splitStatsWindow)
	{
		if (Settings::Settings[SPLIT_STATS_WINDOW] != Settings::splitStatsWindow)
		{
			Settings::Settings[SPLIT_STATS_WINDOW] = Settings::splitStatsWindow;
			Settings::Save(SettingsPath);
		}
	}

	ImGui::SetNextWindowPos(ImVec2(1030, 300), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(300, 500), ImGuiCond_FirstUseEver);
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoCollapse;

	if (!Settings::showScrollBar) window_flags |= ImGuiWindowFlags_NoScrollbar;
	if (!Settings::showWindowTitle) window_flags |= ImGuiWindowFlags_NoTitleBar;
	if (!Settings::allowWindowFocus) window_flags |= ImGuiWindowFlags_NoFocusOnAppearing;
	if (!Settings::allowWindowFocus) window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
	if (!Settings::showWindowBackground) window_flags |= ImGuiWindowFlags_NoBackground;
	if (Settings::disableMovingWindow) window_flags |= ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
	if (Settings::disableClickingWindow) window_flags |= ImGuiWindowFlags_NoInputs;

	if (ImGui::Begin("WvW Fight Analysis", &Settings::IsAddonWindowEnabled, window_flags))
	{
		ImGuiStyle& style = ImGui::GetStyle();
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(style.FramePadding.x, 5));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(style.ItemSpacing.x, 10));

		if (parsedLogs.empty())
		{
			ImGui::Text(initialParsingComplete ? "No logs parsed yet." : "Parsing logs...");
			ImGui::PopStyleVar(2);
			ImGui::End();
			return;
		}

		const auto& currentLog = parsedLogs[currentLogIndex];
		const auto& currentLogData = currentLog.data;

		if (Settings::showLogName) {
			
			std::string displayName = generateLogDisplayName(currentLog.filename, currentLog.data.combatStartTime, currentLog.data.combatEndTime);
			ImGui::Text("%s", displayName.c_str());
		}

		std::array<TeamRenderInfo, 3> teams;
		int teamsWithData = 0;

		const char* team_names[] = { "Red", "Blue", "Green" };
		ImVec4 team_colors[] = {
			ImGui::ColorConvertU32ToFloat4(IM_COL32(0xFF, 0x44, 0x44, 0xFF)),
			ImGui::ColorConvertU32ToFloat4(IM_COL32(0x33, 0xB5, 0xE5, 0xFF)),
			ImGui::ColorConvertU32ToFloat4(IM_COL32(0x99, 0xCC, 0x00, 0xFF))
		};

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

		if (teamsWithData == 0)
		{
			ImGui::Text("No team data available meeting the player threshold.");
		}
		else
		{
			if (Settings::useTabbedView)
			{
				if (ImGui::BeginTabBar("TeamTabBar", ImGuiTabBarFlags_None))
				{
					for (int i = 0; i < 3; ++i) {
						if (teams[i].hasData) {
							ImGui::PushStyleColor(ImGuiCol_Text, teams[i].color);
							std::string tabName = teams[i].stats->isPOVTeam ?
								"* " + teams[i].name : teams[i].name;

							if (ImGui::BeginTabItem(tabName.c_str())) {
								ImGui::PopStyleColor();
								RenderTeamData(i, *teams[i].stats, hSelf);
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
			else
			{
				if (ImGui::BeginTable("TeamTable", teamsWithData, ImGuiTableFlags_BordersInner))
				{
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
							RenderTeamHeader(teams[i].name, *teams[i].stats, teams[i].color, hSelf);
						}
					}
					ImGui::TableNextRow();
					columnIndex = 0;
					for (int i = 0; i < 3; ++i) {
						if (teams[i].hasData) {
							ImGui::TableSetColumnIndex(columnIndex++);
							RenderTeamData(i, *teams[i].stats, hSelf);
						}
					}

					ImGui::EndTable();
				}
			}
		}

		// Render specialization bars in separate window
		if (Settings::showSpecBars && Settings::splitStatsWindow)
		{
			window_flags |= ImGuiWindowFlags_NoFocusOnAppearing;
			ImGui::SetNextWindowSize(ImVec2(300, 500), ImGuiCond_FirstUseEver);
			ImVec2 mainWindowPos = ImGui::GetWindowPos();
			ImVec2 mainWindowSize = ImGui::GetWindowSize();
			ImGui::SetNextWindowPos(ImVec2(mainWindowPos.x + mainWindowSize.x + 10, mainWindowPos.y), ImGuiCond_FirstUseEver);

			if (ImGui::Begin("Specializations", &Settings::splitStatsWindow, window_flags))
			{
				if (Settings::useTabbedView)
				{
					if (ImGui::BeginTabBar("SpecTabBar", ImGuiTabBarFlags_None))
					{
						for (int i = 0; i < 3; ++i) {
							if (teams[i].hasData) {
								ImGui::PushStyleColor(ImGuiCol_Text, teams[i].color);
								std::string tabName = teams[i].stats->isPOVTeam ?
									"* " + teams[i].name : teams[i].name;

								if (ImGui::BeginTabItem(tabName.c_str())) {
									ImGui::PopStyleColor();
									RenderSpecializationBars(*teams[i].stats, i, hSelf);
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
				else
				{
					if (ImGui::BeginTable("SpecTable", teamsWithData, ImGuiTableFlags_BordersInner))
					{
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
								RenderTeamHeader(teams[i].name, *teams[i].stats, teams[i].color, hSelf);
							}
						}

						ImGui::TableNextRow();
						columnIndex = 0;
						for (int i = 0; i < 3; ++i) {
							if (teams[i].hasData) {
								ImGui::TableSetColumnIndex(columnIndex++);
								RenderSpecializationBars(*teams[i].stats, i, hSelf);
							}
						}

						ImGui::EndTable();
					}
				}

				if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup | ImGuiHoveredFlags_ChildWindows) &&
					ImGui::IsMouseClicked(ImGuiMouseButton_Right))
				{
					ImGui::OpenPopup("Specializations Settings");
				}

				RenderSpecializationsSettingsPopup();
			}
			ImGui::End();
		}

		ImGui::PopStyleVar(2);

		if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup | ImGuiHoveredFlags_ChildWindows) &&
			ImGui::IsMouseClicked(ImGuiMouseButton_Right))
		{
			ImGui::OpenPopup("Log Selection");
		}

		RenderLogSelectionPopup(parsedLogs, currentLogIndex);

		ImGui::End();
	}
}