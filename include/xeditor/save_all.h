#ifndef XEDITOR_SAVE_ALL_H
#define XEDITOR_SAVE_ALL_H
#pragma once

#include "dependencies/xdelegate/source/xdelegate.h"

#include <string>
#include <string_view>
#include <vector>

namespace xeditor
{
    // The icons of the two verbs (Segoe MDL2, which the editor's font has merged in): the buttons show the icon instead of the word, and the menus put them in front of Save and Save All.
    inline constexpr const char* save_icon_v     = "\xEE\x9D\x8E";      // U+E74E Save (one floppy disk)
    inline constexpr const char* save_all_icon_v = "\xEE\xA8\xB5";      // U+EA35 SaveAll (two)
    inline constexpr const char* menu_arrow_v    = "\xEE\x9C\x8D";      // U+E70D ChevronDown: the down arrow of the editors' menu button
    inline constexpr const char* library_icon_v  = "\xEE\xA3\xB1";      // U+E8F1 Library: the icon of the Resources (the books of the tree's root)

    // Two verbs: Save saves the local work (what the person is looking at: a descriptor, a Level, the renames and moves in the resource view), Save All makes sure everything in the editor is saved,
    // whoever owns it. Save All is this event: anything that keeps unsaved work subscribes (host.m_SaveAll.m_OnSave.Register...) and saves it when it fires, telling the report what it saved and what it could not.
    // A subscriber that points at something with a shorter life than the host (a Level's editor) removes itself first (RemoveDelegates) - the same rule as host.m_IdleWork.m_OnRun.
    struct save_report
    {
        std::vector<std::string> m_Saved;        // what was written
        std::vector<std::string> m_Failed;       // "what: why"

        void Saved (std::string_view What)                          { m_Saved.emplace_back(What); }
        void Failed(std::string_view What, std::string_view Why)    { m_Failed.push_back(std::string(What) + ": " + std::string(Why)); }
        bool empty() const noexcept                                 { return m_Saved.empty() && m_Failed.empty(); }
    };

    struct save_all
    {
        xdelegate::thread_unsafe<save_report&> m_OnSave;

        save_report Run() noexcept
        {
            save_report Report;
            m_OnSave.NotifyAll(Report);
            return Report;
        }
    };
}

#endif
