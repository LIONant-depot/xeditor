#ifndef XEDITOR_POPUP_H
#define XEDITOR_POPUP_H
#pragma once

// The one way to open a modal window (an error, a question, anything that needs the user's attention before the editor goes on).
// ImGui centers a modal on the whole application window, but an editor is only a part of it (a full editor has its own dockspace inside the
// window): a modal that belongs to an editor opens in the middle of that editor, with the same flags everywhere.
//
//      ImGui::OpenPopup("Save changes?");                  // as before, from the top-level ID scope of the frame (see notify.h)
//      if (xeditor::BeginModal("Save changes?")) { ... ImGui::EndPopup(); }
//
// The editor is found from the dock tree: the dockspace the current window sits in (a panel of the editor), else the one of the window that has
// the focus (for a popup drawn from the top level of the frame, like the error popup), else the application window.
#include "imgui.h"
#include "imgui_internal.h"

namespace xeditor
{
    // The part of the screen of the editor the current window belongs to (screen coordinates). bFocusedFirst asks for the editor that has
    // the focus instead: right for an event that happens "now" and shows its window later, from somewhere else (an error raised by a command).
    inline ImRect EditorRect( bool bFocusedFirst = false ) noexcept
    {
        ImGuiContext& g = *ImGui::GetCurrentContext();
        const ImGuiWindow* Candidates[2] = { bFocusedFirst ? g.NavWindow : g.CurrentWindow, bFocusedFirst ? g.CurrentWindow : g.NavWindow };
        for (const ImGuiWindow* pWindow : Candidates)
        {
            if (pWindow == nullptr || pWindow->DockNode == nullptr) continue;
            if (const ImGuiDockNode* pRoot = ImGui::DockNodeGetRootNode(pWindow->DockNode); pRoot != nullptr && pRoot->Size.x > 0.0f && pRoot->Size.y > 0.0f)
                return ImRect(pRoot->Pos, ImVec2(pRoot->Pos.x + pRoot->Size.x, pRoot->Pos.y + pRoot->Size.y));
        }
        const ImGuiViewport* pViewport = ImGui::GetMainViewport();
        return ImRect(pViewport->WorkPos, ImVec2(pViewport->WorkPos.x + pViewport->WorkSize.x, pViewport->WorkPos.y + pViewport->WorkSize.y));
    }

    // Call right before any popup that should open centered on its editor (or on pCenter, when the caller knows better). Only when it appears:
    // the user can move it after that, and it stays where they put it until it closes.
    inline void CenterNextPopup( const ImVec2* pCenter = nullptr ) noexcept
    {
        ImGui::SetNextWindowPos(pCenter ? *pCenter : EditorRect().GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    }

    // BeginPopupModal for every modal window of the editors: centered on its editor, sized to its content, never remembered in the .ini.
    // Returns what BeginPopupModal returns (call ImGui::EndPopup when it is true).
    inline bool BeginModal( const char* pName, ImGuiWindowFlags Flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings, const ImVec2* pCenter = nullptr ) noexcept
    {
        CenterNextPopup(pCenter);
        return ImGui::BeginPopupModal(pName, nullptr, Flags);
    }
}

#endif // XEDITOR_POPUP_H
