#pragma once
#include "settings/Settings.h"
#include "shared/Shared.h"
#include "utils/Utils.h"
#include "gui/BarTemplate.h"
#include "imgui/imgui.h"
#include "resource.h"


namespace wvwfightanalysis::gui {

    struct TeamRenderInfo {
        bool hasData;
        const TeamStats* stats;
        std::string name;
        ImVec4 color;
    };

    class MainWindow {
    public:
        void Render(HINSTANCE hSelf, MainWindowSettings* settings);

    private:
        void RenderTeamData(const TeamStats& teamData,
            const std::string& teamName,
            const MainWindowSettings* settings,
            HINSTANCE hSelf);
        void RenderSpecializationBars(const TeamStats& teamData,
            const std::string& teamName,
            const MainWindowSettings* settings,
            HINSTANCE hSelf);
        void RenderMainWindowSettingsPopup(MainWindowSettings* settings);
        void RenderExcludeMenu(MainWindowSettings* settings);
        void RenderTemplateSettings(MainWindowSettings* settings);
        float CalculateBarWidth(
            const SpecStats& stats,
            const std::vector<std::pair<std::string, SpecStats>>& sortedClasses,
            const MainWindowSettings* settings,
            uint64_t maxValue
        ) const;

        void CacheBarColors(
            const std::string& profession,
            ImVec4& primary,
            ImVec4& secondary
        ) const;
    };

} // namespace wvwfightanalysis::gui