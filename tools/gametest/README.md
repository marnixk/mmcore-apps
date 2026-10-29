# Game test toolkit (headless mmcore)

Runs any game under `games/<name>/` through the native SDL harness: synthetic
keys, serial log, exit status, and PNG screenshots. Games are always copied to a
scratch drive; sources in `games/` are never modified.

## Prerequisites

```bash
bash tools/mmcore/build-mmcore.sh   # once per VM / after MMCORE_REF change
bash .cursor/install.sh             # or Cloud Agent environment install
```

Default binary: `.mmcore/native/mmcore` (with the `001-harness-during-run.patch`
applied — see below).

## Smoke test (five resolutions)

Requires `tools/gametest/profiles/<game>.body.hs`.

```bash
tools/gametest/smoke.sh games/crystalcore
tools/gametest/smoke.sh games/corecaving
```

Modes (from `modes.txt`): MODE 19 (1366×768), MODE 20 (683×384), MODE 16
(1920×1080), MODE 11 (1280×720), MODE 1 (800×600).

Outputs live under `tools/gametest/out/smoke-<game>-<mode>/` (PNG screenshots,
`serial.log`, `check.txt`).

## Single run

```bash
tools/gametest/run.sh myrun games/crystalcore \
  tools/gametest/profiles/crystalcore.body.hs "MODE 11,16"
```

## Harness script language

Verified against `mmcore/native/sdl_harness.c` (one command per host frame while
at the prompt; during `RUN`, our patch also polls the harness from
`sdl_poll_input` so keys and `wait` work inside games).

| Command | Meaning |
|--------|---------|
| `wait MS` | Wall-clock delay (pumps interpreter while waiting) |
| `feed TEXT` | Type into the REPL / program as if typed + Enter |
| `key NAME` | Key press (`enter`, `esc`, `right`, `a`, …; optional `ctrl+` mods) |
| `text STRING` | SDL text-input event |
| `shot FILE.ppm` | Framebuffer capture (converted to PNG after the run) |
| `quit` | End the mmcore session |

Environment variables used by `run.sh`:

| Variable | Purpose |
|----------|---------|
| `SDL_VIDEODRIVER=dummy` | Headless video |
| `SDL_AUDIODRIVER=dummy` | No audio device |
| `MMB_SDL_HARNESS` | Harness script path |
| `MMB_SDL_SERIAL=1` | Mirror serial to stdout |
| `MMB_DRIVE_ROOT` | Host dir whose `C/` is drive C: (high scores) |
| `--drive <dir>` | Host dir mounted as D: (game copy under `g/`) |

`MMB_HARNESS_NONBLOCK_WAIT` appears in older notes but is **not** read by current
mmcore; rely on `wait` in the harness script instead.

## Test hooks (`hook.py`)

`run.sh` calls `hook.py` on the drive copy. Optional per-game logic:
`tools/gametest/hooks/<game>.py` with `apply(game_dir, extra)`. Pass constants via
`TH="TH_START=0 TH_NOBANNER=1"` (writes `includes/testhook.inc` when no custom
hook exists).

## Serial checks

`check_serial.py` fails on `?…` BASIC errors in `serial.log`. `run.sh` records
the result in `out/<run>/check.txt` and exits non-zero on timeout or errors.

## Harness during `RUN` (local patch)

Upstream mmcore only calls `sdl_harness_poll()` from the main SDL loop. While a
BASIC `RUN` is active that loop does not spin, so scripted keys would never run
inside games. `tools/mmcore/patches/001-harness-during-run.patch` adds one
`sdl_harness_poll()` call to `sdl_poll_input()` (already invoked from
`mmb_check_break` on each program line and during `PAUSE`). If upstream merges an
equivalent fix, drop the patch.

`tools/mmcore/patches/002-harness-queue-resume.patch` stops the harness from
discarding script lines past the 256-step queue. `PAUSE` drains many steps per
millisecond, so a profile that has to reach a later frame (tetris title is a
few thousand statements in) depends on that patch. Rebuild mmcore after pulling
it.
