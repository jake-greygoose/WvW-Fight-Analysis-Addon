#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include "imgui/imgui.h"
#include "Shared.h"

struct TemplateVariable {
    enum class Type {
        PlayerCount = 1,    // {1} - player count
        ClassIcon,          // {2} - class icon
        ClassName,          // {3} - class name
        Damage,             // {4} - damage
        DownCont,           // {5} - down contribution
        KillCont,           // {6} - kill contribution
        Deaths,             // {7} - deaths
        Downs,              // {8} - downs
        StrikeDamage,       // {9} - strike damage
        CondiDamage,        // {10} - condition damage
        Custom
    };
    Type type;
    size_t position;
};

struct BarTemplateDefaults {
    static const inline std::unordered_map<std::string, std::string> defaultTemplates = {
        {"players", "{1} {2} {3} ({4})"},
        {"damage", "{1} {2} {3} ({4})"},
        {"down_cont", "{1} {2} {3} ({5} / {4})"},
        {"kill_cont", "{1} {2} {3} ({6} / {4})"},
        {"deaths", "{1} {2} {3} ({7})"},
        {"downs", "{1} {2} {3} ({8})"}
    };
};

class BarTemplateRenderer {
public:
    static void ParseTemplate(const std::string& templateStr);
    static void RenderTemplate(
        int playerCount,
        const std::string& eliteSpec,
        bool useShortNames,
        const SpecStats& stats,
        bool vsLoggedPlayersOnly,
        const std::string& sortType,
        HINSTANCE hSelf,
        float fontSize,
        bool showTooltips
    );
    static std::string GetTemplateForSort(
        const std::string& sortType,
        const std::string& customTemplate = ""
    );

    // Helper methods for unified tooltip
    static void RenderTooltipHeader(const std::string& eliteSpec);
    static void RenderTooltipStats(
        const SpecStats& stats,
        bool vsLoggedPlayersOnly,
        int playerCount
    );

private:
    static std::vector<TemplateVariable> variables_;
    static std::vector<std::string> textSegments_;
    static void RenderVariable(
        const TemplateVariable& var,
        int playerCount,
        const std::string& eliteSpec,
        bool useShortNames,
        const SpecStats& stats,
        bool vsLoggedPlayersOnly,
        HINSTANCE hSelf,
        float fontSize
    );
    static TemplateVariable::Type GetVariableType(int number);
    static std::string FormatStatValue(
        const TemplateVariable::Type& type,
        const SpecStats& stats,
        bool vsLoggedPlayersOnly
    );
};

namespace TemplateUtils {
    inline const char* GetTypeName(TemplateVariable::Type type) {
        switch (type) {
        case TemplateVariable::Type::PlayerCount: return "Player Count";
        case TemplateVariable::Type::ClassIcon: return "Class Icon";
        case TemplateVariable::Type::ClassName: return "Class Name";
        case TemplateVariable::Type::Damage: return "Damage";
        case TemplateVariable::Type::DownCont: return "Down Contribution";
        case TemplateVariable::Type::KillCont: return "Kill Contribution";
        case TemplateVariable::Type::Deaths: return "Deaths";
        case TemplateVariable::Type::Downs: return "Downs";
        case TemplateVariable::Type::StrikeDamage: return "Strike Damage";
        case TemplateVariable::Type::CondiDamage: return "Condition Damage";
        default: return "Unknown";
        }
    }

    inline bool IsStat(TemplateVariable::Type type) {
        switch (type) {
        case TemplateVariable::Type::Damage:
        case TemplateVariable::Type::DownCont:
        case TemplateVariable::Type::KillCont:
        case TemplateVariable::Type::Deaths:
        case TemplateVariable::Type::Downs:
        case TemplateVariable::Type::StrikeDamage:
        case TemplateVariable::Type::CondiDamage:
            return true;
        default:
            return false;
        }
    }
}