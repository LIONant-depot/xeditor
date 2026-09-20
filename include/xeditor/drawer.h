#ifndef XEDITOR_DRAWER_H
#define XEDITOR_DRAWER_H
#pragma once

// Host Drawer (DESIGN 2.2 / 4.5): one logical process-services overlay.
// Manifested per OS window (ImGui viewport). Space toggles open state.
//
// Edge attach (hard rules):
// - Flush to the chosen OS-window edge (full viewport Pos/Size).
// - Only depth scales (farther/closer). Free axis spans the window
//   with a tiny inset. NoMove / NoResize on the host window.
// - Service UI is an *internal* TabBar (not dockable child windows),
//   so tabs cannot float out into the Level dock or free-drag.
// v1: stub tab bodies. Real SC/Log/Resources panels plug in later.

#include "imgui.h"
#include "imgui_internal.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>

namespace xeditor {

enum class drawer_edge : std::uint8_t
{
    bottom = 0,
    top,
    left,
    right,
};

struct drawer
{
    bool         m_bOpen     = false;
    drawer_edge  m_Edge      = drawer_edge::bottom;
    float        m_Depth     = 280.0f;
    float        m_SideInset = 4.0f;
    int          m_ActiveTab = 0; // index into kDrawerTabs
    int          m_LastRenderFrame = -1; // host: render at most once per viewport per frame
};

// Viewport that should receive Space toggle this frame (focused ImGui window's OS window).
inline ImGuiViewport* FocusedDrawerViewport() noexcept
{
    ImGuiViewport* vp = ImGui::GetMainViewport();
    if (ImGuiContext* ctx = ImGui::GetCurrentContext())
    {
        if (ctx->NavWindow && ctx->NavWindow->Viewport)
            vp = ctx->NavWindow->Viewport;
    }
    return vp;
}

inline void DrawerHandleToggle(drawer& D) noexcept
{
    const ImGuiIO& io = ImGui::GetIO();
    if (io.WantTextInput) return;
    if (io.KeyCtrl || io.KeyAlt || io.KeySuper) return;
    // Route over focused/active items so Space still opens the drawer when a
    // peer editor (e.g. Texture preview InvisibleButton) has focus.
    const ImGuiInputFlags Flags =
        ImGuiInputFlags_RouteGlobal
      | ImGuiInputFlags_RouteOverFocused
      | ImGuiInputFlags_RouteOverActive;
    if (ImGui::Shortcut(ImGuiKey_Space, Flags))
        D.m_bOpen = !D.m_bOpen;
}

inline void DrawerComputeRect(const drawer& D, const ImGuiViewport& Vp,
                              ImVec2& OutPos, ImVec2& OutSize) noexcept
{
    const ImVec2 VpPos  = Vp.Pos;
    const ImVec2 VpSize = Vp.Size;
    const float  Inset  = (std::max)(0.0f, D.m_SideInset);
    const float  MaxY   = (std::max)(120.0f, VpSize.y * 0.85f);
    const float  MaxX   = (std::max)(120.0f, VpSize.x * 0.85f);
    const float  DepthY = (std::clamp)(D.m_Depth, 120.0f, MaxY);
    const float  DepthX = (std::clamp)(D.m_Depth, 120.0f, MaxX);

    switch (D.m_Edge)
    {
    case drawer_edge::top:
        OutPos  = ImVec2(VpPos.x + Inset, VpPos.y);
        OutSize = ImVec2(VpSize.x - Inset * 2.0f, DepthY);
        break;
    case drawer_edge::left:
        OutPos  = ImVec2(VpPos.x, VpPos.y + Inset);
        OutSize = ImVec2(DepthX, VpSize.y - Inset * 2.0f);
        break;
    case drawer_edge::right:
        OutSize = ImVec2(DepthX, VpSize.y - Inset * 2.0f);
        OutPos  = ImVec2(VpPos.x + VpSize.x - OutSize.x, VpPos.y + Inset);
        break;
    case drawer_edge::bottom:
    default:
        OutSize = ImVec2(VpSize.x - Inset * 2.0f, DepthY);
        OutPos  = ImVec2(VpPos.x + Inset, VpPos.y + VpSize.y - OutSize.y);
        break;
    }
}

// Hit strip on the free edge (opposite the attached edge). Uses InvisibleButton
// so drag works reliably; drawn separator so the edge is visible.
inline void DrawerDepthGrip(drawer& D) noexcept
{
    const bool bHorizontal = (D.m_Edge == drawer_edge::bottom || D.m_Edge == drawer_edge::top);
    const float Hit = 10.0f;
    const ImVec2 GripSize = bHorizontal ? ImVec2(-1.0f, Hit) : ImVec2(Hit, -1.0f);

    const ImVec2 Cursor = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("##DrawerDepthGrip", GripSize);
    const bool bActive = ImGui::IsItemActive();
    const bool bHovered = ImGui::IsItemHovered();
    if (bHovered || bActive)
        ImGui::SetMouseCursor(bHorizontal ? ImGuiMouseCursor_ResizeNS : ImGuiMouseCursor_ResizeEW);

    if (bActive)
    {
        const ImVec2 Delta = ImGui::GetIO().MouseDelta;
        switch (D.m_Edge)
        {
        case drawer_edge::bottom: D.m_Depth -= Delta.y; break;
        case drawer_edge::top:    D.m_Depth += Delta.y; break;
        case drawer_edge::left:   D.m_Depth += Delta.x; break;
        case drawer_edge::right:  D.m_Depth -= Delta.x; break;
        }
        D.m_Depth = (std::clamp)(D.m_Depth, 120.0f, 900.0f);
    }

    // Visual line along the free edge.
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 Max = ImGui::GetItemRectMax();
    const ImU32 Col = ImGui::GetColorU32(bHovered || bActive ? ImGuiCol_SeparatorHovered : ImGuiCol_Separator);
    if (bHorizontal)
    {
        const float Y = (D.m_Edge == drawer_edge::bottom) ? Cursor.y + Hit * 0.5f : Max.y - Hit * 0.5f;
        dl->AddLine(ImVec2(Cursor.x, Y), ImVec2(Max.x, Y), Col, 2.0f);
    }
    else
    {
        const float X = (D.m_Edge == drawer_edge::left) ? Max.x - Hit * 0.5f : Cursor.x + Hit * 0.5f;
        dl->AddLine(ImVec2(X, Cursor.y), ImVec2(X, Max.y), Col, 2.0f);
    }
}

template<typename T_RENDER_TAB>
inline void DrawerRender(drawer& D, ImGuiViewport* pViewport, T_RENDER_TAB&& RenderTab) noexcept
{
    if (!D.m_bOpen || pViewport == nullptr) return;
    const int Frame = ImGui::GetFrameCount();
    if (D.m_LastRenderFrame == Frame) return;
    D.m_LastRenderFrame = Frame;

    ImVec2 Pos, Size;
    DrawerComputeRect(D, *pViewport, Pos, Size);

    ImGui::SetNextWindowViewport(pViewport->ID);
    ImGui::SetNextWindowPos(Pos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(Size, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.98f);
    // Match peer-editor dock tab strip (E29 TitleBg / TabDimmed), not Window (0x38).
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImGui::GetStyleColorVec4(ImGuiCol_TitleBg));


    const ImGuiWindowFlags Flags =
        ImGuiWindowFlags_NoCollapse
      | ImGuiWindowFlags_NoDocking
      | ImGuiWindowFlags_NoTitleBar
      | ImGuiWindowFlags_NoMove
      | ImGuiWindowFlags_NoResize
      | ImGuiWindowFlags_NoSavedSettings
      | ImGuiWindowFlags_NoNavFocus
      | ImGuiWindowFlags_NoFocusOnAppearing;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 4.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    char RootName[64];
    std::snprintf(RootName, sizeof(RootName), "xeditor.Drawer###Drawer.%08X",
                  static_cast<unsigned>(pViewport->ID));

    if (ImGui::Begin(RootName, &D.m_bOpen, Flags))
    {
        ImGui::SetWindowPos(Pos, ImGuiCond_Always);
        ImGui::SetWindowSize(Size, ImGuiCond_Always);

        if (D.m_Edge == drawer_edge::bottom || D.m_Edge == drawer_edge::right)
            DrawerDepthGrip(D);

        static const char* kTabs[] = {
            "Resources",
            "Assets",
            "Source Control",
            "Idle Work",
            "Log",
            "Commands",
            "Compilation",
            "Project Settings",
        };
        constexpr int kTabCount = (int)(sizeof(kTabs) / sizeof(kTabs[0]));
        if (D.m_ActiveTab < 0 || D.m_ActiveTab >= kTabCount) D.m_ActiveTab = 0;

        if (ImGui::BeginTabBar("##DrawerTabs", ImGuiTabBarFlags_FittingPolicyScroll))
        {
            for (int i = 0; i < kTabCount; ++i)
            {
                if (ImGui::BeginTabItem(kTabs[i]))
                {
                    D.m_ActiveTab = i;
                    ImGui::BeginChild("##DrawerTabBody", ImVec2(0, 0), false);
                    RenderTab(i, kTabs[i]);
                    ImGui::EndChild();
                    ImGui::EndTabItem();
                }
            }
            ImGui::EndTabBar();
        }

        if (D.m_Edge == drawer_edge::top || D.m_Edge == drawer_edge::left)
            DrawerDepthGrip(D);
    }
        ImGui::PopStyleColor(1);
    ImGui::End();
    ImGui::PopStyleVar(3);
}

// Overload: stub bodies when no callback supplied.
inline void DrawerRender(drawer& D, ImGuiViewport* pViewport) noexcept
{
    DrawerRender(D, pViewport, [](int, const char* Name)
    {
        ImGui::TextUnformatted(Name);
        ImGui::TextDisabled("Host Drawer stub - service UI moves here next.");
    });
}

} // namespace xeditor

#endif
