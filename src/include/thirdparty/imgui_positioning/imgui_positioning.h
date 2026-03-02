///----------------------------------------------------------------------------------------------------
/// Copyright (c) RaidcoreGG - Licensed under the MIT license.
///
/// Name         :  imgui_positioning.h
/// Description  :  Positioning extension for ImGui.
///----------------------------------------------------------------------------------------------------

#pragma once

#include <assert.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <windows.h>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

///----------------------------------------------------------------------------------------------------
/// ImGuiExt Namespace
///----------------------------------------------------------------------------------------------------
namespace ImGuiExt
{
/* INI Keys */
#define KEY_MODE         "Mode"
#define KEY_ORIGINCORNER "OriginCorner"
#define KEY_TARGETCORNER "TargetCorner"
#define KEY_TARGETID     "TargetID"
#define KEY_X            "X"
#define KEY_Y            "Y"

	///----------------------------------------------------------------------------------------------------
	/// EPositioning Enumeration
	///----------------------------------------------------------------------------------------------------
	enum class EPositioning
	{
		Manual,
		Anchor,
		ScreenRelative
	};

	///----------------------------------------------------------------------------------------------------
	/// EAnchorPoint Enumeration
	///----------------------------------------------------------------------------------------------------
	enum class EAnchorPoint
	{
		TopLeft,
		TopRight,
		BottomRight,
		BottomLeft
	};

	///----------------------------------------------------------------------------------------------------
	/// StringFrom(EAnchorPoint):
	/// 	Enum to string.
	///----------------------------------------------------------------------------------------------------
	static std::string StringFrom(EAnchorPoint aAnchorPoint)
	{
		switch (aAnchorPoint)
		{
			default:
			case EAnchorPoint::TopLeft:
			{
				return "Top Left";
			}
			case EAnchorPoint::TopRight:
			{
				return "Top Right";
			}
			case EAnchorPoint::BottomRight:
			{
				return "Bottom Right";
			}
			case EAnchorPoint::BottomLeft:
			{
				return "Bottom Left";
			}
		}
	}

	///----------------------------------------------------------------------------------------------------
	/// Positioning_t Struct
	/// 	Contains a configuration for a given window.
	///----------------------------------------------------------------------------------------------------
	struct Positioning_t
	{
		EPositioning Mode         = EPositioning::Manual;

		EAnchorPoint OriginCorner = EAnchorPoint::TopLeft;
		EAnchorPoint TargetCorner = EAnchorPoint::TopLeft;
		ImGuiID      TargetID     = 0;
		float        X            = 0.f;
		float        Y            = 0.f;

		bool         ShouldSave   = false;
	};

	///----------------------------------------------------------------------------------------------------
	/// AnchorPointCombo:
	/// 	Widget to draw an anchor point selector combo box.
	/// 	Returns true if the value was modified.
	///----------------------------------------------------------------------------------------------------
	static bool AnchorPointCombo(std::string aLabel, EAnchorPoint* aAnchorPoint)
	{
		assert(aAnchorPoint);
		bool result = false;

		if (ImGui::BeginCombo(aLabel.c_str(), StringFrom(*aAnchorPoint).c_str()))
		{
			if (ImGui::Selectable(StringFrom(EAnchorPoint::TopLeft).c_str(), *aAnchorPoint == EAnchorPoint::TopLeft))
			{
				*aAnchorPoint = EAnchorPoint::TopLeft;
				result = true;
			}
			if (ImGui::Selectable(StringFrom(EAnchorPoint::TopRight).c_str(), *aAnchorPoint == EAnchorPoint::TopRight))
			{
				*aAnchorPoint = EAnchorPoint::TopRight;
				result = true;
			}
			if (ImGui::Selectable(StringFrom(EAnchorPoint::BottomRight).c_str(), *aAnchorPoint == EAnchorPoint::BottomRight))
			{
				*aAnchorPoint = EAnchorPoint::BottomRight;
				result = true;
			}
			if (ImGui::Selectable(StringFrom(EAnchorPoint::BottomLeft).c_str(), *aAnchorPoint == EAnchorPoint::BottomLeft))
			{
				*aAnchorPoint = EAnchorPoint::BottomLeft;
				result = true;
			}

			ImGui::EndCombo();
		}

		return result;
	}

