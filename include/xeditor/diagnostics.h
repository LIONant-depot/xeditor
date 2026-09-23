#ifndef XEDITOR_DIAGNOSTICS_H
#define XEDITOR_DIAGNOSTICS_H
#pragma once

#include <cstdarg>
#include <cstdlib>
#include <exception>
#include <cstdio>
#include <filesystem>
#include <iterator>
#include <mutex>
#include <string>
#include <system_error>
#include <typeinfo>

#if defined(_MSC_VER)
#include <share.h>
#endif

#if defined(_MSC_VER) && defined(_DEBUG)
#include <crtdbg.h>
#include <dbghelp.h>
#include <windows.h>
#pragma comment(lib, "Dbghelp.lib")
#endif

namespace xeditor::diagnostics
{
    inline std::mutex g_TraceMutex;
    inline FILE*      g_pTraceFile = nullptr;
    inline std::string g_TracePath;
    inline std::terminate_handler g_PreviousTerminateHandler = nullptr;

    inline FILE* OpenTraceFile(const char* pPath) noexcept
    {
#if defined(_MSC_VER)
        return _fsopen(pPath, "w", _SH_DENYNO);
#else
        return std::fopen(pPath, "w");
#endif
    }

    inline void LogUnlocked(const char* pFormat, va_list Args) noexcept
    {
        if (g_pTraceFile == nullptr) return;
        std::vfprintf(g_pTraceFile, pFormat, Args);
        std::fputc('\n', g_pTraceFile);
        std::fflush(g_pTraceFile);
    }

    inline void Log(const char* pFormat, ...) noexcept
    {
        std::lock_guard Lock(g_TraceMutex);
        if (g_pTraceFile == nullptr) return;

        va_list Args;
        va_start(Args, pFormat);
        LogUnlocked(pFormat, Args);
        va_end(Args);
    }

    inline const char* Path() noexcept
    {
        return g_TracePath.c_str();
    }

    inline void Start(const char* pFileName) noexcept
    {
        std::lock_guard Lock(g_TraceMutex);
        if (g_pTraceFile != nullptr) return;

        std::error_code Ec;
        const auto CurrentPath = std::filesystem::current_path(Ec);
        g_TracePath = Ec ? pFileName : (CurrentPath / pFileName).string();

        g_pTraceFile = OpenTraceFile(g_TracePath.c_str());
        if (g_pTraceFile == nullptr)
        {
            g_TracePath = pFileName;
            g_pTraceFile = OpenTraceFile(pFileName);
        }

        if (g_pTraceFile != nullptr)
        {
            std::fprintf(g_pTraceFile, "trace start\n");
            std::fprintf(g_pTraceFile, "trace_path=%s\n", g_TracePath.c_str());
            std::fflush(g_pTraceFile);
        }
    }

#if defined(_MSC_VER) && defined(_DEBUG)
    inline void LogCrtStack() noexcept
    {
        static std::once_flag SymbolsOnce;
        std::call_once(SymbolsOnce, []
        {
            ::SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME);
            (void)::SymInitialize(::GetCurrentProcess(), nullptr, TRUE);
        });

        void* Frames[32]{};
        const USHORT Count = ::CaptureStackBackTrace(1, static_cast<DWORD>(std::size(Frames)), Frames, nullptr);
        HANDLE Process = ::GetCurrentProcess();
        alignas(SYMBOL_INFO) char SymbolStorage[sizeof(SYMBOL_INFO) + 512]{};
        auto* Symbol = reinterpret_cast<SYMBOL_INFO*>(SymbolStorage);
        Symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        Symbol->MaxNameLen = 511;

