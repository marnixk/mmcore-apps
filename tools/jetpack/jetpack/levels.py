from __future__ import annotations

import json
import struct
from dataclasses import dataclass, asdict
from pathlib import Path
from typing import Any

from .constants import (
    ENEMY_NAMES,
    LEVEL_HEADER_SKIP,
    LEVEL_SIZE,
    MAP_H,
    MAP_W,
    TILE_NAMES,
)


@dataclass
class Enemy:
    type_id: int
    type_name: str
    x: int
    y: int


@dataclass
class Level:
    index: int
    name: str
    tiles: list[int]
    door_x: int
    door_y: int
    player_x: int
    player_y: int
    enemies: list[Enemy]

    def to_dict(self) -> dict[str, Any]:
        d = asdict(self)
        return d


def _jetscii_name(raw: bytes) -> str:
    return raw.decode("ascii", errors="replace").strip()


def parse_level_blob(blob: bytes, index: int) -> Level:
    if len(blob) != LEVEL_SIZE:
        raise ValueError(f"level blob must be {LEVEL_SIZE} bytes, got {len(blob)}")
    tiles = list(blob[0:416])
    door_x, door_y = blob[0x1A0], blob[0x1A1]
    player_x, player_y = blob[0x1A2], blob[0x1A3]
    enemies: list[Enemy] = []
    for i in range(20):
        off = 0x1A4 + i * 3
        eid, ex, ey = blob[off], blob[off + 1], blob[off + 2]
        if eid == 0 or eid > 8:
            continue
        enemies.append(
            Enemy(
                type_id=eid,
                type_name=ENEMY_NAMES.get(eid, f"unknown_{eid}"),
                x=ex,
                y=ey,
            )
        )
    name = _jetscii_name(blob[0x1E0:0x1FA])
    return Level(
        index=index,
        name=name,
        tiles=tiles,
        door_x=door_x,
        door_y=door_y,
        player_x=player_x,
        player_y=player_y,
        enemies=enemies,
    )


def load_jetlev(path: Path) -> list[Level]:
    data = path.read_bytes()
    if len(data) < LEVEL_HEADER_SKIP + LEVEL_SIZE:
        raise ValueError("JETLEV.DAT too small")
    body = data[LEVEL_HEADER_SKIP:]
    n = len(body) // LEVEL_SIZE
    levels = []
    for i in range(n):
        blob = body[i * LEVEL_SIZE : (i + 1) * LEVEL_SIZE]
        levels.append(parse_level_blob(blob, i))
    return levels


def load_jet_file(path: Path, index: int = 0) -> Level:
    return parse_level_blob(path.read_bytes(), index)


def write_level_json(level: Level, path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    tiles_named = [
        {"id": tid, "name": TILE_NAMES.get(tid, f"tile_{tid:02x}")} for tid in level.tiles
    ]
    payload = level.to_dict()
    payload["tiles_named"] = tiles_named
    path.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
