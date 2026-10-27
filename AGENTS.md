# Cloud agents: mmcore BASIC games in this repo

This repository holds **mmcore** games under `games/<name>/`. Gameplay, art, and
fixes are done here; the interpreter lives in [marnixk/mmcore](https://github.com/marnixk/mmcore)
and is built beside this repo (not edited from here).

## Environment

- Cloud Agent config: `.cursor/environment.json` → `.cursor/install.sh`
- Rebuild interpreter (after `tools/mmcore/MMCORE_REF` change or mmcore fixes):
  `bash tools/mmcore/build-mmcore.sh`
- Validate a game: `tools/gametest/smoke.sh games/<name>` (needs a smoke profile in
  `tools/gametest/profiles/<name>.body.hs`)

## Game build standards

Follow the house rules (input, drawing, resolutions, high scores, silence, etc.).
A copy is in the repo task docs; summary:

1. **Art first** — mockups approved before code.
2. **Structure** — thin `main.bas`, `includes/*.inc`, `gfx/` (and `sfx/` if needed);
   high scores on `C:/.<game>`.
3. **Input** — menus use `INKEY$` loops; in-game uses `ON KEY` + `SETTICK` tick
   (see Tronix / standards doc). Pause via Esc/P; quit from pause with Q.
4. **Drawing** — back page, dirty rects, sprites; full screen at all five targets;
   no live score; silent by default.
5. **Do not change gameplay** unless the user asks.

## Agent workflow

1. Confirm art/mockups and controls (for new games or reworks).
2. Implement in `games/<name>/` only.
3. Run `tools/gametest/smoke.sh games/<name>` on all five resolutions.
4. Attach **title** and **gameplay** PNGs from `tools/gametest/out/` to the PR as
   artifacts; include the smoke pass/fail summary in the PR description.
5. Do **not** patch mmcore in this repo beyond the pinned fetch + documented patch
   in `tools/mmcore/patches/`. If you hit an interpreter bug, describe it with a
   minimal repro in the PR and file mmcore separately.

## Interpreter quirks (work around in game code)

| Issue | Guidance |
|-------|----------|
| `EXIT SUB` | Must be on its own line |
| Name `quit` | Never use as a variable name |
| Parameter vs `CONST` | Parameter names must not match `CONST` names |
| `DO` mid-line | Restarts the line — avoid |
| `<<` | Not available |
| `LOCAL` / `DIM` in hot loops | Slow on Pi — prefer module-level `dim` |

## Testing notes

- Headless harness: `tools/gametest/README.md`
- Test hooks inject into the **drive copy** via `hook.py`, never into published
  `games/` sources.
- Serial log must be free of `?…` BASIC errors (`check_serial.py`).
