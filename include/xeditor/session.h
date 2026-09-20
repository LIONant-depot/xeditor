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
    xundo::system              m_Undo;

    // Plugin / host-bridge sessions (Texture): document + undo live elsewhere.
    // When set, dispatch / list / dirty use these instead of the owned members.
    IDocument*      m_pBorrowedDocument = nullptr;
    xundo::system*  m_pBorrowedUndo     = nullptr;

    const editor_descriptor* m_pDesc = nullptr;
    bool m_bRequestFocus = false;

    IDocument* document_ptr() noexcept
    {
        if (m_pBorrowedDocument) return m_pBorrowedDocument;
        return m_Document.get();
    }
    const IDocument* document_ptr() const noexcept
    {
        if (m_pBorrowedDocument) return m_pBorrowedDocument;
        return m_Document.get();
    }

    IDocument& document() noexcept { return *document_ptr(); }

    xundo::system& undo() noexcept
    {
        return m_pBorrowedUndo ? *m_pBorrowedUndo : m_Undo;
    }

    std::string display_name() const noexcept
    {
        const IDocument* p = document_ptr();
        return p ? p->getDisplayName() : std::string{};
    }

    xresource::full_guid guid() const noexcept
    {
        const IDocument* p = document_ptr();
        return p ? p->getGuid() : xresource::full_guid{};
    }

    bool is_borrowed() const noexcept
    {
        return m_pBorrowedUndo != nullptr || m_pBorrowedDocument != nullptr;
    }
};

} // namespace xeditor

#endif