	///----------------------------------------------------------------------------------------------------
	/// IniFilename:
	/// 	Helper to get
	///----------------------------------------------------------------------------------------------------
	static std::string IniFilename()
	{
		assert(ImGui::GetIO().IniFilename);
		return ImGui::GetIO().IniFilename + std::string(".ext");
	}

	///----------------------------------------------------------------------------------------------------
	/// IniSectionFromWindowName:
	/// 	Helper to generate ini sections from a given window name.
	///----------------------------------------------------------------------------------------------------
	static std::string IniSectionFromWindowName(std::string& aWindowName)
	{
		std::string result;

		size_t offset = aWindowName.find("###");
		if (offset != std::string::npos)
		{
			result = aWindowName.substr(offset);
		}
		else
		{
			result = aWindowName;
		}

		/* ImGui INI sections follow format: [Window][ID]. */
		result = "EXT_POS_" + result;

		return result;
	}

	///----------------------------------------------------------------------------------------------------
	/// s_Storage:
	/// 	Runtime storage for positioning settings.
	///----------------------------------------------------------------------------------------------------
	static std::unordered_map<ImGuiID, Positioning_t> s_Storage;

	///----------------------------------------------------------------------------------------------------
	/// UpdatePosition:
	/// 	Sets the position for the window given the settings.
	/// 	Return value should be combined with window flags, as it controls NoMove depending on config.
	///----------------------------------------------------------------------------------------------------
	static ImGuiWindowFlags UpdatePosition(std::string aWindowName)
	{
		Positioning_t* position = nullptr;

		ImGuiID id = ImHashStr(aWindowName.c_str());

		auto it = s_Storage.find(id);

		/* If in memory storage. */
		if (it != s_Storage.end())
		{
			position = &it->second;
		}
		else
		{
			/* Read from disk. */
			std::string section = IniSectionFromWindowName(aWindowName);

			char buffer[256];

			/* Emplace empty and grab pointer. */
			auto result = s_Storage.emplace(id, Positioning_t{});
			position = &result.first->second;

			GetPrivateProfileStringA(section.c_str(), KEY_MODE, "0", buffer, sizeof(buffer), IniFilename().c_str());
			position->Mode = static_cast<EPositioning>(std::atoi(buffer));

			GetPrivateProfileStringA(section.c_str(), KEY_ORIGINCORNER, "0", buffer, sizeof(buffer), IniFilename().c_str());
			position->OriginCorner = static_cast<EAnchorPoint>(std::atoi(buffer));

			GetPrivateProfileStringA(section.c_str(), KEY_TARGETCORNER, "0", buffer, sizeof(buffer), IniFilename().c_str());
			position->TargetCorner = static_cast<EAnchorPoint>(std::atoi(buffer));

			GetPrivateProfileStringA(section.c_str(), KEY_TARGETID, "0", buffer, sizeof(buffer), IniFilename().c_str());
			position->TargetID = static_cast<ImGuiID>(std::strtoul(buffer, nullptr, 10));

			GetPrivateProfileStringA(section.c_str(), KEY_X, "0", buffer, sizeof(buffer), IniFilename().c_str());
			position->X = static_cast<float>(std::atof(buffer));

			GetPrivateProfileStringA(section.c_str(), KEY_Y, "0", buffer, sizeof(buffer), IniFilename().c_str());
			position->Y = static_cast<float>(std::atof(buffer));
		}

		assert(position);

		ImGuiWindowFlags result = 0;

		switch (position->Mode)
		{
			default:
			case EPositioning::Manual: { break; }
			case EPositioning::Anchor:
			{
				ImVec2 pivot;
				switch (position->OriginCorner)
				{
					default:
					case EAnchorPoint::TopLeft:
						pivot = ImVec2(0.f, 0.f);
						break;
					case EAnchorPoint::TopRight:
						pivot = ImVec2(1.f, 0.f);
						break;
					case EAnchorPoint::BottomRight:
						pivot = ImVec2(1.f, 1.f);
						break;
					case EAnchorPoint::BottomLeft:
						pivot = ImVec2(0.f, 1.f);
						break;
				}
				ImVec2 pos;
				ImGuiWindow* targetWindow = ImGui::FindWindowByID(position->TargetID);

				if (!targetWindow)
				{
					break;
				}

				switch (position->TargetCorner)
				{
					default:
					case EAnchorPoint::TopLeft:
						pos = ImVec2(targetWindow->Pos.x + position->X, targetWindow->Pos.y + position->Y);
						break;
					case EAnchorPoint::TopRight:
						pos = ImVec2(targetWindow->Pos.x + targetWindow->Size.x + position->X, targetWindow->Pos.y + position->Y);
						break;
					case EAnchorPoint::BottomRight:
						pos = ImVec2(targetWindow->Pos.x + targetWindow->Size.x + position->X, targetWindow->Pos.y + targetWindow->Size.y + position->Y);
						break;
					case EAnchorPoint::BottomLeft:
						pos = ImVec2(targetWindow->Pos.x + position->X, targetWindow->Pos.y + targetWindow->Size.y + position->Y);
						break;
				}
				ImGui::SetNextWindowPos(pos, ImGuiCond_Always, pivot);
				result |= ImGuiWindowFlags_NoMove;
				break;
			}
			case EPositioning::ScreenRelative:
			{
				ImVec2 pivot;
				ImVec2 pos;
				switch (position->TargetCorner)
				{
					default:
					case EAnchorPoint::TopLeft:
						pivot = ImVec2(0.f, 0.f);
						pos = ImVec2(0.f + position->X, 0.f + position->Y);
						break;
					case EAnchorPoint::TopRight:
						pivot = ImVec2(1.f, 0.f);
						pos = ImVec2(ImGui::GetIO().DisplaySize.x - position->X, 0.f + position->Y);
						break;
					case EAnchorPoint::BottomRight:
						pivot = ImVec2(1.f, 1.f);
						pos = ImVec2(ImGui::GetIO().DisplaySize.x - position->X, ImGui::GetIO().DisplaySize.y - position->Y);
						break;
					case EAnchorPoint::BottomLeft:
						pivot = ImVec2(0.f, 1.f);
						pos = ImVec2(0.f + position->X, ImGui::GetIO().DisplaySize.y - position->Y);
						break;
				}
				ImGui::SetNextWindowPos(pos, ImGuiCond_Always, pivot);
				result |= ImGuiWindowFlags_NoMove;
				break;
			}
		}

		if (position->ShouldSave)
		{
			std::string section = IniSectionFromWindowName(aWindowName);

			WritePrivateProfileStringA(section.c_str(), KEY_MODE, std::to_string(static_cast<int>(position->Mode)).c_str(), IniFilename().c_str());
			WritePrivateProfileStringA(section.c_str(), KEY_ORIGINCORNER, std::to_string(static_cast<int>(position->OriginCorner)).c_str(), IniFilename().c_str());
			WritePrivateProfileStringA(section.c_str(), KEY_TARGETCORNER, std::to_string(static_cast<int>(position->TargetCorner)).c_str(), IniFilename().c_str());
			WritePrivateProfileStringA(section.c_str(), KEY_TARGETID, std::to_string(static_cast<unsigned int>(position->TargetID)).c_str(), IniFilename().c_str());
			WritePrivateProfileStringA(section.c_str(), KEY_X, std::to_string(position->X).c_str(), IniFilename().c_str());
			WritePrivateProfileStringA(section.c_str(), KEY_Y, std::to_string(position->Y).c_str(), IniFilename().c_str());

			position->ShouldSave = false;
		}

		return result;
	}

