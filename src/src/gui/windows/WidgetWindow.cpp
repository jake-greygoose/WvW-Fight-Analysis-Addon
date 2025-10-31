#include "gui/windows/WidgetWindow.h"
#include "utils/Utils.h"
#include "resource.h"
#include "imgui/imgui_internal.h"
#include "thirdparty/imgui_positioning/imgui_positioning.h" 

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

        const ImVec2 widgetSize(settings->widgetWidth, settings->widgetHeight);
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
                    counts.reserve(teamsToDisplay.size());
                    colors.reserve(teamsToDisplay.size());
                    texts.reserve(teamsToDisplay.size());
                    for (const auto& team : teamsToDisplay) {
                        counts.push_back(team.count);
                        colors.push_back(team.backgroundColor);
                        texts.push_back(team.text.c_str());
                    }

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


} // namespace wvwfightanalysis::gui