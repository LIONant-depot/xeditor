#ifndef XEDITOR_COMMANDS_H
#define XEDITOR_COMMANDS_H
#pragma once

#include "host.h"

#include <format>
#include <string>
#include <string_view>
#include <vector>

// Running commands from UI code. Every edit goes through the same steps: ask the host whether the resource may be
// edited here (the write-lock gate), log the command like a typed or piped one, execute it, and report a refusal to
// the person (NotifyError) as well as to the console log. The AI/CLI path (host::dispatch) needs none of this: it gets
// the failure text as the command's reply.
namespace xeditor
{
    // Adds a line to the host's console log (no-op without a host).
    inline void LogConsole(std::string Text, log_source Source) noexcept
    {
        if (auto* pHost = host::current()) pHost->m_ConsoleLog.push_back({ std::move(Text), Source });
    }

    namespace details
    {
        inline bool MayEdit(xundo::system& System) noexcept
        {
            auto* pHost = host::current();
            return pHost == nullptr || !pHost->m_OnBeforeEdit || pHost->m_OnBeforeEdit(System);
        }

        // Executes and logs one command; false (after reporting) if it failed.
        inline bool Execute(xundo::system& System, const std::string& Cmd) noexcept
        {
            LogConsole(Cmd, log_source::User);
            auto Err = System.Execute(Cmd);
            if (Err.empty()) return true;
            NotifyError(std::format("command failed: '{}' ({})", Cmd, Err));
            LogConsole(std::move(Err), log_source::System);
            return false;
        }
    }

    // One undoable edit command.
    inline void Run(xundo::system& System, const std::string& Cmd) noexcept
    {
        if (!details::MayEdit(System))
        {
            constexpr std::string_view Refused = "Edit refused: resource is being edited in another session";
            NotifyError(Refused);
            LogConsole(std::string(Refused), log_source::System);
            return;
        }
        details::Execute(System, Cmd);
    }

    // A query command (xundo keeps these in a separate registry from edit commands, so they need this entry point).
    // Query replies are not always errors, so only a reply that reads as a failure is reported to the person.
    inline void RunQuery(xundo::system& System, const std::string& Cmd) noexcept
    {
        LogConsole(Cmd, log_source::User);
        auto Result = System.Query(Cmd);
        if (Result.empty()) return;
        const bool bFailed = Result.find(": ") != std::string::npos || Result.starts_with("Unable") || Result.starts_with("Malformed");
        if (bFailed) NotifyError(std::format("command failed: '{}' ({})", Cmd, Result));
        LogConsole(std::move(Result), log_source::System);
    }

    // Several commands as ONE undo/redo step. False if refused or failed, so a caller can tell (a cut clipboard must not be
    // spent by a paste that did not happen).
    [[nodiscard]] inline bool RunGroup(xundo::system& System, std::string_view GroupName, const std::vector<std::string>& Cmds) noexcept
    {
        if (!details::MayEdit(System)) return false;
        if (Cmds.empty()) return true;
        if (Cmds.size() == 1) return details::Execute(System, Cmds.front());       // a group of one is just a command

        for (auto& Cmd : Cmds) LogConsole(Cmd, log_source::User);
        auto Err = System.Execute(GroupName, Cmds);
        if (Err.empty()) return true;
        NotifyError(std::format("grouped command failed: '{}' ({})", GroupName, Err));
        LogConsole(std::move(Err), log_source::System);
        return false;
    }
}

#endif
