#include "gui/windows/OptionsWindow.h"
#include "settings/Settings.h"
#include "shared/Shared.h"
#include "utils/Utils.h"
#include "parser/evtc_parser.h"
#include "imgui/imgui.h"

namespace {

    void RenderNexusEscCloseCheckbox(BaseWindowSettings* window, const char* defaultName) {
        if (ImGui::Checkbox("Use Nexus Esc to Close", &window->useNexusEscClose)) {
            if (window->useNexusEscClose) {
                RegisterWindowForNexusEsc(window, defaultName);
            }
            else {
                UnregisterWindowFromNexusEsc(window, defaultName);
            }
            Settings::Save(SettingsPath);
        }
    }

    void RenderBaseWindowSettings(BaseWindowSettings* window) {
        if (ImGui::Checkbox("Hide In Combat", &window->hideInCombat)) {
            Settings::Save(SettingsPath);
        }
        if (ImGui::Checkbox("Hide Out Of Combat", &window->hideOutOfCombat)) {
            Settings::Save(SettingsPath);
        }
        if (ImGui::Checkbox("Lock Position", &window->disableMoving)) {
            Settings::Save(SettingsPath);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Disables moving & resizing.");
        }
        if (ImGui::Checkbox("Enable Mouse-Through", &window->disableClicking)) {
            Settings::Save(SettingsPath);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Window cannot be interacted with via mouse.");
        }
    }

} // namespace

namespace wvwfightanalysis::gui {

