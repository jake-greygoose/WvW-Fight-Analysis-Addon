#pragma once
#include <string>
#include <thread>
#include "imgui/imgui.h"
#include "shared/Shared.h"
#include "settings/Settings.h"

// MursaatPanel API definitions
struct RenderOptions {
    float iconWidth;
    float iconHeight;
    bool renderIcon;
};

typedef void (*RenderFunc)(RenderOptions options);
typedef void (*RenderContextMenuFunc)();
typedef float (*GetWidthFunc)();

struct RegisterWidgetEvent {
    const char* id;
    const char* title;
    RenderFunc render;
    RenderContextMenuFunc renderContextMenu;
    GetWidthFunc getWidth;
};

struct UnregisterWidgetEvent {
    const char* id;
};

// Widget configuration settings
struct WvWMursaatWidgetSettings {
    bool showTeamColors = true;
    bool showPlayerCounts = true;
    bool showSquadOnly = false;
    bool compact = false;
};

extern WvWMursaatWidgetSettings mursaatWidgetSettings;

// Forward declarations for the widget functions
void RenderWvWWidget(RenderOptions options);
void RenderWvWWidgetContextMenu();
float GetWvWWidgetWidth();

// Handler for MursaatPanel ready event
void HandleMursaatPanelReady(void* eventArgs);

// Function to register our widget with MursaatPanel
void RegisterWvWWidgetWithMursaatPanel();

// Function to unregister our widget from MursaatPanel
void UnregisterWvWWidgetFromMursaatPanel();

// Function to initialize MursaatPanel integration
void InitializeMursaatPanelIntegration();

// Function to clean up MursaatPanel integration
void CleanupMursaatPanelIntegration();