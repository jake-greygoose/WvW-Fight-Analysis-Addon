#define NOMINMAX
#include "gui/windows/MainWindow.h"
#include "resource.h"
#include "thirdparty/imgui_positioning/imgui_positioning.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <unordered_map>
#include <string>


struct SpecCacheKey {
    std::string logFilename;
    std::string teamName;
    bool        useSquadStats;
    std::string windowSort;
    bool        vsLoggedPlayersOnly;
    bool        barRepIndependent;
    std::string barRepresentation;

    bool operator==(const SpecCacheKey& other) const {
        return (logFilename == other.logFilename &&
            teamName == other.teamName &&
            useSquadStats == other.useSquadStats &&
            windowSort == other.windowSort &&
            vsLoggedPlayersOnly == other.vsLoggedPlayersOnly &&
            barRepIndependent == other.barRepIndependent &&
            barRepresentation == other.barRepresentation);
    }
};

struct SpecCacheKeyHasher {
    static void hashCombine(std::size_t& seed, std::size_t value)
    {
        // A standard way to combine hashes (similar to boost::hash_combine)
        seed ^= value + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
    }

    std::size_t operator()(const SpecCacheKey& key) const {
        std::size_t h = 0;
        auto strHash = std::hash<std::string>{};
        auto boolHash = std::hash<bool>{};

        // Combine each field in the key
        hashCombine(h, strHash(key.logFilename));
        hashCombine(h, strHash(key.teamName));
        hashCombine(h, boolHash(key.useSquadStats));
        hashCombine(h, strHash(key.windowSort));
        hashCombine(h, boolHash(key.vsLoggedPlayersOnly));
        hashCombine(h, boolHash(key.barRepIndependent));
        hashCombine(h, strHash(key.barRepresentation));

        return h;
    }
};

static std::unordered_map<
    SpecCacheKey,
    std::vector<std::pair<std::string, SpecStats>>,
    SpecCacheKeyHasher
> s_specRenderCache;


namespace wvwfightanalysis::gui {

