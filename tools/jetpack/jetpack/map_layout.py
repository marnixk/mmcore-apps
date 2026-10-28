"""Empirical JETLEV 416-byte map layout (row/col major, offset, flips)."""

from __future__ import annotations

import json
from dataclasses import dataclass
from enum import Enum
from pathlib import Path
from typing import Iterable

import numpy as np

from .constants import MAP_H, MAP_W, TILE_PX

MAP_BYTES = MAP_W * MAP_H
_LAYOUT_PATH = Path(__file__).resolve().parents[1] / "reference" / "map_layout.json"


class Order(str, Enum):
    ROW_MAJOR = "row_major"
    COL_MAJOR = "col_major"


@dataclass(frozen=True)
class MapLayout:
    """How to read 416 tile bytes from a level record and index (row, col)."""

    name: str
    byte_offset: int
    order: Order
    flip_col: bool = False
    flip_row: bool = False
    transpose: bool = False

    def tile_id_at(self, blob: bytes, row: int, col: int) -> int:
        r, c = row, col
        if self.flip_row:
            r = MAP_H - 1 - r
        if self.flip_col:
            c = MAP_W - 1 - c
        if self.transpose:
            r, c = c, r
            if self.order == Order.COL_MAJOR:
                idx = r * MAP_H + c
            else:
                idx = r * MAP_W + c
        elif self.order == Order.ROW_MAJOR:
            idx = r * MAP_W + c
        else:
            idx = c * MAP_H + r
        pos = self.byte_offset + idx
        if pos < 0 or pos >= len(blob):
            return 0
        return blob[pos]

    def read_grid(self, blob: bytes) -> list[int]:
        return [self.tile_id_at(blob, row, col) for row in range(MAP_H) for col in range(MAP_W)]

    def to_dict(self) -> dict:
        return {
            "name": self.name,
            "byte_offset": self.byte_offset,
            "order": self.order.value,
            "flip_col": self.flip_col,
            "flip_row": self.flip_row,
            "transpose": self.transpose,
        }

    @staticmethod
    def from_dict(data: dict) -> MapLayout:
        return MapLayout(
            name=data["name"],
            byte_offset=int(data["byte_offset"]),
            order=Order(data["order"]),
            flip_col=bool(data.get("flip_col", False)),
            flip_row=bool(data.get("flip_row", False)),
            transpose=bool(data.get("transpose", False)),
        )


def default_layout() -> MapLayout:
    """Locked layout: 416-byte map at record offset 0, row-major (26×16)."""
    if _LAYOUT_PATH.is_file():
        return MapLayout.from_dict(json.loads(_LAYOUT_PATH.read_text(encoding="utf-8")))
    return MapLayout("off00|row_major", 0, Order.ROW_MAJOR)


def save_default_layout(layout: MapLayout) -> None:
    _LAYOUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    _LAYOUT_PATH.write_text(json.dumps(layout.to_dict(), indent=2) + "\n", encoding="utf-8")


def iter_layout_candidates(max_offset: int = 32) -> list[MapLayout]:
    out: list[MapLayout] = []
    for off in range(max_offset + 1):
        for order in Order:
            for flip_col in (False, True):
                for flip_row in (False, True):
                    for transpose in (False, True):
                        parts = [f"off{off:02d}", order.value]
                        if transpose:
                            parts.append("T")
                        if flip_col:
                            parts.append("fc")
                        if flip_row:
                            parts.append("fr")
                        out.append(
                            MapLayout(
                                name="|".join(parts),
                                byte_offset=off,
                                order=order,
                                flip_col=flip_col,
                                flip_row=flip_row,
                                transpose=transpose,
                            )
                        )
    return out


def load_level_record(data: bytes, index: int, header_skip: int = 2) -> bytes:
    from .constants import LEVEL_SIZE

    body = data[header_skip:]
    return body[index * LEVEL_SIZE : (index + 1) * LEVEL_SIZE]


def context_key(tiles: list[int], row: int, col: int, tid: int) -> tuple:
    """8-neighbour tile IDs (N,E,S,W + diagonals); -1 outside playfield."""
    offs = [(-1, 0), (0, 1), (1, 0), (0, -1), (-1, -1), (-1, 1), (1, -1), (1, 1)]
    n = []
    for dr, dc in offs:
        r, c = row + dr, col + dc
        n.append(tiles[r * MAP_W + c] if 0 <= r < MAP_H and 0 <= c < MAP_W else -1)
    return (tid, *n)


def within_id_variance_score(
    layout: MapLayout,
    level_bytes: bytes,
    field: np.ndarray,
    skip_ids: frozenset[int] | None = None,
) -> tuple[float, int, int]:
    """Mean per-pixel variance within each tile ID (lower = better)."""
    skip = skip_ids or frozenset()
    buckets: dict[int, list[np.ndarray]] = {}
    for row in range(MAP_H):
        for col in range(MAP_W):
            tid = layout.tile_id_at(level_bytes, row, col)
            if tid in skip:
                continue
            y0, x0 = row * TILE_PX, col * TILE_PX
            buckets.setdefault(tid, []).append(field[y0 : y0 + TILE_PX, x0 : x0 + TILE_PX])
    per_id: list[float] = []
    for tid, samples in buckets.items():
        if len(samples) < 2:
            continue
        stack = np.stack(samples, axis=0).astype(np.float32)
        per_id.append(float(np.mean(np.var(stack, axis=0))))
    if not per_id:
        return float("inf"), len(buckets), sum(len(v) for v in buckets.values())
    return float(np.mean(per_id)), len(per_id), sum(len(v) for v in buckets.values())
