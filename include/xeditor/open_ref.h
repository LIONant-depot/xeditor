#ifndef XEDITOR_OPEN_REF_H
#define XEDITOR_OPEN_REF_H
#pragma once

// "Open source" of the Logs: a typed reference (xlog::ref) opened the way the host can. A file opens with the system's handler for it and its
// "path:line" goes to the clipboard (a jump to the line needs to know the person's IDE: a setting for later); the other kinds (an asset, an
// entity, a graph node) are navigated by the editor that owns them, which is why they are not handled here.
#include "dependencies/xlog/source/xlog_hub.h"

#include "imgui.h"

#include <filesystem>
#include <functional>
#include <utility>
#include <vector>

#ifndef NOMINMAX
    #define NOMINMAX
#endif
#include <windows.h>
#include <shellapi.h>
#undef ERROR
#pragma comment(lib, "shell32.lib")

namespace xeditor
{
    // An editor that shows files of its own (the script module editor) claims the ones it can show: a file that an open editor takes is opened there, at its line, and is not handed
    // to the system. An opener answers true when it opened the file.
    using file_opener = std::function<bool(const xlog::ref&)>;
    inline std::vector<std::pair<std::uint64_t, file_opener>>& FileOpeners() noexcept { static std::vector<std::pair<std::uint64_t, file_opener>> Openers; return Openers; }
    inline std::uint64_t AddFileOpener(file_opener Opener) noexcept { static std::uint64_t Next = 1; FileOpeners().emplace_back(Next, std::move(Opener)); return Next++; }
    inline void RemoveFileOpener(std::uint64_t Id) noexcept { std::erase_if(FileOpeners(), [&](const auto& O) { return O.first == Id; }); }

    // Hands the file to the system (the program Windows uses for it); its path and line go to the clipboard.
    inline void OpenInShell(const xlog::ref& R) noexcept
    {
        if (R.m_Type != xlog::ref::type::File || R.m_Path.empty()) return;
        const std::string Location = R.m_Line > 0 ? std::format("{}:{}", R.m_Path, R.m_Line) : R.m_Path;
        ImGui::SetClipboardText(Location.c_str());
        std::error_code Ec;
        if (std::filesystem::exists(R.m_Path, Ec))          // a path that is not there (a compiler's output from another machine) is not handed to the shell: it would answer with a dialog
            ShellExecuteA(nullptr, "open", R.m_Path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    }

    // "Open source" of the Logs, F8, the Feedback of an editor: an open editor that shows the file takes it, otherwise the system does.
    inline void OpenRef(const xlog::ref& R) noexcept
    {
        if (R.m_Type != xlog::ref::type::File || R.m_Path.empty()) return;
        for (auto& [Id, Opener] : FileOpeners()) if (Opener && Opener(R)) return;
        OpenInShell(R);
    }
}

#endif // XEDITOR_OPEN_REF_H