	///----------------------------------------------------------------------------------------------------
	/// ContextMenuPosition:
	/// 	Renders the context menu with the position configuration.
	/// 	Must call before ImGui::End();
	///----------------------------------------------------------------------------------------------------
	static void ContextMenuPosition(std::string aContextMenuName)
	{
		ImGuiWindow* currentWindow = ImGui::GetCurrentWindow();

		assert(currentWindow);

		Positioning_t* position = &s_Storage.find(currentWindow->ID)->second;

		if (ImGui::BeginPopupContextWindow(aContextMenuName.c_str(), ImGuiPopupFlags_MouseButtonRight))
		{
			if (ImGui::BeginMenu("Position"))
			{
				if (ImGui::RadioButton("Manual", position->Mode == EPositioning::Manual))
				{
					position->Mode = EPositioning::Manual;
					position->ShouldSave = true;
				}
				if (ImGui::RadioButton("Anchor to window", position->Mode == EPositioning::Anchor))
				{
					position->Mode = EPositioning::Anchor;
					position->ShouldSave = true;
				}
				if (ImGui::RadioButton("Screen-relative", position->Mode == EPositioning::ScreenRelative))
				{
					position->Mode = EPositioning::ScreenRelative;
					position->ShouldSave = true;
				}

				switch (position->Mode)
				{
					default:
					case EPositioning::Manual:
					{
						/* No additional options, nothing to do. */
						break;
					}
					case EPositioning::Anchor:
					{
						ImGui::Separator();

						if (AnchorPointCombo("This Anchor", &position->OriginCorner))
						{
							position->ShouldSave = true;
						}
						if (AnchorPointCombo("Target Anchor", &position->TargetCorner))
						{
							position->ShouldSave = true;
						}

						std::vector<ImGuiWindow*> targetWindows;

						/* GET_WINDOW_IDS */
						{
							ImGuiContext* ctx = ImGui::GetCurrentContext();
							const int oldestSelectableFrame = ctx->FrameCount - 1;
							for (ImGuiWindow* wnd : ctx->Windows)
							{
								if (strcmp(wnd->Name, "Debug##Default") == 0) { continue; }
								if (wnd->ID == currentWindow->ID) { continue; }
								if (wnd->ParentWindow != NULL) { continue; }
								if (wnd->Hidden) { continue; }
								// "Active" is only true after Begin() in the current frame.
								// Including last-frame-active windows allows selecting windows
								// that are rendered later in the current frame.
								if (wnd->LastFrameActive < oldestSelectableFrame) { continue; }

								targetWindows.push_back(wnd);
							}
						}

						ImGuiWindow* targetWindow = ImGui::FindWindowByID(position->TargetID);

						ImGuiID hoveredID = 0;

						if (ImGui::BeginCombo("Target Window", targetWindow ? targetWindow->Name : "(null)"))
						{
							for (ImGuiWindow* wnd : targetWindows)
							{
								if (ImGui::Selectable(wnd->Name))
								{
									position->TargetID = wnd->ID;
									position->ShouldSave = true;
								}
								if (ImGui::IsItemHovered())
								{
									hoveredID = wnd->ID;
								}
							}
							ImGui::EndCombo();
						}

						if (ImGui::InputFloat("X", &position->X, 1.f, 50.f, "%.0f"))
						{
							position->ShouldSave = true;
						}
						if (ImGui::InputFloat("Y", &position->Y, 1.f, 50.f, "%.0f"))
						{
							position->ShouldSave = true;
						}

						/* RENDER_WINDOW_IDS */
						{
							ImDrawList* dl = ImGui::GetForegroundDrawList();

							bool canConnect = false;
							ImVec2 centerTarget;

							for (ImGuiWindow* wnd : targetWindows)
							{
								ImVec2 nameSz = ImGui::CalcTextSize(wnd->Name);

								ImColor highlight = wnd->ID == position->TargetID || wnd->ID == hoveredID
									? ImColor(1.f, 0.f, 0.f, 1.f)
									: ImColor(1.f, 1.f, 0.f, 1.f);

								/* Draw background for label. */
								dl->AddRectFilled(ImVec2(wnd->Pos.x, wnd->Pos.y), ImVec2(wnd->Pos.x + nameSz.x + 2.f, wnd->Pos.y + nameSz.y + 2.f), ImColor(0.f, 0.f, 0.f, 1.f));

								/* Draw outline for label. */
								dl->AddRect(ImVec2(wnd->Pos.x, wnd->Pos.y), ImVec2(wnd->Pos.x + nameSz.x + 2.f, wnd->Pos.y + nameSz.y + 2.f), highlight);

								/* Draw outline for window. */
								dl->AddRect(ImVec2(wnd->Pos.x, wnd->Pos.y), ImVec2(wnd->Pos.x + wnd->Size.x, wnd->Pos.y + wnd->Size.y), highlight);

								/* Draw label text. */
								dl->AddText(ImVec2(wnd->Pos.x + 1.f, wnd->Pos.y + 1.f), highlight, wnd->Name);

								if (wnd->ID == position->TargetID)
								{
									switch (position->TargetCorner)
									{
										default:
										case EAnchorPoint::TopLeft:
											centerTarget = ImVec2(wnd->Pos.x, wnd->Pos.y);
											break;
										case EAnchorPoint::TopRight:
											centerTarget = ImVec2(wnd->Pos.x + wnd->Size.x, wnd->Pos.y);
											break;
										case EAnchorPoint::BottomRight:
											centerTarget = ImVec2(wnd->Pos.x + wnd->Size.x, wnd->Pos.y + wnd->Size.y);
											break;
										case EAnchorPoint::BottomLeft:
											centerTarget = ImVec2(wnd->Pos.x, wnd->Pos.y + wnd->Size.y);
											break;
									}

									canConnect = true;
								}
							}

							ImVec2 centerOrigin;
							switch (position->OriginCorner)
							{
								default:
								case EAnchorPoint::TopLeft:
									centerOrigin = ImVec2(currentWindow->Pos.x, currentWindow->Pos.y);
									break;
								case EAnchorPoint::TopRight:
									centerOrigin = ImVec2(currentWindow->Pos.x + currentWindow->Size.x, currentWindow->Pos.y);
									break;
								case EAnchorPoint::BottomRight:
									centerOrigin = ImVec2(currentWindow->Pos.x + currentWindow->Size.x, currentWindow->Pos.y + currentWindow->Size.y);
									break;
								case EAnchorPoint::BottomLeft:
									centerOrigin = ImVec2(currentWindow->Pos.x, currentWindow->Pos.y + currentWindow->Size.y);
									break;
							}

							if (canConnect)
							{
								dl->AddLine(centerOrigin, centerTarget, ImColor(1.f, 1.f, 0.f, 1.f));
								dl->AddCircleFilled(centerTarget, 3.f, ImColor(1.f, 0.f, 0.f, 1.f));
							}

							dl->AddCircleFilled(centerOrigin, 3.f, ImColor(1.f, 0.f, 0.f, 1.f));
						}

						break;
					}
					case EPositioning::ScreenRelative:
					{
						ImGui::Separator();

						if (AnchorPointCombo("Screen Anchor", &position->TargetCorner))
						{
							position->ShouldSave = true;
						}
						if (ImGui::InputFloat("X", &position->X, 1.f, 50.f, "%.0f"))
						{
							position->ShouldSave = true;
						}
						if (ImGui::InputFloat("Y", &position->Y, 1.f, 50.f, "%.0f"))
						{
							position->ShouldSave = true;
						}

						{
							ImDrawList* dl = ImGui::GetForegroundDrawList();

							ImVec2 centerOrigin;
							ImVec2 centerTarget;
							switch (position->TargetCorner)
							{
								default:
								case EAnchorPoint::TopLeft:
									centerOrigin = ImVec2(currentWindow->Pos.x, currentWindow->Pos.y);
									centerTarget = ImVec2(0.f, 0.f);
									break;
								case EAnchorPoint::TopRight:
									centerOrigin = ImVec2(currentWindow->Pos.x + currentWindow->Size.x, currentWindow->Pos.y);
									centerTarget = ImVec2(ImGui::GetIO().DisplaySize.x, 0.f);
									break;
								case EAnchorPoint::BottomRight:
									centerOrigin = ImVec2(currentWindow->Pos.x + currentWindow->Size.x, currentWindow->Pos.y + currentWindow->Size.y);
									centerTarget = ImVec2(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y);
									break;
								case EAnchorPoint::BottomLeft:
									centerOrigin = ImVec2(currentWindow->Pos.x, currentWindow->Pos.y + currentWindow->Size.y);
									centerTarget = ImVec2(0.f, ImGui::GetIO().DisplaySize.y);
									break;
							}

							dl->AddLine(centerOrigin, centerTarget, ImColor(1.f, 1.f, 0.f, 1.f));
							dl->AddCircleFilled(centerTarget, 3.f, ImColor(1.f, 0.f, 0.f, 1.f));
							dl->AddCircleFilled(centerOrigin, 3.f, ImColor(1.f, 0.f, 0.f, 1.f));
						}

						break;
					}
				}

				ImGui::EndMenu();
			}
			ImGui::EndPopup();
		}
	}
}
