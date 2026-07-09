#pragma once
#include "settings/Settings.h"
#include "shared/Shared.h"
#include <unordered_map>
#include <vector>

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
        struct BarAnimState {
            std::vector<float> smoothedFractions;
            std::vector<float> fractionVel;
            float labelAlpha = 1.0f;
        };

        struct PieAnimState {
            std::vector<float> previousRatios;
            std::vector<float> targetRatios;
            float transitionStartTime = 0.0f;
            bool  wasInCombat = false;
            float leftCombatTime = 0.0f;
            float enteredCombatTime = 0.0f;
        };

        std::unordered_map<const WidgetWindowSettings*, BarAnimState> m_barAnimStates;
        std::unordered_map<const WidgetWindowSettings*, PieAnimState> m_pieAnimStates;

        ImTextureID GetStatIcon(const WidgetWindowSettings* settings);
        void RenderSettingsPopup(WidgetWindowSettings* settings);
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