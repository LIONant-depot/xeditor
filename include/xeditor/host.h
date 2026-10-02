#ifndef XEDITOR_HOST_H

#define XEDITOR_HOST_H

#pragma once



#include "session.h"
#include "drawer.h"
#include "dependencies/xlog/source/xlog_view.h"

#include "registry.h"
#include "log.h"
#include "notify.h"
#include "idle_work.h"

#include <algorithm>
#include <cassert>

#include <cstdio>

#include <string>
#include <functional>

#include <string_view>

#include <vector>
#include <unordered_map>



namespace xeditor

{

    class host

    {

    public:

        // The one process-wide root: editors, plugins and extensions reach shared state through it.
        // Constructing a host makes it current; destroying it clears that.
        static inline host* s_pCurrent = nullptr;
        static host* current() noexcept { return s_pCurrent; }
        host() noexcept { s_pCurrent = this; m_Logs.make_current(); }
        ~host() noexcept { release_current(); }
        void release_current() noexcept { if (s_pCurrent == this) s_pCurrent = nullptr; }

        // Non-owning services (game world, asset manager, source control, ...). Keyed by a compile-time hash of
        // the type, so the key is identical in every binary. The owner provides the object and withdraws it
        // before destroying it; consumers call find/get each time and never cache the pointer.
        template<class T> void provide(T& Service) noexcept { m_Services[key<T>()] = &Service; }
        template<class T> void withdraw() noexcept          { m_Services.erase(key<T>()); }
        template<class T> T*   find() const noexcept        { auto It = m_Services.find(key<T>()); return It == m_Services.end() ? nullptr : static_cast<T*>(It->second); }
        template<class T> T&   get() const noexcept         { auto* p = find<T>(); assert(p && "service not provided"); return *p; }

        xundo::system                         m_Workspace;

        xundo::system*                        m_pExternalWorkspace = nullptr;

        notifier                              m_Notifier;      // the last user-visible error, shown as a modal
        xlog::hub                            m_Logs;          // every event, problem and operation (the xlog library; documentation/Editors/DESIGN_logs.md)
        xlog::view_state                      m_LogsUi;        // what the Logs window remembers (query, preset, selection): it follows the person between editors
        console_log                           m_ConsoleLog;    // every command run through the host
        idle_work                             m_IdleWork;      // background maintenance that runs once the editor has been quiet for a while
        std::function<bool(xundo::system&, std::string_view)> m_OnBeforeEdit;  // write-lock gate, given the undo system and the command line: return false to refuse the edit

        // Domain host services (E29 wires its per-frame pumps and the Game.dll focus-reload).
        std::function<void()> m_OnPumpServices;
        std::function<void()> m_OnFocusRegain;
        std::function<void()> m_OnSourceChanged;   // files changed behind the editor's back (a source control pull)

        // The host thread commits what the other threads recorded (a bounded batch per frame; a backlog is reported by LogStatus, never dropped silently).
        void pump_services() noexcept { m_Logs.Drain(); m_IdleWork.Pump(); if (m_OnPumpServices) m_OnPumpServices(); }
        void on_focus_regain() noexcept { if (m_OnFocusRegain) m_OnFocusRegain(); }

        // --- Edit vs view write locks (DESIGN 4.2) ---
        struct write_lock
        {
            xresource::full_guid guid{};
            session*             pWriter = nullptr;
        };
        template<class T> static consteval xresource::type_guid key() noexcept { return xresource::type_guid{ __FUNCSIG__ }; }
        std::unordered_map<xresource::type_guid, void*> m_Services;

        std::vector<write_lock> m_WriteLocks;

        session* writer_for(xresource::full_guid Guid) const noexcept
        {
            for (const auto& L : m_WriteLocks)
                if (L.guid == Guid) return L.pWriter;
            return nullptr;
        }

        bool can_write(xresource::full_guid Guid, session* pSession) const noexcept
        {
            session* pW = writer_for(Guid);
            return pW == nullptr || pW == pSession;
        }

        // First mutator wins; returns false if another session holds the lock.
        bool try_acquire_write(xresource::full_guid Guid, session* pSession) noexcept
        {
            if (pSession == nullptr || Guid.empty()) return false;
            session* pW = writer_for(Guid);
            if (pW == pSession) return true;
            if (pW != nullptr) return false;
            m_WriteLocks.push_back(write_lock{ Guid, pSession });
            return true;
        }

        void release_write(xresource::full_guid Guid, session* pSession) noexcept
        {
            m_WriteLocks.erase(std::remove_if(m_WriteLocks.begin(), m_WriteLocks.end(),
                [&](const write_lock& L) { return L.guid == Guid && L.pWriter == pSession; }),
                m_WriteLocks.end());
        }

