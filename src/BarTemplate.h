#pragma once
#include <string>
#include <vector>
#include "imgui/imgui.h"
#include "Shared.h"

struct TemplateVariable {
    enum class Type {
        PlayerCount,    // @1
        ClassIcon,      // @2
        ClassName,      // @3
        PrimaryStat,    // @4
        SecondaryStat   // @5
    };

    Type type;
    size_t position;
};

class BarTemplateRenderer {
public:
    // Parse template string and cache variable positions
    static void ParseTemplate(const std::string& templateStr);

    // Render the template for a specific bar
    static void RenderTemplate(
        int playerCount,
        const std::string& eliteSpec,
        bool useShortNames,
        uint64_t primaryValue,
        uint64_t secondaryValue,
        HINSTANCE hSelf,
        float fontSize
    );

private:
    static std::vector<TemplateVariable> variables_;
    static std::vector<std::string> textSegments_;

    static void RenderVariable(
        const TemplateVariable& var,
        int playerCount,
        const std::string& eliteSpec,
        bool useShortNames,
        uint64_t primaryValue,
        uint64_t secondaryValue,
        HINSTANCE hSelf,
        float fontSize
    );
};
