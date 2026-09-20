#ifndef XEDITOR_SESSION_H
#define XEDITOR_SESSION_H
#pragma once
#include "types.h"
#include "dependencies/xundo/source/xundo_system.h"
#include <memory>
namespace xeditor {
class session {
public:
    std::unique_ptr<IDocument> m_Document;
    xundo::system m_Undo;
    const editor_descriptor* m_pDesc = nullptr;
    bool m_bRequestFocus = false;
    IDocument& document() noexcept { return *m_Document; }
    xundo::system& undo() noexcept { return m_Undo; }
    std::string display_name() const noexcept {
        return m_Document ? m_Document->getDisplayName() : std::string{};
    }
};
}
#endif