        // --- Play singleton (DESIGN 4.6) — opaque owner (e.g. &editor_state) ---
        void* m_pPlayOwner = nullptr;

        bool try_begin_play(void* pOwner) noexcept
        {
            if (pOwner == nullptr) return false;
            if (m_pPlayOwner != nullptr && m_pPlayOwner != pOwner) return false;
            m_pPlayOwner = pOwner;
            return true;
        }

        void end_play(void* pOwner) noexcept
        {
            if (m_pPlayOwner == pOwner) m_pPlayOwner = nullptr;
        }

        bool is_play_active() const noexcept { return m_pPlayOwner != nullptr; }

        // --- Host Drawer (DESIGN 2.2 / 4.5): one logical drawer, per-OS-window manifestation ---
        std::unordered_map<ImGuiID, drawer>          m_Drawers;
        std::function<void(int, const char*)>        m_OnDrawerTab; // app fills tab bodies (SC, Assets, ...)
        int                                          m_DrawerInputFrame = -1;

        drawer& drawer_for(ImGuiID ViewportId) noexcept { return m_Drawers[ViewportId]; }

        // The drawer of the focused OS window, toggled. What the Space key does - either through pump_drawer_input() below, or,
        // when the app binds its keys to actions (m_bDrawerToggleByAction), through the app's own "toggle drawer" action.
        void toggle_drawer_focused() noexcept
        {
            if (ImGuiViewport* vp = FocusedDrawerViewport()) { drawer& D = drawer_for(vp->ID); D.m_bOpen = !D.m_bOpen; }
        }
        bool m_bDrawerToggleByAction = false;

        // Opens the focused window's drawer on the Logs tab with Query in the query bar ("op:12" for one operation, empty for everything).
        void show_logs(const std::string& Query) noexcept
        {
            ImGuiViewport* vp = FocusedDrawerViewport();
            if (vp == nullptr) return;
            drawer& D = drawer_for(vp->ID);
            // Back returns here: the Logs window as it was, and what the drawer had in front (another tab, or closed), unless the person was already looking at the Logs.
            const bool bWasOnLogs = D.m_bOpen && D.m_ActiveTab == kLogsDrawerTab;
            m_LogsUi.PushBack(!bWasOnLogs, D.m_ActiveTab, D.m_bOpen);
            D.m_bOpen = true;
            D.m_ActiveTab = D.m_RequestTab = kLogsDrawerTab;
            std::snprintf(m_LogsUi.m_Query, sizeof(m_LogsUi.m_Query), "%s", Query.c_str());
            m_LogsUi.m_RequestPage = Query.empty() ? 0 : 1;           // a filtered view is evidence (Events: the whole output of that operation); with no filter the state is the better start
            m_LogsUi.m_View = xlog::problem_view::All;
            m_LogsUi.m_bFollow = false;                               // stay where the filter puts the reader; the list is short and does not need to chase the newest
            m_LogsUi.m_Selected = 0; m_LogsUi.m_Expanded.clear();
            m_LogsUi.m_BackButton[0] = m_LogsUi.m_BackButton[1] = -1.0f;          // drawn again next frame, where it now is
        }

        // The Logs window's Forward: back to the Logs view Back left, and the drawer on the Logs tab again.
        bool logs_forward() noexcept
        {
            xlog::view_snapshot Entry;
            if (!m_LogsUi.PopForward(Entry)) return false;
            if (Entry.m_bHasReturn)
                if (ImGuiViewport* vp = FocusedDrawerViewport())
                {
                    drawer& D = drawer_for(vp->ID);
                    D.m_ActiveTab = D.m_RequestTab = kLogsDrawerTab;
                    D.m_bOpen = true;
                }
            return true;
        }

        // The Logs window's Back: the window returns to where it was and the drawer to what it had in front (the tab the person came from, or closed).
        bool logs_back() noexcept
        {
            xlog::view_snapshot Entry;
            if (!m_LogsUi.PopBack(Entry)) return false;
            if (Entry.m_bHasReturn)
                if (ImGuiViewport* vp = FocusedDrawerViewport())
                {
                    drawer& D = drawer_for(vp->ID);
                    D.m_ActiveTab = D.m_RequestTab = Entry.m_ReturnTab;
                    D.m_bOpen = Entry.m_bReturnDrawerOpen;
                }
            return true;
        }

        // Space toggles the *focused* OS window's manifestation. Call from any editor; once/frame.
        void pump_drawer_input() noexcept
        {
            if (m_bDrawerToggleByAction) return;
            const int Frame = ImGui::GetFrameCount();
            if (m_DrawerInputFrame == Frame) return;
            m_DrawerInputFrame = Frame;
            ImGuiViewport* vp = FocusedDrawerViewport();
            if (vp == nullptr) return;
            DrawerHandleToggle(drawer_for(vp->ID));
        }