    void OptionsWindow::Render()
    {
        ImGui::Text("WvW Fight Analysis Settings");

        if (ImGui::BeginTabBar("AddonOptionsTabBar"))
        {
            // Global Settings
            if (ImGui::BeginTabItem("Global Settings"))
            {
                if (ImGui::InputText("Custom Log Path", Settings::LogDirectoryPathC, sizeof(Settings::LogDirectoryPathC))) {
                    Settings::LogDirectoryPath = Settings::LogDirectoryPathC;
                    Settings::Settings[CUSTOM_LOG_PATH] = Settings::LogDirectoryPath;
                    Settings::Save(SettingsPath);
                }

                int tempLogHistorySize = static_cast<int>(Settings::logHistorySize);
                if (ImGui::InputInt("Log History Size", &tempLogHistorySize)) {
                    Settings::logHistorySize = static_cast<size_t>(std::clamp(tempLogHistorySize, 1, 20));
                    Settings::Settings[LOG_HISTORY_SIZE] = Settings::logHistorySize;
                    Settings::Save(SettingsPath);
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("How many parsed logs to keep.");
                }

                if (ImGui::InputInt("Team Player Threshold", &Settings::teamPlayerThreshold)) {
                    Settings::teamPlayerThreshold = std::clamp(Settings::teamPlayerThreshold, 0, 100);
                    Settings::Settings[TEAM_PLAYER_THRESHOLD] = Settings::teamPlayerThreshold;
                    Settings::Save(SettingsPath);
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Set a minimum number of team players required to render team.");
                }

                if (ImGui::InputInt("Min Total Players", &Settings::minTotalPlayers)) {
                    Settings::minTotalPlayers = std::clamp(Settings::minTotalPlayers, 0, 50);
                    Settings::Settings[MIN_TOTAL_PLAYERS] = Settings::minTotalPlayers;
                    Settings::Save(SettingsPath);
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Skip parsing logs with fewer than this many total identified players.");
                }

                if (ImGui::InputInt("Min Total Deaths", &Settings::minTotalDeaths)) {
                    Settings::minTotalDeaths = std::clamp(Settings::minTotalDeaths, 0, 50);
                    Settings::Settings[MIN_TOTAL_DEATHS] = Settings::minTotalDeaths;
                    Settings::Save(SettingsPath);
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Skip parsing logs with fewer than this many total deaths.");
                }

                if (ImGui::InputInt("Min Total Downs", &Settings::minTotalDowns)) {
                    Settings::minTotalDowns = std::clamp(Settings::minTotalDowns, 0, 50);
                    Settings::Settings[MIN_TOTAL_DOWNS] = Settings::minTotalDowns;
                    Settings::Save(SettingsPath);
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Skip parsing logs with fewer than this many total downs.");
                }

                if (ImGui::InputInt("Min Combat Duration (s)", &Settings::minCombatDuration)) {
                    Settings::minCombatDuration = std::clamp(Settings::minCombatDuration, 0, 120);
                    Settings::Settings[MIN_COMBAT_DURATION] = Settings::minCombatDuration;
                    Settings::Save(SettingsPath);
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Skip parsing logs with combat duration shorter than this (seconds).");
                }

                if (ImGui::Checkbox("Show Alert On Log Parse", &Settings::showNewParseAlert)) {
                    Settings::Settings[SHOW_NEW_PARSE_ALERT] = Settings::showNewParseAlert;
                    Settings::Save(SettingsPath);
                }

                if (ImGui::Checkbox("Enable Wine Compatibility Mode", &Settings::forceLinuxCompatibilityMode)) {
                    Settings::Settings[FORCE_LINUX_COMPAT] = Settings::forceLinuxCompatibilityMode;
                    Settings::Save(SettingsPath);
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Wine/Proton doesn't support ReadDirectoryChangesW, use directory polling instead.");
                }

                if (Settings::forceLinuxCompatibilityMode) {
                    int tempPollIntervalMilliseconds = static_cast<int>(Settings::pollIntervalMilliseconds);
                    if (ImGui::InputInt("ms Polling Interval", &tempPollIntervalMilliseconds)) {
                        Settings::pollIntervalMilliseconds =
                            static_cast<size_t>(std::clamp(tempPollIntervalMilliseconds, 500, 10000));
                        Settings::Settings[POLL_INTERVAL_MILLISECONDS] = Settings::pollIntervalMilliseconds;
                        Settings::Save(SettingsPath);
                    }
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("Polling Interval when using Wine compatibility mode.");
                    }
                }

                if (ImGui::Checkbox("Enable Debug Logging", &Settings::debugStringsMode)) {
                    Settings::Settings[DEBUG_STRINGS_MODE] = Settings::debugStringsMode;
                    Settings::Save(SettingsPath);
                }

                const char* specIconOptions[] = {
                    "Simple White",
                    "White",
                    "Colored",
                    "Simple Colored"
                };
                int selectedSpecIconStyle = Settings::scrapperIconStyle;
                if (ImGui::Combo("Spec Icon Style", &selectedSpecIconStyle, specIconOptions, IM_ARRAYSIZE(specIconOptions))) {
                    Settings::scrapperIconStyle = std::clamp(selectedSpecIconStyle, 0, 3);
                    Settings::Settings[SCRAPPER_ICON_STYLE] = Settings::scrapperIconStyle;
                    InvalidateProfessionIconTextures();
                    Settings::Save(SettingsPath);
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Select which profession/spec icon variant to use.");
                }


                bool enabled = !isRestartInProgress.load();
                if (!enabled) {
                    ImVec4 disabledColor = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
                    ImGui::PushStyleColor(ImGuiCol_Button, disabledColor);
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, disabledColor);
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, disabledColor);
                }

                if (ImGui::Button("Restart Directory Monitoring") && enabled) {
                    isRestartInProgress.store(true);
                    std::thread([]() {
                        stopMonitoring = true;
                        if (directoryMonitorThread.joinable()) {
                            directoryMonitorThread.join();
                        }
                        stopMonitoring = false;
                        directoryMonitorThread = std::thread(
                            monitorDirectory,
                            Settings::logHistorySize,
                            Settings::pollIntervalMilliseconds
                        );
                        isRestartInProgress.store(false);
                        }).detach();
                }

                if (!enabled) {
                    ImGui::PopStyleColor(3);
                }

