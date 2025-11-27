#include "gui/PieChart.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace wvwfightanalysis::gui {

    // Main function to draw the complete pie chart
    void DrawLayeredPieChart(const PieChartData& data, ImVec2 position, float size) {
        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        // Layer 1: Draw the black background circle
        DrawBackgroundLayer(draw_list, data.background, position, size, data.backgroundTint);

        // Layer 2: Draw the globe texture in colored slices
        DrawGlobeSlices(draw_list, data.globe, position, size, data.ratios, data.colors, data.globalRotation);

        // Layer 3: Draw separator lines at slice boundaries
        DrawSeparatorLines(draw_list, data.separator,
                          ImVec2(position.x + size * 0.5f, position.y + size * 0.5f),
                          size, data.ratios, data.globalRotation);
    }

    // Layer 1: Background (simple, no slicing)
    void DrawBackgroundLayer(ImDrawList* draw_list, ImTextureID backgroundTex,
                            ImVec2 position, float size, ImU32 tint) {
        if (!backgroundTex) return;

        // Draw the full square texture as-is (it contains the black circle)
        draw_list->AddImage(
            backgroundTex,
            position,
            ImVec2(position.x + size, position.y + size),
            ImVec2(0, 0), ImVec2(1, 1),
            tint  // Apply tint for fade effects
        );
    }

    // Layer 2: Globe with colored pie slices
    void DrawGlobeSlices(ImDrawList* draw_list, ImTextureID globeTex,
                        ImVec2 position, float size,
                        const std::vector<float>& ratios,
                        const std::vector<ImU32>& colors,
                        float globalRotation) {
        if (!globeTex || ratios.empty()) return;

        ImVec2 center = ImVec2(position.x + size * 0.5f, position.y + size * 0.5f);
        float radius = size * 0.5f;

        // Calculate angles based on ratios, starting from globalRotation offset
        std::vector<float> angles;
        angles.push_back(globalRotation);

        for (size_t i = 0; i < ratios.size(); ++i) {
            float nextAngle = angles.back() + ratios[i] * 2.0f * static_cast<float>(M_PI);
            angles.push_back(nextAngle);
        }

        // Draw each slice with global rotation applied
        for (size_t i = 0; i < ratios.size() && i < colors.size(); ++i) {
            // Skip drawing slice for zero values
            if (ratios[i] > 0.0f) {
                DrawSingleSlice(draw_list, globeTex, position, size, center, radius,
                               angles[i], angles[i + 1], colors[i]);
            }
        }
    }

    // Draw a single colored slice of the globe using triangle fan
    void DrawSingleSlice(ImDrawList* draw_list, ImTextureID globeTex,
                        ImVec2 position, float size, ImVec2 center, float radius,
                        float startAngle, float endAngle, ImU32 tintColor) {
        const int segments = 64;  // Increased for smoother rendering

        // Calculate how many segments we need for this slice
        float angleRange = endAngle - startAngle;
        int numSegments = static_cast<int>((angleRange / (2.0f * static_cast<float>(M_PI))) * segments);
        if (numSegments < 2) numSegments = 2;

        // Reserve space for vertices and UVs
        const int vtx_count = numSegments + 2;  // center + arc points
        const int idx_count = numSegments * 3;   // triangles

        // Set texture BEFORE reserving primitives
        draw_list->PushTextureID(globeTex);
        draw_list->PrimReserve(idx_count, vtx_count);

        // Add center vertex
        const unsigned int vtx_base = draw_list->_VtxCurrentIdx;
        draw_list->PrimWriteVtx(center, ImVec2(0.5f, 0.5f), tintColor);

        // Add arc vertices
        for (int i = 0; i <= numSegments; ++i) {
            float t = i / static_cast<float>(numSegments);
            float angle = startAngle + angleRange * t;

            // World position
            ImVec2 pos(
                center.x + radius * cosf(angle),
                center.y + radius * sinf(angle)
            );

            // UV position (map to circular texture)
            // The globe is a circle within the texture, so UV radius should be 0.5
            ImVec2 uv(
                0.5f + 0.5f * cosf(angle),
                0.5f + 0.5f * sinf(angle)
            );

            draw_list->PrimWriteVtx(pos, uv, tintColor);
        }

        // Add triangle indices (fan from center)
        for (int i = 0; i < numSegments; ++i) {
            draw_list->PrimWriteIdx(vtx_base);          // center
            draw_list->PrimWriteIdx(vtx_base + i + 1);  // current arc point
            draw_list->PrimWriteIdx(vtx_base + i + 2);  // next arc point
        }

        // Pop texture ID after drawing
        draw_list->PopTextureID();
    }

    // Layer 3: Separator lines
    void DrawSeparatorLines(ImDrawList* draw_list, ImTextureID separatorTex,
                           ImVec2 center, float size, const std::vector<float>& ratios,
                           float globalRotation) {
        if (!separatorTex || ratios.empty()) return;

        // Calculate where to place separator lines based on ratios
        // Separator texture has line pointing up (12 o'clock), but angles start at 3 o'clock (0 radians)
        // Add 90 degrees (π/2) to rotate from 3 o'clock to 12 o'clock, then align with slice boundary
        float currentAngle = globalRotation;

        for (size_t i = 0; i < ratios.size(); ++i) {
            // Skip drawing separator for zero values
            if (ratios[i] > 0.0f) {
                DrawRotatedTexture(draw_list, separatorTex, center, size, currentAngle + static_cast<float>(M_PI) / 2.0f);
            }
            currentAngle += ratios[i] * 2.0f * static_cast<float>(M_PI);
        }
    }

    // Helper to draw a rotated square texture
    void DrawRotatedTexture(ImDrawList* draw_list, ImTextureID tex,
                           ImVec2 center, float size, float rotation) {
        if (!tex) return;

        float cos_a = cosf(rotation);
        float sin_a = sinf(rotation);
        float half = size * 0.5f;

        // Define corners of the square
        ImVec2 corners[4] = {
            {-half, -half},  // Top-left
            { half, -half},  // Top-right
            { half,  half},  // Bottom-right
            {-half,  half}   // Bottom-left
        };

        // Rotate corners around center
        ImVec2 rotatedPos[4];
        for (int i = 0; i < 4; i++) {
            float x = corners[i].x * cos_a - corners[i].y * sin_a;
            float y = corners[i].x * sin_a + corners[i].y * cos_a;
            rotatedPos[i] = ImVec2(center.x + x, center.y + y);
        }

        // Draw the rotated square texture
        draw_list->AddImageQuad(
            tex,
            rotatedPos[0], rotatedPos[1], rotatedPos[2], rotatedPos[3],
            ImVec2(0, 0), ImVec2(1, 0), ImVec2(1, 1), ImVec2(0, 1),
            IM_COL32_WHITE  // No tinting for separators
        );
    }

} // namespace wvwfightanalysis::gui
