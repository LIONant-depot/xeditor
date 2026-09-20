# xeditor

Modern editor framework for xGPU (E29 Level + Texture proving ports).

**Work tree:** `xGPU/dependencies/xeditor` (this folder). Design: [DESIGN.md](DESIGN.md).

## Layout

- `include/xeditor/` — public API: `types`, `session`, `host`, `registry`, `dock_isolation`, `full_editor_shell`, `console`
- `source/Tools/Editor/xeditor_*.h` (in xGPU) — thin shims into `include/xeditor/`
- `Src/` — legacy (remove when fully migrated)

## How to add an editor

1. Implement `xeditor::IDocument` (guid, display name, Load/Save/dirty). **Do not** put undo on the document.
2. Own an `xundo::system` on the session (or borrow one into `xeditor::session` via `m_pBorrowedUndo` / `m_pBorrowedDocument` like Texture).
3. Register domain commands on that undo system (same path UI buttons and CLI use).
4. `xeditor::registry::Register` an `editor_descriptor` (`TypeGuid`, `TypeName`, `CreateDocument`).
5. Each frame (or on open/close): put the session in `host.m_Sessions` (owned like Level, or borrowed like Texture).
6. Full-editor chrome: root `Name###stableId`, zero `WindowPadding`, menu bar, nested `DockSpace`, then `FinishFullEditorDockspace(id, guid)`.
7. No private CLI pipe — use the process console (`\\.\pipe\xEditor_Console`) → `host::dispatch`.

## CLI (AI / scripts)

With `xGPU_unit_test` running E29:

```text
xeditorcli "list"
xeditorcli "help"
xeditorcli "LevelName\AddComponent -Scene ... -Id ..."
xeditorcli "OpenTextureEditor -Library ... -Asset ..."
E29CLI "list"                          # same pipe; thin alias
E29CLI "E29/Edit/Select -Scene ... -Id ..."   # legacy History path still works
```

Default pipe: `\\.\pipe\xEditor_Console`. Override: `xeditorcli "list" --pipe \\.\pipe\Other`.

## Status (2026-09-20)

- Deliverable 1 core: host dual plane, sessions, dock helpers, console routing — **done** (E30 headless windowless host still open).
- Deliverable 2 proving ports: Level session + document cmds, Texture host bridge, idle/Game.dll hooks, E29CLI→host — **done**.
