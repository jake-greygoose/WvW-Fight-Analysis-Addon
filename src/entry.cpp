#include "nexus/Nexus.h"
#include "mumble/Mumble.h"
#include "imgui/imgui.h"
#include "Shared.h"
#include "Settings.h"
#include "gui.h"
#include "utils.h"
#include "evtc_parser.h"


// Function prototypes
void AddonLoad(AddonAPI* aApi);
void AddonUnload();
void AddonRender();
void AddonOptions();
void ProcessKeybinds(const char* aIdentifier, bool aIsRelease);

AddonDefinition AddonDef = {};



void AddonLoad(AddonAPI* aApi)
{
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
   
    if (!std::filesystem::exists(AddonPath))
    {
        firstInstall = true;
    }

    std::filesystem::create_directory(AddonPath);
    Settings::Load(SettingsPath);

    if (Settings::useNexusEscClose) {
        APIDefs->UI.RegisterCloseOnEscape("WvW Fight Analysis", &Settings::IsAddonWindowEnabled);
    }

    APIDefs->InputBinds.RegisterWithString(KB_WINDOW_TOGGLEVISIBLE, ProcessKeybinds, "(null)");
    APIDefs->InputBinds.RegisterWithString("KB_WIDGET_TOGGLEVISIBLE", ProcessKeybinds, "(null)");

    APIDefs->InputBinds.RegisterWithString("LOG_INDEX_UP", ProcessKeybinds, "(null)");
    APIDefs->InputBinds.RegisterWithString("LOG_INDEX_DOWN", ProcessKeybinds, "(null)");
    APIDefs->InputBinds.RegisterWithString("SHOW_SQUAD_PLAYERS_ONLY", ProcessKeybinds, "(null)");

    initMaps();

    directoryMonitorThread = std::thread(monitorDirectory, Settings::logHistorySize, Settings::pollIntervalMilliseconds);

    APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME, "Addon loaded successfully.");
}

void AddonUnload()
{
    stopMonitoring = true;


    if (directoryMonitorThread.joinable())
    {
        directoryMonitorThread.join();
    }

    if (initialParsingThread.joinable())
    {
        initialParsingThread.join();
    }

    if (Settings::useNexusEscClose) {
        APIDefs->UI.DeregisterCloseOnEscape("WvW Fight Analysis");
    }

    APIDefs->Renderer.Deregister(AddonRender);
    APIDefs->Renderer.Deregister(AddonOptions);
    APIDefs->InputBinds.Deregister("KB_WINDOW_TOGGLEVISIBLE");
    APIDefs->InputBinds.Deregister("KB_WIDGET_TOGGLEVISIBLE");
    APIDefs->InputBinds.Deregister("LOG_INDEX_UP");
    APIDefs->InputBinds.Deregister("LOG_INDEX_DOWN");
    APIDefs->InputBinds.Deregister("SHOW_SQUAD_PLAYERS_ONLY");
    APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME, "Addon unloaded successfully.");
}

void ProcessKeybinds(const char* aIdentifier, bool aIsRelease)
{
    std::string str = aIdentifier;
    if (aIsRelease) return;

    if (str == "KB_WINDOW_TOGGLEVISIBLE")
    {
        Settings::IsAddonWindowEnabled = !Settings::IsAddonWindowEnabled;
        Settings::Save(SettingsPath);
    }
    else if (str == "KB_WIDGET_TOGGLEVISIBLE")
    {
        Settings::IsAddonWidgetEnabled = !Settings::IsAddonWidgetEnabled;
        Settings::Save(SettingsPath);
    }
    else if (str == "LOG_INDEX_DOWN") 
    {
        if (!parsedLogs.empty()) {
            if (currentLogIndex == 0) {
                currentLogIndex = static_cast<int>(parsedLogs.size()) - 1;
            }
            else {
                currentLogIndex--;
            }
        }
        else {
            APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME,
                ("Log Index: " + std::to_string(currentLogIndex)).c_str());
        }

    }
    else if (str == "LOG_INDEX_UP")
    {
        if (!parsedLogs.empty()) {
            currentLogIndex = (currentLogIndex + 1) % static_cast<int>(parsedLogs.size());
        }
        else {
            APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME,
                ("Log Index: " + std::to_string(currentLogIndex)).c_str());
        }
    }
    else if (str == "SHOW_SQUAD_PLAYERS_ONLY")
    {
        Settings::squadPlayersOnly = !Settings::squadPlayersOnly;
        Settings::Save(SettingsPath);
    }
}

