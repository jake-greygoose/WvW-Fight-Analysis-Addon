#include "BarTemplate.h"
#include "Settings.h"
#include "utils.h"
#include <regex>

std::vector<TemplateVariable> BarTemplateRenderer::variables_;
std::vector<std::string> BarTemplateRenderer::textSegments_;

TemplateVariable::Type BarTemplateRenderer::GetVariableType(int number) {
    switch (number) {
    case 1: return TemplateVariable::Type::PlayerCount;
    case 2: return TemplateVariable::Type::ClassIcon;
    case 3: return TemplateVariable::Type::ClassName;
    case 4: return TemplateVariable::Type::Damage;
    case 5: return TemplateVariable::Type::DownCont;
    case 6: return TemplateVariable::Type::KillCont;
    case 7: return TemplateVariable::Type::Deaths;
    case 8: return TemplateVariable::Type::Downs;
    case 9: return TemplateVariable::Type::StrikeDamage;
    case 10: return TemplateVariable::Type::CondiDamage;
    default: return TemplateVariable::Type::Custom;
    }
}

void BarTemplateRenderer::ParseTemplate(const std::string& templateStr) {
    variables_.clear();
    textSegments_.clear();

    textSegments_.push_back("");

    if (templateStr.empty()) {
        ParseTemplate(BarTemplateDefaults::defaultTemplates.at("players"));
        return;
    }

    std::regex varPattern("\\{([0-9]+)\\}");
    std::string::const_iterator searchStart(templateStr.cbegin());
    std::smatch match;

    if (!templateStr.empty() && templateStr[0] != '{') {
        size_t firstVarPos = templateStr.find('{');
        if (firstVarPos == std::string::npos) {
            textSegments_[0] = templateStr;
            return;
        }
        textSegments_[0] = templateStr.substr(0, firstVarPos);
        searchStart = templateStr.cbegin() + firstVarPos;
    }

    while (std::regex_search(searchStart, templateStr.cend(), match, varPattern)) {
        int varNum = std::stoi(match[1]);
        TemplateVariable var;
        var.type = GetVariableType(varNum);
        var.position = variables_.size();
        variables_.push_back(var);

        size_t matchEnd = match.position() + match.length() + (searchStart - templateStr.cbegin());
        size_t nextVarPos = templateStr.find('{', matchEnd);

        if (nextVarPos == std::string::npos) {
            textSegments_.push_back(templateStr.substr(matchEnd));
        }
        else {
            textSegments_.push_back(templateStr.substr(matchEnd, nextVarPos - matchEnd));
        }

        searchStart = match.suffix().first;
    }

    if (searchStart != templateStr.cend() && textSegments_.size() <= variables_.size()) {
        textSegments_.push_back(std::string(searchStart, templateStr.cend()));
    }

    if (variables_.empty() && !templateStr.empty()) {
        ParseTemplate(BarTemplateDefaults::defaultTemplates.at("players"));
    }
}

std::string BarTemplateRenderer::FormatStatValue(
    const TemplateVariable::Type& type,
    const SpecStats& stats,
    bool vsLoggedPlayersOnly
) {
    switch (type) {
    case TemplateVariable::Type::Damage:
        return formatDamage(vsLoggedPlayersOnly ? stats.totalDamageVsPlayers : stats.totalDamage);
    case TemplateVariable::Type::DownCont:
        return formatDamage(vsLoggedPlayersOnly ?
            stats.totalDownedContributionVsPlayers : stats.totalDownedContribution);
    case TemplateVariable::Type::KillCont:
        return formatDamage(vsLoggedPlayersOnly ?
            stats.totalKillContributionVsPlayers : stats.totalKillContribution);
    case TemplateVariable::Type::Deaths:
        return std::to_string(stats.totalDeaths);
    case TemplateVariable::Type::Downs:
        return std::to_string(stats.totalDowned);
    case TemplateVariable::Type::StrikeDamage:
        return formatDamage(vsLoggedPlayersOnly ?
            stats.totalStrikeDamageVsPlayers : stats.totalStrikeDamage);
    case TemplateVariable::Type::CondiDamage:
        return formatDamage(vsLoggedPlayersOnly ?
            stats.totalCondiDamageVsPlayers : stats.totalCondiDamage);
    default:
        return "0";
    }
}

std::string BarTemplateRenderer::GetTemplateForSort(
    const std::string& sortType,
    const std::string& customTemplate
) {
    if (!customTemplate.empty()) {
        return customTemplate;
    }

    auto it = BarTemplateDefaults::defaultTemplates.find(sortType);
    if (it != BarTemplateDefaults::defaultTemplates.end()) {
        return it->second;
    }

    return BarTemplateDefaults::defaultTemplates.at("players");
}

void BarTemplateRenderer::RenderTooltipHeader(const std::string& eliteSpec) {

    ImVec4 profColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    std::string professionName = "Unknown";
    auto profIt = eliteSpecToProfession.find(eliteSpec);
    if (profIt != eliteSpecToProfession.end()) {
        professionName = profIt->second;
        auto colorIt = std::find_if(professionColorPair.begin(), professionColorPair.end(),
            [&](const ProfessionColor& pc) { return pc.name == professionName; });
        if (colorIt != professionColorPair.end()) {
            profColor = colorIt->primaryColor;
        }
    }

    ImGui::PushStyleColor(ImGuiCol_Text, profColor);
    ImGui::Text("%s", eliteSpec.c_str());
    ImGui::PopStyleColor();
    ImGui::Separator();
}

