#ifndef XEDITOR_DRAWER_H
#define XEDITOR_DRAWER_H
#pragma once

// Host Drawer (DESIGN 2.2 / 4.5): one logical process-services overlay.
// Manifested per OS window (ImGui viewport). Space toggles the focused
// viewport's open state. Children dock only inside the drawer ClassId.
// v1: overlay + empty stub tabs. Moving real SC/Log/Resources panels is Phase D later.

#include "dock_isolation.h"
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
    bool         m_bOpen  = false;
    drawer_edge  m_Edge   = drawer_edge::bottom;
    float        m_Depth  = 280.0f;
    ImGuiID      m_ClassId = 0xD4A00001u;
};

inline ImGuiWindowClass DrawerWindowClass(const drawer& D) noexcept
{
    ImGuiWindowClass Wc;
    Wc.ClassId               = D.m_ClassId ? D.m_ClassId : 0xD4A00001u;
    Wc.DockingAllowUnclassed = false;
    Wc.DockingAlwaysTabBar   = true;
    return Wc;
}

inline void DrawerHandleToggle(drawer& D) noexcept
{
    const ImGuiIO& io = ImGui::GetIO();
    if (io.WantTextInput) return;
    if (io.KeyCtrl || io.KeyAlt || io.KeySuper) return;
    if (ImGui::IsKeyPressed(ImGuiKey_Space, false))
        D.m_bOpen = !D.m_bOpen;
}

inline void DrawerComputeRect(const drawer& D, const ImGuiViewport& Vp,
                              ImVec2& OutPos, ImVec2& OutSize) noexcept
{
    const ImVec2 Wp = Vp.WorkPos;
    const ImVec2 Ws = Vp.WorkSize;
    const float  Depth = (std::max)(80.0f, D.m_Depth);
    switch (D.m_Edge)
    {
    case drawer_edge::top:
        OutPos  = Wp;
        OutSize = ImVec2(Ws.x, (std::min)(Depth, Ws.y * 0.9f));
        break;
    case drawer_edge::left:
        OutPos  = Wp;
        OutSize = ImVec2((std::min)(Depth, Ws.x * 0.9f), Ws.y);
        break;
    case drawer_edge::right:
        OutSize = ImVec2((std::min)(Depth, Ws.x * 0.9f), Ws.y);
        OutPos  = ImVec2(Wp.x + Ws.x - OutSize.x, Wp.y);
        break;
    case drawer_edge::bottom:
    default:
        OutSize = ImVec2(Ws.x, (std::min)(Depth, Ws.y * 0.9f));
        OutPos  = ImVec2(Wp.x, Wp.y + Ws.y - OutSize.y);
        break;
    }
}

inline void DrawerRender(drawer& D, ImGuiViewport* pViewport) noexcept
{
    if (!D.m_bOpen || pViewport == nullptr) return;

    ImVec2 Pos, Size;
    DrawerComputeRect(D, *pViewport, Pos, Size);

    ImGui::SetNextWindowViewport(pViewport->ID);
    ImGui::SetNextWindowPos(Pos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(Size, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.96f);

    const ImGuiWindowFlags Flags =
        ImGuiWindowFlags_NoCollapse
      | ImGuiWindowFlags_NoDocking
      | ImGuiWindowFlags_NoTitleBar
      | ImGuiWindowFlags_NoMove
      | ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

    char RootName[64];
    std::snprintf(RootName, sizeof(RootName), "xeditor.Drawer###Drawer.%08X",
                  static_cast<unsigned>(pViewport->ID));

    if (ImGui::Begin(RootName, &D.m_bOpen, Flags))
    {
        const ImGuiID DockId = ImGui::GetID("xeditor.Drawer.Dockspace");
        const ImGuiWindowClass Wc = DrawerWindowClass(D);

        if (ImGui::BeginChild("##DrawerDepthGrip", ImVec2(0, 6), false,
                              ImGuiWindowFlags_NoScrollbar))
        {
            if (ImGui::IsWindowHovered() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
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
        }
        ImGui::EndChild();

        ImGui::DockSpace(DockId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None, &Wc);
        ApplyDockClassToTree(ImGui::DockBuilderGetNode(DockId), Wc);

        static const char* kTabs[] = {
            "Resources###xeditor.Drawer.Resources",
            "Assets###xeditor.Drawer.Assets",
            "Source Control###xeditor.Drawer.SC",
            "Idle Work###xeditor.Drawer.Idle",
            "Log###xeditor.Drawer.Log",
            "Commands###xeditor.Drawer.Commands",
            "Compilation###xeditor.Drawer.Compile",
            "Project Settings###xeditor.Drawer.Project",
        };
        for (const char* Title : kTabs)
        {
            ImGui::SetNextWindowClass(&Wc);
            if (ImGui::Begin(Title))
            {
                ImGui::TextUnformatted(Title);
                ImGui::TextDisabled("Host Drawer stub - service UI moves here next.");
            }
            ImGui::End();
        }
    }
    ImGui::End();
    ImGui::PopStyleVar(2);
}

} // namespace xeditor

#endif
