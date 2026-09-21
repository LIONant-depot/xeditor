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
    inline bool ContainsCaseInsensitive(std::string_view Haystack, std::string_view Needle) noexcept
    {
        if (Needle.empty()) return true;
        auto It = std::search(Haystack.begin(), Haystack.end(), Needle.begin(), Needle.end(),
            [](char A, char B) noexcept { return std::tolower(static_cast<unsigned char>(A)) == std::tolower(static_cast<unsigned char>(B)); });
        return It != Haystack.end();
    }

    // The same look as the asset browser's search box: a magnifying-glass placeholder, a gray "X" to clear and a rounded
    // input. It edits a caller-owned string, so each panel keeps its own search text.
    inline void RenderTreeSearchBar(std::string& SearchString, float AvailWidth) noexcept
    {
        std::array<char, 256> Buffer{};
        strcpy_s(Buffer.data(), Buffer.size(), SearchString.c_str());

        const auto StartX = ImGui::GetCursorPosX();
        if (Buffer[0] != 0)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
            if (ImGui::SmallButton("X")) Buffer[0] = 0;
            ImGui::PopStyleColor();
            ImGui::SameLine(0, 0.1f);
        }
        AvailWidth -= ImGui::GetCursorPosX() - StartX;

        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 7.0f);
        ImGui::PushItemWidth(AvailWidth);
        ImGui::InputText("##TreeSearch", Buffer.data(), Buffer.size());
        const bool bActive  = ImGui::IsItemActive();
        const bool bHasText = (Buffer[0] != 0);
        if (!bActive && !bHasText)
        {
            const ImVec2 InputPos  = ImGui::GetItemRectMin();
            const ImVec2 CursorPos = ImGui::GetCursorScreenPos();
            ImGui::SetCursorScreenPos(ImVec2(InputPos.x + 10.0f, InputPos.y + 4.0f));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
            ImGui::Text("\xee\x9c\xa1");
            ImGui::PopStyleColor();
            ImGui::SetCursorScreenPos(CursorPos);
        }
        ImGui::PopItemWidth();
        ImGui::PopStyleVar();

        SearchString = std::string_view(Buffer.data());
    }
}

#endif // XEDITOR_WIDGETS_H
