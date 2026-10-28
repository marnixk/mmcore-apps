#!/usr/bin/env python3
"""Identify true campaign level for each native f0 capture (render-match)."""

from __future__ import annotations

import json
import shutil
import sys
from pathlib import Path

import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from jetpack.atlas_median import masked_mse, temporal_static_mask
from jetpack.levels import Level, load_jetlev
from jetpack.native_vga import load_geometry, playfield_from_native
from jetpack.tile_composite import DatCompositor


def load_capture_pairs(native: Path) -> list[tuple[int, Level, list[np.ndarray]]]:
    levels = {lv.index: lv for lv in load_jetlev(ROOT / "_download/extracted/jetpak15/JETLEV.DAT")}
    geom = load_geometry()
    out: list[tuple[int, Level, list[np.ndarray]]] = []
    for p in sorted(native.glob("level_*_f*.png")):
        parts = p.stem.split("_")
        label = int(parts[1])
        fi = int(parts[2][1:])
        if label not in levels:
            continue
        if fi == 0:
            fields = []
            for fn in range(3):
                fp = native / f"level_{label:02d}_f{fn}.png"
                if fp.is_file():
                    fields.append(np.array(playfield_from_native(Image.open(fp).convert("RGB"), geom)))
            out.append((label, levels[label], fields))
    return out


def main() -> int:
    import argparse

    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--native", type=Path, default=ROOT / "reference" / "native")
    ap.add_argument("--rename", action="store_true", help="Rename files to matched level (backup first)")
    args = ap.parse_args()
    native = args.native.expanduser().resolve()
    all_levels = load_jetlev(ROOT / "_download/extracted/jetpak15/JETLEV.DAT")

    game = ROOT / "_download/extracted/jetpak15"
    comp = DatCompositor.open(game, native, all_levels, exclude_level=None)

    results = []
    mis = 0
    geom = load_geometry()
    for label, _lv, fields in load_capture_pairs(native):
        if not fields:
            continue
        cap = fields[0]
        best_i = 0
        best_mse = float("inf")
        for lv in all_levels:
            rend = np.array(comp.render_level(lv.tiles))
            # Compare on non-backdrop cells only, no sprite mask yet
            mask = np.ones((16, 26), dtype=bool)
            for row in range(16):
                for col in range(26):
                    tid = lv.tiles[row * 26 + col]
                    if tid in (0, 60, 0x3C):
                        mask[row, col] = False
            mse = masked_mse(cap, rend, mask)
            if mse < best_mse:
                best_mse = mse
                best_i = lv.index
        ok = best_i == label
        if not ok:
            mis += 1
        results.append(
            {
                "file": f"level_{label:02d}_f0.png",
                "label": label,
                "matched_level": best_i,
                "render_mse": best_mse,
                "mislabelled": not ok,
            }
        )

    audit = {
        "mislabelled_count": mis,
        "total": len(results),
        "method": "dat_compositor_render_mse",
        "entries": results,
    }
    out_path = ROOT / "reference" / "capture_label_audit.json"
    out_path.write_text(json.dumps(audit, indent=2) + "\n", encoding="utf-8")
    print(f"Mislabelled: {mis} / {len(results)} (see {out_path})")

    if args.rename and mis:
        backup = native / "_backup_by_filename"
        backup.mkdir(exist_ok=True)
        # Build mapping label -> matched_level (must be injective for rename)
        mapping = {r["label"]: r["matched_level"] for r in results}
        used: set[int] = set()
        for label, target in sorted(mapping.items()):
            if target in used:
                print(f"Skip rename {label}->{target}: collision", file=sys.stderr)
                continue
            used.add(target)
            for fn in range(3):
                src = native / f"level_{label:02d}_f{fn}.png"
                if not src.is_file():
                    continue
                shutil.copy2(src, backup / src.name)
                dst = native / f"level_{target:02d}_f{fn}.png"
                if label != target and dst.exists() and dst != src:
                    print(f"Collision target {dst}", file=sys.stderr)
                else:
                    src.rename(dst if label != target else src)
        print(f"Backed up originals under {backup}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
