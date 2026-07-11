#pragma once
#include "gui/ContentState.h"
#include "settings/Settings.h"
#include "shared/Shared.h"
#include <unordered_map>
#include <vector>

namespace wvwfightanalysis::gui {

    struct WidgetRenderData {
        std::vector<float> counts;
        std::vector<ImVec4> backgroundColors;
        std::vector<ImVec4> textColors;
        std::vector<std::string> texts;
        std::vector<size_t> teamIndices;
        int povTeamIndex = -1;
        uint64_t logTimestamp = 0;

        std::vector<const char*> TextPointers() const {
            std::vector<const char*> result;
            result.reserve(texts.size());
            for (const std::string& text : texts)
                result.push_back(text.c_str());
            return result;
        }
    };

    class WidgetWindow {
    public:
        void Render(HINSTANCE hSelf, WidgetWindowSettings* settings);

    private:
        struct BarAnimState {
            std::vector<float> smoothedFractions;
            std::vector<float> fractionVel;
            float labelAlpha = 1.0f;
            ContentTransition contentTransition;
        };

        struct PieAnimState {
            std::vector<float> previousRatios;
            std::vector<float> targetRatios;
            float transitionStartTime = 0.0f;
            bool  wasInCombat = false;
            float leftCombatTime = 0.0f;
            float enteredCombatTime = 0.0f;
        };

        struct StackedAnimState {
            ContentTransition contentTransition;
            float labelAlpha = 0.0f;
        };

        std::unordered_map<const WidgetWindowSettings*, BarAnimState> m_barAnimStates;
        std::unordered_map<const WidgetWindowSettings*, PieAnimState> m_pieAnimStates;
        std::unordered_map<const WidgetWindowSettings*, StackedAnimState> m_stackedAnimStates;

        ImTextureID GetStatIcon(const WidgetWindowSettings* settings);
        ImTextureID GetOrLoadStatIcon(HINSTANCE hSelf, const WidgetWindowSettings* settings);
        void RenderSettingsPopup(WidgetWindowSettings* settings);
        void RenderDisplayStatsMenu(WidgetWindowSettings* settings);
        void RenderStyleMenu(WidgetWindowSettings* settings);

        void RenderSimpleRatioBar(
            const WidgetRenderData& data,
            const ImVec2& size,
            ImTextureID statIcon,
            const WidgetWindowSettings* settings
        );

        void RenderRatioBarPlaceholder(
            const ImVec2& size,
            const WidgetWindowSettings* settings,
            ImTextureID statIcon
        );

        void RenderPieChart(
            HINSTANCE hSelf,
            const WidgetRenderData& data,
            const ImVec2& size,
            const WidgetWindowSettings* settings
        );

        void RenderStackedWidget(
            HINSTANCE hSelf,
            const ImVec2& size,
            const WidgetWindowSettings* settings,
            const WidgetRenderData& data,
            ImTextureID statIcon,
            ContentState contentState
        );
    };
} // namespace wvwfightanalysis::gui
