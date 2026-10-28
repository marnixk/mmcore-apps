#!/usr/bin/env python3
"""Extract levels, level previews, and sprite sheet from a Jetpack install directory."""

from __future__ import annotations

import argparse
import hashlib
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from jetpack.frame_atlas import FrameAtlas
from jetpack.graphics import TileAtlas, export_sprite_sheet, load_vga_palette
from jetpack.levels import load_jetlev, write_level_json
from jetpack.render import render_level_preview


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument(
        "game_dir",
        type=Path,
        help="Path to jetpak15 folder containing JETLEV.DAT and JETPACK*.DAT",
    )
    ap.add_argument(
        "-o",
        "--out",
        type=Path,
        default=ROOT,
        help="Base output directory (writes levels/, previews/, gfx/ beneath it)",
    )
    args = ap.parse_args()
    game_dir = args.game_dir.expanduser().resolve()
    out = args.out.expanduser().resolve()
    jetlev = game_dir / "JETLEV.DAT"
    if not jetlev.is_file():
        print(f"Missing {jetlev}", file=sys.stderr)
        return 1

    levels = load_jetlev(jetlev)
    json_dir = out / "levels"
    prev_dir = out / "previews"
    json_dir.mkdir(parents=True, exist_ok=True)
    prev_dir.mkdir(parents=True, exist_ok=True)

    frames_idx = out / "gfx" / "frames" / "index.json"
    if frames_idx.is_file():
        frame_atlas = FrameAtlas.load(out / "gfx" / "frames")
    else:
        frame_atlas = None
    atlas = TileAtlas.open(game_dir)
    for lv in levels:
        write_level_json(lv, json_dir / f"level_{lv.index:02d}.json")
        if frame_atlas is not None:
            img = frame_atlas.render_level(lv.tiles).convert("RGB")
        else:
            img = render_level_preview(lv, scale=1, show_entities=False, atlas=atlas)
        img.save(prev_dir / f"level_{lv.index:02d}.png")

    print(f"Wrote {len(levels)} levels to {json_dir} and {prev_dir}")

    pal_path = game_dir / "JETPAL.DAT"
    if pal_path.is_file():
        vga = load_vga_palette(pal_path)
        gfx_dir = out / "gfx"
        info = export_sprite_sheet(
            game_dir,
            gfx_dir / "sprites_vga.png",
            gfx_dir / "sprites_index.json",
            vga,
        )
        print(f"Sprite sheet: {info['slot_count']} slots -> {gfx_dir}")
    manifest = out / "SOURCE_MANIFEST.txt"
    lines = ["# Hashes of local Jetpack freeware files (not committed to git)\n"]
    for name in sorted(
        [
            "JETLEV.DAT",
            "JETPAL.DAT",
            "JETPACK.EXE",
            "JETPACK0.DAT",
            "JETPACK1.DAT",
            "JETPACK2.DAT",
            "JETPACK3.DAT",
            "JETPACK4.DAT",
            "JETPACK5.DAT",
            "README.TXT",
            "MANUAL.TXT",
        ]
    ):
        p = game_dir / name
        if p.is_file():
            lines.append(f"{name}\tsha256={sha256(p)}\tsize={p.stat().st_size}\n")
    manifest.write_text("".join(lines), encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
