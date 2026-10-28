#!/usr/bin/env python3
"""Export decoded tiles/sprites to gfx/frames/ (see jetpack/frames_export.py)."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from jetpack.frames_export import export_frame_pngs
from jetpack.graphics import TileAtlas, export_sprite_sheet, load_vga_palette


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("game_dir", type=Path, help="jetpak15 install directory")
    ap.add_argument(
        "-o",
        "--out",
        type=Path,
        default=ROOT / "gfx" / "frames",
        help="Output directory for PNG frames",
    )
    args = ap.parse_args()
    game_dir = args.game_dir.expanduser().resolve()
    gfx = ROOT / "gfx"
    sheet = gfx / "sprites_vga.png"
    index = gfx / "sprites_index.json"
    if not sheet.is_file():
        export_sprite_sheet(
            game_dir,
            sheet,
            index,
            load_vga_palette(game_dir / "JETPAL.DAT"),
        )

    atlas = TileAtlas.open(game_dir)
    info = export_frame_pngs(atlas, args.out.expanduser().resolve(), index)
    print(f"Exported {info['frame_count']} frames -> {args.out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
