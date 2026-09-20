# xeditor - Design (modern editor framework)

**Status:** living design - Deliverable 1-2 proving port shipped; Host Drawer / Level-as-peer (2.2, 4.5-4.7, Phase D) awaiting implementation
**Repo / work tree:** `xGPU/dependencies/xeditor` -> [LIONant-depot/xeditor](https://github.com/LIONant-depot/xeditor) (this is the dependency E29 links; replaces the legacy `Src/` tree when the new library ships)
**Related:** `xGPU/Build/EDITOR_VIEWER_FRAMEWORK_PROBLEM_STATEMENT.md` (requirements spine; this doc is the concrete API + migration plan)  
**Date:** 2026-09-20; addendum 2026-09-21  

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

### 2.2 Host Drawer + peer editors (locked 2026-09-21)

| Topic | Decision |
|---|---|
| Level is not the core | **Level is a peer editor** like Texture. It must not own the process shell. |
| Real core (host services) | Resources, Assets, Source Control, Idle Work, Log, Commands, Compilation, Project Settings - plus the process CLI/workspace undo plane. |
| Drawer | Unreal Content Drawer-like **edge overlay** toggled with **Space** (focus-gated: not while typing in text fields). Children dock **only inside** the drawer. |
| Per window | Drawer opens in the **focused OS/ImGui root window only**, not every window. A floated Texture window has its own drawer instance. |
| Edge + size | User-chosen edge (default bottom). "Resize" = distance from that edge; the other axis stays near max. Remember size/edge per window. |
| Local clones | Any editor may open a *local* Resources (etc.) panel docked only in **that editor's** dockspace. Same UI type; different ownership; no cross-dock into drawer or other editors. |
| Play | **Belongs to the Level editor** UI. Process-wide: **at most one Play** even if many Level editors are open. |
| Edit vs View | **Every resource** (including scenes) has edit/view. At most one **writer** per identity. First editor that **starts modifying** takes the write lock; others are **read-only**. Level edits both **levels** and **scenes** - scene locks matter when the same scene appears in multiple levels. |
| Modern | Prefer this host/drawer/peer model over growing E29-as-shell further. |



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

- One open context for one resource identity (level, texture, **scene**, ...).
- Owns `xundo::system` and registers that document's commands.
- Identity for addressing: prefer **stable resource path / display name + disambiguator**, not a separate opaque session UUID as the primary human surface (see section 6). Internally may still key by `xresource::full_guid`.
- **Edit vs View (all resources):** a session is either **writable** or **read-only view**.
  - Process-wide: **at most one writable session per resource identity**.
  - Opening additional views of the same identity is allowed as **read-only**.
  - **Write lock:** the first session that **begins a mutating command** on that identity becomes the writer; others stay/become read-only and show a clear "edited elsewhere" state. Releasing the writer does not silently promote another view - explicit user action required.
- **Level's double duty:** a Level editor may reference multiple **scenes**. Each scene is its own identity for edit/view locks. Mutating entities takes that **scene's** write lock. Two Level editors showing scene S: only one may mutate S.

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

### 4.5 Host window vs peer editors vs Drawer

Clarify the three layers that E29 currently conflates:

| Layer | What it is | Examples |
|---|---|---|
| **Host process** | One process, one CLI pipe, workspace undo, shared services | `xeditorcli`, scheduler, lib mgr |
| **Host window** | One OS / ImGui root viewport (main app or a floated editor window) | Main frame; undocked Texture window |
| **Peer editor** | Full-editor shell for **one** resource session | Level, Texture, future types |
| **Drawer** | Per-host-window edge overlay for **process services** UI | Resources, SC, Idle, Log, Commands, ... |

**Level is not the core editor.** It is a peer full-editor like Texture. The "Parent Editor" dock that today owns Resources / SC / Log / Commands / Idle while also hosting Level must be split:

1. **Peer editor root** - Level (or Texture) only: viewport, hierarchy, inspectors, Level Play/Stop, .... Isolated dock ClassId keyed by that editor's resource identity (as today).
2. **Drawer** - host services listed below. Isolated dock ClassId **distinct** from every peer editor so panels cannot cross-dock either way.

#### Drawer behavior (Unreal Content Drawer-like, modernized)

- **Toggle:** Space opens/closes the drawer in the **focused host window only**. Other windows unchanged.
- **Focus gate:** Space ignored while a text field / console input / modal wants keys.
- **Stacking:** Drawer draws **above** peer editors in that window (overlay). v1 = overlay only; optional later "Dock in layout" (Unreal).
- **Edge:** User preference per window - bottom (default), top, left, or right.
- **Size:** Primary dimension = distance from the chosen edge; orthogonal axis stays near maximized. Persist edge + depth (+ optional last tab) per window.
- **Isolation:** Nested `DockSpace` + WindowClass so **only drawer children** dock inside.
- **Starting tabs (v1):** Resources · Assets · Source Control · Idle Work · Log · Commands · Compilation · Project Settings.
- **Not in the drawer:** Play/Stop (Level), per-asset Texture Save/Compile toolbar, domain inspectors for the open resource.

#### Local service panels inside a peer editor

Any peer editor may spawn a **local** Resources / Log / ... panel docked only in **that** editor's dockspace (same widget type, editor-owned instance):

- Local panel ClassId = that editor's isolation class - **not** the drawer class.
- Cannot drag into the drawer or into another editor.
- Closing the editor destroys its local panels; the window's drawer remains.

#### Multi-window

If Texture is floated to its own OS window, that window has its **own drawer** (own edge/size memory). Opening the drawer there does not open it on the main window. Process services behind the UI stay shared (one SC system, one compile queue); each drawer is a **view** onto them.

### 4.6 Play policy (Level)

- Play controls live on the **Level editor** chrome (toolbar), never in the drawer.
- **Process-wide singleton:** at most one Play/PIE-style run at a time, even with many Level editors open. Starting Play while another Level is playing **fails loud** (default); auto-stop is a product option later.
- Headless/AI: `Play` / `Stop` still honor the singleton.

### 4.7 Mapping to CLI planes

| Surface | Plane |
|---|---|
| Drawer services (SC, compile, open asset, idle, ...) | Workspace / Host |
| Level entity/scene/prefab edits | LevelName + scene write lock |
| Texture domain edits | TextureName |
| Play/Stop | Level UI + commands; singleton enforced in host |


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

Three isolation domains (never share ClassId across domains):

1. **Peer editor** - keyed by open resource `full_guid` (`DockClassForResource` / `FinishFullEditorDockspace`).
2. **Drawer** - keyed by host-window id + a stable drawer namespace (not a resource guid).
3. **Embedded viewer** (later) - **must not** call isolation; child rect of someone else's session.

Full editor: Texture pattern - menu bar on the root, children only dock inside that root.
Drawer: edge overlay; children only dock inside the drawer dockspace.
Root tabs: `xeditor_resource_tab.h` - icon + `Name###stableId`; do **not** fight ImGui with tall `FramePadding` for root tabs.


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

Non-document **service** commands (Workspace, Source Control, Compilation, Asset Browser, Chat, Game.dll/idle hooks, open-Texture) land on the **workspace** plane and are surfaced in the **Drawer** UI. **Play stays on the Level editor** (process-wide singleton). AI uses workspace/Host for services and LevelName for document cmds. Phase C order: (1) library supports both planes (2) lift E29 commands onto workspace with same names (3) add Level session; move only obvious document commands (4) host hooks for Game.dll/idle (5) delete E29CLI. Risk: bare `Undo` stays workspace (= today's E29Undo); Level undo is `LevelName\Undo`.

**Exit (success criteria):**

- E29 looks the same or better.  
- Texture + Level both use the same host/console/undo rules.  
- Code volume for “editor plumbing” in E29 drops; new editor checklist (§5) is obviously shorter than today’s E29 bootstrap.  
- E30: Texture open/edit/save via CLI with no window; plus one workspace command and one `LevelName\...` once Phase C routing exists.

### Phase D - Host Drawer + demote Level to peer (next major)

1. Introduce `xeditor` **Drawer** API: per host-window overlay, edge + depth persistence, Space toggle, isolated dockspace.
2. Move Resources / Assets / SC / Idle / Log / Commands / Compilation / Project Settings out of the Level "Parent Editor" into the drawer.
3. Level becomes a peer full-editor tab/window like Texture (own root dock only).
4. Enforce **edit/view** write locks for scenes (and all resources); multi-Level open safe.
5. Enforce **single Play** in the host.
6. Optional: "Dock drawer in layout" after overlay v1.

### Phase E - Later

Embedded viewers, thumbnails, asset-browser open polish, more resource types (E19/E20/...).

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

- [x] Public headers build as a library consumed by xGPU Debug
- [x] `xeditorcli` talks to a running host (GUI) — pipe `\\.\pipe\xEditor_Console` (E29 listens here; E29CLI is a thin alias)
- [x] `Name\Command` routes to the correct session's undo system
- [x] Dock isolation helper usable by a sample full editor (Level + Texture via `FinishFullEditorDockspace`)
- [ ] Headless host runs without creating a window/ImGui — **E30** (not Deliverable 1 blocking for E29 GUI proof)
- [x] DESIGN.md + short README "how to add an editor"

Deliverable 2 (E29+Texture parity): Level/Texture on host, document vs workspace undo split, idle/Game.dll hooks, modern CLI via `host::dispatch` with legacy `E29/...` kept for AI — **shipped 2026-09-20**. Remaining polish is optional bootstrap thinning and E30.

---

## 14. Open points to resolve during implementation (not blockers for approving this design)

1. Exact display-name rules for Level vs Texture (asset name vs file stem).
2. **Resolved (2026-09-21):** Level is a peer editor; scenes lock independently via edit/view; Level session grain can remain one session per level asset.
3. Prefer **host-owned** open-instance / write-lock table (works for headless + multi-window).
4. Shim strategy while headers move from `xGPU/source/Tools/Editor` to this repo.
5. Drawer v1 overlay-only vs also shipping "Dock in Layout" in the first drawer milestone.
6. Play conflict UX: fail loud (default) vs auto-stop previous Play.


---

## 15. Ask before coding

Please confirm or amend:

1. This API split (Document vs Session vs Host vs IUI) is acceptable.
2. ResourceName\Command as the primary CLI surface is correct.
3. Phase order A -> B (Texture) -> C (E29) -> **D (Drawer + Level-as-peer)** -> E30/E is OK.
4. Sections 2.2 / 4.5-4.7 (Drawer per window, Level peer, edit/view locks, single Play) match intent.
5. Anything in section 9 that must move **into** the Drawer milestone.

---

*Last design addendum: Host Drawer + peer editors + edit/view + Play - 2026-09-21.*
