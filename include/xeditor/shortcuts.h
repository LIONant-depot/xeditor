#ifndef XEDITOR_SHORTCUTS_H
#define XEDITOR_SHORTCUTS_H
#pragma once

#include "host.h"

#include <functional>
#include <optional>
#include <string>
#include <string_view>

// "What key does this action have right now?" - the host's answer, for menus. A menu item that used to carry a hard-coded shortcut text
// ("F2") asks here instead, so the text is always the key that really runs it, whatever the user rebound it to. The host provides one of these
// (m_KeysOf: the action's keys, or nullopt when it knows no such action); without one - or for an action it does not know - the menu shows the
// text it always had. Nothing here depends on how the host stores its keys.
namespace xeditor
{
    struct shortcut_labels
    {
        std::function<std::optional<std::string>(std::string_view /*action path, e.g. "Assets/Rename"*/)> m_KeysOf;
    };

    inline std::string ShortcutText(std::string_view ActionPath, const char* pDefault = "") noexcept
    {
        if (auto* pHost = host::current())
            if (auto* pLabels = pHost->find<shortcut_labels>(); pLabels && pLabels->m_KeysOf)
                if (auto Keys = pLabels->m_KeysOf(ActionPath); Keys) return *Keys;
        return pDefault ? pDefault : "";
    }
}

#endif // XEDITOR_SHORTCUTS_H
