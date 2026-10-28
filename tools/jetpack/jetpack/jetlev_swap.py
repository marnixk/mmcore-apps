"""Build JETLEV.DAT with a given campaign level copied into slot 0."""

from __future__ import annotations

import shutil
from pathlib import Path

from .constants import LEVEL_HEADER_SKIP, LEVEL_SIZE


def write_slot0_level(src_jetlev: Path, dest_jetlev: Path, level_index: int) -> None:
    data = bytearray(src_jetlev.read_bytes())
    if len(data) < LEVEL_HEADER_SKIP + (level_index + 1) * LEVEL_SIZE:
        raise ValueError(f"JETLEV too small for level {level_index}")
    rec_off = LEVEL_HEADER_SKIP + level_index * LEVEL_SIZE
    slot_off = LEVEL_HEADER_SKIP
    data[slot_off : slot_off + LEVEL_SIZE] = data[rec_off : rec_off + LEVEL_SIZE]
    dest_jetlev.parent.mkdir(parents=True, exist_ok=True)
    dest_jetlev.write_bytes(data)


def prepare_runtime_game_dir(src_dir: Path, runtime_dir: Path) -> Path:
    """Copy jetpak15 tree once; caller overwrites JETLEV.DAT per level."""
    if not runtime_dir.is_dir():
        shutil.copytree(src_dir, runtime_dir)
    return runtime_dir