    void MainWindow::Render(HINSTANCE hSelf, MainWindowSettings* settings) {

        static bool wasEnabled = settings->isEnabled;

        if (!settings->isEnabled)
            return;
        std::string windowName = settings->getDisplayName("WvW Fight Analysis");
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoCollapse;
        if (!settings->showScrollBar)
            window_flags |= ImGuiWindowFlags_NoScrollbar;
        if (!settings->showTitle)
            window_flags |= ImGuiWindowFlags_NoTitleBar;
        if (!settings->allowFocus) {
            window_flags |= ImGuiWindowFlags_NoFocusOnAppearing;
            window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
        }
        if (!settings->showBackground)
            window_flags |= ImGuiWindowFlags_NoBackground;
        if (settings->disableMoving)
            window_flags |= ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
        if (settings->disableClicking)
            window_flags |= ImGuiWindowFlags_NoInputs;

        // Update positioning from imgui_positioning library
        // Note: Don't call SetNextWindowPos/Size as it conflicts with imgui_positioning
        window_flags |= ImGuiExt::UpdatePosition(windowName);

        // Only set default size on first use (position is handled by imgui_positioning)
        ImGui::SetNextWindowSize(settings->size, ImGuiCond_FirstUseEver);

        bool pushedTitleStyle = false;
        if (settings->useWindowStyleForTitle) {
            ImGuiStyle& style = ImGui::GetStyle();
            ImVec4 titleColor = settings->showBackground
                ? style.Colors[ImGuiCol_WindowBg]
                : ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
            ImGui::PushStyleColor(ImGuiCol_TitleBg, titleColor);
            ImGui::PushStyleColor(ImGuiCol_TitleBgActive, titleColor);
            ImGui::PushStyleColor(ImGuiCol_TitleBgCollapsed, titleColor);
            pushedTitleStyle = true;
        }

        bool windowOpen = ImGui::Begin(windowName.c_str(), &settings->isEnabled, window_flags);

        // Check for window closing conditions
        if ((!windowOpen && settings->isEnabled) || (wasEnabled && !settings->isEnabled)) {
            Settings::Save(SettingsPath);
        }
        wasEnabled = settings->isEnabled;

        if (!windowOpen) {
            ImGui::End();
            if (pushedTitleStyle)
                ImGui::PopStyleColor(3);
            return;
        }

        if (parsedLogs.empty()) {
            ImGui::Text(initialParsingComplete ? "No logs parsed yet." : "Parsing logs...");
            ImGui::End();
            if (pushedTitleStyle)
                ImGui::PopStyleColor(3);
            return;
        }

        // --- Render window content ---
        // (The rest of your rendering code remains unchanged.)
        // The current log and its data:
        const auto& currentLog = parsedLogs[currentLogIndex];
        const auto& currentLogData = currentLog.data;

        // Optionally display the log name.
        if (settings->showLogName) {
            std::string displayName = generateLogDisplayName(
                currentLog.filename,
                currentLog.data.combatStartTime,
                currentLog.data.combatEndTime
            );
            ImGui::Text("%s", displayName.c_str());
        }

        // Setup team names and colors.
        const char* team_names[] = { "Red", "Blue", "Green" };
        const ImVec4 team_colors[] = {
            ImGui::ColorConvertU32ToFloat4(IM_COL32(0xFF, 0x44, 0x44, 0xFF)),
            ImGui::ColorConvertU32ToFloat4(IM_COL32(0x33, 0xB5, 0xE5, 0xFF)),
            ImGui::ColorConvertU32ToFloat4(IM_COL32(0x99, 0xCC, 0x00, 0xFF))
        };

        std::array<TeamRenderInfo, 3> teams;
        int teamsWithData = 0;
        // Populate the teams array.
        for (int i = 0; i < 3; ++i) {
            
            if ((i == 0 && settings->excludeRedTeam) ||
                (i == 1 && settings->excludeBlueTeam) ||
                (i == 2 && settings->excludeGreenTeam)) {
                teams[i].hasData = false;
                continue;
            }
            
            auto teamIt = currentLogData.teamStats.find(team_names[i]);
            teams[i].hasData = (teamIt != currentLogData.teamStats.end() &&
                teamIt->second.totalPlayers >= Settings::teamPlayerThreshold);
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
                        std::string tabName = teams[i].stats->isPOVTeam
                            ? "* " + teams[i].name
                            : teams[i].name;
                        if (ImGui::BeginTabItem(tabName.c_str())) {
                            ImGui::PopStyleColor();
                            // Render team-specific data (team name is passed in).
                            RenderTeamData(*teams[i].stats, teams[i].name, settings, hSelf);
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
            // Render teams in a table view.
            
            
            if (settings->overideTableBackgroundStyle && settings->showBackground) {
                ImGui::PushStyleColor(ImGuiCol_TableHeaderBg, IM_COL32(48, 48, 51, 255));     // #303033FF
                ImGui::PushStyleColor(ImGuiCol_TableBorderStrong, IM_COL32(79, 79, 89, 255)); // #4F4F59FF
                ImGui::PushStyleColor(ImGuiCol_TableBorderLight, IM_COL32(59, 59, 64, 255));  // #3B3B40FF;
            }

            if (settings->overideTableBackgroundStyle && !settings->showBackground) {
                ImGui::PushStyleColor(ImGuiCol_TableHeaderBg, IM_COL32(48, 48, 51, 63));     // #303033FF
                ImGui::PushStyleColor(ImGuiCol_TableBorderStrong, IM_COL32(79, 79, 89, 63)); // #4F4F59FF
                ImGui::PushStyleColor(ImGuiCol_TableBorderLight, IM_COL32(59, 59, 64, 63));  // #3B3B40FF;
            }

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

                        
                        if (teams[i].stats->isPOVTeam && (!Home || !Home->Resource)) {
                            Home = APIDefs->Textures.GetOrCreateFromResource("HOME_ICON", HOME, hSelf);
                        }
                        if (teams[i].stats->isPOVTeam && Home && Home->Resource) {
                            float sz = ImGui::GetFontSize() - 1;
                            ImGui::Image(Home->Resource, ImVec2(sz, sz));
                            // Adjust spacing after the image if desired:
                            ImGui::SameLine(0, 5); // Use a smaller spacing than before
                        }
                        else {
                            // Even when there's no image, add a little padding:
                            ImVec2 pos = ImGui::GetCursorPos();
                            ImGui::SetCursorPos(ImVec2(pos.x + 5.0f, pos.y));
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
                        RenderTeamData(*teams[i].stats, teams[i].name, settings, hSelf);
                    }
                }
                ImGui::EndTable();
            }
            
            if (settings->overideTableBackgroundStyle) {
                ImGui::PopStyleColor(3);
            }
            

        }

        // Render the settings popup with positioning integration
        RenderMainWindowSettingsPopup(settings);

        // Append positioning menu to the same context menu
        ImGuiExt::ContextMenuPosition("MainWindowContextMenu");

        // Update window position & size for persistent settings.
        settings->position = ImGui::GetWindowPos();
        settings->size = ImGui::GetWindowSize();

        // End this window.
        ImGui::End();