        for (USHORT Index = 0; Index < Count; ++Index)
        {
            const DWORD64 Address = reinterpret_cast<DWORD64>(Frames[Index]);
            DWORD64 Displacement = 0;
            IMAGEHLP_LINE64 Line{};
            Line.SizeOfStruct = sizeof(Line);
            if (::SymFromAddr(Process, Address, &Displacement, Symbol))
            {
                DWORD LineDisplacement = 0;
                if (::SymGetLineFromAddr64(Process, Address, &LineDisplacement, &Line))
                    Log("CRT stack[%u] %s+0x%llx (%s:%lu)", Index, Symbol->Name
                        , static_cast<unsigned long long>(Displacement)
                        , Line.FileName, static_cast<unsigned long>(Line.LineNumber));
                else
                    Log("CRT stack[%u] %s+0x%llx", Index, Symbol->Name
                        , static_cast<unsigned long long>(Displacement));
            }
            else
            {
                Log("CRT stack[%u] address=0x%llx", Index, static_cast<unsigned long long>(Address));
            }
        }
    }

    inline int __cdecl CrtReportHook(int ReportType, wchar_t* pMessage, int* pReturnValue) noexcept
    {
        static thread_local bool InHook = false;
        if (InHook) return 0;
        InHook = true;

        char Message[4096]{};
        if (pMessage != nullptr)
        {
            const int Count = ::WideCharToMultiByte
            ( CP_UTF8, 0, pMessage, -1, Message, static_cast<int>(sizeof(Message)), nullptr, nullptr );
            if (Count == 0) std::snprintf(Message, sizeof(Message), "<CRT message conversion failed>");
        }
        else
        {
            std::snprintf(Message, sizeof(Message), "<null CRT message>");
        }

        Log("CRT report type=%d message=%s", ReportType, Message);
        LogCrtStack();
        if (pReturnValue != nullptr) *pReturnValue = 0;
        InHook = false;
        return 0;
    }

    inline void InstallCrtReportHook() noexcept
    {
        const int Result = _CrtSetReportHookW2(_CRT_RPTHOOK_INSTALL, &CrtReportHook);
        Log("CRT assertion hook installed result=%d", Result);
    }

    inline void RemoveCrtReportHook() noexcept
    {
        const int Result = _CrtSetReportHookW2(_CRT_RPTHOOK_REMOVE, &CrtReportHook);
        Log("CRT assertion hook removed result=%d", Result);
    }
#else
    inline void InstallCrtReportHook() noexcept {}
    inline void RemoveCrtReportHook() noexcept {}
#endif

    inline void TerminateHandler() noexcept
    {
        try
        {
            if (const auto Exception = std::current_exception())
            {
                std::rethrow_exception(Exception);
            }
        }
        catch (const std::filesystem::filesystem_error& Error)
        {
            Log("terminate: filesystem_error what=%s code=%d category=%s path1=%s path2=%s"
                , Error.what(), Error.code().value(), Error.code().category().name()
                , Error.path1().string().c_str(), Error.path2().string().c_str());
        }
        catch (const std::exception& Error)
        {
            Log("terminate: exception what=%s type=%s", Error.what(), typeid(Error).name());
        }
        catch (...)
        {
            Log("terminate: unknown exception");
        }

        std::abort();
    }

    inline void InstallTerminateHandler() noexcept
    {
        g_PreviousTerminateHandler = std::set_terminate(&TerminateHandler);
        Log("terminate handler installed");
    }

    inline void RemoveTerminateHandler() noexcept
    {
        std::set_terminate(g_PreviousTerminateHandler);
        Log("terminate handler removed");
        g_PreviousTerminateHandler = nullptr;
    }

#if defined(_MSC_VER) && defined(_DEBUG)
    // Raw SEH crashes (access violation, stack overflow, ...) go through neither the CRT report
    // hook nor std::terminate - only this reaches them. Logs the faulting thread's real stack (via
    // the exception's own CONTEXT record, not CaptureStackBackTrace's caller-frame guess) then lets
    // the OS proceed to its normal unhandled-exception behavior (JIT debugger / process exit).
    inline LONG WINAPI UnhandledExceptionFilterImpl(EXCEPTION_POINTERS* pInfo) noexcept
    {
        Log("SEH exception code=0x%08lX address=0x%p", pInfo->ExceptionRecord->ExceptionCode, pInfo->ExceptionRecord->ExceptionAddress);
        if (pInfo->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && pInfo->ExceptionRecord->NumberParameters >= 2)
        {
            Log("SEH access violation %s address=0x%p"
                , pInfo->ExceptionRecord->ExceptionInformation[0] ? "writing to" : "reading from"
                , reinterpret_cast<void*>(pInfo->ExceptionRecord->ExceptionInformation[1]));
        }

        static std::once_flag SymbolsOnce;
        std::call_once(SymbolsOnce, []
        {
            ::SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME);
            (void)::SymInitialize(::GetCurrentProcess(), nullptr, TRUE);
        });

        HANDLE Process = ::GetCurrentProcess();
        HANDLE Thread  = ::GetCurrentThread();
        CONTEXT Ctx    = *pInfo->ContextRecord;

        STACKFRAME64 Frame{};
