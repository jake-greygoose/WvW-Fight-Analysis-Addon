#include "nexus/Nexus.h"
#include "mumble/Mumble.h"
#include "imgui/imgui.h"
#include "gui/WindowRenderer.h"
#include "gui/windows/OptionsWindow.h"
#include "shared/Shared.h"
#include "settings/Settings.h"
#include "utils/Utils.h"
#include "parser/evtc_parser.h"
#include "integration/MursaatPanelIntegration.h"
#include "resource.h"

#include "autoversion.h"



// Function prototypes
void AddonLoad(AddonAPI* aApi);
void AddonUnload();
void AddonRender();
void AddonOptions();
void ReceiveFont(const char* aIdentifier, void* aFont);

AddonDefinition AddonDef = {};
std::unique_ptr<wvwfightanalysis::gui::WindowRenderer> g_windowRenderer;
std::unique_ptr<wvwfightanalysis::gui::OptionsWindow> g_optionsWindow;

struct FontEntry {
    const char* name;
    float size;
    ImFont** fontPtr;
};

static const FontEntry kFonts[] = {
    {"MENOMONIA_EXTRA_SMALL",    10.0f, &MenomoniaSansExtraSmall},
    {"MENOMONIA_VERY_SMALL",     12.0f, &MenomoniaSansVerySmall},
    {"MENOMONIA_SMALL",          14.0f, &MenomoniaSansSmall},
    {"MENOMONIA_SMALL_MEDIUM",   15.0f, &MenomoniaSansSmallMedium},
    {"MENOMONIA_MEDIUM_SMALL",   17.0f, &MenomoniaSansMediumSmall},
    {"MENOMONIA_MEDIUMISH",      18.0f, &MenomoniaSansMediumish},
    {"MENOMONIA_MEDIUM",         20.0f, &MenomoniaSansMedium},
    {"MENOMONIA_MEDIUM_LARGE",   26.0f, &MenomoniaSansMediumLarge},
    {"MENOMONIA_LARGE",          32.0f, &MenomoniaSansLarge},
    {"MENOMONIA_EXTRA_LARGE",    40.0f, &MenomoniaSansExtraLarge},
    {"MENOMONIA_HUGE",           50.0f, &MenomoniaSansHuge},
    {"MENOMONIA_EXTRA_HUGE",     64.0f, &MenomoniaSansExtraHuge},
    {"MENOMONIA_MASSIVE",        80.0f, &MenomoniaSansMassive},
};

static const char* const kKeybinds[] = {
    KB_WINDOW_TOGGLEVISIBLE,
    "KB_WIDGET_TOGGLEVISIBLE",
    "LOG_INDEX_UP",
    "LOG_INDEX_DOWN",
    "SHOW_SQUAD_PLAYERS_ONLY"
};

// Font loading callback
void ReceiveFont(const char* aIdentifier, void* aFont) {
    for (const auto& f : kFonts) {
        if (strcmp(aIdentifier, f.name) == 0) {
            *f.fontPtr = (ImFont*)aFont;
            return;
        }
    }
}