        // --- Pop the title style colors so they don't affect other windows ---
        if (pushedTitleStyle)
            ImGui::PopStyleColor(3);
    }

    void MainWindow::RenderTeamData(
        const TeamStats& teamData,
        const std::string& teamName,      // NEW PARAM
        const MainWindowSettings* settings,
        HINSTANCE hSelf)
    {
        //
        // 1) Figure out the template to use for the current sort
        //
        auto templateIt = settings->sortTemplates.find(settings->windowSort);
        std::string currentTemplate = (templateIt != settings->sortTemplates.end())
            ? templateIt->second
            : "";

        std::string template_to_use = BarTemplateRenderer::GetTemplateForSort(
            settings->windowSort,
            currentTemplate
        );

        //
        // 2) Only parse if the template changed from last time
        //
        static std::string s_lastParsedTemplate;
        if (template_to_use != s_lastParsedTemplate) {
            BarTemplateRenderer::ParseTemplate(template_to_use);
            s_lastParsedTemplate = template_to_use;
        }

        //
        // 3) Existing code that draws team-level stats
        //
        ImGui::Spacing();
        const bool useSquadStats = settings->squadPlayersOnly && teamData.isPOVTeam;
        const float sz = ImGui::GetFontSize();

        // --- 1) TOTAL PLAYERS ---
        if (settings->showTeamTotalPlayers) {
            const uint32_t totalPlayers = useSquadStats
                ? teamData.squadStats.totalPlayers
                : teamData.totalPlayers;

            if (settings->showClassIcons) {
                if (!Squad || !Squad->Resource) {
                    Squad = APIDefs->Textures.GetOrCreateFromResource("SQUAD_ICON", SQUAD, hSelf);
                }
                if (Squad && Squad->Resource) {
                    ImGui::Image(Squad->Resource, ImVec2(sz, sz));
                    ImGui::SameLine(0, 5);
                }
            }

            if (settings->showClassNames) {
                ImGui::Text("Total:  %d", totalPlayers);
            }
            else {
                ImGui::Text("%d", totalPlayers);
            }
        }

        // --- 2) K/D RATIO ---
        if (settings->showTeamKDR) {
            const uint32_t kills = useSquadStats
                ? teamData.squadStats.totalKills
                : teamData.totalKills;
            const uint32_t deathsKB = useSquadStats
                ? teamData.squadStats.totalDeathsFromKillingBlows
                : teamData.totalDeathsFromKillingBlows;
            const double kdr = useSquadStats
                ? teamData.squadStats.getKillDeathRatio()
                : teamData.getKillDeathRatio();

            if (settings->showClassIcons) {
                if (!Kdr || !Kdr->Resource) {
                    Kdr = APIDefs->Textures.GetOrCreateFromResource("KDR_ICON", KDR, hSelf);
                }
                if (Kdr && Kdr->Resource) {
                    ImGui::Image(Kdr->Resource, ImVec2(sz, sz));
                    ImGui::SameLine(0, 5);
                }
            }

            if (settings->showClassNames) {
                ImGui::Text("K/D: %d/%d (%.2f)", kills, deathsKB, kdr);
            }
            else {
                ImGui::Text("%d/%d (%.2f)", kills, deathsKB, kdr);
            }
        }

        // --- 3) DEATHS ---
        if (settings->showTeamDeaths) {
            const uint32_t deaths = useSquadStats
                ? teamData.squadStats.totalDeaths
                : teamData.totalDeaths;

            if (settings->showClassIcons) {
                if (!Death || !Death->Resource) {
                    Death = APIDefs->Textures.GetOrCreateFromResource("DEATH_ICON", DEATH, hSelf);
                }
                if (Death && Death->Resource) {
                    ImGui::Image(Death->Resource, ImVec2(sz, sz));
                    ImGui::SameLine(0, 5);
                }
            }

            if (settings->showClassNames) {
                ImGui::Text("Deaths: %d", deaths);
            }
            else {
                ImGui::Text("%d", deaths);
            }
        }

        // --- 4) DOWNS (totalDowned) ---
        if (settings->showTeamDowned) {
            const uint32_t totalDowned = useSquadStats
                ? teamData.squadStats.totalDowned
                : teamData.totalDowned;

            if (settings->showClassIcons) {
                if (!Downed || !Downed->Resource) {
                    Downed = APIDefs->Textures.GetOrCreateFromResource("DOWNED_ICON", DOWNED, hSelf);
                }
                if (Downed && Downed->Resource) {
                    ImGui::Image(Downed->Resource, ImVec2(sz, sz));
                    ImGui::SameLine(0, 5);
                }
            }

            if (settings->showClassNames) {
                ImGui::Text("Downs:  %d", totalDowned);
            }
            else {
                ImGui::Text("%d", totalDowned);
            }
        }

        // --- 5) STRIPS (totalStrips) ---
        if (settings->showTeamStrips) {
            const uint32_t totalStrips = useSquadStats
                ? teamData.squadStats.totalStrips
                : teamData.totalStrips;

            if (settings->showClassIcons) {
                if (!Strips || !Strips->Resource) {
                    Strips = APIDefs->Textures.GetOrCreateFromResource("STRIPS_ICON", STRIPS, hSelf);
                }
                if (Strips && Strips->Resource) {
                    ImGui::Image(Strips->Resource, ImVec2(sz, sz));
                    ImGui::SameLine(0, 5);
                }
            }

            if (settings->showClassNames) {
                ImGui::Text("Strips:  %d", totalStrips);
            }
            else {
                ImGui::Text("%d", totalStrips);
            }
        }

        // --- 6) DAMAGE (vsPlayers or overall) ---
        if (settings->showTeamDamage) {
            const uint64_t damage = settings->vsLoggedPlayersOnly
                ? (useSquadStats
                    ? teamData.squadStats.totalDamageVsPlayers
                    : teamData.totalDamageVsPlayers)
                : (useSquadStats
                    ? teamData.squadStats.totalDamage
                    : teamData.totalDamage);

            if (settings->showClassIcons) {
                if (!Damage || !Damage->Resource) {
                    Damage = APIDefs->Textures.GetOrCreateFromResource("DAMAGE_ICON", DAMAGE, hSelf);
                }
                if (Damage && Damage->Resource) {
                    ImGui::Image(Damage->Resource, ImVec2(sz, sz));
                    ImGui::SameLine(0, 5);
                }
            }

            std::string formattedDamage = formatDamage(damage);

            if (settings->showClassNames) {
                ImGui::Text("Damage: %s", formattedDamage.c_str());
            }
            else {
                ImGui::Text("%s", formattedDamage.c_str());
            }
        }

        // --- 7) DOWN CONTRIBUTION ---
        if (settings->showTeamDownCont) {
            const uint64_t downCont = settings->vsLoggedPlayersOnly
                ? (useSquadStats
                    ? teamData.squadStats.totalDownedContributionVsPlayers
                    : teamData.totalDownedContributionVsPlayers)
                : (useSquadStats
                    ? teamData.squadStats.totalDownedContribution
                    : teamData.totalDownedContribution);

            if (settings->showClassIcons) {
                if (!Downcont || !Downcont->Resource) {
                    Downcont = APIDefs->Textures.GetOrCreateFromResource("DOWNCONT_ICON", DOWNCONT, hSelf);
                }
                if (Downcont && Downcont->Resource) {
                    ImGui::Image(Downcont->Resource, ImVec2(sz, sz));
                    ImGui::SameLine(0, 5);
                }
            }

            std::string formattedDownCont = formatDamage(downCont);

            if (settings->showClassNames) {
                ImGui::Text("Down Cont: %s", formattedDownCont.c_str());
            }
            else {
                ImGui::Text("%s", formattedDownCont.c_str());
            }
        }

        // --- 8) KILL CONTRIBUTION ---
        if (settings->showTeamKillCont) {
            const uint64_t killCont = settings->vsLoggedPlayersOnly
                ? (useSquadStats
                    ? teamData.squadStats.totalKillContributionVsPlayers
                    : teamData.totalKillContributionVsPlayers)
                : (useSquadStats
                    ? teamData.squadStats.totalKillContribution
                    : teamData.totalKillContribution);

            if (settings->showClassIcons) {
                if (!Killcont || !Killcont->Resource) {
                    Killcont = APIDefs->Textures.GetOrCreateFromResource("KILLCONT_ICON", KILLCONT, hSelf);
                }
                if (Killcont && Killcont->Resource) {
                    ImGui::Image(Killcont->Resource, ImVec2(sz, sz));
                    ImGui::SameLine(0, 5);
                }
            }

            std::string formattedKillCont = formatDamage(killCont);

            if (settings->showClassNames) {
                ImGui::Text("Kill Cont: %s", formattedKillCont.c_str());
            }
            else {
                ImGui::Text("%s", formattedKillCont.c_str());
            }
        }

        // --- 9) STRIKE DAMAGE ---
        if (settings->showTeamStrikeDamage) {
            const uint64_t strikeDmg = settings->vsLoggedPlayersOnly
                ? (useSquadStats
                    ? teamData.squadStats.totalStrikeDamageVsPlayers
                    : teamData.totalStrikeDamageVsPlayers)
                : (useSquadStats
                    ? teamData.squadStats.totalStrikeDamage
                    : teamData.totalStrikeDamage);

            if (settings->showClassIcons) {
                if (!Strike || !Strike->Resource) {
                    Strike = APIDefs->Textures.GetOrCreateFromResource("STRIKE_ICON", STRIKE, hSelf);
                }
                if (Strike && Strike->Resource) {
                    ImGui::Image(Strike->Resource, ImVec2(sz, sz));
                    ImGui::SameLine(0, 5);
                }
            }

            std::string formattedStrikeDamage = formatDamage(strikeDmg);

            if (settings->showClassNames) {
                ImGui::Text("Strike: %s", formattedStrikeDamage.c_str());
            }
            else {
                ImGui::Text("%s", formattedStrikeDamage.c_str());
            }
        }

        // --- 10) CONDITION DAMAGE ---
        if (settings->showTeamCondiDamage) {
            const uint64_t condiDmg = settings->vsLoggedPlayersOnly
                ? (useSquadStats
                    ? teamData.squadStats.totalCondiDamageVsPlayers
                    : teamData.totalCondiDamageVsPlayers)
                : (useSquadStats
                    ? teamData.squadStats.totalCondiDamage
                    : teamData.totalCondiDamage);

            if (settings->showClassIcons) {
                if (!Condi || !Condi->Resource) {
                    Condi = APIDefs->Textures.GetOrCreateFromResource("CONDI_ICON", CONDI, hSelf);
                }
                if (Condi && Condi->Resource) {
                    ImGui::Image(Condi->Resource, ImVec2(sz, sz));
                    ImGui::SameLine(0, 5);
                }
            }

            std::string formattedCondiDamage = formatDamage(condiDmg);

            if (settings->showClassNames) {
                ImGui::Text("Condi:  %s", formattedCondiDamage.c_str());
            }
            else {
                ImGui::Text("%s", formattedCondiDamage.c_str());
            }
        }

        // --- 11) SPEC BARS (unchanged) ---
        if (settings->showSpecBars) {
            ImGui::Separator();
            RenderSpecializationBars(teamData, teamName, settings, hSelf);
        }
    }



    void MainWindow::RenderSpecializationBars(const TeamStats& teamData,
        const std::string& teamName,      // NEW PARAM
        const MainWindowSettings* settings,
        HINSTANCE hSelf)
    {
        // 1) Identify the current log filename
        const std::string& currentLogFilename = parsedLogs[currentLogIndex].filename;

        // Decide if we use squad stats
        const bool useSquadStats = (settings->squadPlayersOnly && teamData.isPOVTeam);
        const auto& specs = useSquadStats ? teamData.squadStats.eliteSpecStats : teamData.eliteSpecStats;

        // 2) Build a cache key for (filename + teamName + relevant settings)
        SpecCacheKey key{
            currentLogFilename,
            teamName,                    // NEW
            useSquadStats,
            settings->windowSort,
            settings->vsLoggedPlayersOnly,
            settings->barRepIndependent,
            settings->barRepresentation
        };

        // 3) Look up in our global s_specRenderCache
        auto it = s_specRenderCache.find(key);
        if (it == s_specRenderCache.end()) {
            // Not found => build a fresh sorted vector
            std::vector<std::pair<std::string, SpecStats>> newSorted;
            newSorted.reserve(specs.size());
            for (const auto& p : specs) {
                newSorted.emplace_back(p);
            }

            // Sort it
            std::sort(
                newSorted.begin(),
                newSorted.end(),
                getSpecSortComparator(settings->windowSort, settings->vsLoggedPlayersOnly)
            );

            // Insert into the map
            auto [insertIt, success] = s_specRenderCache.emplace(key, std::move(newSorted));
            it = insertIt; // 'it' now points to the cached sorted vector
        }

        // 4) Use the cached data
        const auto& sortedClasses = it->second;

        // ----- The rest is your existing bar-drawing code -----
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        const float spacing = 2.0f;

        // Find maxValue for normalizing bar widths
        uint64_t maxValue = 0;
        for (const auto& specPair : sortedClasses) {
            const auto& stat = specPair.second;
            uint64_t value = (settings->barRepIndependent)
                ? getBarValue(settings->barRepresentation, stat, settings->vsLoggedPlayersOnly)
                : getBarValue(settings->windowSort, stat, settings->vsLoggedPlayersOnly);

            if (value > maxValue)
                maxValue = value;
        }

        // Extend our BarData struct to hold the full width (primary bar width)
        struct BarData {
            ImVec2 screen_pos;
            float  width;
            float  height;
            ImU32  color;
            float  fullWidth; // store the primary bar's width (for comparison)
        };

        std::vector<BarData> primaryBars, secondaryBars;
        std::vector<std::function<void()>> textRenderers;

        // Loop through each spec
        for (const auto& specPair : sortedClasses) {
            const auto& eliteSpec = specPair.first;
            const auto& stat = specPair.second;

            // Base profession from elite spec
            std::string profession = "Unknown";
            if (eliteSpecToProfession.count(eliteSpec))
                profession = eliteSpecToProfession.at(eliteSpec);

            // Colors
            ImVec4 primaryColor(1.0f, 1.0f, 1.0f, 1.0f);
            ImVec4 secondaryColor(0.0f, 0.0f, 0.0f, 0.0f);
            CacheBarColors(profession, primaryColor, secondaryColor);

            ImVec2 initialCursorPos = ImGui::GetCursorPos();
            ImVec2 screenPos = ImGui::GetCursorScreenPos();
            float barHeight = ImGui::GetTextLineHeight() + 4.0f;

            // Calculate bar width
            float barWidth = CalculateBarWidth(stat, sortedClasses, settings, maxValue);

            // Primary bar (store barWidth as fullWidth)
            primaryBars.push_back({
                screenPos,
                barWidth,
                barHeight,
                ImGui::ColorConvertFloat4ToU32(primaryColor),
                barWidth
                });

            // Possibly add a secondary bar
            const auto& effectiveRep = (settings->barRepIndependent
                ? settings->barRepresentation
                : settings->windowSort);

            const auto secondIt = MainWindowSettings::secondaryStats.find(effectiveRep);
            if (secondIt != MainWindowSettings::secondaryStats.end() && secondIt->second.enabled)
            {
                uint64_t baseValue = (settings->barRepIndependent
                    ? getBarValue(settings->barRepresentation, stat, settings->vsLoggedPlayersOnly)
                    : getBarValue(settings->windowSort, stat, settings->vsLoggedPlayersOnly));

                if (baseValue > 0) {
                    uint64_t secondaryValue = getBarValue(
                        secondIt->second.secondaryStat,
                        stat,
                        settings->vsLoggedPlayersOnly
                    );

                    if (secondaryValue > 0) {
                        float ratio = static_cast<float>(secondaryValue) / static_cast<float>(baseValue);
                        ratio = std::min(ratio, secondIt->second.maxRatio);
                        float secBarWidth = barWidth * ratio;

                        secondaryBars.push_back({
                            screenPos,
                            secBarWidth,
                            barHeight,
                            ImGui::ColorConvertFloat4ToU32(secondaryColor),
                            barWidth  // full width of the primary bar
                            });
                    }
                }
            }

            // Defer text rendering
            textRenderers.push_back([=, &stat, &settings]() {
                ImGui::SetCursorPos(ImVec2(
                    initialCursorPos.x + 5.0f,
                    initialCursorPos.y + 2.0f
                ));
                // Render the text/label
                BarTemplateRenderer::RenderTemplate(
                    stat.count,
                    eliteSpec,
                    settings->useShortClassNames,
                    stat,
                    settings->vsLoggedPlayersOnly,
                    settings->windowSort,
                    hSelf,
                    ImGui::GetFontSize(),
                    settings->showSpecTooltips
                );
                });

            // Advance cursor
            ImGui::SetCursorPosY(initialCursorPos.y + barHeight + spacing);
        }

        // --- Determine the rounding value ---
        // Use ImGui's FrameBorderSize if it's greater than 0; otherwise use our setting.
        float currentFrameRounding = ImGui::GetStyle().FrameRounding;
        float roundingValue = (currentFrameRounding > 0.0f)
            ? currentFrameRounding
            : static_cast<float>(settings->barCornerRounding);

        // Batch draw primary bars (round all corners)
        for (const auto& bar : primaryBars) {
            drawList->AddRectFilled(
                bar.screen_pos,
                ImVec2(bar.screen_pos.x + bar.width, bar.screen_pos.y + bar.height),
                bar.color,
                roundingValue, // use the computed rounding value
                ImDrawCornerFlags_All
            );
        }

        // Batch draw secondary bars (conditionally round corners)
        for (const auto& bar : secondaryBars) {
            int roundingFlags = 0;
            if (fabs(bar.width - bar.fullWidth) < 0.001f) {
                // Full width: round all corners.
                roundingFlags = ImDrawCornerFlags_All;
            }
            else {
                // Otherwise, only round the left side.
                roundingFlags = ImDrawCornerFlags_TopLeft | ImDrawCornerFlags_BotLeft;
            }
            drawList->AddRectFilled(
                bar.screen_pos,
                ImVec2(bar.screen_pos.x + bar.width, bar.screen_pos.y + bar.height),
                bar.color,
                roundingValue,
                roundingFlags
            );
        }

        // Render text
        for (auto& fn : textRenderers) {
            fn();
        }
    }



    float MainWindow::CalculateBarWidth(
        const SpecStats& stats,
        const std::vector<std::pair<std::string, SpecStats>>& sortedClasses,
        const MainWindowSettings* settings,
        uint64_t maxValue
    ) const
    {
        const uint64_t currentValue = settings->barRepIndependent
            ? getBarValue(settings->barRepresentation, stats, settings->vsLoggedPlayersOnly)
            : getBarValue(settings->windowSort, stats, settings->vsLoggedPlayersOnly);

        const float frac = (maxValue > 0)
            ? static_cast<float>(currentValue) / static_cast<float>(maxValue)
            : 0.0f;
        return ImGui::GetContentRegionAvail().x * frac;
    }

    void MainWindow::CacheBarColors(const std::string& profession,
        ImVec4& primary,
        ImVec4& secondary) const
    {
        auto colorIt = std::find_if(
            professionColorPair.begin(),
            professionColorPair.end(),
            [&](const ProfessionColor& pc) { return pc.name == profession; }
        );

        if (colorIt != professionColorPair.end()) {
            primary = colorIt->primaryColor;
            secondary = colorIt->secondaryColor;
        }
    }

    void MainWindow::RenderMainWindowSettingsPopup(MainWindowSettings* settings) {
        if (!ImGui::BeginPopupContextWindow("MainWindowContextMenu", ImGuiPopupFlags_MouseButtonRight)) return;

        // Window Name Editor
        if (!settings->isWindowNameEditing) {
            strncpy(settings->tempWindowName,
                settings->windowName.c_str(),
                sizeof(settings->tempWindowName) - 1);
            settings->isWindowNameEditing = true;
        }

        ImGui::Text("Window Name:");
        float windowWidth = ImGui::GetWindowWidth();
        ImGui::SetNextItemWidth(windowWidth * 0.5f);
        if (ImGui::InputText(" ##Window Name", settings->tempWindowName, sizeof(settings->tempWindowName))) {}

        ImGui::SameLine();
        if (ImGui::Button("Apply")) {
            // No need to unregister/re-register from Nexus since we're using ### now
            settings->windowName = settings->tempWindowName;
            settings->updateDisplayName("WvW Fight Analysis");
            Settings::Save(SettingsPath);
        }

        RenderHistoryMenu(settings);
        RenderExcludeMenu(settings);

        // Display Settings
        if (ImGui::BeginMenu("Display")) {
            if (ImGui::Checkbox("log name", &settings->showLogName)) Settings::Save(SettingsPath);
            if (ImGui::Checkbox("short spec names", &settings->useShortClassNames)) Settings::Save(SettingsPath);
            if (ImGui::Checkbox("draw bars", &settings->showSpecBars)) Settings::Save(SettingsPath);
            if (ImGui::Checkbox("tooltips", &settings->showSpecTooltips)) Settings::Save(SettingsPath);

            if (ImGui::BeginMenu("Team Stats")) {
                if (ImGui::Checkbox("player count", &settings->showTeamTotalPlayers)) { Settings::Save(SettingsPath); }
                if (ImGui::Checkbox("k/d ratio", &settings->showTeamKDR)) { Settings::Save(SettingsPath); }
                if (ImGui::Checkbox("deaths", &settings->showTeamDeaths)) { Settings::Save(SettingsPath); }
                if (ImGui::Checkbox("downs", &settings->showTeamDowned)) { Settings::Save(SettingsPath); }
                //if (ImGui::Checkbox("strips", &settings->showTeamStrips)) { Settings::Save(SettingsPath); }
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
                            if (ImGui::RadioButton(
                                displayOptions[i],
                                settings->barRepresentation == displayValues[i]))
                            {
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

        // Style Settings
        if (ImGui::BeginMenu("Style")) {
            if (ImGui::Checkbox("use tabbed view", &settings->useTabbedView)) Settings::Save(SettingsPath);
            if (ImGui::Checkbox("show title", &settings->showTitle)) Settings::Save(SettingsPath);
            if (ImGui::Checkbox("window style title", &settings->useWindowStyleForTitle)) Settings::Save(SettingsPath);
            if (ImGui::Checkbox("scroll bar", &settings->showScrollBar)) Settings::Save(SettingsPath);
            if (ImGui::Checkbox("allow focus", &settings->allowFocus)) Settings::Save(SettingsPath);
            if (ImGui::Checkbox("background", &settings->showBackground)) Settings::Save(SettingsPath);
            if (ImGui::Checkbox("overide tableheaderbg", &settings->overideTableBackgroundStyle)) Settings::Save(SettingsPath);
            
            float currentFrameRounding = ImGui::GetStyle().FrameRounding;
            if (!currentFrameRounding) {
                ImGui::PushItemWidth(100);
                if (ImGui::InputInt("bar rounding##BarCornerRounding", &settings->barCornerRounding, 1, 10))
                {
                    if (settings->barCornerRounding < 0)
                        settings->barCornerRounding = 0;
                    if (settings->barCornerRounding > 12)
                        settings->barCornerRounding = 12;

                    Settings::Save(SettingsPath);
                }
                ImGui::PopItemWidth();
                
            }
            ImGui::EndMenu();
        }


        // Sort Settings
        if (ImGui::BeginMenu("Sort")) {
            const char* sortOptions[] = { "Players", "Damage", "Down Cont", "Kill Cont", "Deaths", "Downs" };
            const char* sortValues[] = { "players", "damage", "down cont", "kill cont", "deaths", "downs" };

            for (int i = 0; i < IM_ARRAYSIZE(sortOptions); i++) {
                if (ImGui::RadioButton(sortOptions[i], settings->windowSort == sortValues[i])) {
                    settings->windowSort = sortValues[i];
                    Settings::Save(SettingsPath);
                }
            }
            ImGui::EndMenu();
        }

        if (ImGui::Checkbox("Show Squad Players Only", &settings->squadPlayersOnly)) Settings::Save(SettingsPath);
        if (ImGui::Checkbox("Damage vs Logged Players Only", &settings->vsLoggedPlayersOnly)) Settings::Save(SettingsPath);

        ImGui::EndPopup();
    }

    void MainWindow::RenderExcludeMenu(MainWindowSettings* settings) {
        if (ImGui::BeginMenu("exclude")) {
            if (ImGui::Checkbox("red team", &settings->excludeRedTeam)) Settings::Save(SettingsPath);
            if (ImGui::Checkbox("blue team", &settings->excludeBlueTeam)) Settings::Save(SettingsPath);
            if (ImGui::Checkbox("green team", &settings->excludeGreenTeam)) Settings::Save(SettingsPath);

            ImGui::EndMenu();
        }
    }

    void MainWindow::RenderHistoryMenu(MainWindowSettings* settings) {
        if (ImGui::BeginMenu("History")) {
            for (int i = 0; i < parsedLogs.size(); ++i) {
                const auto& log = parsedLogs[i];
                const std::string fnstr = log.filename.substr(0, log.filename.find_last_of('.'));
                const uint64_t durationMs = log.data.combatEndTime - log.data.combatStartTime;
                const auto duration = std::chrono::milliseconds(durationMs);
                const int minutes = std::chrono::duration_cast<std::chrono::minutes>(duration).count();
                const int seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count() % 60;
                const std::string displayName =
                    fnstr + " (" + std::to_string(minutes) + "m " + std::to_string(seconds) + "s)";

                if (ImGui::RadioButton(displayName.c_str(), &currentLogIndex, i)) {
                    // Selection update handled automatically by RadioButton
                }
            }
            ImGui::EndMenu();
        }
    }

    void MainWindow::RenderTemplateSettings(MainWindowSettings* settings) {
        if (!ImGui::BeginMenu("Bar Templates"))
            return;

        const char* sortTypes[] = { "players", "damage", "down cont", "kill cont", "deaths", "downs" };
        const char* sortLabels[] = { "Players", "Damage", "Down Contribution", "Kill Contribution", "Deaths", "Downs" };

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

                // Define the template legend that will be shown as a tooltip.
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

                // Prepare the InputText widget.
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.8f);
                std::string inputLabel = "##Template" + sortType;  // Ensure concatenation works as expected.
                if (ImGui::InputText(inputLabel.c_str(), tempBuffer, sizeof(tempBuffer))) {
                    currentTemplate = tempBuffer;
                    if (settings->windowSort == sortType) {
                        BarTemplateRenderer::ParseTemplate(currentTemplate);
                    }
                    Settings::Save(SettingsPath);
                }

                // If the user hovers over the InputText, show the tooltip.
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("%s", templateLegend);
                }

                // Button to reset the template to its default value.
                std::string resetButtonLabel = "Reset to Default##" + sortType;
                if (ImGui::Button(resetButtonLabel.c_str())) {
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


} // namespace wvwfightanalysis::gui