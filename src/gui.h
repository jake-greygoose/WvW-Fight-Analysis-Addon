#pragma once
#include <cstdint>
#include <string>
#include "Shared.h"
#include "Settings.h"
#include "nexus/Nexus.h"
#include "mumble/Mumble.h"
#include "imgui/imgui.h"

// Helper rendering functions
void DrawBar(
    float frac,
    int count,
    const std::string& barRep,
    const SpecStats& stats,
    const MainWindowSettings* settings,
    const ImVec4& primaryColor,
    const ImVec4& secondaryColor,
    const std::string& eliteSpec,
    HINSTANCE hSelf
);

void RenderSimpleRatioBar(
    const std::vector<float>& counts,
    const std::vector<ImVec4>& colors,
    const ImVec2& size,
    const std::vector<const char*>& texts,
    ImTextureID statIcon,
    const WidgetWindowSettings* settings
);

// Core window rendering functions
void RenderMainWindow(MainWindowSettings* settings, HINSTANCE hSelf);
void RenderMainWindowSettingsPopup(MainWindowSettings* settings);

void RenderSpecializationWindow(MainWindowSettings* settings, HINSTANCE hSelf);
void RenderSpecializationSettingsPopup(MainWindowSettings* settings);
void RenderSpecializationBars(const TeamStats& teamData, const MainWindowSettings* settings, HINSTANCE hSelf);

void RenderWidgetWindow(WidgetWindowSettings* settings, HINSTANCE hSelf);
void RenderWidgetSettingsPopup(WidgetWindowSettings* settings);

void DrawAggregateStatsWindow(HINSTANCE hSelf);
void RenderAggregateSettingsPopup(AggregateWindowSettings* settings);

// Support functions
void RenderTeamData(const TeamStats& teamData, const MainWindowSettings* settings, HINSTANCE hSelf);
void UpdateAggregateStats(const ParsedData& data);

// Main orchestrator
void RenderAllWindows(HINSTANCE hSelf);