        // Render drawer overlay into this OS window's viewport (no-op if closed / already drawn).
        void render_drawer(ImGuiViewport* pViewport) noexcept
        {
            if (pViewport == nullptr) return;
            drawer& D = drawer_for(pViewport->ID);
            if (m_OnDrawerTab)
                DrawerRender(D, pViewport, m_OnDrawerTab);
            else
                DrawerRender(D, pViewport);
        }

        // Open drawer on a viewport to a tab index (toolbar Assets, etc.).
        void open_drawer_tab(ImGuiViewport* pViewport, int TabIndex) noexcept
        {
            if (pViewport == nullptr) return;
            drawer& D = drawer_for(pViewport->ID);
            D.m_bOpen = true;
            D.m_ActiveTab = TabIndex;
        }

        // Call ONCE per frame from the app (not from each editor). Handles Space on the
        // focused OS window and draws every open per-viewport manifestation. Editors stay
        // drawer-agnostic — full_editor_shell / peer editors do not wire this themselves.
        void draw_host_drawers() noexcept
        {
            pump_drawer_input();
            ImGuiPlatformIO& PlatformIO = ImGui::GetPlatformIO();
            for (ImGuiViewport* pVp : PlatformIO.Viewports)
                render_drawer(pVp);
        }

        std::vector<std::unique_ptr<session>> m_Sessions;



        struct attached_session

        {

            std::string          name;

            xresource::full_guid guid{};

            xundo::system*       pUndo = nullptr;

        };

        std::vector<attached_session> m_Attached;



        xundo::system& workspace() noexcept

        {

            return m_pExternalWorkspace ? *m_pExternalWorkspace : m_Workspace;

        }



        void clear_attached() noexcept { m_Attached.clear(); }



        void attach(std::string Name, xresource::full_guid Guid, xundo::system* pUndo) noexcept

        {

            if (!pUndo) return;

            m_Attached.push_back({ std::move(Name), Guid, pUndo });

        }



        session* find_by_guid(xresource::full_guid G) noexcept

        {

            for (auto& S : m_Sessions)

                if (S->document_ptr() && S->guid() == G) return S.get();

            return nullptr;

        }



        session* find_by_name(std::string_view Name) noexcept

        {

            session* Hit = nullptr;

            for (auto& S : m_Sessions)

            {

                if (!S->document_ptr() || S->display_name() != Name) continue;

                if (Hit) return nullptr;

                Hit = S.get();

            }

            return Hit;

        }



        xundo::system* find_attached_undo(std::string_view Name) noexcept

        {

            xundo::system* Hit = nullptr;

            for (auto& A : m_Attached)

            {

                if (A.name != Name) continue;

                if (Hit) return nullptr; // ambiguous

                Hit = A.pUndo;

            }

            return Hit;

        }



        session* open(xresource::full_guid Guid) noexcept

        {

            if (auto* E = find_by_guid(Guid)) { E->m_bRequestFocus = true; return E; }

            auto* Desc = registry::Get().Resolve(Guid.m_Type);

            if (!Desc || !Desc->m_CreateDocument) return nullptr;

            auto S = std::make_unique<session>();

            S->m_pDesc = Desc;

            S->m_Document = Desc->m_CreateDocument(Guid);

            if (!S->document_ptr()) return nullptr;

            S->m_Undo.Init({}, false);

            S->m_Document->Load();

            m_Sessions.push_back(std::move(S));

            return m_Sessions.back().get();

        }



        void close(session* S) noexcept

        {

            if (!S) return;

            m_Sessions.erase(std::remove_if(m_Sessions.begin(), m_Sessions.end(),

                [&](auto& U) { return U.get() == S; }), m_Sessions.end());

        }



        std::string run_on(xundo::system& Sys, const std::string& Cmd) noexcept

        {

            const auto Name = Cmd.substr(0, Cmd.find(' '));

            const auto Q = Sys.GetQueryCommandNames();

            const bool bQ = std::find(Q.begin(), Q.end(), Name) != Q.end();

            if (bQ) return Sys.Query(Cmd);
            // Edits typed or piped go through the same write-lock gate as the ones made in the UI.
            if (m_OnBeforeEdit && !m_OnBeforeEdit(Sys, Cmd)) return "Edit refused: resource is being edited in another session";
            return Sys.Execute(Cmd);

        }