void AddonLoad(AddonAPI* aApi) {
    APIDefs = aApi;
    ImGui::SetCurrentContext((ImGuiContext*)APIDefs->ImguiContext);
    ImGui::SetAllocatorFunctions((void* (*)(size_t, void*))APIDefs->ImguiMalloc, (void(*)(void*, void*))APIDefs->ImguiFree);
    MumbleLink = (Mumble::Data*)APIDefs->DataLink.Get("DL_MUMBLE_LINK");
    NexusLink = (NexusLinkData*)APIDefs->DataLink.Get("DL_NEXUS_LINK");
    APIDefs->Renderer.Register(ERenderType_OptionsRender, AddonOptions);
    APIDefs->Renderer.Register(ERenderType_Render, AddonRender);
    GW2Root = APIDefs->Paths.GetGameDirectory();
    AddonPath = APIDefs->Paths.GetAddonDirectory("WvWFightAnalysis");
    SettingsPath = APIDefs->Paths.GetAddonDirectory("WvWFightAnalysis/settings.json");
    if (!std::filesystem::exists(AddonPath)) {
        firstInstall = true;
    }
    std::filesystem::create_directory(AddonPath);
    Settings::Load(SettingsPath);
    initMaps();

    // Load custom fonts in multiple sizes
    for (const auto& f : kFonts) {
        APIDefs->Fonts.AddFromResource(f.name, f.size, MENOMONIA_SANS_FONT, hSelf, ReceiveFont, nullptr);
    }

    g_windowRenderer = std::make_unique<wvwfightanalysis::gui::WindowRenderer>();
    g_optionsWindow = std::make_unique<wvwfightanalysis::gui::OptionsWindow>();

    for (auto& mainWindow : Settings::windowManager.mainWindows) {
        if (mainWindow->useNexusEscClose) {
            APIDefs->UI.RegisterCloseOnEscape(
                mainWindow->windowId.c_str(),
                &mainWindow->isEnabled
            );
        }
    }

    for (auto& widgetWindow : Settings::windowManager.widgetWindows) {
        if (widgetWindow->useNexusEscClose) {
            APIDefs->UI.RegisterCloseOnEscape(
                widgetWindow->windowId.c_str(),
                &widgetWindow->isEnabled
            );
        }
    }

    if (Settings::windowManager.aggregateWindow && Settings::windowManager.aggregateWindow->useNexusEscClose) {
        APIDefs->UI.RegisterCloseOnEscape(
            Settings::windowManager.aggregateWindow->windowId.c_str(),
            &Settings::windowManager.aggregateWindow->isEnabled
        );
    }

    for (const auto& kb : kKeybinds) {
        APIDefs->InputBinds.RegisterWithString(kb, ProcessKeybinds, "(null)");
    }
    directoryMonitorThread = std::thread(monitorDirectory, Settings::logHistorySize, Settings::pollIntervalMilliseconds);
    
    InitializeMursaatPanelIntegration();
    
    APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME, "Addon loaded successfully.");
}

void AddonUnload() {
    // IMPORTANT: Stop rendering BEFORE releasing resources
    APIDefs->Renderer.Deregister(AddonRender);
    APIDefs->Renderer.Deregister(AddonOptions);

    stopMonitoring = true;
    if (directoryMonitorThread.joinable()) {
        directoryMonitorThread.join();
    }
    if (initialParsingThread.joinable()) {
        initialParsingThread.join();
    }

    // Now safe to destroy window renderer
    g_windowRenderer.reset();
    g_optionsWindow.reset();

    // Release custom fonts after rendering has stopped
    for (const auto& f : kFonts) {
        APIDefs->Fonts.Release(f.name, ReceiveFont);
    }

    for (auto& mainWindow : Settings::windowManager.mainWindows) {
        if (mainWindow->useNexusEscClose) {
            APIDefs->UI.DeregisterCloseOnEscape(mainWindow->windowId.c_str());
        }
    }

    for (auto& widgetWindow : Settings::windowManager.widgetWindows) {
        if (widgetWindow->useNexusEscClose) {
            APIDefs->UI.DeregisterCloseOnEscape(widgetWindow->windowId.c_str());
        }
    }

    if (Settings::windowManager.aggregateWindow && Settings::windowManager.aggregateWindow->useNexusEscClose) {
        APIDefs->UI.DeregisterCloseOnEscape(Settings::windowManager.aggregateWindow->windowId.c_str());
    }

    for (const auto& kb : kKeybinds) {
        APIDefs->InputBinds.Deregister(kb);
    }
    
    CleanupMursaatPanelIntegration();
    APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME, "Addon unloaded successfully.");
}

void AddonRender() {
    if (g_windowRenderer) {
        g_windowRenderer->RenderAllWindows(hSelf);
    }
}

void AddonOptions()
{
    if (g_optionsWindow) {
        g_optionsWindow->Render();
    }
}

extern "C" __declspec(dllexport) AddonDefinition * GetAddonDef()
{
    AddonDef.Signature = -996748;
    AddonDef.APIVersion = NEXUS_API_VERSION;
    AddonDef.Name = ADDON_NAME;
    SeedAddonVersionFromBuild(AddonDef);
    AddonDef.Author = "Unreal.2358";
    AddonDef.Description = "WvW log analysis tool.";
    AddonDef.Load = AddonLoad;
    AddonDef.Unload = AddonUnload;
    AddonDef.Flags = EAddonFlags_None;
    AddonDef.Provider = EUpdateProvider_GitHub;
    AddonDef.UpdateLink = "https://github.com/jake-greygoose/WvW-Fight-Analysis-Addon";
    return &AddonDef;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH: hSelf = hModule; break;
    case DLL_PROCESS_DETACH: break;
    case DLL_THREAD_ATTACH: break;
    case DLL_THREAD_DETACH: break;
    }
    return TRUE;
}
