#include "gui/windows/WidgetWindow.h"
#include "gui/PieChart.h"
#include "utils/Utils.h"
#include "resource.h"
#include "imgui/imgui_internal.h"
#include "thirdparty/imgui_positioning/imgui_positioning.h"
#include "ImAnimate.h" 

namespace wvwfightanalysis::gui {

    void WidgetWindow::Render(HINSTANCE hSelf, WidgetWindowSettings* settings) {
        if (!settings->isEnabled)
            return;

        std::string windowName = settings->getDisplayName("Team Ratio Bar");
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoBackground;  // Always disable the window background

        if (!settings->allowFocus) {
            window_flags |= ImGuiWindowFlags_NoFocusOnAppearing |
                ImGuiWindowFlags_NoBringToFrontOnFocus;
        }
        if (settings->disableMoving)
            window_flags |= ImGuiWindowFlags_NoMove;
        if (settings->disableClicking)
            window_flags |= ImGuiWindowFlags_NoInputs;

        // Push window style variables
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 4.0f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));  // Transparent background

        // Use different window size for pie chart mode (square) vs bar mode (rectangle)
        const ImVec2 widgetSize = settings->usePieChartStyle
            ? ImVec2(settings->pieChartSize, settings->pieChartSize)
            : ImVec2(settings->widgetWidth, settings->widgetHeight);
        ImGui::SetNextWindowSize(widgetSize);

        // Update positioning from imgui_positioning library
        window_flags |= ImGuiExt::UpdatePosition(windowName);

        if (ImGui::Begin(windowName.c_str(), &settings->isEnabled, window_flags)) {
            if (parsedLogs.empty()) {
                ImGui::Text(initialParsingComplete ? "No logs parsed yet." : "Parsing logs...");
            }
            else {
                const auto& currentLogData = parsedLogs[currentLogIndex].data;
                const std::vector<std::string> team_names = { "Red", "Blue", "Green" };

                // Original team background colors (non-combat)
                const std::vector<ImVec4> team_colors = {
                    settings->colors.redBackground,
                    settings->colors.blueBackground,
                    settings->colors.greenBackground
                };

                // Alternate background colors when in combat
                const std::vector<ImVec4> team_colors_combat = {
                    settings->colors.redBackgroundCombat,
                    settings->colors.blueBackgroundCombat,
                    settings->colors.greenBackgroundCombat
                };

                // Original team text colors (non-combat)
                const std::vector<ImVec4> text_colors = {
                    settings->colors.redText,
                    settings->colors.blueText,
                    settings->colors.greenText
                };

                // Alternate text colors when in combat
                const std::vector<ImVec4> text_colors_combat = {
                    settings->colors.redTextCombat,
                    settings->colors.blueTextCombat,
                    settings->colors.greenTextCombat
                };

                // Choose the correct colors based on combat state.
                const std::vector<ImVec4>& current_colors = MumbleLink->Context.IsInCombat ? team_colors_combat : team_colors;
                const std::vector<ImVec4>& team_text_colors = MumbleLink->Context.IsInCombat ? text_colors_combat : text_colors;

                // Build the list of teams to display along with the original index for each team.
                std::vector<TeamDisplayData> teamsToDisplay;
                std::vector<size_t> teamIndices;  // Record the original index for each team that exists.
                for (size_t i = 0; i < team_names.size(); ++i) {
                    const auto teamIt = currentLogData.teamStats.find(team_names[i]);
                    if (teamIt == currentLogData.teamStats.end())
                        continue;

                    const bool useSquadStats = settings->squadPlayersOnly && teamIt->second.isPOVTeam;
                    float teamCountValue = 0.0f;
                    if (settings->widgetStats == "players") {
                        teamCountValue = static_cast<float>(
                            useSquadStats ? teamIt->second.squadStats.totalPlayers : teamIt->second.totalPlayers);
                    }
                    else if (settings->widgetStats == "deaths") {
                        teamCountValue = static_cast<float>(
                            useSquadStats ? teamIt->second.squadStats.totalDeaths : teamIt->second.totalDeaths);
                    }
                    else if (settings->widgetStats == "downs") {
                        teamCountValue = static_cast<float>(
                            useSquadStats ? teamIt->second.squadStats.totalDowned : teamIt->second.totalDowned);
                    }
                    else if (settings->widgetStats == "damage") {
                        teamCountValue = static_cast<float>(
                            settings->vsLoggedPlayersOnly ?
                            (useSquadStats ? teamIt->second.squadStats.totalDamageVsPlayers : teamIt->second.totalDamageVsPlayers) :
                            (useSquadStats ? teamIt->second.squadStats.totalDamage : teamIt->second.totalDamage));
                    }
                    else if (settings->widgetStats == "kdr") {
                        teamCountValue = useSquadStats ?
                            teamIt->second.squadStats.getKillDeathRatio() : teamIt->second.getKillDeathRatio();
                    }

                    char buf[64];
                    if (settings->widgetStats == "damage") {
                        snprintf(buf, sizeof(buf), "%s", formatDamage(static_cast<uint64_t>(teamCountValue)).c_str());
                    }
                    else if (settings->widgetStats == "kdr") {
                        snprintf(buf, sizeof(buf), "%.2f", teamCountValue);
                    }
                    else {
                        snprintf(buf, sizeof(buf), "%.0f", teamCountValue);
                    }

                    teamsToDisplay.push_back({ teamCountValue, current_colors[i], team_text_colors[i], std::string(buf) });
                    teamIndices.push_back(i);
                }

                if (teamsToDisplay.empty()) {
                    ImGui::Text("No team data available.");
                }
                else {
                    ImTextureID currentStatIcon = nullptr;
                    if (settings->showWidgetIcon) {
                        if (settings->widgetStats == "players" && !Squad) {
                            Squad = APIDefs->Textures.GetOrCreateFromResource("SQUAD_ICON", SQUAD, hSelf);
                        }
                        else if (settings->widgetStats == "deaths" && !Death) {
                            Death = APIDefs->Textures.GetOrCreateFromResource("DEATH_ICON", DEATH, hSelf);
                        }
                        else if (settings->widgetStats == "downs" && !Downed) {
                            Downed = APIDefs->Textures.GetOrCreateFromResource("DOWNED_ICON", DOWNED, hSelf);
                        }
                        else if (settings->widgetStats == "damage" && !Damage) {
                            Damage = APIDefs->Textures.GetOrCreateFromResource("DAMAGE_ICON", DAMAGE, hSelf);
                        }
                        else if (settings->widgetStats == "kdr" && !Kdr) {
                            Kdr = APIDefs->Textures.GetOrCreateFromResource("KDR_ICON", KDR, hSelf);
                        }
                        currentStatIcon = GetStatIcon(settings);
                    }

                    // Extract filtered vectors for counts, background colors, and texts.
                    std::vector<float> counts;
                    std::vector<ImVec4> colors;
                    std::vector<const char*> texts;
                    std::vector<ImVec4> textColorsVec;
                    counts.reserve(teamsToDisplay.size());
                    colors.reserve(teamsToDisplay.size());
                    texts.reserve(teamsToDisplay.size());
                    textColorsVec.reserve(teamsToDisplay.size());
                    for (const auto& team : teamsToDisplay) {
                        counts.push_back(team.count);
                        colors.push_back(team.backgroundColor);
                        texts.push_back(team.text.c_str());
                        textColorsVec.push_back(team.textColor);
                    }

                    // Choose rendering style based on settings
                    if (settings->usePieChartStyle) {
                        RenderPieChart(
                            hSelf,
                            counts,
                            colors,
                            widgetSize,
                            settings,
                            texts,
                            textColorsVec
                        );
                    }
                    else {
                        RenderSimpleRatioBar(
                            counts,
                            colors,
                            widgetSize,
                            texts,
                            currentStatIcon,
                            settings,
                            team_text_colors,
                            teamIndices
                        );
                    }
                }
            }

            RenderSettingsPopup(settings);

            // Append positioning menu to the same context menu
            ImGuiExt::ContextMenuPosition("WidgetContextMenu");

            // Pop style vars after all popup content (including positioning) is added
            ImGui::PopStyleVar(2);

            settings->position = ImGui::GetWindowPos();

            ImGui::PopStyleColor();
            ImGui::PopStyleVar(4);
            ImGui::End();
        }
        else {
            ImGui::PopStyleColor();
            ImGui::PopStyleVar(4);
        }
    }

    ImTextureID WidgetWindow::GetStatIcon(const WidgetWindowSettings* settings) {
        if (settings->widgetStats == "players") return Squad ? Squad->Resource : nullptr;
        if (settings->widgetStats == "deaths") return Death ? Death->Resource : nullptr;
        if (settings->widgetStats == "downs") return Downed ? Downed->Resource : nullptr;
        if (settings->widgetStats == "damage") return Damage ? Damage->Resource : nullptr;
        if (settings->widgetStats == "kdr") return Kdr ? Kdr->Resource : nullptr;
        return nullptr;
    }

    void WidgetWindow::RenderSettingsPopup(WidgetWindowSettings* settings) {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5.0f, 5.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(5.0f, 5.0f));

        if (ImGui::BeginPopupContextWindow("WidgetContextMenu", ImGuiPopupFlags_MouseButtonRight)) {
            if (!settings->isWindowNameEditing) {
                strncpy(settings->tempWindowName, settings->windowName.c_str(),
                    sizeof(settings->tempWindowName) - 1);
                settings->isWindowNameEditing = true;
            }

            ImGui::Text("Widget Name:");
            const float windowWidth = ImGui::GetWindowWidth();
            ImGui::SetNextItemWidth(windowWidth * 0.5f);
            ImGui::InputText(" ##Widget Name", settings->tempWindowName, sizeof(settings->tempWindowName));

            ImGui::SameLine();
            if (ImGui::Button("Apply")) {
                UnregisterWindowFromNexusEsc(settings, "WvW Fight Analysis");
                settings->windowName = settings->tempWindowName;
                settings->updateDisplayName("WvW Fight Analysis");
                RegisterWindowForNexusEsc(settings, "WvW Fight Analysis");
                Settings::Save(SettingsPath);
            }

            RenderHistoryMenu(settings);
            RenderDisplayStatsMenu(settings);
            RenderStyleMenu(settings);

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

        // Don't pop style vars here - let them apply to ContextMenuPosition too
    }

    void WidgetWindow::RenderHistoryMenu(WidgetWindowSettings* settings) {
        if (ImGui::BeginMenu("History")) {
            for (int i = 0; i < parsedLogs.size(); ++i) {
                const auto& log = parsedLogs[i];
                const std::string fnstr = log.filename.substr(0, log.filename.find_last_of('.'));
                const uint64_t durationMs = log.data.combatEndTime - log.data.combatStartTime;
                const auto duration = std::chrono::milliseconds(durationMs);
                const int minutes = std::chrono::duration_cast<std::chrono::minutes>(duration).count();
                const int seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count() % 60;
                const std::string displayName = fnstr + " (" + std::to_string(minutes) + "m " + std::to_string(seconds) + "s)";

                if (ImGui::RadioButton(displayName.c_str(), &currentLogIndex, i)) {
                    // Selection update handled automatically by RadioButton
                }
            }
            ImGui::EndMenu();
        }
    }

    void WidgetWindow::RenderDisplayStatsMenu(WidgetWindowSettings* settings) {
        if (ImGui::BeginMenu("Display Stats")) {
            constexpr std::array<const char*, 5> statOptions = { "Players", "K/D Ratio", "Deaths", "Downs", "Damage" };
            constexpr std::array<const char*, 5> statValues = { "players", "kdr", "deaths", "downs", "damage" };

            for (size_t i = 0; i < statOptions.size(); ++i) {
                const bool isSelected = settings->widgetStats == statValues[i];
                if (ImGui::RadioButton(statOptions[i], isSelected)) {
                    settings->widgetStats = statValues[i];
                    Settings::Settings["widgetStats"] = settings->widgetStats;
                    Settings::Save(SettingsPath);
                }
            }
            ImGui::EndMenu();
        }
    }

    void WidgetWindow::RenderStyleMenu(WidgetWindowSettings* settings) {
        if (ImGui::BeginMenu("Style")) {
            if (ImGui::Checkbox("use larger font", &settings->largerFont)) {
                Settings::Settings["largerFont"] = settings->largerFont;
                Settings::Save(SettingsPath);
            }

            if (ImGui::Checkbox("show widget icon", &settings->showWidgetIcon)) {
                Settings::Settings["ShowWidgetIcon"] = settings->showWidgetIcon;
                Settings::Save(SettingsPath);
            }

            if (ImGui::Checkbox("use pie chart style", &settings->usePieChartStyle)) {
                Settings::Settings["usePieChartStyle"] = settings->usePieChartStyle;
                Settings::Save(SettingsPath);
            }

            // Pie chart size controls (only shown in pie chart mode)
            if (settings->usePieChartStyle) {
                ImGui::SetNextItemWidth(200.0f);
                if (ImGui::SliderFloat("##pie chart size", &settings->pieChartSize, 100.0f, 600.0f, "%.0f", ImGuiSliderFlags_NoInput)) {
                    settings->pieChartSize = std::round(settings->pieChartSize);
                    settings->pieChartSize = std::clamp(settings->pieChartSize, 100.0f, 600.0f);
                    Settings::Settings["pieChartSize"] = settings->pieChartSize;
                    Settings::Save(SettingsPath);
                }
                ImGui::SameLine();
                ImGui::SetNextItemWidth(60.0f);
                if (ImGui::InputFloat("pie chart size", &settings->pieChartSize, 1.0f, 1.0f, "%.0f")) {
                    settings->pieChartSize = std::clamp(settings->pieChartSize, 100.0f, 600.0f);
                    Settings::Settings["pieChartSize"] = settings->pieChartSize;
                    Settings::Save(SettingsPath);
                }
            }

            // Bar mode controls (only shown in bar mode)
            if (!settings->usePieChartStyle) {
                float currentFrameRounding = ImGui::GetStyle().FrameRounding;
                if (!currentFrameRounding) {
                    ImGui::SetNextItemWidth(200.0f);
                    if (ImGui::SliderFloat("##widget roundness", &settings->widgetRoundness, 0.0f, 12.0f, "%.0f", ImGuiSliderFlags_NoInput)) {
                        settings->widgetRoundness = std::round(settings->widgetRoundness);
                        settings->widgetRoundness = std::clamp(settings->widgetRoundness, 0.0f, 12.0f);
                        Settings::Settings["widgetRoundness"] = settings->widgetRoundness;
                        Settings::Save(SettingsPath);
                    }
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(60.0f);
                    if (ImGui::InputFloat("widget roundness", &settings->widgetRoundness, 1.0f, 1.0f, "%.0f")) {
                        settings->widgetRoundness = std::clamp(settings->widgetRoundness, 0.0f, 12.0f);
                        Settings::Settings["widgetRoundness"] = settings->widgetRoundness;
                        Settings::Save(SettingsPath);
                    }

                }

                ImGui::SetNextItemWidth(200.0f);
                if (ImGui::SliderFloat("##border thickness", &settings->widgetBorderThickness, 0.0f, 12.0f, "%.0f", ImGuiSliderFlags_NoInput)) {
                    settings->widgetBorderThickness = std::round(settings->widgetBorderThickness);
                    settings->widgetBorderThickness = std::clamp(settings->widgetBorderThickness, 0.0f, 12.0f);
                    Settings::Settings["widgetBorderThickness"] = settings->widgetBorderThickness;
                    Settings::Save(SettingsPath);
                }
                ImGui::SameLine();
                ImGui::SetNextItemWidth(60.0f);
                if (ImGui::InputFloat("border thickness", &settings->widgetBorderThickness, 1.0f, 1.0f, "%.0f")) {
                    settings->widgetBorderThickness = std::clamp(settings->widgetBorderThickness, 0.0f, 12.0f);
                    Settings::Settings["widgetBorderThickness"] = settings->widgetBorderThickness;
                    Settings::Save(SettingsPath);
                }

                ImGui::SetNextItemWidth(200.0f);
                if (ImGui::SliderFloat("##widget height", &settings->widgetHeight, 0.0f, 900.0f, "%.0f", ImGuiSliderFlags_NoInput)) {
                    settings->widgetHeight = std::round(settings->widgetHeight);
                    settings->widgetHeight = std::clamp(settings->widgetHeight, 0.0f, 900.0f);
                    Settings::Settings["WidgetHeight"] = settings->widgetHeight;
                    Settings::Save(SettingsPath);
                }
                ImGui::SameLine();
                ImGui::SetNextItemWidth(60.0f);
                if (ImGui::InputFloat("widget height", &settings->widgetHeight, 1.0f, 1.0f, "%.0f")) {
                    settings->widgetHeight = std::clamp(settings->widgetHeight, 0.0f, 900.0f);
                    Settings::Settings["WidgetHeight"] = settings->widgetHeight;
                    Settings::Save(SettingsPath);
                }

                // Widget Width controls (now discrete)
                ImGui::SetNextItemWidth(200.0f);
                if (ImGui::SliderFloat("##widget width", &settings->widgetWidth, 0.0f, 900.0f, "%.0f", ImGuiSliderFlags_NoInput)) {
                    settings->widgetWidth = std::round(settings->widgetWidth);
                    settings->widgetWidth = std::clamp(settings->widgetWidth, 0.0f, 900.0f);
                    Settings::Settings["WidgetWidth"] = settings->widgetWidth;
                    Settings::Save(SettingsPath);
                }
                ImGui::SameLine();
                ImGui::SetNextItemWidth(60.0f);
                if (ImGui::InputFloat("widget width", &settings->widgetWidth, 1.0f, 1.0f, "%.0f")) {
                    settings->widgetWidth = std::clamp(settings->widgetWidth, 0.0f, 900.0f);
                    Settings::Settings["WidgetWidth"] = settings->widgetWidth;
                    Settings::Save(SettingsPath);
                }
            }

            // Text Vertical Alignment controls (now discrete)
            ImGui::SetNextItemWidth(200.0f);
            if (ImGui::SliderFloat("##text vertical align", &settings->textVerticalAlignOffset, -50.0f, 50.0f, "%.0f", ImGuiSliderFlags_NoInput)) {
                settings->textVerticalAlignOffset = std::round(settings->textVerticalAlignOffset);
                settings->textVerticalAlignOffset = std::clamp(settings->textVerticalAlignOffset, -50.0f, 50.0f);
                Settings::Settings["TextVerticalAlignOffset"] = settings->textVerticalAlignOffset;
                Settings::Save(SettingsPath);
            }
            ImGui::SameLine();
            ImGui::SetNextItemWidth(60.0f);
            if (ImGui::InputFloat("text vertical align", &settings->textVerticalAlignOffset, 1.0f, 1.0f, "%.0f")) {
                settings->textVerticalAlignOffset = std::clamp(settings->textVerticalAlignOffset, -50.0f, 50.0f);
                Settings::Settings["TextVerticalAlignOffset"] = settings->textVerticalAlignOffset;
                Settings::Save(SettingsPath);
            }

            // Text Horizontal Alignment controls (now discrete)
            ImGui::SetNextItemWidth(200.0f);
            if (ImGui::SliderFloat("##text horizontal align", &settings->textHorizontalAlignOffset, -50.0f, 50.0f, "%.0f", ImGuiSliderFlags_NoInput)) {
                settings->textHorizontalAlignOffset = std::round(settings->textHorizontalAlignOffset);
                settings->textHorizontalAlignOffset = std::clamp(settings->textHorizontalAlignOffset, -50.0f, 50.0f);
                Settings::Settings["TextHorizontalAlignOffset"] = settings->textHorizontalAlignOffset;
                Settings::Save(SettingsPath);
            }
            ImGui::SameLine();
            ImGui::SetNextItemWidth(60.0f);
            if (ImGui::InputFloat("text horizontal align", &settings->textHorizontalAlignOffset, 1.0f, 1.0f, "%.0f")) {
                settings->textHorizontalAlignOffset = std::clamp(settings->textHorizontalAlignOffset, -50.0f, 50.0f);
                Settings::Settings["TextHorizontalAlignOffset"] = settings->textHorizontalAlignOffset;
                Settings::Save(SettingsPath);
            }

            bool colorsChanged = false;

            if (ImGui::ColorEdit4("widget border color", (float*)&settings->colors.widgetBorder))
            {
                Settings::Settings["widgetBorder"] = settings->colors.widgetBorder;
                colorsChanged = true;
            }

            if (ImGui::ColorEdit4("red background color", (float*)&settings->colors.redBackground))
            {
                Settings::Settings["redBackground"] = settings->colors.redBackground;
                colorsChanged = true;
            }

            if (ImGui::ColorEdit4("green background color", (float*)&settings->colors.greenBackground))
            {
                Settings::Settings["greenBackground"] = settings->colors.greenBackground;
                colorsChanged = true;
            }

            if (ImGui::ColorEdit4("blue background color", (float*)&settings->colors.blueBackground))
            {
                Settings::Settings["blueBackground"] = settings->colors.blueBackground;
                colorsChanged = true;
            }

            if (ImGui::ColorEdit4("red label color", (float*)&settings->colors.redText))
            {
                Settings::Settings["redText"] = settings->colors.redText;
                colorsChanged = true;
            }

            if (ImGui::ColorEdit4("green label color", (float*)&settings->colors.greenText))
            {
                Settings::Settings["greenText"] = settings->colors.greenText;
                colorsChanged = true;
            }

            if (ImGui::ColorEdit4("blue label color", (float*)&settings->colors.blueText))
            {
                Settings::Settings["blueText"] = settings->colors.blueText;
                colorsChanged = true;
            }
            if (ImGui::BeginMenu("In Combat Style")) {
                bool colorsChangedCombat = false;

                if (ImGui::ColorEdit4("widget border color", (float*)&settings->colors.widgetBorderCombat))
                {
                    Settings::Settings["widgetBorderCombat"] = settings->colors.widgetBorderCombat;
                    colorsChangedCombat = true;
                }
                if (ImGui::ColorEdit4("red background color", (float*)&settings->colors.redBackgroundCombat)) {
                    Settings::Settings["redBackgroundCombat"] = settings->colors.redBackgroundCombat;
                    colorsChangedCombat = true;
                }
                if (ImGui::ColorEdit4("green background color", (float*)&settings->colors.greenBackgroundCombat)) {
                    Settings::Settings["greenBackgroundCombat"] = settings->colors.greenBackgroundCombat;
                    colorsChangedCombat = true;
                }
                if (ImGui::ColorEdit4("blue background color", (float*)&settings->colors.blueBackgroundCombat)) {
                    Settings::Settings["blueBackgroundCombat"] = settings->colors.blueBackgroundCombat;
                    colorsChangedCombat = true;
                }
                if (ImGui::ColorEdit4("red label color", (float*)&settings->colors.redTextCombat)) {
                    Settings::Settings["redTextCombat"] = settings->colors.redTextCombat;
                    colorsChangedCombat = true;
                }
                if (ImGui::ColorEdit4("green label color", (float*)&settings->colors.greenTextCombat)) {
                    Settings::Settings["greenTextCombat"] = settings->colors.greenTextCombat;
                    colorsChangedCombat = true;
                }
                if (ImGui::ColorEdit4("blue label color", (float*)&settings->colors.blueTextCombat)) {
                    Settings::Settings["blueTextCombat"] = settings->colors.blueTextCombat;
                    colorsChangedCombat = true;
                }
                if (colorsChangedCombat)
                    Settings::Save(SettingsPath);
                ImGui::EndMenu();
            }

            // Save only once if any color has changed.
            if (colorsChanged)
            {
                Settings::Save(SettingsPath);
            }


            ImGui::EndMenu();
        }
    }


    void WidgetWindow::RenderSimpleRatioBar(
        const std::vector<float>& counts,
        const std::vector<ImVec4>& colors,
        const ImVec2& size,
        const std::vector<const char*>& texts,
        ImTextureID statIcon,
        const WidgetWindowSettings* settings,
        const std::vector<ImVec4>& textColors,
        const std::vector<size_t>& teamIndices
    )
    {
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
 
        // Get current position in the window
        const ImVec2 p = ImGui::GetCursorScreenPos();

        float currentFrameRounding = ImGui::GetStyle().FrameRounding;
        float rounding = (currentFrameRounding > 0.0f)
            ? currentFrameRounding
            : static_cast<float>(settings->widgetRoundness);

        // Choose the correct font
        if (settings->largerFont)
            ImGui::PushFont((ImFont*)NexusLink->FontBig);
        else
            ImGui::PushFont((ImFont*)NexusLink->Font);

        const float sz = ImGui::GetFontSize();
        const float iconWidth = sz;
        const float iconHeight = sz;

        const size_t numTeams = counts.size();
        float total = std::accumulate(counts.begin(), counts.end(), 0.0f);
        std::vector<float> fractions(numTeams);
        if (total <= 0.0f) {
            const float equalFrac = (numTeams > 0) ? (1.0f / numTeams) : 0.0f;
            std::fill(fractions.begin(), fractions.end(), equalFrac);
        }
        else {
            for (size_t i = 0; i < numTeams; ++i) {
                fractions[i] = counts[i] / total;
            }
        }

        std::vector<ImU32> colorsU32;
        colorsU32.reserve(numTeams);
        for (const auto& color : colors) {
            colorsU32.push_back(ImGui::ColorConvertFloat4ToU32(color));
        }

        // Basic layout
        float x = p.x;
        float y = p.y;
        const float width = size.x;
        const float height = size.y;

        const float leftPadding = 10.0f;
        const float rightPadding = 5.0f;
        const float interPadding = 5.0f;

        float iconAreaWidth = 0.0f;
        float barX = x;
        const bool hasIcon = (settings->showWidgetIcon && statIcon != nullptr);
        if (hasIcon) {
            iconAreaWidth = leftPadding + iconWidth + rightPadding;
            barX = x + iconAreaWidth + interPadding;
        }

        if (hasIcon)
        {
            float iconBGWidth = iconAreaWidth + interPadding;

            draw_list->AddRectFilled(
                ImVec2(x, y),
                ImVec2(x + iconBGWidth, y + height),
                IM_COL32(0, 0, 0, 230),
                rounding,
                ImDrawCornerFlags_Left
            );

            const float iconX = x + leftPadding;
            const float iconY = y + (height - iconHeight) * 0.5f;
            draw_list->AddImage(
                statIcon,
                ImVec2(iconX, iconY),
                ImVec2(iconX + iconWidth, iconY + iconHeight)
            );
        }

        const float barWidth = width - ((hasIcon ? (iconAreaWidth + interPadding) : 0.0f));

        constexpr float kZeroThreshold = 0.0001f;
        std::vector<size_t> activeIndices;
        activeIndices.reserve(numTeams);
        for (size_t i = 0; i < numTeams; ++i) {
            if (fractions[i] * barWidth >= kZeroThreshold) {
                activeIndices.push_back(i);
            }
        }

        float x_start = barX;
        for (size_t j = 0; j < activeIndices.size(); ++j)
        {
            size_t idx = activeIndices[j];
            float section_width = barWidth * fractions[idx];
            if (section_width < kZeroThreshold) {
                continue;
            }

            float x_start_rounded = IM_ROUND(x_start);
            float x_end_rounded = IM_ROUND(x_start + section_width);

            int corners = 0;
            bool isFirst = (j == 0);
            bool isLast = (j == activeIndices.size() - 1);

            if (isFirst && !hasIcon) {
                corners |= ImDrawCornerFlags_Left;
            }
            if (isLast) {
                corners |= ImDrawCornerFlags_Right;
            }

            // Draw the colored rectangle
            draw_list->AddRectFilled(
                ImVec2(x_start_rounded, y),
                ImVec2(x_end_rounded, y + height),
                colorsU32[idx],
                rounding,
                corners
            );

            // Draw text if there's room
            const ImVec2 textSize = ImGui::CalcTextSize(texts[idx]);
            float sectionDrawWidth = x_end_rounded - x_start_rounded;
            if (sectionDrawWidth >= textSize.x) {
                float text_center_x = x_start_rounded + (sectionDrawWidth - textSize.x) * 0.5f + settings->textHorizontalAlignOffset;
                float center_y = y + (height - textSize.y) * 0.5f + settings->textVerticalAlignOffset;

                size_t origTeam = teamIndices[j];
                draw_list->AddText(
                    ImVec2(text_center_x, center_y),
                    ImGui::ColorConvertFloat4ToU32(textColors[origTeam]),
                    texts[idx]
                );
            }

            x_start += section_width;
        }

        // Optional: draw a border around the entire widget
        if (settings->widgetBorderThickness > 0.0f) {
            draw_list->AddRect(
                ImVec2(x, y),
                ImVec2(x + width, y + height),
                ImGui::ColorConvertFloat4ToU32(settings->colors.widgetBorder),
                rounding,
                ImDrawCornerFlags_All,
                settings->widgetBorderThickness
            );
        }

        // Reserve the layout space so ImGui doesn't overlap subsequent items
        ImGui::Dummy(size);
        ImGui::PopFont();

    }

    void WidgetWindow::RenderPieChart(
        HINSTANCE hSelf,
        const std::vector<float>& counts,
        const std::vector<ImVec4>& colors,
        const ImVec2& size,
        const WidgetWindowSettings* settings,
        const std::vector<const char*>& texts,
        const std::vector<ImVec4>& textColors) {

        // Load pie chart textures if not already loaded
        if (!PieBackground) {
            PieBackground = APIDefs->Textures.GetOrCreateFromResource("PIE_BACKGROUND", PIE_BACKGROUND, hSelf);
        }
        if (!PieGlobe) {
            PieGlobe = APIDefs->Textures.GetOrCreateFromResource("PIE_GLOBE", PIE_GLOBE, hSelf);
        }
        if (!PieSeparator) {
            PieSeparator = APIDefs->Textures.GetOrCreateFromResource("PIE_SEPARATOR", PIE_SEPARATOR, hSelf);
        }

        // Calculate ratios from counts
        float total = 0.0f;
        for (float count : counts) {
            total += count;
        }

        if (total <= 0.0f) {
            ImGui::Text("No data available.");
            return;
        }

        std::vector<float> ratios;
        for (float count : counts) {
            ratios.push_back(count / total);
        }

        // Convert ImVec4 colors to ImU32
        std::vector<ImU32> colorsU32;
        for (const auto& color : colors) {
            colorsU32.push_back(ImGui::ColorConvertFloat4ToU32(color));
        }

        // Animation state for smooth transitions
        static std::vector<float> previousRatios;
        static std::vector<float> targetRatios;
        static float transitionStartTime = 0.0f;
        static const float transitionDuration = 1.5f; // 1.5 seconds for smooth transition

        std::vector<float> displayRatios = ratios;
        float globalRotation = 0.0f;
        bool inCombat = MumbleLink->Context.IsInCombat;
        float time = ImGui::GetTime();

        // Track combat state changes
        static bool wasInCombat = false;
        static float leftCombatTime = 0.0f;
        static float enteredCombatTime = 0.0f;

        // Detect entering combat
        if (!wasInCombat && inCombat) {
            enteredCombatTime = time;
        }
        // Detect leaving combat
        else if (wasInCombat && !inCombat) {
            leftCombatTime = time;
        }
        wasInCombat = inCombat;

        if (inCombat) {
            // Apply sine wave variations directly (no ImAnimate smoothing needed)
            for (size_t i = 0; i < ratios.size(); ++i) {
                // Each slice gets a different phase offset and frequency (slower now)
                float frequency = 0.5f + (i * 0.15f); // Slower speeds for each slice
                float phase = i * 2.0f; // Different starting points

                // Large variation: ±30% of the slice's value
                float variation = sinf(time * frequency + phase) * 0.3f;
                displayRatios[i] = ratios[i] * (1.0f + variation);

                // Ensure no negative values
                if (displayRatios[i] < 0.0f) displayRatios[i] = 0.0f;
            }

            // Normalize animated ratios to ensure they sum to 1.0
            // This ensures slices always fill the entire circle with no gaps
            float animatedTotal = 0.0f;
            for (float ratio : displayRatios) {
                animatedTotal += ratio;
            }
            if (animatedTotal > 0.0f) {
                for (size_t i = 0; i < displayRatios.size(); ++i) {
                    displayRatios[i] = displayRatios[i] / animatedTotal;
                }
            }

            // Add global rotation to make it appear to grow/shrink from all sides
            // Slow rotation that cycles through different starting angles
            globalRotation = sinf(time * 0.3f) * 0.5f; // Oscillate ±0.5 radians (~±28 degrees)
        }

        // Determine chart size (window is already square from pieChartSize setting)
        float baseChartSize = size.x;  // size.x == size.y for pie chart mode
        float displayChartSize = baseChartSize;  // This will be modified by animations

        // Check for event-triggered animations
        bool hideLabelsForAnimation = false;

        // New log detected animation: Subtle fade for 1 second
        float newLogTime = newLogDetectedTime.load();
        float newLogAlpha = 1.0f;
        if (newLogTime > 0.0f) {
            float timeSinceDetection = time - newLogTime;
            if (timeSinceDetection < 1.0f) {
                // Subtle fade: fade to 80% and back
                float fadeProgress = timeSinceDetection / 1.0f;
                newLogAlpha = 0.8f + 0.2f * cosf(fadeProgress * 3.14159265358979323846f);

                hideLabelsForAnimation = true;

                // Start transition when new log is detected
                if (previousRatios.empty() || previousRatios.size() != ratios.size()) {
                    previousRatios = ratios;
                    targetRatios = ratios;
                }
            } else {
                // Animation finished, reset trigger
                newLogDetectedTime.store(0.0f);
            }
        }

        // Parse complete animation: Initiate smooth transition and subtle single pulse
        float parseTime = parseCompleteTime.load();
        if (parseTime > 0.0f) {
            float timeSinceComplete = time - parseTime;

            // Initialize transition on first frame
            if (timeSinceComplete < 0.05f && targetRatios != ratios) {
                // Save current display values as starting point
                previousRatios = (displayRatios.size() == ratios.size()) ? displayRatios : ratios;
                targetRatios = ratios;
                transitionStartTime = time;
            }

            // Single subtle pulse for 0.8 seconds
            if (timeSinceComplete < 0.8f) {
                float pulseProgress = timeSinceComplete / 0.8f;
                float pulseAmount = sinf(pulseProgress * 3.14159265358979323846f); // Single sine wave
                float scaleFactor = 1.0f + pulseAmount * 0.08f; // Up to 8% larger
                displayChartSize *= scaleFactor;  // Only modify display size, not base size

                hideLabelsForAnimation = true;
            } else if (timeSinceComplete > transitionDuration + 0.8f) {
                // Animation and transition finished, reset trigger
                parseCompleteTime.store(0.0f);
            }
        }

        // Entered combat animation: Quick bright flash for 0.5 seconds
        float enteredCombatBrightness = 1.0f;
        if (enteredCombatTime > 0.0f) {
            float timeSinceEnteredCombat = time - enteredCombatTime;
            if (timeSinceEnteredCombat < 0.5f) {
                // Quick flash: bright at start, fade out
                float flashProgress = timeSinceEnteredCombat / 0.5f;
                // Start at 150% brightness, fade to 100%
                enteredCombatBrightness = 1.0f + (1.0f - flashProgress) * 0.5f;
            } else {
                // Animation finished, reset
                enteredCombatTime = 0.0f;
            }
        }

        // Left combat animation: Quick bright flash for 0.5 seconds
        float leftCombatBrightness = 1.0f;
        if (leftCombatTime > 0.0f) {
            float timeSinceLeftCombat = time - leftCombatTime;
            if (timeSinceLeftCombat < 0.5f) {
                // Quick flash: bright at start, fade out
                float flashProgress = timeSinceLeftCombat / 0.5f;
                // Start at 150% brightness, fade to 100%
                leftCombatBrightness = 1.0f + (1.0f - flashProgress) * 0.5f;
            } else {
                // Animation finished, reset
                leftCombatTime = 0.0f;
            }
        }

        // Combine both combat state animations
        float combatBrightness = (std::max)(enteredCombatBrightness, leftCombatBrightness);

        // Smooth transition from previous to new ratios
        float transitionProgress = 0.0f;
        if (transitionStartTime > 0.0f && time - transitionStartTime < transitionDuration) {
            transitionProgress = (time - transitionStartTime) / transitionDuration;
            // Use smooth easing (ease-in-out)
            transitionProgress = transitionProgress * transitionProgress * (3.0f - 2.0f * transitionProgress);

            // Interpolate from previous to target ratios
            if (!inCombat && previousRatios.size() == ratios.size() && targetRatios.size() == ratios.size()) {
                for (size_t i = 0; i < ratios.size(); ++i) {
                    displayRatios[i] = previousRatios[i] + (targetRatios[i] - previousRatios[i]) * transitionProgress;
                }
            }
        } else if (transitionStartTime > 0.0f) {
            // Transition complete
            transitionStartTime = 0.0f;
            previousRatios = targetRatios;
        }

        // Apply animations to colors
        std::vector<ImU32> displayColors = colorsU32;
        if (newLogAlpha < 1.0f || combatBrightness > 1.0f) {
            for (size_t i = 0; i < displayColors.size(); ++i) {
                ImU32 color = colorsU32[i];
                // Extract RGBA components
                int r = (color >> 0) & 0xFF;
                int g = (color >> 8) & 0xFF;
                int b = (color >> 16) & 0xFF;
                int a = (color >> 24) & 0xFF;

                // Apply alpha fade from new log detection
                if (newLogAlpha < 1.0f) {
                    a = static_cast<int>(a * newLogAlpha);
                }

                // Apply brightness boost from combat state changes
                if (combatBrightness > 1.0f) {
                    r = (std::min)(255, static_cast<int>(r * combatBrightness));
                    g = (std::min)(255, static_cast<int>(g * combatBrightness));
                    b = (std::min)(255, static_cast<int>(b * combatBrightness));
                }

                // Reconstruct color
                displayColors[i] = IM_COL32(r, g, b, a);
            }
        }

        // Setup pie chart data
        gui::PieChartData pieData;
        pieData.ratios = displayRatios;
        pieData.colors = displayColors;
        pieData.globalRotation = globalRotation; // Rotate entire pie chart

        // Apply combined effects to background tint
        int bgBrightness = static_cast<int>(255 * combatBrightness);
        bgBrightness = (std::min)(255, bgBrightness);
        pieData.backgroundTint = IM_COL32(bgBrightness, bgBrightness, bgBrightness, static_cast<int>(newLogAlpha * 255));

        pieData.background = PieBackground ? PieBackground->Resource : nullptr;
        pieData.globe = PieGlobe ? PieGlobe->Resource : nullptr;
        pieData.separator = PieSeparator ? PieSeparator->Resource : nullptr;

        // Get drawing position and draw list
        ImVec2 baseDrawPos = ImGui::GetCursorScreenPos();

        // Calculate center based on base size (for consistent positioning)
        ImVec2 center = ImVec2(baseDrawPos.x + baseChartSize * 0.5f, baseDrawPos.y + baseChartSize * 0.5f);

        // For font calculations, always use base size (not pulsed size)
        float radius = baseChartSize * 0.5f;

        // Adjust draw position to center the pulsed chart (so it grows from center, not top-left)
        float sizeDiff = displayChartSize - baseChartSize;
        ImVec2 displayDrawPos = ImVec2(baseDrawPos.x - sizeDiff * 0.5f, baseDrawPos.y - sizeDiff * 0.5f);

        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        // Draw the pie chart using display size
        gui::DrawLayeredPieChart(pieData, displayDrawPos, displayChartSize);

        // Draw text labels inside each slice with smart positioning and font fitting
        // Hide labels during combat animation or event animations
        if (!inCombat && !hideLabelsForAnimation) {
            float currentAngle = 0.0f;

            for (size_t i = 0; i < displayRatios.size() && i < texts.size() && i < textColors.size(); ++i) {
            // Skip rendering for zero values
            if (displayRatios[i] <= 0.0f) {
                continue;
            }

            // Calculate the middle angle of this slice
            float sliceAngle = displayRatios[i] * 2.0f * 3.14159265358979323846f;
            float midAngle = currentAngle + sliceAngle * 0.5f;

            // Dynamic label radius based on slice size
            // Positioned more conservatively to avoid edge protrusion
            float baseLabelRadius;
            if (displayRatios[i] > 0.5f) {
                baseLabelRadius = radius * 0.35f;  // Closer to center for large slices
            }
            else if (displayRatios[i] < 0.15f) {
                baseLabelRadius = radius * 0.55f;  // Moderate position for small slices
            }
            else {
                baseLabelRadius = radius * 0.5f;  // Middle position for normal slices
            }

            // Alternate radius offset to prevent labels from being on same horizontal plane
            // This allows some protrusion without labels touching each other
            float radiusOffset = (i % 2 == 0) ? 0.0f : radius * 0.08f;
            float labelRadius = baseLabelRadius + radiusOffset;

            // Estimate available space for the text
            // Arc length at label radius for this slice
            float arcLength = labelRadius * sliceAngle;

            // Calculate actual radial space based on label position
            // Space from label center to edge of circle
            float distanceToEdge = radius - labelRadius;
            // Available radial space (can use space on both sides of label position)
            float radialSpace = distanceToEdge * 2.1f;  // Radial space for text (increased by ~5%)

            // Scale factor based on overall chart size (100px = 0.4x, 250px = 1.0x, 500px = 2.0x)
            float sizeScale = baseChartSize / 250.0f;

            // Get the actual text length to adjust tolerance
            size_t textLength = strlen(texts[i]);

            // Base tolerance by text length - increased by ~5%
            float lengthTolerance = textLength <= 2 ? 2.1f : (textLength == 3 ? 1.7f : 1.4f);

            // Scale tolerance based on chart size - smaller charts need more conservative sizing
            float chartSizeFactor = 1.0f;
            if (sizeScale < 0.6f) {
                // Very small charts: reduce tolerance significantly
                chartSizeFactor = 0.7f;
            }
            else if (sizeScale < 1.0f) {
                // Small charts: reduce tolerance moderately
                chartSizeFactor = 0.85f;
            }
            else if (sizeScale > 2.0f) {
                // Very large charts: can be more generous
                chartSizeFactor = 1.2f;
            }

            lengthTolerance *= chartSizeFactor;

            // Try fonts from largest to smallest until one fits
            ImFont* fontsToTry[] = {
                MenomoniaSansMassive,       // 0: 80pt
                MenomoniaSansExtraHuge,     // 1: 64pt
                MenomoniaSansHuge,          // 2: 50pt
                MenomoniaSansExtraLarge,    // 3: 40pt
                MenomoniaSansLarge,         // 4: 32pt
                MenomoniaSansMediumLarge,   // 5: 26pt
                MenomoniaSansMedium,        // 6: 20pt
                MenomoniaSansMediumish,     // 7: 18pt
                MenomoniaSansMediumSmall,   // 8: 17pt
                MenomoniaSansSmallMedium,   // 9: 15pt
                MenomoniaSansSmall,         // 10: 14pt
                MenomoniaSansVerySmall,     // 11: 12pt
                MenomoniaSansExtraSmall     // 12: 10pt
            };
            ImFont* selectedFont = nullptr;
            ImVec2 textSize;

            // Cap max font size based on chart size and text length
            // Smaller charts need caps even for short text to prevent overflow
            int maxFontIndex = 0;
            if (textLength >= 4) {
                if (sizeScale < 0.6f) {
                    // Very small charts (<150px): cap at MediumLarge (26pt)
                    maxFontIndex = 5;
                }
                else if (sizeScale < 1.0f) {
                    // Small charts (<250px): cap at Large (32pt)
                    maxFontIndex = 4;
                }
                else if (sizeScale < 1.5f) {
                    // Medium charts (<375px): cap at ExtraLarge (40pt)
                    maxFontIndex = 3;
                }
                // Large charts (>=375px): no cap
            }
            else if (textLength == 3) {
                if (sizeScale < 0.6f) {
                    // Very small charts: cap at Large (32pt)
                    maxFontIndex = 4;
                }
                else if (sizeScale < 1.0f) {
                    // Small charts: cap at ExtraLarge (40pt)
                    maxFontIndex = 3;
                }
                // Larger charts: no cap
            }
            else if (textLength <= 2) {
                // Short text also needs caps at very small sizes
                if (sizeScale < 0.6f) {
                    // Very small charts: cap at ExtraLarge (40pt)
                    maxFontIndex = 3;
                }
                else if (sizeScale < 1.0f) {
                    // Small charts: cap at Huge (50pt)
                    maxFontIndex = 2;
                }
                // Larger charts: no cap (can use up to 80pt)
            }

            for (int fontIdx = maxFontIndex; fontIdx < 13; ++fontIdx) {
                ImFont* testFont = fontsToTry[fontIdx];
                if (!testFont) continue;

                ImGui::PushFont(testFont);
                textSize = ImGui::CalcTextSize(texts[i]);
                ImGui::PopFont();

                // Scale available space by chart size and text length tolerance
                float fillFactor = 1.0f;
                float scaledArcLength = arcLength * lengthTolerance * sizeScale * fillFactor;
                float scaledRadialSpace = radialSpace * lengthTolerance * sizeScale * fillFactor;

                // Check if text fits in the available space
                bool fitsWidth = textSize.x <= scaledArcLength;
                bool fitsHeight = textSize.y <= scaledRadialSpace;

                if (fitsWidth && fitsHeight) {
                    selectedFont = testFont;
                    break;
                }
            }

            // Fallback to minimum readable font if nothing was selected
            // Better to protrude slightly than be unreadable
            if (!selectedFont) {
                selectedFont = MenomoniaSansMediumSmall;  // 17pt minimum for readability
            }

            // Skip this label if no font is available (shouldn't happen, but safety check)
            if (!selectedFont) {
                currentAngle += sliceAngle;
                continue;
            }

            // Push selected font for rendering
            ImGui::PushFont(selectedFont);

            // Recalculate text size with selected font
            textSize = ImGui::CalcTextSize(texts[i]);

            // Position text at calculated radius
            ImVec2 labelCenter(
                center.x + labelRadius * cosf(midAngle),
                center.y + labelRadius * sinf(midAngle)
            );

            // Center the text at the label position
            ImVec2 textPos(
                labelCenter.x - textSize.x * 0.5f,
                labelCenter.y - textSize.y * 0.5f
            );

            // Apply vertical/horizontal alignment offsets from settings
            textPos.y += settings->textVerticalAlignOffset;
            textPos.x += settings->textHorizontalAlignOffset;

            // Draw drop shadow (offset text in dark color)
            ImVec2 shadowOffset(2.0f, 2.0f);
            draw_list->AddText(
                ImVec2(textPos.x + shadowOffset.x, textPos.y + shadowOffset.y),
                IM_COL32(0, 0, 0, 180),  // Semi-transparent black shadow
                texts[i]
            );

            // Draw the main text on top
            draw_list->AddText(
                textPos,
                ImGui::ColorConvertFloat4ToU32(textColors[i]),
                texts[i]
            );

            // Pop font (we always push one at this point)
            ImGui::PopFont();

            currentAngle += sliceAngle;
            }
        }

        // Reserve the layout space (use base size, not pulsed size)
        ImGui::Dummy(ImVec2(baseChartSize, baseChartSize));
    }


} // namespace wvwfightanalysis::gui