#if defined(_M_X64)
        DWORD MachineType = IMAGE_FILE_MACHINE_AMD64;
        Frame.AddrPC.Offset    = Ctx.Rip; Frame.AddrPC.Mode    = AddrModeFlat;
        Frame.AddrFrame.Offset = Ctx.Rbp; Frame.AddrFrame.Mode = AddrModeFlat;
        Frame.AddrStack.Offset = Ctx.Rsp; Frame.AddrStack.Mode = AddrModeFlat;
#else
        DWORD MachineType = IMAGE_FILE_MACHINE_I386;
        Frame.AddrPC.Offset    = Ctx.Eip; Frame.AddrPC.Mode    = AddrModeFlat;
        Frame.AddrFrame.Offset = Ctx.Ebp; Frame.AddrFrame.Mode = AddrModeFlat;
        Frame.AddrStack.Offset = Ctx.Esp; Frame.AddrStack.Mode = AddrModeFlat;
#endif

        alignas(SYMBOL_INFO) char SymbolStorage[sizeof(SYMBOL_INFO) + 512]{};
        auto* Symbol = reinterpret_cast<SYMBOL_INFO*>(SymbolStorage);
        Symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        Symbol->MaxNameLen = 511;

        for (int Index = 0; Index < 64; ++Index)
        {
            if (!::StackWalk64(MachineType, Process, Thread, &Frame, &Ctx, nullptr, ::SymFunctionTableAccess64, ::SymGetModuleBase64, nullptr))
                break;
            if (Frame.AddrPC.Offset == 0) break;

            const DWORD64 Address = Frame.AddrPC.Offset;
            DWORD64 Displacement = 0;
            IMAGEHLP_LINE64 Line{};
            Line.SizeOfStruct = sizeof(Line);
            if (::SymFromAddr(Process, Address, &Displacement, Symbol))
            {
                DWORD LineDisplacement = 0;
                if (::SymGetLineFromAddr64(Process, Address, &LineDisplacement, &Line))
                    Log("SEH stack[%d] %s+0x%llx (%s:%lu)", Index, Symbol->Name
                        , static_cast<unsigned long long>(Displacement), Line.FileName, static_cast<unsigned long>(Line.LineNumber));
                else
                    Log("SEH stack[%d] %s+0x%llx", Index, Symbol->Name, static_cast<unsigned long long>(Displacement));
            }
            else
            {
                Log("SEH stack[%d] address=0x%llx", Index, static_cast<unsigned long long>(Address));
            }
        }

        return EXCEPTION_CONTINUE_SEARCH;
    }

    inline void InstallUnhandledExceptionFilter() noexcept
    {
        ::SetUnhandledExceptionFilter(&UnhandledExceptionFilterImpl);
        Log("SEH unhandled exception filter installed");
    }
#else
    inline void InstallUnhandledExceptionFilter() noexcept {}
#endif

    inline void Stop() noexcept
    {
        std::lock_guard Lock(g_TraceMutex);
        if (g_pTraceFile == nullptr) return;
        std::fprintf(g_pTraceFile, "trace stop\n");
        std::fflush(g_pTraceFile);
        std::fclose(g_pTraceFile);
        g_pTraceFile = nullptr;
    }
}

#endif // XEDITOR_DIAGNOSTICS_H
