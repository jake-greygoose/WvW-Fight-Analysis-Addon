# imgui_positioning
Plug and play extension for imgui allowing window docking or screen relative positions.

The configuration window will highlight the selected, as well as hovered windows in docking/anchoring mode, to easily visualise which window is target.

A line is drawn from the anchor to the window.

These extended settings will be stored at `imgui.ini.ext`. The current `imgui.ini` path is read from the IO context.

# Usage
You will have to call `ImGuiExt::UpdatePosition()` before `ImGui::Begin()`. Its return value determines whether the window should have a `NoMove` flag or not.

Before `ImGui::End()` you should call `ImGuiExt::ContextMenuPosition()`. You need specify a name for your context menu. This way you can append to an existing context menu.

```cpp
void Render()
{
	ImGuiWindowFlags wndFlags
		= ImGuiWindowFlags_AlwaysAutoResize
		| ImGuiWindowFlags_NoCollapse
		| ImGuiExt::UpdatePosition("MyWindow###EvenWithID"); // adds 0 or the NoMove flag. Depending on configuration.

	if (ImGui::Begin("MyWindow###EvenWithID", 0, wndFlags))
	{
		/* window contents */
	}

	if (ImGui::BeginPopupContextWindow("MyWindow###EvenWithID##ContextMenu", ImGuiPopupFlags_MouseButtonRight))
	{
		/* other context menu options */
		ImGui::EndPopup();
	}
	ImGuiExt::ContextMenuPosition("MyWindow###EvenWithID##ContextMenu"); // ID matches other context menu

	ImGui::End();
}
```
<img width="640" height="359" alt="image" src="https://github.com/user-attachments/assets/d25282d8-7330-4bc7-9163-a8fc03636d39" />