        // Every command the console can run, with its one-line help: the workspace's (bare) and each open session's ("Name\\Command").
        struct routable { std::string m_FullName, m_Help; };
        std::vector<routable> routable_commands() noexcept
        {
            std::vector<routable> Out;
            auto Add = [&](const std::string& Prefix, xundo::system& Sys)
            {
                for (auto& N : Sys.GetCommandNames())      { const char* pH = Sys.GetCommandHelp(N);      Out.push_back({ Prefix + N, pH ? pH : "" }); }
                for (auto& N : Sys.GetQueryCommandNames()) { const char* pH = Sys.GetQueryCommandHelp(N); Out.push_back({ Prefix + N, pH ? pH : "" }); }
            };
            Add({}, workspace());
            for (auto& S : m_Sessions) if (S->document_ptr()) Add(S->display_name() + "\\", S->undo());
            return Out;
        }

        std::string dispatch(std::string_view Line) noexcept

        {

            while (!Line.empty() && (Line.back() == '\n' || Line.back() == '\r'))

                Line.remove_suffix(1);

            if (Line.empty()) return {};



            if (Line == "help")

            {

                std::string R = "Workspace: bare Command or Host\\Command. Session: Name\\Command.\n";

                for (auto& N : workspace().GetCommandNames()) R += N + "\n";

                for (auto& N : workspace().GetQueryCommandNames()) R += N + "\n";

                R += "Sessions:\n";

                for (auto& S : m_Sessions)

                    if (S->document_ptr()) R += "  " + S->display_name() + "\n";

                for (auto& A : m_Attached)

                    R += "  " + A.name + "\n";

                return R;

            }



            if (Line == "list")

            {

                std::string R;

                for (auto& S : m_Sessions)

                {

                    if (!S->document_ptr()) continue;

                    auto G = S->guid();

                    char Buf[160];

                    std::snprintf(Buf, sizeof(Buf), "%s  type=%016llX inst=%016llX dirty=%d\n",

                        S->display_name().c_str(),

                        (unsigned long long)G.m_Type.m_Value,

                        (unsigned long long)G.m_Instance.m_Value,

                        S->document_ptr()->isDirty() ? 1 : 0);

                    R += Buf;

                }

                for (auto& A : m_Attached)

                {

                    char Buf[160];

                    std::snprintf(Buf, sizeof(Buf), "%s  type=%016llX inst=%016llX (attached)\n",

                        A.name.c_str(),

                        (unsigned long long)A.guid.m_Type.m_Value,

                        (unsigned long long)A.guid.m_Instance.m_Value);

                    R += Buf;

                }

                return R.empty() ? std::string("(no open sessions)\n") : R;

            }



            const auto Slash = Line.find('\\');

            if (Slash != std::string_view::npos)

            {

                auto Target = Line.substr(0, Slash);

                auto Rest = Line.substr(Slash + 1);

                if (Target == "Host" || Target == ".")

                    return run_on(workspace(), std::string(Rest));

                if (auto* S = find_by_name(Target))

                    return run_on(S->undo(), std::string(Rest));

                if (auto* pUndo = find_attached_undo(Target))

                    return run_on(*pUndo, std::string(Rest));

                return std::string("No open session '") + std::string(Target) + "'. Use list.\n";

            }



            return run_on(workspace(), std::string(Line));

        }

    };


    // Tells the person something went wrong: always in the process log, and as a modal when the host has a UI.
    inline void NotifyError(std::string_view Message) noexcept
    {
        std::printf("%.*s\n", static_cast<int>(Message.size()), Message.data());
        std::fflush(stdout);                                  // flushed so a crash cannot swallow the line that explains it
        if (auto* pHost = host::current())
        {
            pHost->m_Notifier.raise(Message);
            // Also an event of the Logs: a diagnostic with no stable code, so its problem is labelled a heuristic grouping (numbers and quoted names are not part of it).
            xlog::event E;
            E.m_Producer = "xlion.notify"; E.m_Origin = { xlog::origin::type::System, "editor", 0 };
            E.m_Severity = xlog::severity::Error; E.m_Kind = xlog::kind::Diagnostic; E.m_Channel = "editor.ui"; E.m_bHeuristic = true;
            xlog::SetMessage(E, Message);
            pHost->m_Logs.Emit(std::move(E));
        }
    }

    // A command run through the host (typed, clicked, or from the pipe), recorded as an event of kind Command. The Logs' own commands are not
    // recorded: asking about the log must not fill it.
    inline void RecordCommand(std::string_view Text, log_source Source) noexcept
    {
        auto* pHost = host::current();
        if (!pHost || Text.empty() || Text.substr(0, 3) == "Log") return;
        xlog::event E;
        E.m_Producer = "xlion.commands";
        E.m_Origin = { xlog::origin::type::System, Source == log_source::Pipe ? "pipe" : Source == log_source::User ? "user" : "system", 0 };
        E.m_Severity = xlog::severity::Info; E.m_Kind = xlog::kind::Command; E.m_Channel = "editor.command";
        xlog::SetMessage(E, Text);
        pHost->m_Logs.Emit(std::move(E));
    }
}



#endif

