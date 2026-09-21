#ifndef XEDITOR_NOTIFY_H
#define XEDITOR_NOTIFY_H
#pragma once

#include "imgui.h"

#include <string>
#include <string_view>

namespace xeditor
{
    // The most recent user-visible error. Anything may raise one (see NotifyError in host.h); the UI shows it as a
    // modal, and a headless host still gets the text in the process log and as the failing command's reply.
    struct notifier
    {
        std::string m_Message;
        bool        m_bOpenRequested = false;

        void raise(std::string_view Message) noexcept
        {
            m_Message        = Message;
            m_bOpenRequested = true;            // only a flag: the popup is opened from render(), never from the caller's ID scope
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
            if (ImGui::BeginPopupModal("Error###xeditor.notify", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
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
