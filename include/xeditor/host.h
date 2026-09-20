#ifndef XEDITOR_HOST_H
#define XEDITOR_HOST_H
#pragma once
#include "session.h"
#include "registry.h"
#include <algorithm>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>
namespace xeditor {
class host {
public:
    xundo::system m_Workspace;
    std::vector<std::unique_ptr<session>> m_Sessions;

    session* find_by_guid(xresource::full_guid G) noexcept {
        for (auto& S : m_Sessions)
            if (S->m_Document && S->m_Document->getGuid() == G) return S.get();
        return nullptr;
    }
    session* find_by_name(std::string_view Name) noexcept {
        session* Hit = nullptr;
        for (auto& S : m_Sessions) {
            if (!S->m_Document || S->display_name() != Name) continue;
            if (Hit) return nullptr;
            Hit = S.get();
        }
        return Hit;
    }
    session* open(xresource::full_guid Guid) noexcept {
        if (auto* E = find_by_guid(Guid)) { E->m_bRequestFocus = true; return E; }
        auto* Desc = registry::Get().Resolve(Guid.m_Type);
        if (!Desc || !Desc->m_CreateDocument) return nullptr;
        auto S = std::make_unique<session>();
        S->m_pDesc = Desc;
        S->m_Document = Desc->m_CreateDocument(Guid);
        if (!S->m_Document) return nullptr;
        S->m_Undo.Init({}, false);
        S->m_Document->Load();
        m_Sessions.push_back(std::move(S));
        return m_Sessions.back().get();
    }
    void close(session* S) noexcept {
        if (!S) return;
        m_Sessions.erase(std::remove_if(m_Sessions.begin(), m_Sessions.end(),
            [&](auto& U){ return U.get()==S; }), m_Sessions.end());
    }
    static std::string run_on(xundo::system& Sys, const std::string& Cmd) noexcept {
        const auto Name = Cmd.substr(0, Cmd.find(' '));
        const auto Q = Sys.GetQueryCommandNames();
        const bool bQ = std::find(Q.begin(), Q.end(), Name) != Q.end();
        return bQ ? Sys.Query(Cmd) : Sys.Execute(Cmd);
    }
    std::string dispatch(std::string_view Line) noexcept {
        while (!Line.empty() && (Line.back()=='\n' || Line.back()=='\r')) Line.remove_suffix(1);
        if (Line.empty()) return {};
        if (Line == "help") {
            std::string R = "Workspace: bare Command or Host\\Command. Session: Name\\Command.\n";
            for (auto& N : m_Workspace.GetEditCommandNames()) R += N + "\n";
            for (auto& N : m_Workspace.GetQueryCommandNames()) R += N + "\n";
            R += "Sessions:\n";
            for (auto& S : m_Sessions) if (S->m_Document) R += "  " + S->display_name() + "\n";
            return R;
        }
        if (Line == "list") {
            std::string R;
            for (auto& S : m_Sessions) {
                if (!S->m_Document) continue;
                auto G = S->m_Document->getGuid();
                char Buf[128];
                std::snprintf(Buf, sizeof(Buf), "%s  type=%016llX inst=%016llX dirty=%d\n",
                    S->display_name().c_str(),
                    (unsigned long long)G.m_Type.m_Value,
                    (unsigned long long)G.m_Instance.m_Value,
                    S->m_Document->isDirty() ? 1 : 0);
                R += Buf;
            }
            return R.empty() ? std::string("(no open sessions)\n") : R;
        }
        const auto Slash = Line.find('\\');
        if (Slash != std::string_view::npos) {
            auto Target = Line.substr(0, Slash);
            auto Rest = Line.substr(Slash + 1);
            if (Target == "Host" || Target == ".")
                return run_on(m_Workspace, std::string(Rest));
            auto* S = find_by_name(Target);
            if (!S) return std::string("No open session '") + std::string(Target) + "'. Use list.\n";
            return run_on(S->m_Undo, std::string(Rest));
        }
        return run_on(m_Workspace, std::string(Line));
    }
};
}
#endif
