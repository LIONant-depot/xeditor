# xeditor - Design (modern editor framework)

**Status:** design for review - no implementation commitment until approved
**Repo / work tree:** `xGPU/dependencies/xeditor` -> [LIONant-depot/xeditor](https://github.com/LIONant-depot/xeditor) (this is the dependency E29 links; replaces the legacy `Src/` tree when the new library ships)
**Related:** `xGPU/Build/EDITOR_VIEWER_FRAMEWORK_PROBLEM_STATEMENT.md` (requirements spine; this doc is the concrete API + migration plan)  
**Date:** 2026-09-20  

---

## 1. Goal

Ship a **modern `xeditor` library** so authoring a resource editor is mostly domain work:

- one document model + one command set per resource type  
- several **hosts** (full editor, embedded viewer, headless session, later thumbnail job) without forking that logic  
- **E29 Level** + **Texture** as the proving migration (success = same or better UX, markedly simpler code)  
- **E30** = headless host example (process stays up until Ctrl-C; humans/AIs talk only via the CLI)

Future editors should be obviously easier because constraints and extension points are clear.

---

## 2. Decisions locked with the owner (2026-09-20)

| Topic | Decision |
|---|---|
| Home | New library **overwrites** `LIONant-depot/xeditor`. Old sources may be deleted when the new library is ready. |
| Legacy | **No** loyalty to old xeditor class trees / xcore-era APIs. Greenfield design on today’s stack. |
| Vertical slice | **E29** is the hard proof (all constraints). **Texture** (`xtexture.plugin`) ports onto the same API. Success = looks the same or better + cleaner code. |
| Phasing | Design for viewer / thumbnail / multi-host now; **implement after** E29+Texture are on the framework. Priority = E29 port. |
| Commands | Readable English for humans **and** AIs. |
| Headless | Process runs until Ctrl-C. Interaction is **always** through the separate CLI app (named pipe). |
| Globals | `xscheduler` shared; other shared services emerge during the port. |
| Deliverables | (1) usable `xeditor` library (2) full E29 + Texture port |

### 2.1 Peer-review ratification (2026-09-20)

Keep: Session owns undo; Document does not. Embedded viewers must not call dock isolation. One writable session per identity (second open focuses). Phase A -> Texture -> E29.

Added: host/workspace CLI plane for non-resource commands (6.2-6.3) and Phase C sub-plan 10.C.1 (Game.dll / SC / compile / browser / idle).


---

## 3. Starting point in the tree today

Useful **seed** already lives under `xGPU/source/Tools/Editor/` (not the old xeditor `Src/`):

| Piece | Role |
|---|---|
| `xeditor_types.h` | `IDocument`, `IUI`, `editor_descriptor` |
| `xeditor_registry.h` | type_guid → descriptor, `auto_register` |
| `xeditor_dock_isolation.h` | per-`full_guid` ImGui dock ClassId (E29 pattern, multi-instance) |
| `xeditor_console.h` / `xeditorcli.cpp` | named-pipe pump + generic CLI |
| `xeditor_toolbar.h` / `xeditor_inspector.h` | shared chrome |
| `xeditor_resource_tab.h` | root-tab icon + `Name###Id` |

Texture already implements `xeditor::IDocument` and a plugin `session` with `xundo` commands shared by UI and headless. E29 still owns its own panels, dockspace, and CLI pipe.

**Implementation move:** evolve the `xGPU/source/Tools/Editor` seed into this dependency (`dependencies/xeditor`). E29 and plugins include/link this path. Do not grow a second parallel framework under `source/Tools/Editor` long-term - fold it in here.

---

## 4. Core concepts

Keep the problem-statement vocabulary; tighten ownership.

```
Resource (TypeGUID + instance)     on disk / asset mgr
    │
Document                           load / save / dirty / domain state
    │
Session                            Document + xundo::system + command registrations
    │                                addressable; optional views
    ├── View / IUI                 optional presentation (0..N)
    └── Host                       FullEditor | EmbeddedViewer | Headless | (later) ThumbnailJob
```

### 4.1 Document

- Owns **domain state** and serialization only.  
- Does **not** own the undo stack (fix today’s Texture quirk: `IDocument` embeds `m_Undo` while `session` also owns `m_Undo` - Session wins; Document drops undo).  
- No ImGui, no window, no GPU requirement unless a specific command needs render-headless.

### 4.2 Session

- One open editing context for one resource identity.  
- Owns `xundo::system` and registers that document’s commands.  
- Identity for addressing: prefer **stable resource path / display name + disambiguator**, not a separate opaque session UUID as the primary human surface (see §6). Internally may still key by `xresource::full_guid`.  
- Policy (default): **at most one writable session per resource identity per process**; a second open focuses the existing session. (Revisit only if E29 port hits a hard conflict.)

### 4.3 View (`IUI`)

- Optional. `Render(Document&, ViewContext)` where context says: full / embedded / (later) offscreen target.  
- Headless logic-only sessions construct **zero** views.  
- Thumbnail (later): same presentation entry with a fixed profile → 128×128 readback - designed for, not built in deliverable 1-2.

### 4.4 Host

| Host | Window / ImGui | Mutation | Undo | Notes |
|---|---|---|---|---|
| **Full editor** | Yes - root dock + menu bar; children only dock inside | Yes | Yes | Texture’s shape generalized |
| **Embedded viewer** | Parent supplies a rect; **no** isolated dockspace | Read-only default | No (unless explicitly editable mini-session later) | Deferred past E29 |
| **Headless session** | None | Yes | Yes | E30; CLI-only |
| **Thumbnail job** | None | No | No | Deferred; snapshot/saved revision only |

Graphical bootstrap must be **optional**: logic-headless must not create a window or ImGui context.

---

## 5. Proposed public API (sketch)

Names are illustrative; keep them short and consistent with existing `xeditor::` seed.

```cpp
namespace xeditor {

struct IDocument {
  virtual ~IDocument() = default;
  virtual xresource::full_guid getGuid() const noexcept = 0;
  virtual std::string          getDisplayName() const noexcept = 0; // CLI path segment
  virtual bool                 Load() noexcept = 0;
  virtual std::string          Save() noexcept = 0;  // empty = ok; else human-readable error
  virtual bool                 isDirty() const noexcept = 0;
};

struct ViewContext {
  enum class kind { Full, Embedded, Offscreen /* later */ } Kind = kind::Full;
  // host-provided size / device / render target as needed
};

struct IUI {
  virtual ~IUI() = default;
  virtual void Render(IDocument& Doc, const ViewContext& Ctx) noexcept = 0;
};

struct editor_descriptor {
  xresource::type_guid TypeGuid;
  const char*          TypeName;           // "Texture", "Level", …
  std::function<std::unique_ptr<IDocument>(xresource::full_guid)> CreateDocument;
  std::function<std::unique_ptr<IUI>()>                             CreateUI; // optional
  bool SupportsHeadless = true;
  // later: SupportsThumbnail, SupportsEmbeddedEdit, …
};

class registry { /* Register / Resolve by type_guid - keep auto_register */ };

// Runtime: open instances
class session {
public:
  IDocument&       document() noexcept;
  xundo::system&   undo() noexcept;
  const editor_descriptor& descriptor() const noexcept;
  // RegisterCommands() called by type author or factory
};

class host {
public:
  // Process-wide services (filled as port surfaces them)
  // xscheduler*, asset/lib mgr handles, console, …

  session* open(xresource::full_guid Guid);      // or focus existing writable
  session* find_by_name(std::string_view Name);  // CLI routing
  void     close(session&);

  void pump();   // one interactive frame OR one headless loop tick (console + jobs)
};

} // namespace xeditor
```

**Full-editor shell** (library-provided helpers, type fills content):

- Root window: `Name###guid`, menu bar (Undo/Redo/Save/Compile/… via shared toolbar model).  
- Nested `DockSpace` + `IsolateEditorDockspace(guid)` so panels cannot cross-dock.  
- Type registers child panel factories or just draws known panels (E29-style) behind the same isolation helper.

**Type author checklist**

1. Implement `IDocument` (+ domain commands on the session’s `xundo::system`).  
2. Optionally implement `IUI` / panels.  
3. `auto_register` an `editor_descriptor`.  
4. No private CLI, no private pipe, no private undo core.

---

## 6. Commands and CLI

### 6.1 Rules

- **All mutations** go through `xundo` commands (UI buttons call the same `Execute`/`Query` as the CLI).  
- Command names and help strings are **readable English** (`AddComponent`, `SetSRGB`, `SaveLevel` - not cryptic codes).  
- Help / schema listing must be enough for an AI or new human without reading source.

### 6.2 Routing (human surface)

Two planes (needed to retire E29CLI without losing Source Control / compile / Asset Browser / workspace Save):

| Plane | Form | Target |
|---|---|---|
| Session | `Name\Command args` | That resource session's xundo |
| Workspace | `Command args` or `Host\Command` / `.\Command` | Host workspace xundo |

Dispatch: `Host` or `.` -> workspace; other `Name\` -> session; no backslash -> workspace only (do not silently pick a session; error lists open names). Ambiguous display names: fail loud or require `Name#<hex>`.

### 6.3 Workspace command set

Host-owned xundo separate from each session. Process verbs (`help`/`list`/`open`/`close`) plus shell services (Source Control, compilation queue, Asset Browser, Play gates, chat). Phase C: lift today's E29Undo table here first; move clearly Level-document commands to the Level session when obvious. Bare `Undo` = workspace stack; `SheKnewMe\Undo` = that session only.

### 6.4 Transport

### 6.3 Transport

- One named pipe per **host process** (default `\\.\pipe\xEditor_Console`).  
- `xeditorcli "…"` is the only supported client UX for headless (and for scripting against a live GUI host).  
- Retire per-editor CLIs (E29CLI, etc.) once that editor is on the framework.

### 6.5 Headless process (E30)

- Starts host with **no** window / ImGui.  
- Opens sessions on demand via CLI (`open` / first command).  
- `pump()` loop until Ctrl-C.  
- Dirty exit: refuse silent discard - require save or explicit discard flag (align with problem statement).

---

## 7. Shared services

| Service | Ownership |
|---|---|
| `xscheduler` | Process-wide; host holds/refcounts |
| Asset / library mgr (`e10::g_LibMgr`, `xresource::g_Mgr`) | Existing globals initially; host may wrap accessors as the port forces clarity |
| Command console | One per host process |
| Icon atlas / ImGui fonts | Graphical hosts only |
| Undo | **Per session**, never process-global |

Further globals are discovered while porting E29 - document them in this file as they appear; avoid inventing a service locator up front.

---

## 8. Dock isolation and chrome

- Generalize E29’s nested dockspace + `WindowClass` (already sketched in `xeditor_dock_isolation.h`) keyed by **open resource `full_guid`**.  
- Full editor: Texture pattern - menu bar on the root, children dock only inside that root.  
- Embedded viewer (later): **must not** call isolation; it is a child rect of someone else’s session.

Root tabs: keep `xeditor_resource_tab.h` policy - icon + `Name###stableId`; do **not** fight ImGui with tall `FramePadding` for root tabs.

---

## 9. What we design for but do **not** build yet

Explicitly deferred until E29+Texture are on the library:

1. Embedded viewer host used inside foreign editors  
2. Thumbnail job (128×128, cache key, GPU readback path)  
3. Asset-browser double-click → open editor wiring as a polished product feature (may stub for Texture during port)  
4. Multi-writer / exotic concurrency beyond “one writable session per identity”  
5. DLL-loaded editor plugins (static registration in-process is enough)

The API must not paint us into a corner that blocks these.

---

## 10. Migration outline

### Phase A - Library in this repo (Deliverable 1)

1. Replace legacy `Src/` with a modern layout, e.g.:

   ```text
   include/xeditor/          # public headers
   src/                      # console, host, registry, dock, toolbar…
   cli/xeditorcli.cpp
   docs/DESIGN.md            # this file
   ```

2. Move/evolve the `xGPU/source/Tools/Editor` seed into this dependency; leave thin shims or `#include` redirects under `source/Tools/Editor` until consumers switch.  
3. Implement: registry, session table, host pump, console routing with `Name\Command`, dock isolation helpers, toolbar/inspector as today.  
4. Headless host skeleton compilable (E30 can be a thin xGPU example that only links the lib).  
5. Unit-level checks: open fake document, execute command via pump string, undo/redo, no window.

**Exit:** another engineer can implement a tiny document type against public headers without reading E29.

### Phase B - Texture onto the library

1. Keep editor in `xtexture.plugin` (correct place).  
2. Session/document/commands call framework host APIs; delete duplicated pipe/CLI if any.  
3. Full-editor chrome uses library shell (menu bar + isolated dock).  
4. Verify UI parity + CLI: `SheKnewMe\SetSRGB -Value 1`, save, undo.

### Phase C - E29 onto the library (vertical slice)

1. Map Level document(s) / open scenes to `IDocument` + session(s) - exact grain (one session per level resource vs per open scene) decided during port; prefer **one session per editable level resource** if that matches asset identity.  
2. Panels (`LevelTree`, `EntityProperties`, …) become children of the framework full-editor shell (same visuals).  
3. All `e29::commands::Run` paths remain the real mutations; console routes `LevelName\AddComponent …`.  
4. Remove E29-only dock ClassId literal; use per-guid isolation.  
5. Remove E29CLI in favor of `xeditorcli`.  
6. Domain shell subsystems: see **10.C.1**.

#### 10.C.1 E29 global / shell subsystems

Non-document commands (Workspace, Source Control, Compilation, Asset Browser, Play, Chat, Game.dll/idle hooks, open-Texture) land on the **workspace** plane when E29CLI retires. Phase C order: (1) library supports both planes (2) lift E29 commands onto workspace with same names (3) add Level session; move only obvious document commands (4) host hooks for Game.dll/idle (5) delete E29CLI. Risk: bare `Undo` stays workspace (= today's E29Undo); Level undo is `LevelName\Undo`.

**Exit (success criteria):**

- E29 looks the same or better.  
- Texture + Level both use the same host/console/undo rules.  
- Code volume for “editor plumbing” in E29 drops; new editor checklist (§5) is obviously shorter than today’s E29 bootstrap.  
- E30: Texture open/edit/save via CLI with no window; plus one workspace command and one `LevelName\...` once Phase C routing exists.

### Phase D - Later

Embedded viewers, thumbnails, asset-browser open, more resource types (E19/E20/…).

---

## 11. E30 sketch

`E30_HeadlessEditor` (name flexible):

- `xeditor::host` in headless mode  
- starts console thread  
- loop: `host.pump()` until Ctrl-C  
- no `Create(MainWindow)`, no ImGui  
- documented examples in README for `xeditorcli`

---

## 12. Non-goals (for deliverables 1-2)

- Redesigning xGPU rendering  
- Replacing `xproperty` / `xundo` wholesale (only multi-session-safe usage)  
- Pixel-perfect redesign of E29 UX  
- Porting every Exx editor

---

## 13. Acceptance checklist (before calling Deliverable 1 done)

- [ ] Public headers build as a library consumed by xGPU Debug  
- [ ] `xeditorcli` talks to a running host (GUI or E30)  
- [ ] `Name\Command` routes to the correct session’s undo system  
- [ ] Dock isolation helper usable by a sample full editor  
- [ ] Headless host runs without creating a window/ImGui  
- [ ] DESIGN.md + short README “how to add an editor”  

Deliverable 2 adds E29+Texture parity checks (manual smoke + existing E29 smokes still meaningful).

---

## 14. Open points to resolve during implementation (not blockers for approving this design)

1. Exact display-name rules for Level vs Texture (asset name vs file stem).  
2. Whether Level is one document for the whole editor workspace or one per open scene resource.  
3. Where process-wide open-instance table lives (host vs asset mgr) - seed comments said asset mgr; host-owned table may be simpler for headless.  
4. Shim strategy while headers move from `xGPU/source/Tools/Editor` → this repo (avoid breaking Texture mid-move).

---

## 15. Ask before coding

Please confirm or amend:

1. This API split (Document vs Session vs Host vs IUI) is acceptable.  
2. `ResourceName\Command` as the primary CLI surface is correct.  
3. Phase order A → B (Texture) → C (E29) → E30 alongside A/B is OK (or prefer E30 earlier).  
4. Anything in §9 that must move **into** deliverable 1-2 after all.