                ImGui::EndTabItem();
            }

            // Main Windows
            if (ImGui::BeginTabItem("Windows"))
            {
                ImGui::BeginGroup();
                if (ImGui::Button("Add Window")) {
                    Settings::windowManager.AddMainWindow();
                    Settings::Save(SettingsPath);
                }
                ImGui::SameLine();
                if (ImGui::Button("Remove Window") && Settings::windowManager.mainWindows.size() > 1) {
                    Settings::windowManager.RemoveMainWindow();
                    Settings::Save(SettingsPath);
                }
                ImGui::EndGroup();
                ImGui::Separator();

                int windowIndex = 0;
                for (auto& window : Settings::windowManager.mainWindows) {
                    ImGui::PushID(windowIndex);

                    if (ImGui::Checkbox("##Enabled", &window->isEnabled)) {
                        Settings::Save(SettingsPath);
                    }
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("Enable or disable this main window.");
                    }

                    ImGui::SameLine();

                    std::string displayName = window->windowName.empty() ?
                        "Main Window " + std::to_string(windowIndex + 1) : window->windowName;
                    bool nodeOpen = ImGui::TreeNodeEx(
                        "##Node",
                        ImGuiTreeNodeFlags_AllowItemOverlap,
                        "%s",
                        displayName.c_str()
                    );

                    if (nodeOpen) {

                        RenderNexusEscCloseCheckbox(window.get(), "WvW Fight Analysis");
                        RenderBaseWindowSettings(window.get());

                        ImGui::TreePop();
                    }

                    ImGui::PopID();
                    windowIndex++;
                }

                ImGui::EndTabItem();
            }

            // Widget Windows
            if (ImGui::BeginTabItem("Widgets"))
            {
                ImGui::BeginGroup();
                if (ImGui::Button("Add Widget")) {
                    Settings::windowManager.AddWidgetWindow();
                    Settings::Save(SettingsPath);
                }
                ImGui::SameLine();
                if (ImGui::Button("Remove Widget") && Settings::windowManager.widgetWindows.size() > 1) {
                    Settings::windowManager.RemoveWidgetWindow();
                    Settings::Save(SettingsPath);
                }
                ImGui::EndGroup();
                ImGui::Separator();

                int windowIndex = 0;
                for (auto& window : Settings::windowManager.widgetWindows) {
                    ImGui::PushID(windowIndex);

                    if (ImGui::Checkbox("##Enabled", &window->isEnabled)) {
                        Settings::Save(SettingsPath);
                    }
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("Enable or disable this widget window.");
                    }

                    ImGui::SameLine();

                    std::string displayName = window->windowName.empty() ?
                        "Widget Window " + std::to_string(windowIndex + 1) : window->windowName;
                    bool nodeOpen = ImGui::TreeNodeEx(
                        "##Node",
                        ImGuiTreeNodeFlags_AllowItemOverlap,
                        "%s",
                        displayName.c_str()
                    );

                    if (nodeOpen) {

                        RenderNexusEscCloseCheckbox(window.get(), "Team Ratio Bar");
                        RenderBaseWindowSettings(window.get());

                        ImGui::TreePop();
                    }

                    ImGui::PopID();
                    windowIndex++;
                }

                ImGui::EndTabItem();
            }

            // Aggregate Window
            if (ImGui::BeginTabItem("Aggregate Window"))
            {
                auto& window = Settings::windowManager.aggregateWindow;

                if (ImGui::Checkbox("Enabled", &window->isEnabled)) {
                    Settings::Save(SettingsPath);
                }

                RenderNexusEscCloseCheckbox(window.get(), "Aggregate Stats");
                RenderBaseWindowSettings(window.get());
                if (ImGui::Checkbox("Hide When Empty", &window->hideWhenEmpty)) {
                    Settings::Save(SettingsPath);
                }

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
    }

} // namespace wvwfightanalysis::gui
