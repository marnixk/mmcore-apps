#!/usr/bin/env python3
"""Gameplay mockups only (683×384, 1280×720, 1:1 entity crop). Title/pause UI is out of scope."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from jetpack.graphics import TileAtlas
from jetpack.levels import load_jetlev
from jetpack.render import composite_gameplay_frame, render_entity_showcase_crop


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("game_dir", type=Path, help="jetpak15 install directory")
    ap.add_argument(
        "-o",
        "--out",
        dest="out_dir",
        type=Path,
        default=ROOT / "mockups",
        help="Output directory for PNG mockups",
    )
    ap.add_argument(
        "--level",
        type=int,
        default=15,
        help="Campaign level index (0–99) used in gameplay mockups",
    )
    args = ap.parse_args()
    game_dir = args.game_dir.expanduser().resolve()
    out = args.out_dir.expanduser().resolve()
    out.mkdir(parents=True, exist_ok=True)

    levels = load_jetlev(game_dir / "JETLEV.DAT")
    if not (0 <= args.level < len(levels)):
        print(f"Level index out of range: {args.level}", file=sys.stderr)
        return 1
    lv = levels[args.level]
    label = f"LEVEL {lv.index:02d}"

    atlas = TileAtlas.open(game_dir)
    composite_gameplay_frame(lv, 683, 384, label, atlas=atlas).save(out / "gameplay_683x384.png")
    composite_gameplay_frame(lv, 1280, 720, label, atlas=atlas).save(out / "gameplay_1280x720.png")
    render_entity_showcase_crop(lv, atlas).save(out / "entity_showcase_1x1.png")

    print(f"Mockups written to {out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
