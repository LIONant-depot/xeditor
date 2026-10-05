#ifndef XEDITOR_GROUPED_LIST_H
#define XEDITOR_GROUPED_LIST_H
#pragma once

// THE "add something" popup of the editors: a search box at the top, then the items by group (each group collapsible, with how many it holds, open while searching, its open/closed state remembered
// by the caller), a hint on each item. The Add Component popup of the Entity Properties and the "+" of the resource view are this widget; they only say what the items are.
//
// Render it inside a popup the caller opened (OpenPopup / BeginPopup). BeginPopup resizes the popup to its content every frame, so the size of what is inside is fixed here (the search box and the list
// do not take theirs from the window: that shrank the popup when nothing matched, and it stayed small). A fix to the look or to the behavior is made here, once.
#include "widgets.h"

#include "imgui.h"

#include <algorithm>
#include <format>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace xeditor
{
    struct grouped_list_item
    {
        std::string             m_Name;             // what is shown and searched
        std::string             m_Group;            // the section ("" is shown as Uncategorized, and comes first)
        std::string             m_SearchExtra;      // more text the search looks at (the module a component comes from)
        int                     m_Priority = 0;     // inside its group, before the name

        std::function<void()>   m_DrawIcon;         // optional: draws something in front of the name (one line high, the cursor is left for the name)
        std::function<void()>   m_OnHover;          // optional: the hint (called while the mouse is over the item)
        std::string             m_RightText;        // optional: small, grayed, at the right edge
    };

    struct grouped_list_options
    {
        float               m_Width     = 304.0f;
        float               m_Height    = 300.0f;
        const char*         m_NoMatch   = "No matching items.";
        const char*         m_NoItems   = "Nothing to add.";
        std::string_view    m_LastGroup = {};       // a group that goes after all the others (the types that did not say theirs)
    };

    // Returns the index (into Items) of the item that was chosen this frame (and closes the popup), or -1.
    // Search: the text of the box (the caller keeps it); Open: which groups are open (the caller keeps it).
    inline int RenderGroupedList( std::string&                          Search
                                , std::unordered_map<std::string, bool>& Open
                                , const std::vector<grouped_list_item>&  Items
                                , const grouped_list_options&            Options = {} ) noexcept
    {
        RenderTreeSearchBar(Search, Options.m_Width, ImGui::IsWindowAppearing());
        ImGui::Separator();

        const bool bHasSearch = !Search.empty();

        std::vector<int> Order;
        Order.reserve(Items.size());
        for (int i = 0; i < static_cast<int>(Items.size()); ++i)
        {
            const auto& I = Items[i];
            if (bHasSearch && !ContainsCaseInsensitive(I.m_Name, Search) && !ContainsCaseInsensitive(I.m_Group, Search) && !ContainsCaseInsensitive(I.m_SearchExtra, Search)) continue;
            Order.push_back(i);
        }

        std::stable_sort(Order.begin(), Order.end(), [&](int A, int B) noexcept
        {
            const auto& IA = Items[A];
            const auto& IB = Items[B];
            const bool  bLastA = !Options.m_LastGroup.empty() && IA.m_Group == Options.m_LastGroup;
            const bool  bLastB = !Options.m_LastGroup.empty() && IB.m_Group == Options.m_LastGroup;
            if (bLastA != bLastB) return bLastB;
            if (IA.m_Group != IB.m_Group) return IA.m_Group < IB.m_Group;
            if (IA.m_Priority != IB.m_Priority) return IA.m_Priority < IB.m_Priority;
            return IA.m_Name < IB.m_Name;
        });

        int Chosen = -1;

        ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 8.0f);
        if (ImGui::BeginChild("##GroupedList", ImVec2(Options.m_Width, Options.m_Height), ImGuiChildFlags_None))
        {
            if (Order.empty()) ImGui::TextDisabled("%s", bHasSearch ? Options.m_NoMatch : Options.m_NoItems);

            for (std::size_t iFirst = 0; iFirst < Order.size();)
            {
                const std::string& Group = Items[Order[iFirst]].m_Group;
                std::size_t        iEnd  = iFirst;
                while (iEnd < Order.size() && Items[Order[iEnd]].m_Group == Group) ++iEnd;

                bool bOpen = bHasSearch;                                                    // searching opens everything
                if (!bHasSearch)
                    if (auto It = Open.find(Group); It != Open.end()) bOpen = It->second;

                ImGui::SetNextItemOpen(bOpen, ImGuiCond_Always);
                const bool bNodeOpen = ImGui::TreeNodeEx(std::format("{} ({})", Group.empty() ? "Uncategorized" : Group, iEnd - iFirst).c_str(), ImGuiTreeNodeFlags_SpanFullWidth);    // the whole row toggles
                if (!bHasSearch) Open[Group] = bNodeOpen;

                if (bNodeOpen)
                {
                    for (std::size_t i = iFirst; i < iEnd; ++i)
                    {
                        const int   Index = Order[i];
                        const auto& I     = Items[Index];

                        ImGui::PushID(Index);
                        if (I.m_DrawIcon) { I.m_DrawIcon(); ImGui::SameLine(); }

                        const bool bClicked = ImGui::Selectable(I.m_Name.c_str());
                        if (ImGui::IsItemHovered() && I.m_OnHover) I.m_OnHover();

                        if (!I.m_RightText.empty())
                        {
                            ImGui::SameLine(ImGui::GetContentRegionMax().x - ImGui::CalcTextSize(I.m_RightText.c_str()).x);
                            ImGui::TextDisabled("%s", I.m_RightText.c_str());
                        }

                        if (bClicked)
                        {
                            Chosen = Index;
                            ImGui::CloseCurrentPopup();
                        }
                        ImGui::PopID();
                    }
                    ImGui::TreePop();
                }
                iFirst = iEnd;
            }
        }
        ImGui::EndChild();
        ImGui::PopStyleVar(1);
        return Chosen;
    }
}

#endif