void AddonRender()
{


    if (ImGui::Begin("Bar Stats Debug"))
    {
        // Display current bar stats
        ImGui::Text("Current Bar Stats (%zu entries):", Settings::barStats.size());

        for (size_t i = 0; i < Settings::barStats.size(); i++)
        {
            const auto& stat = Settings::barStats[i];

            ImGui::PushID(static_cast<int>(i));

            ImGui::Separator();
            ImGui::Text("Bar Stat %zu:", i + 1);

            // Display the values with labels
            ImGui::Text("Representation: %s", stat.representation.c_str());
            ImGui::Text("Primary Stat: %s", stat.primaryStat.c_str());
            ImGui::Text("Secondary Stat: %s", stat.secondaryStat.c_str());

            // Add a button to remove this entry
            if (ImGui::Button("Remove"))
            {
                Settings::barStats.erase(Settings::barStats.begin() + i);
                i--; // Adjust index after removal
                continue;
            }

            ImGui::PopID();
        }

        // Add new entry section
        ImGui::Separator();
        ImGui::Text("Add New Bar Stat:");

        // Static variables to hold new entry data
        static char newRep[128] = "";
        static char newPrimary[128] = "";
        static char newSecondary[128] = "";

        ImGui::InputText("New Representation", newRep, sizeof(newRep));
        ImGui::InputText("New Primary Stat", newPrimary, sizeof(newPrimary));
        ImGui::InputText("New Secondary Stat", newSecondary, sizeof(newSecondary));

        if (ImGui::Button("Add New Entry"))
        {
            if (strlen(newRep) > 0 && strlen(newPrimary) > 0 && strlen(newSecondary) > 0)
            {
                Settings::barStats.push_back(BarStat(newRep, newPrimary, newSecondary));

                // Clear the input fields
                newRep[0] = '\0';
                newPrimary[0] = '\0';
                newSecondary[0] = '\0';
            }
        }

        // Add buttons to save/load/reset
        ImGui::Separator();
        if (ImGui::Button("Save Settings"))
        {
            Settings::Save("settings.json"); // Adjust path as needed
        }

        ImGui::SameLine();
        if (ImGui::Button("Load Settings"))
        {
            Settings::Load("settings.json"); // Adjust path as needed
        }


        ImGui::End();
    }

    if (!NexusLink || !NexusLink->IsGameplay || !MumbleLink || MumbleLink->Context.IsMapOpen)
    {
        return;
    }

    if (
        MumbleLink->Context.MapType != Mumble::EMapType::WvW_EternalBattlegrounds &&
        MumbleLink->Context.MapType != Mumble::EMapType::WvW_BlueBorderlands &&
        MumbleLink->Context.MapType != Mumble::EMapType::WvW_GreenBorderlands &&
        MumbleLink->Context.MapType != Mumble::EMapType::WvW_RedBorderlands &&
        MumbleLink->Context.MapType != Mumble::EMapType::WvW_ObsidianSanctum &&
        MumbleLink->Context.MapType != Mumble::EMapType::WvW_EdgeOfTheMists &&
        MumbleLink->Context.MapType != Mumble::EMapType::WvW_Lounge
        ) {
        return;
    }

    if (MumbleLink->Context.IsInCombat && !Settings::showWindowInCombat)
    {
        return;
    }


    ratioBarSetup(hSelf);
    DrawAggregateStatsWindow(hSelf);
    RenderMainWindow(hSelf);

}

