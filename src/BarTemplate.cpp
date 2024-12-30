#include "BarTemplate.h"
#include "Settings.h"
#include "utils.h"
#include <regex>

std::vector<TemplateVariable> BarTemplateRenderer::variables_;
std::vector<std::string> BarTemplateRenderer::textSegments_;

void BarTemplateRenderer::ParseTemplate(const std::string& templateStr) {
    // If template is empty, use a default
    if (templateStr.empty()) {
        ParseTemplate("@1 @2 @3 (@4)");
        return;
    }

    variables_.clear();
    textSegments_.clear();

    // Always start with an empty text segment to maintain alignment
    textSegments_.push_back("");

    std::regex varPattern("@([1-5])");
    std::string::const_iterator searchStart(templateStr.cbegin());
    std::smatch match;
    size_t lastPos = 0;

    // If template doesn't start with a variable, add the initial text
    if (!templateStr.empty() && templateStr[0] != '@') {
        size_t firstVarPos = templateStr.find('@');
        if (firstVarPos == std::string::npos) {
            // No variables found, just text
            textSegments_[0] = templateStr;
            return;
        }
        textSegments_[0] = templateStr.substr(0, firstVarPos);
        searchStart = templateStr.cbegin() + firstVarPos;
        lastPos = firstVarPos;
    }

    while (std::regex_search(searchStart, templateStr.cend(), match, varPattern)) {
        // Add the variable
        int varNum = std::stoi(match[1]);
        TemplateVariable var;
        switch (varNum) {
        case 1: var.type = TemplateVariable::Type::PlayerCount; break;
        case 2: var.type = TemplateVariable::Type::ClassIcon; break;
        case 3: var.type = TemplateVariable::Type::ClassName; break;
        case 4: var.type = TemplateVariable::Type::PrimaryStat; break;
        case 5: var.type = TemplateVariable::Type::SecondaryStat; break;
        default: continue;
        }
        var.position = variables_.size();
        variables_.push_back(var);

        // Add text segment after the variable (may be empty)
        size_t matchEnd = match.position() + match.length() + (searchStart - templateStr.cbegin());
        size_t nextVarPos = templateStr.find('@', matchEnd);
        if (nextVarPos == std::string::npos) {
            // No more variables, add remaining text
            textSegments_.push_back(templateStr.substr(matchEnd));
        }
        else {
            textSegments_.push_back(templateStr.substr(matchEnd, nextVarPos - matchEnd));
        }

        // Update search position
        searchStart = match.suffix().first;
    }

    // Add remaining text after last variable if not already added
    if (searchStart != templateStr.cend() && textSegments_.size() <= variables_.size()) {
        textSegments_.push_back(std::string(searchStart, templateStr.cend()));
    }

    // If no variables were found in non-empty template, use default
    if (variables_.empty() && !templateStr.empty()) {
        ParseTemplate("@1 @2 @3 (@4)");
    }
}

void BarTemplateRenderer::RenderTemplate(
    int playerCount,
    const std::string& eliteSpec,
    bool useShortNames,
    uint64_t primaryValue,
    uint64_t secondaryValue,
    HINSTANCE hSelf,
    float fontSize
) {
    // Store the cursor position at the start of the bar
    float startX = ImGui::GetCursorPosX();

    for (size_t i = 0; i < variables_.size() + 1; ++i) {
        // Render text segment that comes before this variable (or final segment)
        if (i < textSegments_.size() && !textSegments_[i].empty()) {
            ImGui::Text("%s", textSegments_[i].c_str());
            if (i < variables_.size()) {  // Only SameLine if there's a variable after
                ImGui::SameLine(0, 0);
            }
        }

        // Render variable (if not at the end)
        if (i < variables_.size()) {
            RenderVariable(
                variables_[i],
                playerCount,
                eliteSpec,
                useShortNames,
                primaryValue,
                secondaryValue,
                hSelf,
                fontSize
            );

            // Add SameLine if there's more content coming (either a variable or non-empty text)
            if (i < variables_.size() - 1 ||
                (i + 1 < textSegments_.size() && !textSegments_[i + 1].empty())) {
                ImGui::SameLine(0, 0);
            }
        }
    }

    // Move to next line for the next bar
    ImGui::NewLine();
}
void BarTemplateRenderer::RenderVariable(
    const TemplateVariable& var,
    int playerCount,
    const std::string& eliteSpec,
    bool useShortNames,
    uint64_t primaryValue,
    uint64_t secondaryValue,
    HINSTANCE hSelf,
    float fontSize
) {
    switch (var.type) {
    case TemplateVariable::Type::PlayerCount:
        ImGui::Text("%d", playerCount);
        break;

    case TemplateVariable::Type::ClassIcon:
        if (Settings::showClassIcons) {
            float icon_size = fontSize;
            int resourceId = 0;
            Texture** texturePtrPtr = getTextureInfo(eliteSpec, &resourceId);

            if (texturePtrPtr && *texturePtrPtr && (*texturePtrPtr)->Resource) {
                ImGui::Image((*texturePtrPtr)->Resource, ImVec2(icon_size, icon_size));
            }
            else {
                if (resourceId != 0 && texturePtrPtr) {
                    *texturePtrPtr = APIDefs->Textures.GetOrCreateFromResource((eliteSpec + "_ICON").c_str(), resourceId, hSelf);
                    if (*texturePtrPtr && (*texturePtrPtr)->Resource) {
                        ImGui::Image((*texturePtrPtr)->Resource, ImVec2(icon_size, icon_size));
                    }
                    else {
                        if (eliteSpec.size() >= 2) {
                            ImGui::Text("%c%c", eliteSpec[0], eliteSpec[1]);
                        }
                        else if (eliteSpec.size() == 1) {
                            ImGui::Text("%c", eliteSpec[0]);
                        }
                        else {
                            ImGui::Text("??");
                        }
                    }
                }
                else {
                    if (eliteSpec.size() >= 2) {
                        ImGui::Text("%c%c", eliteSpec[0], eliteSpec[1]);
                    }
                    else if (eliteSpec.size() == 1) {
                        ImGui::Text("%c", eliteSpec[0]);
                    }
                    else {
                        ImGui::Text("??");
                    }
                }
            }
        }
        break;

    case TemplateVariable::Type::ClassName:
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
        break;

    case TemplateVariable::Type::PrimaryStat:
        ImGui::Text("%s", formatDamage(primaryValue).c_str());
        break;

    case TemplateVariable::Type::SecondaryStat:
        if (secondaryValue > 0 && secondaryValue <= primaryValue) {
            std::string formattedValue = formatDamage(secondaryValue);
            if (!formattedValue.empty()) {
                ImGui::Text("%s", formattedValue.c_str());
            }
        }
        break;
    }
}