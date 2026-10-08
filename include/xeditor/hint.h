#ifndef XEDITOR_HINT_H
#define XEDITOR_HINT_H
#pragma once

// THE hint of the editors: what appears when the mouse rests on a button, a menu item, a property or a tab.
//
//      Play  [F5]                    topic, and the shortcut key as a key cap (when it has one)
//      Starts playing this Level.    body: what it does, wrapped
//      Unavailable: nothing selected why it cannot be used right now (when it cannot)
//      Level/Play                    detail: the small print (the action's path, a type...)
//
// It is always fully on screen: it opens on the side of the cursor that has room, on the monitor the cursor is on (the same placement the
// property inspector uses for its help). Everything that explains something on hover goes through here, so the look and the placement
// are decided in one place:
//
//      xeditor::hint::Show({ .m_Topic = "Play", .m_Body = "Starts playing this Level.", .m_Shortcut = "F5" });   // right after the item
//      xeditor::hint::Text("Save\nSave the descriptor");     // the quick form: first line = topic, the rest = body (what SetTooltip took)
#include "imgui.h"

#include <cfloat>
#include <cstdarg>
#include <cstdio>
#include <string>
#include <string_view>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace xeditor::hint
{
    struct content
    {
        std::string_view m_Topic;       // the title
        std::string_view m_Body;        // what it does; may hold several lines
        std::string_view m_Shortcut;    // optional: "F5", "Ctrl+Shift+P"
        std::string_view m_Disabled;    // optional: why it cannot be used right now
        std::string_view m_Detail;      // optional: small print
    };

    // Opens the next window beside the cursor, on the side that has room, on the monitor the cursor is on. (The Win32 monitor is asked on
    // purpose: ImGui's viewport is this app window, not the screen, so it cannot tell where the real screen edge is.)
    inline void PlaceAwayFromEdges(float Offset, ImVec2 AssumedSize) noexcept
    {
        const ImVec2   Mouse = ImGui::GetIO().MousePos;
        const POINT    At{ static_cast<LONG>(Mouse.x), static_cast<LONG>(Mouse.y) };
        MONITORINFO    Info{ sizeof(MONITORINFO) };
        ::GetMonitorInfo(::MonitorFromPoint(At, MONITOR_DEFAULTTONEAREST), &Info);

        const float SpaceRight = static_cast<float>(Info.rcWork.right)  - Mouse.x;
        const float SpaceLeft  = Mouse.x - static_cast<float>(Info.rcWork.left);
        const float SpaceBelow = static_cast<float>(Info.rcWork.bottom) - Mouse.y;
        const float SpaceAbove = Mouse.y - static_cast<float>(Info.rcWork.top);
        const ImVec2 Pivot((SpaceRight < AssumedSize.x && SpaceLeft  > SpaceRight) ? 1.0f : 0.0f
                         , (SpaceBelow < AssumedSize.y && SpaceAbove > SpaceBelow) ? 1.0f : 0.0f);
        ImGui::SetNextWindowPos(ImVec2(Mouse.x + (Pivot.x > 0.0f ? -Offset : Offset), Mouse.y + (Pivot.y > 0.0f ? -Offset : Offset)), ImGuiCond_Always, Pivot);
    }

    // A key as a small cap: the text on a rounded plate.
    inline void KeyCap(std::string_view Text) noexcept
    {
        const std::string S(Text);
        const ImVec2 TextSize = ImGui::CalcTextSize(S.c_str());
        const ImVec2 Pad(6.0f, 1.0f);
        const ImVec2 Min = ImGui::GetCursorScreenPos();
        const ImVec2 Max(Min.x + TextSize.x + Pad.x * 2.0f, Min.y + TextSize.y + Pad.y * 2.0f);
        ImDrawList*  pList = ImGui::GetWindowDrawList();
        pList->AddRectFilled(Min, Max, IM_COL32(60, 62, 68, 255), 4.0f);
        pList->AddRect(Min, Max, IM_COL32(120, 124, 134, 255), 4.0f);
        pList->AddText(ImVec2(Min.x + Pad.x, Min.y + Pad.y), IM_COL32(235, 235, 240, 255), S.c_str());
        ImGui::Dummy(ImVec2(Max.x - Min.x, Max.y - Min.y));
    }

    // The window of the card, for what draws its own content (a list, a table): the same placement, padding and MAXIMUM WIDTH (400 px) as every hint, so a long line never makes the hint as wide as the
    // screen. Draw what is inside with Wrapped and Bullet (or any item: nothing in it can make the card wider than the maximum, text that is not wrapped is cut instead of stretching it), and finish with
    // EndCard when BeginCard returned true.
    inline bool BeginCard() noexcept
    {
        PlaceAwayFromEdges(16.0f, ImVec2(380.0f, 220.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 8.0f));
        ImGui::SetNextWindowSizeConstraints(ImVec2(0.0f, 0.0f), ImVec2(400.0f, FLT_MAX));
        if (ImGui::BeginTooltip()) return true;
        ImGui::PopStyleVar();
        return false;
    }

    inline void EndCard() noexcept
    {
        ImGui::EndTooltip();
        ImGui::PopStyleVar();
    }

    // The width text wraps at inside the card.
    inline float CardWrapWidth() noexcept { return ImGui::GetFontSize() * 24.0f; }

    // A paragraph that wraps at the width of the card.
    inline void Wrapped(std::string_view Text) noexcept
    {
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + CardWrapWidth());
        ImGui::TextUnformatted(Text.data(), Text.data() + Text.size());
        ImGui::PopTextWrapPos();
    }

    // A bullet followed by a paragraph that wraps at the width of the card.
    inline void Bullet(std::string_view Text) noexcept
    {
        ImGui::Bullet();
        ImGui::SameLine();
        Wrapped(Text);
    }

    // The card itself, in a tooltip window at the cursor. The caller has already decided it should show.
    inline void Draw(const content& C) noexcept
    {
        if (BeginCard())
        {
            if (!C.m_Topic.empty())
            {
                ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * 1.1f);
                ImGui::TextUnformatted(C.m_Topic.data(), C.m_Topic.data() + C.m_Topic.size());
                ImGui::PopFont();
            }
            if (!C.m_Shortcut.empty())
            {
                if (!C.m_Topic.empty()) ImGui::SameLine(0.0f, 14.0f);
                KeyCap(C.m_Shortcut);
            }
            if (!C.m_Body.empty())
            {
                if (!C.m_Topic.empty()) ImGui::Spacing();
                Wrapped(C.m_Body);
            }
            if (!C.m_Disabled.empty())
            {
                ImGui::Spacing();
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.72f, 0.38f, 1.0f));
                Wrapped(std::string("Unavailable: ") + std::string(C.m_Disabled));
                ImGui::PopStyleColor();
            }
            if (!C.m_Detail.empty())
            {
                ImGui::Spacing();
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.57f, 0.62f, 1.0f));
                Wrapped(C.m_Detail);
                ImGui::PopStyleColor();
            }
            EndCard();
        }
    }

    // Right after any item: shows the card while the mouse rests on it (also when the item is disabled). True when it showed.
    inline bool Show(const content& C, ImGuiHoveredFlags Flags = ImGuiHoveredFlags_AllowWhenDisabled) noexcept
    {
        if (!ImGui::IsItemHovered(Flags | ImGuiHoveredFlags_ForTooltip)) return false;
        Draw(C);
        return true;
    }

    // The quick form, for what used to be ImGui::SetTooltip("..."): the first line is the topic, the rest the body. Call it where SetTooltip was
    // called (inside the item's hover check, or after it: it checks the hover itself when asked to).
    inline void Text(const char* pFormat, ...) noexcept
    {
        char Buffer[1024];
        va_list Args;
        va_start(Args, pFormat);
        std::vsnprintf(Buffer, sizeof(Buffer), pFormat, Args);
        va_end(Args);

        const std::string_view All(Buffer);
        const auto             Eol = All.find('\n');
        content C;
        if (Eol == std::string_view::npos) C.m_Topic = All;
        else { C.m_Topic = All.substr(0, Eol); C.m_Body = All.substr(Eol + 1); }
        Draw(C);
    }
}
namespace xeditor
{
    // The colour the inspector draws the VALUE of a read only property with (the word "False" of a read only bool): the normal text colour faded the way ImGui::BeginDisabled fades it (the style's
    // DisabledAlpha), which is lighter than ImGuiCol_TextDisabled. Use it for anything that is shown and not editable and should read like those values.
    inline ImVec4 ReadOnlyTextColor() noexcept
    {
        ImVec4 C = ImGui::GetStyleColorVec4(ImGuiCol_Text);
        C.w *= ImGui::GetStyle().DisabledAlpha;
        return C;
    }
}