void AddonOptions()
{
    ImGui::Text("WvW Fight Analysis Settings");
    if (ImGui::Checkbox("Window Enabled##WvWFightAnalysis", &Settings::IsAddonWindowEnabled))
    {
        Settings::Settings[IS_ADDON_WINDOW_VISIBLE] = Settings::IsAddonWindowEnabled;
        Settings::Save(SettingsPath);
    }
    if (ImGui::Checkbox("Widget Enabled##WvWFightAnalysis", &Settings::IsAddonWidgetEnabled))
    {
        Settings::Settings[IS_ADDON_WIDGET_VISIBLE] = Settings::IsAddonWidgetEnabled;
        Settings::Save(SettingsPath);
    }
    if (ImGui::Checkbox("Aggregate Stats Enabled##WvWFightAnalysis", &Settings::IsAddonAggWindowEnabled))
    {
        Settings::Settings[IS_ADDON_AGG_WINDOW_VISIBLE] = Settings::IsAddonAggWindowEnabled;
        Settings::Save(SettingsPath);
    }
    if (ImGui::Checkbox("Hide Aggregate Stats Window When Empty##WvWFightAnalysis", &Settings::hideAggWhenEmpty))
    {
        Settings::Settings[HIDE_AGG_WINDOW_WHEN_EMPTY] = Settings::IsAddonAggWindowEnabled;
        Settings::Save(SettingsPath);
    }
    if (ImGui::Checkbox("Visible In Combat##WvWFightAnalysis", &Settings::showWindowInCombat))
    {
        Settings::Settings[IS_WINDOW_VISIBLE_IN_COMBAT] = Settings::showWindowInCombat;
        Settings::Save(SettingsPath);
    }
    if (ImGui::Checkbox("Show Alert On Log Parse##WvWFightAnalysis", &Settings::showNewParseAlert))
    {
        Settings::Settings[SHOW_NEW_PARSE_ALERT] = Settings::showNewParseAlert;
        Settings::Save(SettingsPath);
    }
    if (ImGui::Checkbox("Use Nexus Esc to Close##WvWFightAnalysis", &Settings::useNexusEscClose))
    {
        Settings::Settings[USE_NEXUS_ESC_CLOSE] = Settings::useNexusEscClose;
        Settings::Save(SettingsPath);

        if (Settings::useNexusEscClose)
        {
            APIDefs->UI.RegisterCloseOnEscape("WvW Fight Analysis", &Settings::IsAddonWindowEnabled);
        }
        else
        {
            APIDefs->UI.DeregisterCloseOnEscape("WvW Fight Analysis");
        }
    }
    if (ImGui::Checkbox("Lock Window & Widget Position##WvWFightAnalysis", &Settings::disableMovingWindow))
    {
        Settings::Settings[DISABLE_MOVING_WINDOW] = Settings::disableMovingWindow;
        Settings::Save(SettingsPath);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text("Disables moving & resizing.");
        ImGui::EndTooltip();
    }
    if (ImGui::Checkbox("Enable Mouse-Through##WvWFightAnalysis", &Settings::disableClickingWindow))
    {
        Settings::Settings[DISABLE_CLICKING_WINDOW] = Settings::disableClickingWindow;
        Settings::Save(SettingsPath);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text("Window cannot be interacted with via mouse.");
        ImGui::EndTooltip();
    }
    if (ImGui::Checkbox("Enable Wine Compatibility Mode##WvWFightAnalysis", &Settings::forceLinuxCompatibilityMode))
    {
        Settings::Settings[FORCE_LINUX_COMPAT] = Settings::forceLinuxCompatibilityMode;
        Settings::Save(SettingsPath);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text("Wine / Proton doesn't support ReadDirectoryChangesW, use directory polling instead.");
        ImGui::EndTooltip();
    }
    int tempPollIntervalMilliseconds = static_cast<int>(Settings::pollIntervalMilliseconds);
    if (ImGui::InputInt("ms Polling Interval##WvWFightAnalysis", &tempPollIntervalMilliseconds))
    {
        Settings::pollIntervalMilliseconds = static_cast<size_t>(std::clamp(tempPollIntervalMilliseconds, 500, 10000));
        Settings::Settings[POLL_INTERVAL_MILLISECONDS] = Settings::pollIntervalMilliseconds;
        Settings::Save(SettingsPath);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text("Polling Interval when using Wine compatibility mode.");
        ImGui::EndTooltip();
    }
    if (ImGui::InputInt("Team Player Threshold##WvWFightAnalysis", &Settings::teamPlayerThreshold))
    {
        Settings::teamPlayerThreshold = std::clamp(
            Settings::teamPlayerThreshold, 0,100
        );
        Settings::Settings[TEAM_PLAYER_THRESHOLD] = Settings::teamPlayerThreshold;
        Settings::Save(SettingsPath);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text("Set a minimum amount of team players required to render team.");
        ImGui::EndTooltip();
    }
    int tempLogHistorySize = static_cast<int>(Settings::logHistorySize);
    if (ImGui::InputInt("Log History Size##WvWFightAnalysis", &tempLogHistorySize))
    {
        Settings::logHistorySize = static_cast<size_t>(std::clamp(tempLogHistorySize, 1, 20));
        Settings::Settings[LOG_HISTORY_SIZE] = Settings::logHistorySize;
        Settings::Save(SettingsPath);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text("How many parsed logs to keep.");
        ImGui::EndTooltip();
    }
    if (ImGui::InputText("Custom Log Path##WvWFightAnalysis", Settings::LogDirectoryPathC, sizeof(Settings::LogDirectoryPathC)))
    {
        Settings::LogDirectoryPath = Settings::LogDirectoryPathC;
        Settings::Settings[CUSTOM_LOG_PATH] = Settings::LogDirectoryPath;
        Settings::Save(SettingsPath);
    }

    bool enabled = !isRestartInProgress.load();

    if (!enabled) {
        ImVec4 disabledColor = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_Button, disabledColor);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, disabledColor);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, disabledColor);
    }

    bool buttonClicked = ImGui::Button("Restart Directory Monitoring");

    if (!enabled) {
        ImGui::PopStyleColor(3);
    }

    if (enabled && buttonClicked) {
        isRestartInProgress.store(true);
        std::thread([]() {
            stopMonitoring = true;
            if (directoryMonitorThread.joinable())
            {
                directoryMonitorThread.join();
            }
            stopMonitoring = false;
            directoryMonitorThread = std::thread(monitorDirectory, Settings::logHistorySize, Settings::pollIntervalMilliseconds);
            isRestartInProgress.store(false);
            }).detach();
    }

}

extern "C" __declspec(dllexport) AddonDefinition * GetAddonDef()
{
    AddonDef.Signature = -996748;
    AddonDef.APIVersion = NEXUS_API_VERSION;
    AddonDef.Name = ADDON_NAME;
    AddonDef.Version.Major = 1;
    AddonDef.Version.Minor = 0;
    AddonDef.Version.Build = 4;
    AddonDef.Version.Revision = 1;
    AddonDef.Author = "Unreal";
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