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
#include "imgui_internal.h"                 // ImGuiWindow: the size of the content, for growing_card
#include "hint_placement.h"

#include <cfloat>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <string>
#include <string_view>
#include <unordered_map>

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

    // The most a card may take of the viewport of the window being drawn (what is left after the margin).
    inline ImVec2 ViewportMaxSize(float MaxWidth = FLT_MAX) noexcept
    {
        const ImGuiViewport* pViewport = ImGui::GetWindowViewport();
        return ImVec2(MaxWidth < pViewport->Size.x - 8.0f ? MaxWidth : pViewport->Size.x - 8.0f, pViewport->Size.y - 8.0f);
    }

    // Is the mouse resting on the last item, for a card that should stay open while the mouse is pressed on it? ImGui's own IsItemHovered says no from the press to the release: a press on
    // the background of a window makes the window the "active item" (to move it, even when it cannot be moved), and any other active item blocks the hover. So a card that used the plain
    // test closed on every press and opened again on the release. Here the press on the window itself does not count; a real widget that is active (a slider being dragged) still does.
    inline bool IsItemHoveredForCard() noexcept
    {
        if (!ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem)) return false;
        const ImGuiID Active = ImGui::GetActiveID();
        return Active == 0 || Active == ImGui::GetItemID() || Active == ImGui::GetCurrentWindow()->MoveId;
    }

    // Where a diagnostic line of the hints goes (the editor points it at its trace file); nothing when not set.
    inline void (*g_Diagnostic)(const char* pLine) = nullptr;

    // A hint card that is ALWAYS inside the window it is drawn in, so it is never given a window of its own (ImGui makes an OS window of any window that leaves the viewport it came
    // from). It does not guess its size: a new card starts at a small size that fits anywhere, and every frame it GROWS toward the size its content measured (about 14 per second,
    // smoothly), and its position is worked out for the size it has THAT frame (PlaceGrowing), growing away from the corner nearest the cursor: every step is inside the window. It looks
    // animated, and it is safe whatever the content turns out to be. Keep one per place that shows a card (a member), then, every frame the cursor rests on the thing:
    //
    //      if (Card.Begin(ImGui::GetItemID())) { ...draw the content (wrap long text at a fixed width)... Card.End(); }
    struct growing_card
    {
        ImVec2   m_Size   = ImVec2(0.0f, 0.0f);      // the size shown this frame
        ImVec2   m_Target = ImVec2(0.0f, 0.0f);      // the size of the content (and its padding), as measured by the frame before
        ImGuiID  m_Owner  = 0;
        int      m_LastFrame = -2;
        int      m_Age = 0;
        ImVec4   m_Asked = ImVec4(0, 0, 0, 0);       // where and how big the card was asked to be this frame (x, y, w, h): the diagnostic line
        std::unordered_map<ImGuiID, ImVec2> m_Known;  // the measured size of every owner that was shown: the animation is only for a size that is not known yet

        // True when the card is open: draw the content and call End(). MaxWidth caps the width (the card is never bigger than the window either).
        bool Begin(ImGuiID Owner, float MaxWidth = 480.0f, float Offset = 16.0f) noexcept
        {
            const ImGuiViewport* pViewport = ImGui::GetWindowViewport();
            const ImVec2 Max   = ViewportMaxSize(MaxWidth);
            const ImVec2 Start(Max.x < 150.0f ? Max.x : 150.0f, Max.y < 64.0f ? Max.y : 64.0f);       // small enough to be safe anywhere
            const int    Frame = ImGui::GetFrameCount();
            if (Owner != m_Owner || Frame - m_LastFrame > 4)                    // a new hover (a gap of a few frames is the same hover)
            {
                m_Owner = Owner;
                m_Age   = 0;                                                    // frames this hover has been measured for
                const auto Known = m_Known.find(Owner);
                // The growing is only how a size that is not known yet is found safely. A size that was measured before is known: the card opens at it at once (it is still measured every
                // frame, so a change of the content is followed). Never past what the window can hold now.
                if (Known != m_Known.end()) m_Size = m_Target = ImVec2(Known->second.x < Max.x ? Known->second.x : Max.x, Known->second.y < Max.y ? Known->second.y : Max.y);
                else                        m_Size = m_Target = Start;
            }
            m_LastFrame = Frame;
            ++m_Age;

            m_Target.x = m_Target.x < Start.x ? Start.x : (m_Target.x > Max.x ? Max.x : m_Target.x);
            m_Target.y = m_Target.y < Start.y ? Start.y : (m_Target.y > Max.y ? Max.y : m_Target.y);

            const float Speed = 1.0f - std::exp(-14.0f * ImGui::GetIO().DeltaTime);
            m_Size.x += (m_Target.x - m_Size.x) * Speed; if (std::fabs(m_Target.x - m_Size.x) < 0.5f) m_Size.x = m_Target.x;
            m_Size.y += (m_Target.y - m_Size.y) * Speed; if (std::fabs(m_Target.y - m_Size.y) < 0.5f) m_Size.y = m_Target.y;

            const ImVec2 Mouse = ImGui::GetIO().MousePos;
            // The finished card the placement is worked out for is never smaller than the card that is shown (a target that shrank below it must not move it): the shown rectangle is always inside it.
            const ImVec2 Done(m_Target.x > m_Size.x ? m_Target.x : m_Size.x, m_Target.y > m_Size.y ? m_Target.y : m_Size.y);
            const point  At = PlaceGrowing({ Mouse.x, Mouse.y }, { m_Size.x, m_Size.y }, { Done.x, Done.y }, { pViewport->Pos.x, pViewport->Pos.y }
                                         , { pViewport->Pos.x + pViewport->Size.x, pViewport->Pos.y + pViewport->Size.y }, Offset);
            m_Asked = ImVec4(At.x, At.y, m_Size.x, m_Size.y);                 // for the diagnostic line in End()
            ImGui::SetNextWindowViewport(pViewport->ID);
            ImGui::SetNextWindowPos(ImVec2(At.x, At.y), ImGuiCond_Always);
            ImGui::SetNextWindowSizeConstraints(m_Size, m_Size);              // the tooltip fits its content, the constraints decide: it is exactly the size we placed
            ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 0.0f);          // while it is smaller than its content it is cut, not given a scrollbar
            const bool bOpen = ImGui::BeginTooltip();
            if (!bOpen) ImGui::PopStyleVar();                                 // End() is not called for a card that did not open
            return bOpen;
        }

        // Measures the content (for the next frame's target) and closes the card.
        void End() noexcept
        {
            const ImGuiWindow* pWindow = ImGui::GetCurrentWindow();
            // ContentSizeIdeal, not ContentSize: it is what the auto-fit of ImGui itself uses, and it counts what a table would need (the card is smaller than its content while it grows)
            // The target only follows a real measurement: ImGui has no content size for the first frame or two of a window (zero: just its padding), and taking that for the size of the content
            // made a card that opened at its known size be placed as if it were tiny (and shrink, and grow again).
            const bool bMeasured = pWindow->ContentSizeIdeal.x > 0.0f && pWindow->ContentSizeIdeal.y > 0.0f;
            if (bMeasured) m_Target = ImVec2(pWindow->ContentSizeIdeal.x + pWindow->WindowPadding.x * 2.0f, pWindow->ContentSizeIdeal.y + pWindow->WindowPadding.y * 2.0f);
            // Only a MEASURED size is known (and only after a few frames of this hover).
            if (m_Age >= 3 && bMeasured)
            {
                if (m_Known.size() > 256) m_Known.clear();                    // a few hundred resources is plenty of memory for sizes
                m_Known[m_Owner] = m_Target;                                  // known from now on: the next hover of this owner opens at this size
            }
            // DIAGNOSTIC (to be removed): the first frames of every hover, what the card asked for and what ImGui gave it, in the trace file of the editor (LevelEditor.trace.log).
            if (m_Age <= 8)
            {
                const ImVec2 Mouse = ImGui::GetIO().MousePos;
                char Line[512];
                std::snprintf(Line, sizeof(Line), "hint-card age=%d frame=%d mouse=%.0f,%.0f viewport=%.0f,%.0f %.0fx%.0f asked=%.0f,%.0f %.0fx%.0f target=%.0fx%.0f got=%.0f,%.0f %.0fx%.0f appearing=%d measured=%d known=%d"
                    , m_Age, ImGui::GetFrameCount(), Mouse.x, Mouse.y, pWindow->Viewport ? pWindow->Viewport->Pos.x : -1.0f, pWindow->Viewport ? pWindow->Viewport->Pos.y : -1.0f
                    , pWindow->Viewport ? pWindow->Viewport->Size.x : -1.0f, pWindow->Viewport ? pWindow->Viewport->Size.y : -1.0f, m_Asked.x, m_Asked.y, m_Asked.z, m_Asked.w
                    , m_Target.x, m_Target.y, pWindow->Pos.x, pWindow->Pos.y, pWindow->Size.x, pWindow->Size.y, pWindow->Appearing ? 1 : 0, bMeasured ? 1 : 0, m_Known.count(m_Owner) ? 1 : 0);
                if (g_Diagnostic) g_Diagnostic(Line);
            }
            ImGui::EndTooltip();
            ImGui::PopStyleVar();
        }
    };

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
