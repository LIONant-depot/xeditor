#ifndef XEDITOR_HOST_H

#define XEDITOR_HOST_H

#pragma once



#include "session.h"
#include "drawer.h"

#include "registry.h"
#include "log.h"
#include "notify.h"

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
        host() noexcept { s_pCurrent = this; }
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
        console_log                           m_ConsoleLog;    // every command run through the host
        std::function<bool(xundo::system&)>   m_OnBeforeEdit;  // write-lock gate: return false to refuse an edit

        // Domain host services (E29 wires Idle Work / SC idle / Game.dll focus-reload).
        std::function<void()> m_OnPumpServices;
        std::function<void()> m_OnFocusRegain;

        void pump_services() noexcept { if (m_OnPumpServices) m_OnPumpServices(); }
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

        // Space toggles the *focused* OS window's manifestation. Call from any editor; once/frame.
        void pump_drawer_input() noexcept
        {
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



        static std::string run_on(xundo::system& Sys, const std::string& Cmd) noexcept

        {

            const auto Name = Cmd.substr(0, Cmd.find(' '));

            const auto Q = Sys.GetQueryCommandNames();

            const bool bQ = std::find(Q.begin(), Q.end(), Name) != Q.end();

            return bQ ? Sys.Query(Cmd) : Sys.Execute(Cmd);

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
        if (auto* pHost = host::current()) pHost->m_Notifier.raise(Message);
    }
}



#endif

