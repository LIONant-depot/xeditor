#ifndef XEDITOR_NOTIFY_H
#define XEDITOR_NOTIFY_H
#pragma once

#include "imgui.h"
#include "popup.h"

#include <algorithm>
#include <functional>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace xeditor
{
    // What an error does to the person (documentation/Editors/DESIGN_logs.md, 6.2). Every error is recorded in the Logs whatever the style: the style is only how loudly it is told.
    //   Badge  records it; the Logs badge counts it. For what nobody asked for (a background failure).
    //   Toast  a line that appears, says what failed and offers the problem, stacks, expires, never takes focus or blocks anything. For what the person just did (Save, Compile, Play, a command).
    //   Modal  the person must decide or acknowledge, or the editor cannot go on (a failed init): takes the interaction, so it is rare.
    enum class notify_style : std::uint8_t { Badge, Toast, Modal };

    // The toasts on screen. raise() may be called from any thread (no ImGui in it); render() draws them, once a frame, from the top-level ID scope.
    struct toaster
    {
        struct toast { std::uint64_t m_Id = 0; std::string m_Text; double m_Born = -1.0; float m_Height = 0.0f; };

        std::mutex          m_Mutex;
        std::vector<toast>  m_List;
        std::uint64_t       m_NextId = 1;
        std::uint64_t       m_Raised = 0;                  // how many were ever raised (a test reads it: a toast is a window that is gone in a few seconds)
        float               m_Seconds = 8.0f;              // how long one stays when nobody is pointing at it

        void raise(std::string_view Message) noexcept
        {
            std::lock_guard Lock(m_Mutex);
            ++m_Raised;
            // the same text again refreshes the one on screen instead of stacking a copy
            for (auto& T : m_List) if (T.m_Text == Message) { T.m_Born = -1.0; return; }
            m_List.push_back({ m_NextId++, std::string(Message), -1.0, 0.0f });
            if (m_List.size() > 4) m_List.erase(m_List.begin());
        }

        std::size_t size() noexcept { std::lock_guard Lock(m_Mutex); return m_List.size(); }

        // OnOpenProblems: what "Open problem" does (the host opens the Logs on the problems). BottomInset: how far above the bottom of the window the lowest toast sits.
        void render(const std::function<void()>& OnOpenProblems, float BottomInset) noexcept
        {
            std::lock_guard Lock(m_Mutex);
            if (m_List.empty()) return;
            const double Now = ImGui::GetTime();
            ImGuiViewport* pVp = ImGui::GetMainViewport();
            float Y = pVp->Pos.y + pVp->Size.y - BottomInset;
            std::vector<std::uint64_t> Gone;
            for (auto It = m_List.rbegin(); It != m_List.rend(); ++It)            // newest at the bottom
            {
                toast& T = *It;
                if (T.m_Born < 0.0) T.m_Born = Now;
                ImGui::SetNextWindowPos(ImVec2(pVp->Pos.x + pVp->Size.x - 12.0f, Y), ImGuiCond_Always, ImVec2(1.0f, 1.0f));
                ImGui::SetNextWindowBgAlpha(0.92f);
                const ImGuiWindowFlags Flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking
                                             | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_AlwaysAutoResize;
                bool bDone = false;
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 8.0f));
                if (ImGui::Begin(("##xeditor.toast" + std::to_string(T.m_Id)).c_str(), nullptr, Flags))
                {
                    const ImVec2 At = ImGui::GetCursorScreenPos();
                    ImGui::GetWindowDrawList()->AddCircleFilled(ImVec2(At.x + 5.0f, At.y + ImGui::GetTextLineHeight() * 0.5f), 4.0f, IM_COL32(212, 107, 105, 255));
                    ImGui::Dummy(ImVec2(14.0f, 1.0f));
                    ImGui::SameLine(0, 0);
                    ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + 340.0f);
                    ImGui::TextUnformatted(T.m_Text.c_str());
                    ImGui::PopTextWrapPos();
                    if (OnOpenProblems && ImGui::SmallButton("Open problem")) { OnOpenProblems(); bDone = true; }
                    ImGui::SameLine();
                    if (ImGui::SmallButton("Dismiss")) bDone = true;
                    if (ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem)) T.m_Born = Now;       // pointing at it keeps it
                    T.m_Height = ImGui::GetWindowSize().y;
                }
                ImGui::End();
                ImGui::PopStyleVar();
                Y -= T.m_Height + 6.0f;
                if (bDone || Now - T.m_Born > m_Seconds) Gone.push_back(T.m_Id);
            }
            for (auto Id : Gone) m_List.erase(std::remove_if(m_List.begin(), m_List.end(), [&](const toast& T) { return T.m_Id == Id; }), m_List.end());
        }
    };

    // The most recent user-visible error. Anything may raise one (see NotifyError in host.h); the UI shows it as a
    // modal, and a headless host still gets the text in the process log and as the failing command's reply.
    struct notifier
    {
        std::string m_Message;
        bool        m_bOpenRequested = false;
        ImVec2      m_Anchor         = {};       // where the popup opens: the middle of the editor that had the focus when the error was raised
        bool        m_bHasAnchor     = false;

        void raise(std::string_view Message) noexcept
        {
            m_Message        = Message;
            m_bOpenRequested = true;            // only a flag: the popup is opened from render(), never from the caller's ID scope
            m_bHasAnchor     = ImGui::GetCurrentContext() != nullptr;      // a headless host has no UI
            if (m_bHasAnchor) m_Anchor = EditorRect(true).GetCenter();     // the error belongs to the editor being used now, not to whoever draws the popup
        }

        // Call once per frame from the top-level ID scope. OpenPopup hashes its id against the ID stack of the call site,
        // so opening it from wherever raise() happened (mid drag-drop, inside a per-row PushID) would give it an id that
        // BeginPopupModal never finds, and the modal would silently never appear.
        void render() noexcept
        {
            if (m_bOpenRequested)
            {
                ImGui::OpenPopup("Error###xeditor.notify");
                m_bOpenRequested = false;
            }

            ImGui::SetNextWindowSize(ImVec2(420.0f, 0.0f), ImGuiCond_Appearing);
            if (xeditor::BeginModal("Error###xeditor.notify", ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings, m_bHasAnchor ? &m_Anchor : nullptr))
            {
                ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + 400.0f);
                ImGui::TextUnformatted(m_Message.c_str());
                ImGui::PopTextWrapPos();
                ImGui::Separator();
                if (ImGui::Button("OK", ImVec2(120.0f, 0.0f)) || ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_Escape))
                    ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
            }
        }
    };
}

#endif