void BarTemplateRenderer::RenderTooltipStats(
    const SpecStats& stats,
    bool vsLoggedPlayersOnly,
    int playerCount
) {
    ImGui::Text("Players: %d", playerCount);
    ImGui::Text("Damage: %s", formatDamage(vsLoggedPlayersOnly ?
        stats.totalDamageVsPlayers : stats.totalDamage).c_str());
    ImGui::Text("Down Contribution: %s", formatDamage(vsLoggedPlayersOnly ?
        stats.totalDownedContributionVsPlayers : stats.totalDownedContribution).c_str());
    ImGui::Text("Kill Contribution: %s", formatDamage(vsLoggedPlayersOnly ?
        stats.totalKillContributionVsPlayers : stats.totalKillContribution).c_str());
    ImGui::Text("Deaths: %d", stats.totalDeaths);
    ImGui::Text("Downs: %d", stats.totalDowned);
    ImGui::Text("Strike Damage: %s", formatDamage(vsLoggedPlayersOnly ?
        stats.totalStrikeDamageVsPlayers : stats.totalStrikeDamage).c_str());
    ImGui::Text("Condition Damage: %s", formatDamage(vsLoggedPlayersOnly ?
        stats.totalCondiDamageVsPlayers : stats.totalCondiDamage).c_str());
}

void BarTemplateRenderer::RenderTemplate(
    int playerCount,
    const std::string& eliteSpec,
    bool useShortNames,
    const SpecStats& stats,
    bool vsLoggedPlayersOnly,
    const std::string& sortType,
    HINSTANCE hSelf,
    float fontSize,
    bool showTooltips
) {
    ImVec2 startPos = ImGui::GetCursorPos();
    ImVec2 startScreenPos = ImGui::GetCursorScreenPos();

    // Create invisible button for tooltip
    float totalWidth = ImGui::GetContentRegionAvail().x;
    float totalHeight = ImGui::GetTextLineHeight() + 4.0f;
    ImGui::InvisibleButton("##tooltipArea", ImVec2(totalWidth, totalHeight));

    // If hovered and tooltips enabled, show tooltip
    if (showTooltips && ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        RenderTooltipHeader(eliteSpec);
        RenderTooltipStats(stats, vsLoggedPlayersOnly, playerCount);
        ImGui::EndTooltip();
    }

    // Reset cursor position after invisible button
    ImGui::SetCursorPos(startPos);

    // Render the template content
    for (size_t i = 0; i < variables_.size() + 1; ++i) {
        if (i < textSegments_.size() && !textSegments_[i].empty()) {
            ImGui::Text("%s", textSegments_[i].c_str());
            if (i < variables_.size()) {
                ImGui::SameLine(0, 0);
            }
        }

        if (i < variables_.size()) {
            RenderVariable(
                variables_[i],
                playerCount,
                eliteSpec,
                useShortNames,
                stats,
                vsLoggedPlayersOnly,
                hSelf,
                fontSize
            );

            if (i < variables_.size() - 1 ||
                (i + 1 < textSegments_.size() && !textSegments_[i + 1].empty())) {
                ImGui::SameLine(0, 0);
            }
        }
    }

    ImGui::NewLine();
}

void BarTemplateRenderer::RenderVariable(
    const TemplateVariable& var,
    int playerCount,
    const std::string& eliteSpec,
    bool useShortNames,
    const SpecStats& stats,
    bool vsLoggedPlayersOnly,
    HINSTANCE hSelf,
    float fontSize
) {
    switch (var.type) {
    case TemplateVariable::Type::PlayerCount:
        ImGui::Text("%d", playerCount);
        break;

    case TemplateVariable::Type::ClassIcon:
    {
        float icon_size = fontSize;
        int resourceId = 0;
        Texture** texturePtrPtr = getTextureInfo(eliteSpec, &resourceId);

        if (texturePtrPtr && *texturePtrPtr && (*texturePtrPtr)->Resource) {
            ImGui::Image((*texturePtrPtr)->Resource, ImVec2(icon_size, icon_size));
        }
        else {
            if (resourceId != 0 && texturePtrPtr) {
                *texturePtrPtr = APIDefs->Textures.GetOrCreateFromResource(
                    (eliteSpec + "_ICON").c_str(), resourceId, hSelf
                );
                if (*texturePtrPtr && (*texturePtrPtr)->Resource) {
                    ImGui::Image((*texturePtrPtr)->Resource, ImVec2(icon_size, icon_size));
                }
                else {
                    ImGui::Text("%s", eliteSpec.substr(0, 2).c_str());
                }
            }
            else {
                ImGui::Text("%s", eliteSpec.substr(0, 2).c_str());
            }
        }
    }
    break;

    case TemplateVariable::Type::ClassName:
    {
        if (useShortNames) {
            std::string shortClassName = "Unk";
            auto clnIt = eliteSpecShortNames.find(eliteSpec);
            if (clnIt != eliteSpecShortNames.end()) {
                shortClassName = clnIt->second;
            }
            ImGui::Text("%s", shortClassName.c_str());
        }
        else {
            ImGui::Text("%s", eliteSpec.c_str());
        }
    }
    break;

    default:
        if (TemplateUtils::IsStat(var.type)) {
            std::string value = FormatStatValue(var.type, stats, vsLoggedPlayersOnly);
            ImGui::Text("%s", value.c_str());
        }
        break;
    }
}