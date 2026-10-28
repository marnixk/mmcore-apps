#!/usr/bin/env python3
"""Bijective Hungarian assign captures to JETLEV levels; rename files."""

from __future__ import annotations

import json
import shutil
import sys
from pathlib import Path

import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from jetpack.level_fingerprint import capture_fingerprint, fingerprint_distance, jetlev_fingerprint
from jetpack.levels import load_jetlev
from jetpack.native_vga import load_geometry, playfield_from_native


def main() -> int:
    import argparse

    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--native", type=Path, default=ROOT / "reference" / "native")
    ap.add_argument("--rename", action="store_true")
    args = ap.parse_args()
    native = args.native.expanduser().resolve()
    geom = load_geometry()
    levels = load_jetlev(ROOT / "_download/extracted/jetpak15/JETLEV.DAT")
    jet_fps = [jetlev_fingerprint(lv.tiles) for lv in levels]

    labels: list[int] = []
    cap_fps: list[np.ndarray] = []
    for p in sorted(native.glob("level_*_f0.png")):
        label = int(p.stem.split("_")[1])
        field = np.array(playfield_from_native(Image.open(p).convert("RGB"), geom))
        labels.append(label)
        cap_fps.append(capture_fingerprint(field))

    n = len(labels)
    cost = np.zeros((n, n), dtype=np.float64)
    for i, cfp in enumerate(cap_fps):
        for j, jfp in enumerate(jet_fps):
            cost[i, j] = fingerprint_distance(cfp, jfp)

    # Greedy minimum-cost matching (fallback without scipy).
    mapping: dict[int, int] = {}
    used_cols: set[int] = set()
    pairs = [(cost[i, j], i, j) for i in range(n) for j in range(n)]
    pairs.sort(key=lambda x: x[0])
    for _, i, j in pairs:
        lab = labels[i]
        if lab in mapping or j in used_cols:
            continue
        mapping[lab] = j
        used_cols.add(j)
    for lab in labels:
        if lab not in mapping:
            mapping[lab] = lab
    mis = int(sum(1 for lab, true in mapping.items() if lab != true))

    entries = [
        {
            "file": f"level_{lab:02d}_f0.png",
            "label": lab,
            "assigned_level": int(mapping[lab]),
            "mislabelled": lab != mapping[lab],
        }
        for lab in sorted(mapping)
    ]
    out = ROOT / "reference" / "capture_assignment.json"
    out.write_text(
        json.dumps({"mislabelled_count": mis, "total": n, "entries": entries}, indent=2) + "\n",
        encoding="utf-8",
    )
    print(f"Assignment mislabels vs filename: {mis}/{n}")

    if args.rename:
        staging = native / "_assigned"
        staging.mkdir(exist_ok=True)
        backup = native / "_backup_by_filename"
        backup.mkdir(exist_ok=True)
        for lab in labels:
            target = mapping[lab]
            for fn in range(3):
                src = native / f"level_{lab:02d}_f{fn}.png"
                if not src.is_file():
                    continue
                shutil.copy2(src, backup / src.name)
                dst = staging / f"level_{target:02d}_f{fn}.png"
                shutil.copy2(src, dst)
        print(f"Wrote assigned captures to {staging} (originals in {backup})")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
