#ifndef XEDITOR_OPEN_REF_H
#define XEDITOR_OPEN_REF_H
#pragma once

// "Open source" of the Logs: a typed reference (xlog::ref) opened the way the host can. A file opens with the system's handler for it and its
// "path:line" goes to the clipboard (a jump to the line needs to know the person's IDE: a setting for later); the other kinds (an asset, an
// entity, a graph node) are navigated by the editor that owns them, which is why they are not handled here.
#include "dependencies/xlog/source/xlog_hub.h"

#include "imgui.h"

#ifndef NOMINMAX
    #define NOMINMAX
#endif
#include <windows.h>
#include <shellapi.h>
#undef ERROR
#pragma comment(lib, "shell32.lib")

namespace xeditor
{
    inline void OpenRef(const xlog::ref& R) noexcept
    {
        if (R.m_Type != xlog::ref::type::File || R.m_Path.empty()) return;
        const std::string Location = R.m_Line > 0 ? std::format("{}:{}", R.m_Path, R.m_Line) : R.m_Path;
        ImGui::SetClipboardText(Location.c_str());
        ShellExecuteA(nullptr, "open", R.m_Path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    }
}

#endif // XEDITOR_OPEN_REF_H
