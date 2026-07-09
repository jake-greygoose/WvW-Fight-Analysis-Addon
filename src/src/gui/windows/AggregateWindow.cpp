// src/gui/windows/AggregateWindow.cpp
#include "gui/windows/AggregateWindow.h"
#include "utils/Utils.h"
#include "resource.h"
#include "thirdparty/imgui_positioning/imgui_positioning.h"

namespace wvwfightanalysis::gui {

    void AggregateWindow::Render(HINSTANCE hSelf, AggregateWindowSettings* settings) {
        if (!settings->isEnabled) return;

        static bool wasEnabled = settings->isEnabled;

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

        window_flags |= ImGuiExt::UpdatePosition(windowName);

        ImGui::SetNextWindowSize(ImVec2(450, 350), ImGuiCond_FirstUseEver);

        bool windowOpen = ImGui::Begin(windowName.c_str(), &settings->isEnabled, window_flags);

        // Check for window closing conditions
        if ((!windowOpen && settings->isEnabled) || (wasEnabled && !settings->isEnabled)) {
            Settings::RequestSave(SettingsPath);
        }
        wasEnabled = settings->isEnabled;

        if (!windowOpen) {
            ImGui::End();
            return;
        }

        float sz = ImGui::GetFontSize();

        if (ImGui::Button("Reset Stats")) {
            std::lock_guard<std::mutex> lock(aggregateStatsMutex);
            globalAggregateStats = GlobalAggregateStats();
            cachedAverages = CachedAverages();
        }

        std::lock_guard<std::mutex> lock(aggregateStatsMutex);

        if (settings->showTotalCombatTime) {
            ImGui::Text("Total Combat Time: %s",
                formatDuration(globalAggregateStats.totalCombatTime).c_str());
        }
        if (settings->showAvgCombatTime) {
            ImGui::Text("Average Combat Time: %s",
                formatDuration(static_cast<uint64_t>(cachedAverages.averageCombatTime)).c_str());
        }
        ImGui::Text("Total Fights: %d", globalAggregateStats.combatInstanceCount);

        ImGui::Separator();

        for (const auto& [teamName, teamAgg] : globalAggregateStats.teamAggregates) {
            ImGui::Spacing();
            RenderTeamSection(teamName, teamAgg, settings, hSelf, sz);
            ImGui::Separator();
        }

        RenderSettingsPopup(settings);

        // Append positioning menu to the same context menu
        ImGuiExt::ContextMenuPosition("AggregateContextMenu");

        ImGui::End();
    }

    void AggregateWindow::RenderTeamSection(
        const std::string& teamName,
        const TeamAggregateStats& teamAgg,
        const AggregateWindowSettings* settings,
        HINSTANCE hSelf,
        float fontSize
    ) {
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
                    ImGui::Image(Squad->Resource, ImVec2(fontSize, fontSize));
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
                    ImGui::Image(Death->Resource, ImVec2(fontSize, fontSize));
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
                    ImGui::Image(Downed->Resource, ImVec2(fontSize, fontSize));
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
    }

    void AggregateWindow::RenderSettingsPopup(AggregateWindowSettings* settings) {
        if (ImGui::BeginPopupContextWindow("AggregateContextMenu", ImGuiPopupFlags_MouseButtonRight)) {
            // Name editing logic
            if (!settings->isWindowNameEditing) {
                strncpy(settings->tempWindowName, settings->windowName.c_str(),
                    sizeof(settings->tempWindowName) - 1);
                settings->isWindowNameEditing = true;
            }

            // Window name input
            ImGui::Text("Window Name:");
            float windowWidth = ImGui::GetWindowWidth();
            ImGui::SetNextItemWidth(windowWidth * 0.5f);
            ImGui::InputText("##Window Name", settings->tempWindowName,
                sizeof(settings->tempWindowName));

            // Apply button
            ImGui::SameLine();
            if (ImGui::Button("Apply")) {
                settings->windowName = settings->tempWindowName;
                settings->updateDisplayName("Aggregate Stats");  // Update the display name immediately
                Settings::RequestSave(SettingsPath);
            }

            // Menu sections
            RenderDisplaySettings(settings);
            RenderStyleSelector(settings);

            // Squad checkbox
            if (ImGui::Checkbox("Squad Players Only", &settings->squadPlayersOnly)) {
                Settings::RequestSave(SettingsPath);
            }

            ImGui::EndPopup();
        }
        else {
            settings->isWindowNameEditing = false;
        }
    }

    void AggregateWindow::RenderDisplaySettings(AggregateWindowSettings* settings) {
        if (ImGui::BeginMenu("Display")) {
            // Combat time settings
            if (ImGui::Checkbox("Average combat time", &settings->showAvgCombatTime)) Settings::RequestSave(SettingsPath);
            if (ImGui::Checkbox("Total combat time", &settings->showTotalCombatTime)) Settings::RequestSave(SettingsPath);
            ImGui::Separator();

            // Stat visibility
            if (ImGui::Checkbox("Average players", &settings->showTeamTotalPlayers)) Settings::RequestSave(SettingsPath);
            if (ImGui::Checkbox("Deaths", &settings->showTeamDeaths)) Settings::RequestSave(SettingsPath);
            if (ImGui::Checkbox("Downs", &settings->showTeamDowned)) Settings::RequestSave(SettingsPath);
            ImGui::Separator();

            // Display options
            if (ImGui::Checkbox("Label names", &settings->showClassNames)) Settings::RequestSave(SettingsPath);
            if (ImGui::Checkbox("Icons", &settings->showClassIcons)) Settings::RequestSave(SettingsPath);
            if (ImGui::Checkbox("Average specs", &settings->showAvgSpecs)) Settings::RequestSave(SettingsPath);

            ImGui::EndMenu();
        }
    }

    void AggregateWindow::RenderStyleSelector(AggregateWindowSettings* settings) {
        if (ImGui::BeginMenu("Style")) {
            // Window appearance
            if (ImGui::Checkbox("Show title", &settings->showTitle)) Settings::RequestSave(SettingsPath);
            if (ImGui::Checkbox("Use window style for title bar", &settings->useWindowStyleForTitle)) Settings::RequestSave(SettingsPath);
            if (ImGui::Checkbox("Scroll bar", &settings->showScrollBar)) Settings::RequestSave(SettingsPath);
            if (ImGui::Checkbox("Background", &settings->showBackground)) Settings::RequestSave(SettingsPath);
            if (ImGui::Checkbox("Allow focus", &settings->allowFocus)) Settings::RequestSave(SettingsPath);

            ImGui::EndMenu();
        }
    }

} // namespace
