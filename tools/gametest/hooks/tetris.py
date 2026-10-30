"""Optional Tetris drive-copy hook. TH_LOG=1 records each control action.

The published game is not modified. With TH_LOG=1 the copy gets a fixed
piece sequence (T, I, L, O, S, Z, J) and appends one line per action to
th.log: TAG,kind,rot,x,y,hold,score
"""
from __future__ import annotations

import os


def apply(game_dir: str, extra: str) -> None:
    flags = {}
    for kv in extra.split():
        if not kv.strip():
            continue
        key, val = kv.split("=", 1)
        flags[key] = val
    if flags.get("TH_LOG") != "1":
        return
    _patch(os.path.join(game_dir, "includes", "game.inc"), _GAME)
    _patch(os.path.join(game_dir, "includes", "pieces.inc"), _PIECES)


def _patch(path: str, pairs: list[tuple[str, str]]) -> None:
    with open(path, encoding="utf-8") as fh:
        text = fh.read()
    for old, new in pairs:
        if old not in text:
            raise SystemExit(f"tetris hook: pattern not found in {path}: {old!r}")
        text = text.replace(old, new, 1)
    with open(path, "w", encoding="utf-8") as fh:
        fh.write(text)


_TRACE_SUB = """
' testhook: one line per action (drive copy only)
sub gm.trace(tag$)
  open "th.log" for append as #2
  print #2, tag$ + "," + str$(p.k) + "," + str$(p.r) + "," + str$(p.x) + "," + str$(p.y) + "," + str$(gm.hold) + "," + str$(gm.score)
  close #2
end sub
"""

_GAME = [
    (
        "  key.reset()\n  gm.sd = FALSE\n",
        "  key.reset()\n  gm.trace(\"BEGIN\")\n  gm.sd = FALSE\n",
    ),
    (
        "        if key.once(k, K_UP) then gm.rotate(0)\n",
        "        if key.once(k, K_UP) then\n"
        "          gm.rotate(0)\n"
        "          gm.trace(\"UP\")\n"
        "        end if\n",
    ),
    (
        "        if key.once(k, K_SPACE) then gm.hardDrop()\n",
        "        if key.once(k, K_SPACE) then\n"
        "          gm.hardDrop()\n"
        "          gm.trace(\"DROP\")\n"
        "        end if\n",
    ),
    (
        "        if key.once(k, asc(\"x\")) then gm.rotate(0)\n",
        "        if key.once(k, asc(\"x\")) then\n"
        "          gm.rotate(0)\n"
        "          gm.trace(\"X\")\n"
        "        end if\n",
    ),
    (
        "        if key.once(k, asc(\"z\")) then gm.rotate(1)\n",
        "        if key.once(k, asc(\"z\")) then\n"
        "          gm.rotate(1)\n"
        "          gm.trace(\"Z\")\n"
        "        end if\n",
    ),
    (
        "        if key.once(k, asc(\"q\")) then gm.doHold()\n",
        "        if key.once(k, asc(\"q\")) then\n"
        "          gm.doHold()\n"
        "          gm.trace(\"HOLD\")\n"
        "        end if\n",
    ),
    (
        "        gm.quit = dlg.pause()\n",
        "        gm.trace(\"PAUSE\")\n        gm.quit = dlg.pause()\n",
    ),
    (
        "    dlg.scores(lv, rk)\n  end if\nend sub\n",
        "    dlg.scores(lv, rk)\n  end if\nend sub\n" + _TRACE_SUB,
    ),
]

_PIECES = [
    (
        """function pc.next()
  local i, j, t
  if pc.bagN = 0 then
    for i = 0 to 6
      pc.bag(i) = i
    next i
    for i = 6 to 1 step -1
      j = int(rnd * (i + 1))
      t = pc.bag(i)
      pc.bag(i) = pc.bag(j)
      pc.bag(j) = t
    next i
    pc.bagN = 7
  end if
  pc.bagN = pc.bagN - 1
  pc.next = pc.bag(pc.bagN)
end function
""",
        """' testhook sequence T I L O S Z J, repeating
function pc.next()
  if pc.bag(0) = 0 and pc.bag(1) = 0 then
    pc.bag(0) = 2
    pc.bag(1) = 0
    pc.bag(2) = 6
    pc.bag(3) = 1
    pc.bag(4) = 3
    pc.bag(5) = 4
    pc.bag(6) = 5
    pc.bagN = 0
  end if
  pc.next = pc.bag(pc.bagN)
  pc.bagN = pc.bagN + 1
  if pc.bagN > 6 then pc.bagN = 0
end function
""",
    ),
]
