#include "integration/MursaatPanelIntegration.h"

// Initialize settings
WvWMursaatWidgetSettings mursaatWidgetSettings;

// Helper function to get colored team text - MAKE IT STATIC to prevent collision
static ImVec4 GetMursaatWidgetTeamColor(const std::string& teamName) {
    if (teamName == "Blue" || teamName == "B") {
        return ImVec4(0.0f, 0.4f, 0.8f, 1.0f);
    }
    else if (teamName == "Red" || teamName == "R") {
        return ImVec4(0.8f, 0.2f, 0.2f, 1.0f);
    }
    else if (teamName == "Green" || teamName == "G") {
        return ImVec4(0.2f, 0.8f, 0.2f, 1.0f);
    }
    else {
        // Default color for unknown teams
        return ImVec4(0.8f, 0.8f, 0.2f, 1.0f);
    }
}

// Function to render the WvW widget
void RenderWvWWidget(RenderOptions options) {
    // Make sure we have logs to display
    if (parsedLogs.empty()) {
        ImGui::Text("No WvW logs");
        return;
    }

    // Get the current log data
    const auto& currentLogData = parsedLogs[currentLogIndex].data;

    // Find Green, Red, and Blue teams
    int greenCount = 0;
    int redCount = 0;
    int blueCount = 0;

    // Track which team is the player's team
    bool isGreenPlayerTeam = false;
    bool isRedPlayerTeam = false;
    bool isBluePlayerTeam = false;

    // Read team player counts
    for (const auto& [teamName, stats] : currentLogData.teamStats) {
        // Skip teams with fewer players than threshold
        if (stats.totalPlayers < Settings::teamPlayerThreshold) {
            continue;
        }

        // Skip non-squad teams if squad-only is enabled
        if (mursaatWidgetSettings.showSquadOnly && !stats.isPOVTeam) {
            continue;
        }

        // Count players by team color and check if it's the player's team
        if (teamName == "Green") {
            greenCount = stats.totalPlayers;
            isGreenPlayerTeam = stats.isPOVTeam;
        }
        else if (teamName == "Red") {
            redCount = stats.totalPlayers;
            isRedPlayerTeam = stats.isPOVTeam;
        }
        else if (teamName == "Blue") {
            blueCount = stats.totalPlayers;
            isBluePlayerTeam = stats.isPOVTeam;
        }
    }

    // Display the Green, Red, Blue player counts in their respective colors
    if (greenCount > 0) {
        ImGui::PushStyleColor(ImGuiCol_Text, GetMursaatWidgetTeamColor("Green"));
        ImGui::Text("G: %d%s", greenCount, isGreenPlayerTeam ? "*" : "");
        ImGui::PopStyleColor();

        // Only add same line if there are more teams to display
        if (redCount > 0 || blueCount > 0) {
            ImGui::SameLine();
        }
    }

    if (redCount > 0) {
        ImGui::PushStyleColor(ImGuiCol_Text, GetMursaatWidgetTeamColor("Red"));
        ImGui::Text("R: %d%s", redCount, isRedPlayerTeam ? "*" : "");
        ImGui::PopStyleColor();

        // Only add same line if there's another team to display
        if (blueCount > 0) {
            ImGui::SameLine();
        }
    }

    if (blueCount > 0) {
        ImGui::PushStyleColor(ImGuiCol_Text, GetMursaatWidgetTeamColor("Blue"));
        ImGui::Text("B: %d%s", blueCount, isBluePlayerTeam ? "*" : "");
        ImGui::PopStyleColor();
    }

    // If no teams were found or displayed
    if (greenCount == 0 && redCount == 0 && blueCount == 0) {
        ImGui::Text("No teams");
    }
}

// Function to render context menu
void RenderWvWWidgetContextMenu() {
    ImGui::Checkbox("Show Squad Only", &mursaatWidgetSettings.showSquadOnly);

    // Save settings when changed
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        Settings::Save(SettingsPath);
    }
}

// Function to calculate the widget width
float GetWvWWidgetWidth() {
    // Since the format is very simple (G: ##* R: ##* B: ##*), we can estimate
    // the width needed based on the characters
    float width = 160.0f;  // Slightly increased base width to account for stars

    return width;
}

// Handler for MursaatPanel ready event
void HandleMursaatPanelReady(void* eventArgs) {
    // Register our widget in a separate thread to avoid deadlocks
    std::thread registerThread(RegisterWvWWidgetWithMursaatPanel);
    registerThread.detach();
}

// Function to register our widget with MursaatPanel
void RegisterWvWWidgetWithMursaatPanel() {
    if (APIDefs && APIDefs->Events.Raise) {
        RegisterWidgetEvent* registerWidget = new RegisterWidgetEvent();
        registerWidget->id = "wvw_fight_analysis_widget";
        registerWidget->title = "WvW Teams";
        registerWidget->render = RenderWvWWidget;
        registerWidget->renderContextMenu = RenderWvWWidgetContextMenu;
        registerWidget->getWidth = GetWvWWidgetWidth;

        APIDefs->Events.Raise("EV_MURSAAT_REGISTER_WIDGET", registerWidget);
        APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME, "Registered widget with MursaatPanel");
    }
}

// Function to unregister our widget from MursaatPanel
void UnregisterWvWWidgetFromMursaatPanel() {
    if (APIDefs && APIDefs->Events.Raise) {
        UnregisterWidgetEvent* unregisterWidget = new UnregisterWidgetEvent();
        unregisterWidget->id = "wvw_fight_analysis_widget";

        APIDefs->Events.Raise("EV_MURSAAT_DEREGISTER_WIDGET", unregisterWidget);
        APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME, "Unregistered widget from MursaatPanel");
    }
}

// Function to initialize MursaatPanel integration
void InitializeMursaatPanelIntegration() {
    // Subscribe to MursaatPanel ready event
    if (APIDefs && APIDefs->Events.Subscribe) {
        APIDefs->Events.Subscribe("EV_MURSAAT_PANEL_READY", HandleMursaatPanelReady);
        APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME, "Subscribed to MursaatPanel ready event");
    }

    // Try to register now in case MursaatPanel is already loaded
    RegisterWvWWidgetWithMursaatPanel();
}

// Function to clean up MursaatPanel integration
void CleanupMursaatPanelIntegration() {
    // Unsubscribe from events
    if (APIDefs && APIDefs->Events.Unsubscribe) {
        APIDefs->Events.Unsubscribe("EV_MURSAAT_PANEL_READY", HandleMursaatPanelReady);
    }

    // Unregister our widget
    UnregisterWvWWidgetFromMursaatPanel();
}