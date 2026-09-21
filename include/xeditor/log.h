#ifndef XEDITOR_LOG_H
#define XEDITOR_LOG_H
#pragma once

#include <string>
#include <vector>

namespace xeditor
{
    // Who a console-log line came from: the editor itself, the user (typed, or a UI click that ran a command) or an
    // external process on the console pipe (the AI-facing colour, on purpose).
    enum class log_source { System, User, Pipe };

    struct log_entry
    {
        std::string m_Text;
        log_source  m_Source;
    };

    // The audit trail of every command run through the host: what a person sees in the Commands panel and what an
    // AI/CLI client reads back. Owned by xeditor::host.
    using console_log = std::vector<log_entry>;
}

#endif
