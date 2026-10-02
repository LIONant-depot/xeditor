#ifndef XEDITOR_GPU_LOG_H
#define XEDITOR_GPU_LOG_H
#pragma once

// The xGPU adapter of the Logs: what the Vulkan validation layers and xGPU's own checks say, as events of channel gpu.vulkan
// (documentation/Editors/DESIGN_logs.md, 3.3). xGPU reports through two plain callbacks (xgpu::instance::setup::m_pLogErrorFunc / m_pLogWarning) and knows nothing of the Logs;
// these two are what the editor gives it. They print the line as before (the process log, and the smoke harness, still read it there) and record it:
//
//   file(line/col) [function] ERROR: <message>               -> an event, severity error, a diagnostic (so a problem), body = where xGPU said it
//   file(line/col) [function] ERROR VK-3 (VK_ERROR_X): msg   -> the same with the code VK_ERROR_X (what xGPU knows)
//   ... [VUID-vkCmdDraw-None-08600] ...                      -> the code is the VUID (what the validation layer knows)
//
// Not thrown at the person: a validation message is a bug of the program, not a thing they did, so it is counted by the Logs badge and never a modal or a toast.
// Called from whichever thread Vulkan calls the layer from; the hub takes events from any thread.
#include "dependencies/xlog/source/xlog_hub.h"

#include <cstdio>
#include <string>
#include <string_view>

namespace xeditor
{
    namespace details
    {
        // The first token of the text that starts with Prefix and runs on while it is part of an identifier (a VUID, "UNASSIGNED-CoreValidation-...")
        inline std::string FindCode(std::string_view Text, std::string_view Prefix) noexcept
        {
            for (std::size_t At = Text.find(Prefix); At != std::string_view::npos; At = Text.find(Prefix, At + 1))
            {
                std::size_t End = At + Prefix.size();
                while (End < Text.size() && (std::isalnum(static_cast<unsigned char>(Text[End])) || Text[End] == '-' || Text[End] == '_')) ++End;
                if (End > At + Prefix.size()) return std::string(Text.substr(At, End - At));
            }
            return {};
        }
    }

    // Producer: the stable namespace of whoever said it; the tests that hand the adapter a made-up line say so ("xlion.simulated"), so they are never mistaken for the real thing.
    inline void LogGpuMessage(std::string_view Text, xlog::severity Severity, std::string Producer = "vulkan.validation") noexcept
    {
        if (Producer == "vulkan.validation")                // a made-up line (a test's) is not printed as if the layers had said it: the harness reads that text too
        {
            std::printf("%.*s\n", static_cast<int>(Text.size()), Text.data());
            std::fflush(stdout);
        }
        auto* pLogs = xlog::hub::current();
        if (!pLogs) return;

        // "<where> ERROR: <what>" / "<where> WARNING VK-3 (VK_...): <what>": the where goes to the body, the what is the title
        std::string_view Where, What = Text;
        std::string      Code;
        for (std::string_view Tag : { std::string_view("] ERROR"), std::string_view("] WARNING") })
            if (const auto At = Text.find(Tag); At != std::string_view::npos)
            {
                Where = Text.substr(0, At + 1);
                std::string_view Rest = Text.substr(At + Tag.size());
                if (const auto Paren = Rest.find('('); Rest.rfind(" VK", 1) == 0 && Paren != std::string_view::npos)          // "ERROR VK-3 (VK_ERROR_X): message"
                    if (const auto Close = Rest.find(')', Paren); Close != std::string_view::npos) { Code = std::string(Rest.substr(Paren + 1, Close - Paren - 1)); Rest = Rest.substr(Close + 1); }
                if (!Rest.empty() && Rest.front() == ':') Rest.remove_prefix(1);
                while (!Rest.empty() && Rest.front() == ' ') Rest.remove_prefix(1);
                What = Rest;
                break;
            }
        if (Code.empty()) Code = details::FindCode(What, "VUID-");
        if (Code.empty()) Code = details::FindCode(What, "UNASSIGNED-");

        xlog::event E;
        E.m_Producer = std::move(Producer);
        E.m_Origin   = { xlog::origin::type::System, "gpu", 0 };
        E.m_Severity = Severity;
        E.m_Kind     = xlog::kind::Diagnostic;
        E.m_Channel  = "gpu.vulkan";
        E.m_Code     = Code;
        E.m_bHeuristic = Code.empty();                       // no code: grouped by what the text says
        std::string Message(What);
        if (!Where.empty()) Message += "\n" + std::string(Where);
        xlog::SetMessage(E, Message);
        pLogs->Emit(std::move(E));
    }

    inline void LogGpuError(std::string_view Text) noexcept   { LogGpuMessage(Text, xlog::severity::Error); }
    inline void LogGpuWarning(std::string_view Text) noexcept { LogGpuMessage(Text, xlog::severity::Warning); }
}

#endif // XEDITOR_GPU_LOG_H
