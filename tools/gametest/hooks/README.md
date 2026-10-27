# Per-game test hooks

`run.sh` copies the game into a scratch drive folder, then runs `hook.py` on
that copy. Add `hooks/<folder-name>.py` with an `apply(game_dir, extra)` function
to inject `includes/testhook.inc`, logging, autopilot moves, shorter banners, etc.

Circuit Quest-style recipes live in other repos; copy the pattern from the
mmcore-apps task docs or from historical `example-hook.py` samples. Never edit
files under `games/<name>/` in this repository for test-only behaviour.
