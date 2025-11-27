#pragma once
#include "settings/Settings.h"
#include "shared/Shared.h"

namespace wvwfightanalysis::gui {

    struct TeamDisplayData {
        float count;
        ImVec4 backgroundColor;
        ImVec4 textColor;
        std::string text;
    };

    class WidgetWindow {
    public:
        void Render(HINSTANCE hSelf, WidgetWindowSettings* settings);

    private:
        ImTextureID GetStatIcon(const WidgetWindowSettings* settings);
        void RenderSettingsPopup(WidgetWindowSettings* settings);
        void RenderHistoryMenu(WidgetWindowSettings* settings);
        void RenderDisplayStatsMenu(WidgetWindowSettings* settings);
        void RenderStyleMenu(WidgetWindowSettings* settings);

        void RenderSimpleRatioBar(
            const std::vector<float>& counts,
            const std::vector<ImVec4>& colors,
            const ImVec2& size,
            const std::vector<const char*>& texts,
            ImTextureID statIcon,
            const WidgetWindowSettings* settings,
            const std::vector<ImVec4>& textColors,
            const std::vector<size_t>& teamIndices
        );

        void RenderPieChart(
            HINSTANCE hSelf,
            const std::vector<float>& counts,
            const std::vector<ImVec4>& colors,
            const ImVec2& size,
            const WidgetWindowSettings* settings,
            const std::vector<const char*>& texts,
            const std::vector<ImVec4>& textColors
        );
    };
} // namespace wvwfightanalysis::gui