# xeditor

Modern editor framework dependency for xGPU / E29.

**Work tree:** `xGPU/dependencies/xeditor` (this folder).

See [DESIGN.md](DESIGN.md).

## Layout

- `include/xeditor/` — public headers (`types`, `session`, `host`, `registry`, dock, console)
- `Src/` — legacy (to be removed when the new library is complete)
- `source/Tools/Editor/xeditor_*.h` — thin shims into `include/xeditor/`

## Status

Deliverable 1 started: core types + host dispatch (`Name\Command` / workspace plane). Texture builds via shims. Next: wire E29/Texture onto `host`, then E30 headless.