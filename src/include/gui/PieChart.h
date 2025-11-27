#pragma once
#include "imgui/imgui.h"
#include <vector>

namespace wvwfightanalysis::gui {

    struct PieChartData {
        std::vector<float> ratios;        // Percentages for each slice (should sum to 1.0)
        std::vector<ImU32> colors;        // Colors for each slice
        float globalRotation = 0.0f;      // Global rotation offset for entire pie chart (radians)
        ImU32 backgroundTint = IM_COL32_WHITE;  // Tint color for background (for fade effects)
        ImTextureID background;           // Black circle background texture
        ImTextureID globe;                // Planet/globe texture
        ImTextureID separator;            // Line separator texture
    };

    // Main function to draw the complete pie chart
    void DrawLayeredPieChart(const PieChartData& data, ImVec2 position, float size);

    // Individual layer drawing functions
    void DrawBackgroundLayer(ImDrawList* draw_list, ImTextureID backgroundTex,
                            ImVec2 position, float size, ImU32 tint = IM_COL32_WHITE);

    void DrawGlobeSlices(ImDrawList* draw_list, ImTextureID globeTex,
                        ImVec2 position, float size,
                        const std::vector<float>& ratios,
                        const std::vector<ImU32>& colors,
                        float globalRotation);

    void DrawSingleSlice(ImDrawList* draw_list, ImTextureID globeTex,
                        ImVec2 position, float size, ImVec2 center, float radius,
                        float startAngle, float endAngle, ImU32 tintColor);

    void DrawSeparatorLines(ImDrawList* draw_list, ImTextureID separatorTex,
                           ImVec2 center, float size, const std::vector<float>& ratios,
                           float globalRotation);

    void DrawRotatedTexture(ImDrawList* draw_list, ImTextureID tex,
                           ImVec2 center, float size, float rotation);

} // namespace wvwfightanalysis::gui
