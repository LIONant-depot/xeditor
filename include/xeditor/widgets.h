#ifndef XEDITOR_WIDGETS_H
#define XEDITOR_WIDGETS_H
#pragma once

// Small ImGui pieces every editor's tree and list panels share.
#include "imgui.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstring>
#include <string>
#include <string_view>

namespace xeditor
{
    // The dots in the middle of a splitter (the bar that is dragged to resize a panel): they say where the drag bar is, which a thin line does not. Min/Max are the rectangle of the bar;
    // bAlongX is true for a bar that runs left to right (dragged up and down), false for one that runs top to bottom. Brighter while the mouse is on it or it is dragged.
    inline void DrawSplitterGrip(ImDrawList* pList, ImVec2 Min, ImVec2 Max, bool bAlongX, bool bHot) noexcept
    {
        const ImVec2 Center((Min.x + Max.x) * 0.5f, (Min.y + Max.y) * 0.5f);
        const ImU32  Dot = ImGui::GetColorU32(bHot ? ImGuiCol_Text : ImGuiCol_TextDisabled);
        for (int i = -2; i <= 2; ++i)
        {
            const ImVec2 At = bAlongX ? ImVec2(Center.x + i * 6.0f, Center.y) : ImVec2(Center.x, Center.y + i * 6.0f);
            pList->AddRectFilled(ImVec2(At.x - 1.5f, At.y - 1.5f), ImVec2(At.x + 1.5f, At.y + 1.5f), Dot);
        }
    }

    inline bool ContainsCaseInsensitive(std::string_view Haystack, std::string_view Needle) noexcept
    {
        if (Needle.empty()) return true;
        auto It = std::search(Haystack.begin(), Haystack.end(), Needle.begin(), Needle.end(),
            [](char A, char B) noexcept { return std::tolower(static_cast<unsigned char>(A)) == std::tolower(static_cast<unsigned char>(B)); });
        return It != Haystack.end();
    }

    // THE back / forward pair of the editors (the asset browser's path bar uses these same two glyphs): transparent arrow buttons, greyed out when there is nowhere
    // to go, each with a tooltip saying where it goes (also while greyed: it says why). Sets bBack / bForward on the frame one is pressed. The centres of the buttons come
    // back through pBackAt / pForwardAt when given (screen coordinates), for whoever wants to point at them. Both end with the cursor on the same line, after the pair.
    inline void RenderBackForwardButtons(bool bCanBack, bool bCanForward, bool& bBack, bool& bForward, const char* pBackTip = nullptr, const char* pForwardTip = nullptr
        , float* pBackAt = nullptr, float* pForwardAt = nullptr) noexcept
    {
        auto One = [](const char* pGlyph, const char* pId, bool bEnabled, const char* pTip, float* pAt) noexcept
        {
            ImGui::BeginDisabled(!bEnabled);
            const bool bPressed = ImGui::Button(std::string(pGlyph).append("##").append(pId).c_str());
            ImGui::EndDisabled();
            if (pTip && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) ImGui::SetTooltip("%s", pTip);
            if (pAt) { pAt[0] = ImGui::GetItemRectMin().x + ImGui::GetItemRectSize().x * 0.5f; pAt[1] = ImGui::GetItemRectMin().y + ImGui::GetItemRectSize().y * 0.5f; }
            return bPressed && bEnabled;
        };
        bBack = bForward = false;
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        bBack = One("\xEE\x9C\xAB", "back", bCanBack, pBackTip, pBackAt);
        ImGui::SameLine(0, 2.0f);
        bForward = One("\xEE\x9C\xAA", "forward", bCanForward, pForwardTip, pForwardAt);
        ImGui::PopStyleColor();
        ImGui::SameLine(0, 6.0f);
    }

    // THE search box of the editors (the asset browser, the Level tree, the component selector, the command palette...): a
    // magnifying-glass placeholder, a gray "X" that kills the whole text (shown once there is some) and a rounded input. It edits a caller-owned string, so each panel keeps
    // its own search text. bFocus puts the keyboard in it (the frame a popup opens). pHelp, when given, is what hovering the box says: for a search that has a
    // grammar of its own (the Logs' "sev>=error channel:game.*"), so it explains itself without a second kind of box. True when the text changed this frame.
    inline bool RenderTreeSearchBar(std::string& SearchString, float AvailWidth, bool bFocus = false, const char* pHelp = nullptr) noexcept
    {
        bool bChanged = false;
        std::array<char, 256> Buffer{};
        strcpy_s(Buffer.data(), Buffer.size(), SearchString.c_str());

        const auto StartX = ImGui::GetCursorPosX();
        if (Buffer[0] != 0)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
            if (ImGui::SmallButton("X")) { Buffer[0] = 0; bChanged = true; }
            ImGui::PopStyleColor();
            ImGui::SameLine(0, 0.1f);
        }
        AvailWidth -= ImGui::GetCursorPosX() - StartX;

        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 7.0f);
        ImGui::PushItemWidth(AvailWidth);
        if (bFocus) ImGui::SetKeyboardFocusHere();
        bChanged |= ImGui::InputText("##TreeSearch", Buffer.data(), Buffer.size());
        const bool bActive  = ImGui::IsItemActive();
        const bool bHasText = (Buffer[0] != 0);
        if (pHelp && ImGui::IsItemHovered()) ImGui::SetTooltip("%s", pHelp);
        if (!bActive && !bHasText)
        {
            // Drawn on the window's draw list, not with Text + SetCursorScreenPos: that would leave the layout believing the icon was the last item, and anything
            // placed beside the box (SameLine) would land next to the icon instead of after the box.
            const ImVec2 InputPos = ImGui::GetItemRectMin();
            ImGui::GetWindowDrawList()->AddText(ImVec2(InputPos.x + 10.0f, InputPos.y + 4.0f), IM_COL32(128, 128, 128, 255), "\xee\x9c\xa1");
        }
        ImGui::PopItemWidth();
        ImGui::PopStyleVar();

        SearchString = std::string_view(Buffer.data());
        return bChanged;
    }
}

#endif // XEDITOR_WIDGETS_H
