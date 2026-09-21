# xeditor - global services and shared UI (proposal)

Status: **proposal, 2026-09-21** - for owner review before any code moves. Extends `DESIGN.md`
(section 2.2 "Globals: xscheduler shared; other shared services emerge during the port").

## 1. Problem

E29 reaches shared state through ~30 free-standing globals (`g_pGameMgr` 285 uses, `g_pState` 52,
`g_pLevelUndo` 24, `g_pGamePlugin` 20, `g_pEditorHost` 26, `g_pUndo`, `g_pConsoleLog`, `g_ScriptConfig`,
`g_OpenTextureEditors`, `g_PendingRemoveDependencyConfirm`, ...) plus `e10::g_LibMgr` (231 uses) from the
asset manager. Symptoms:

- `g_pEditorHost`, `g_pLevelUndo` and `g_pUndo` are each *defined more than once*, guarded by
  `#ifndef E29_G_P_..._DEFINED` macros, purely because of include order.
- Plugins (xtexture.plugin) cannot build outside xGPU: they include `source/Examples/E10_*`.
- 17 hand-rolled `BeginPopupModal` blocks (E29 8, E10 12 more), 41 `Debugger()` error call sites, ~30
  tooltips and ~160 hard-coded colours: nothing can be re-themed or made headless in one place.
- Commands touch UI flags (`editor_state::m_bEntityInspectorDirty`) and `Debugger()` opens an ImGui modal,
  so a command is not usable without a UI.

## 2. Ownership classes (what is global and what is not)

| Class | Lifetime | Examples | Home |
|---|---|---|---|
| Process service | whole process | resource/library manager, scheduler, workspace undo, console log, source control, idle work, game-plugin loader | `xeditor::host` service registry |
| Session state | one open resource | selection, level undo, document, dirty flag, write lock | `xeditor::session` / the plugin's own document |
| Window state | one OS window | drawer edge/size, dock class id, open popups | `xeditor::host` per-viewport maps |
| Editor-private | one editor | tree filter text, column widths | the editor plugin itself - never global |

Rule: if only one editor uses it, it is not a service.

## 3. One root: `xeditor::host`

`xeditor::host` already owns workspace undo, write locks, the Play singleton, drawers and sessions.
It becomes the single process root. The only remaining global is one accessor,
`xeditor::host::current()`, set by the application at startup. Editors hold a `host&`.

### 3.1 Service registry

```cpp
host.services().set<xresource_editor::library_mgr>(MyLibMgr);   // provider registers
auto& Lib = host.services().get<xresource_editor::library_mgr>(); // consumer resolves
```

- Non-owning pointers, registered by whoever owns the object (application or a depot's `source/editor`).
- Keyed by a **stable name string**, not `typeid`: `inline` singletons and `type_index` are per-binary,
  so the key must survive a DLL boundary (Game.dll, plugin DLLs). A service is always passed as a pointer.
- `get` on a missing service returns null; headless hosts simply do not register UI services.
- Rebind-safe: consumers never cache the pointer across a hot reload (the `GameMgr` is destroyed and
  rebuilt on every Game.dll reload) - they call `get` each time.

### 3.2 Domain services live in their own depot

| Service | Depot | Notes |
|---|---|---|
| library / asset manager (was `e10::g_LibMgr`) | xresource_pipeline_v2 `source/editor` | already headless |
| source control | xsource_control `source/editor` | provider + commands + panel |
| scene / entity model, prefab authoring | xscene.plugin `source/Editor` | reused by Level and a future Prefab editor |
| level document, Play | xlevel.plugin `source/Editor` | at most one Play process-wide (host) |

xeditor never includes any of them.

## 4. Shared UI formalised in xeditor

All optional at runtime (headless registers none) and all routed through the host so a theme or a
headless fallback is one change:

| Service | Replaces | Headless behaviour |
|---|---|---|
| `notify` (info / warn / error) | `Debugger()` + `RenderErrorPopup` (41 call sites) | writes to console log, command returns the text |
| `confirm` / `modal` queue | 17 hand-rolled `BeginPopupModal` blocks | auto-resolves to the command's `-Force` / default |
| `tooltip` helper | ~30 ad-hoc tooltips | none |
| `theme` (semantic colour roles) | ~160 literal colours | none |

Modals are opened by request and rendered once per frame from the top-level ID scope (the same
constraint `RenderErrorPopup` already documents), never from inside a nested ID stack.

## 5. Commands are UI-free

- A command mutates document state and returns a string. It never calls ImGui, never opens a popup and
  never sets a UI-only flag.
- UI state that must react to a command subscribes to a change event on the session (e.g. "selection
  changed") instead of the command poking `m_bEntityInspectorDirty`.
- Anything a command needs from the human (a confirmation) is a declared argument (`-Force`) so the AI /
  CLI path never blocks on a dialog.

## 6. Headless split

`host.h` currently includes `drawer.h` (ImGui). Split it:

- `host_core.h` - workspace undo, write locks, Play singleton, sessions, dispatch, service registry. No ImGui.
- `host_ui.h` - drawers, modal/notify/tooltip/theme presenters. Includes ImGui.

A headless host (E30) links `host_core.h` only.

## 7. Migration order (each step builds and behaves identically)

1. Introduce the service registry and `host::current()`; delete the three duplicate global definitions
   and their include-order macros.
2. Move E29's process-level globals into registered services; leave editor-private ones with the editor.
3. Route `Debugger()` and the 17 modals through `notify` / `confirm`.
4. Pull UI flags out of commands (event subscription).
5. Split `host.h` into core + UI.
6. Move the domain services out to their depots (asset manager, source control) and the editor code
   to xscene.plugin / xlevel.plugin.
