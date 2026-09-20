#ifndef XEDITOR_FULL_EDITOR_SHELL_H
#define XEDITOR_FULL_EDITOR_SHELL_H
#pragma once

// Thin full-editor dock helpers. Callers still own Begin/MenuBar/title
// (FormatEditorRootTabTitle + DrawEditorRootTabIcon live in Tools/Editor shims).
// This keeps isolation + zero-padding conventions in one place for Level/Texture.

#include "dock_isolation.h"
#include "imgui.h"
#include "imgui_internal.h"

namespace xeditor {

inline void PushFullEditorRootStyle() noexcept
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
}

inline void PopFullEditorRootStyle() noexcept
{
    ImGui::PopStyleVar();
}

// After DockSpace(...): isolate by resource guid, or apply a fallback class.
inline void FinishFullEditorDockspace(ImGuiID DockspaceId, xresource::full_guid DockGuid,
                                      ImGuiID FallbackClassId = 0xE290A17u) noexcept
{
    if (!DockGuid.empty())
    {
        IsolateEditorDockspace(DockspaceId, DockGuid);
        return;
    }
    ImGuiWindowClass Fallback;
    Fallback.ClassId = FallbackClassId;
    Fallback.DockingAllowUnclassed = false;
    Fallback.DockingAlwaysTabBar = true;
    ApplyDockClassToTree(ImGui::DockBuilderGetNode(DockspaceId), Fallback);
}

} // namespace xeditor

#endif
