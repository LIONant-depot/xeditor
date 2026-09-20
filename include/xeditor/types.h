#ifndef XEDITOR_TYPES_H
#define XEDITOR_TYPES_H
#pragma once
#include "dependencies/xresource_guid/source/xresource_guid.h"
#include <functional>
#include <memory>
#include <string>
namespace xeditor {
struct IDocument {
    virtual ~IDocument() = default;
    virtual xresource::full_guid getGuid() const noexcept = 0;
    virtual std::string getDisplayName() const noexcept = 0;
    virtual bool Load() noexcept = 0;
    virtual std::string Save() noexcept = 0;
    virtual bool isDirty() const noexcept = 0;
};
struct ViewContext {
    enum class kind { Full, Embedded, Offscreen } Kind = kind::Full;
    bool bEmbedded = false;
};
struct IUI {
    virtual ~IUI() = default;
    virtual void Render(IDocument& Doc, const ViewContext& Ctx) noexcept = 0;
};
struct editor_descriptor {
    xresource::type_guid m_TypeGuid{};
    const char* m_TypeName = "";
    std::function<std::unique_ptr<IDocument>(xresource::full_guid)> m_CreateDocument;
    std::function<std::unique_ptr<IUI>()> m_CreateUI;
    bool m_bSupportsHeadless = true;
};
}
#endif