namespace xeditor::popup
{
    // Where a popup of Size goes so that it is seen whole: under the anchor, its left edge on the anchor's, and
    //  - against the right border, its right edge on the anchor's right edge instead (or, with no room at all, flush with the border);
    //  - with no room below, above the anchor when there is more room there; otherwise pushed up until it fits.
    // Pure: the bounds are the work area the popup must stay in.
    inline ImVec2 PlaceUnderAt(ImVec2 AnchorMin, ImVec2 AnchorMax, ImVec2 Size, ImVec2 BoundsMin, ImVec2 BoundsMax) noexcept
    {
        float X = AnchorMin.x;
        if (X + Size.x > BoundsMax.x) X = AnchorMax.x - Size.x;
        if (X + Size.x > BoundsMax.x) X = BoundsMax.x - Size.x;
        if (X < BoundsMin.x)          X = BoundsMin.x;

        float Y = AnchorMax.y;
        if (Y + Size.y > BoundsMax.y)
        {
            const float RoomBelow = BoundsMax.y - AnchorMax.y, RoomAbove = AnchorMin.y - BoundsMin.y;
            if (RoomAbove > RoomBelow && AnchorMin.y - Size.y >= BoundsMin.y) Y = AnchorMin.y - Size.y;
            else                                                              Y = BoundsMax.y - Size.y;
        }
        if (Y < BoundsMin.y) Y = BoundsMin.y;
        return ImVec2(X, Y);
    }

    // Opens the next window (a popup: call right before ImGui::BeginPopup, while it is open) under the item it belongs to, kept inside the window it is drawn in: a popup that crosses the edge of the
    // window becomes a window of the OS of its own, cut at the screen. AssumedSize is the size the popup will have (a menu: its widest label and its rows; a list: its width and its usual height). The anchor is
    // the item's rectangle (ImGui::GetItemRectMin / Max, taken right after the item).
    inline void PlaceUnder(ImVec2 AnchorMin, ImVec2 AnchorMax, ImVec2 AssumedSize) noexcept
    {
        const ImGuiViewport* pViewport = ImGui::GetWindowViewport();
        const ImVec2 Min = pViewport->WorkPos;
        const ImVec2 Max(pViewport->WorkPos.x + pViewport->WorkSize.x, pViewport->WorkPos.y + pViewport->WorkSize.y);
        ImGui::SetNextWindowPos(PlaceUnderAt(AnchorMin, AnchorMax, AssumedSize, Min, Max));
    }
}


#endif // XEDITOR_HINT_H
