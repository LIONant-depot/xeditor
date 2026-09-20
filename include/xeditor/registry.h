#ifndef XEDITOR_REGISTRY_H
#define XEDITOR_REGISTRY_H
#pragma once
#include "types.h"
#include <unordered_map>
namespace xeditor {
class registry {
public:
    static registry& Get() noexcept { static registry I; return I; }
    void Register(editor_descriptor D) noexcept { auto T = D.m_TypeGuid; m_Map[T] = std::move(D); }
    const editor_descriptor* Resolve(xresource::type_guid T) const noexcept {
        auto It = m_Map.find(T); return It == m_Map.end() ? nullptr : &It->second;
    }
private:
    std::unordered_map<xresource::type_guid, editor_descriptor> m_Map;
};
struct auto_register {
    explicit auto_register(editor_descriptor D) noexcept { registry::Get().Register(std::move(D)); }
};
}
#endif
