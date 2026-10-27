"""Render using RGBA frames exported under gfx/frames/ (DOS capture or DAT)."""

from __future__ import annotations

import json
from dataclasses import dataclass
from pathlib import Path

from PIL import Image

from .constants import MAP_H, MAP_W, TILE_PX

_PKG = Path(__file__).resolve().parents[1]
_FRAMES = _PKG / "gfx" / "frames"
_INDEX = _FRAMES / "index.json"


@dataclass
class FrameAtlas:
    tiles: dict[int, Image.Image]
    player_frames: list[Image.Image]
    source: str
    level0_cells: dict[tuple[int, int], Image.Image] | None = None

    @classmethod
    def load(cls, frames_dir: Path | None = None) -> FrameAtlas:
        base = frames_dir or _FRAMES
        idx_path = base / "index.json"
        if not idx_path.is_file():
            raise FileNotFoundError(f"missing {idx_path}")
        meta = json.loads(idx_path.read_text(encoding="utf-8"))
        tiles: dict[int, Image.Image] = {}
        for entry in meta.get("tiles", []):
            tid = int(entry["tile_id"])
            path = base / entry["file"]
            if path.is_file():
                tiles[tid] = Image.open(path).convert("RGBA")
        player: list[Image.Image] = []
        for entry in meta.get("player", []):
            path = base / entry["file"]
            if path.is_file():
                player.append(Image.open(path).convert("RGBA"))
        level0_cells: dict[tuple[int, int], Image.Image] | None = None
        cells_dir = base / "level00_cells"
        if cells_dir.is_dir():
            level0_cells = {}
            for p in cells_dir.glob("cell_*_*.png"):
                parts = p.stem.split("_")
                if len(parts) >= 3:
                    c, r = int(parts[-2]), int(parts[-1])
                    level0_cells[(c, r)] = Image.open(p).convert("RGBA")
        return cls(
            tiles=tiles,
            player_frames=player,
            source=meta.get("source", "unknown"),
            level0_cells=level0_cells,
        )

    def tile_image(self, tile_id: int) -> Image.Image | None:
        return self.tiles.get(tile_id)

    def render_level(
        self,
        tile_ids: list[int],
        background_tile_id: int = 0x3C,
        use_level0_cells: bool = False,
    ) -> Image.Image:
        w, h = MAP_W * TILE_PX, MAP_H * TILE_PX
        _ = use_level0_cells  # deprecated: minimap cells are not 1:1 tiles
        base = self.tiles.get(background_tile_id)
        if base is None:
            frame = Image.new("RGBA", (w, h), (0x28, 0x20, 0x38, 255))
        else:
            frame = Image.new("RGBA", (w, h))
            for y in range(MAP_H):
                for x in range(MAP_W):
                    frame.paste(base, (x * TILE_PX, y * TILE_PX), base)
        for row in range(MAP_H):
            for col in range(MAP_W):
                tid = tile_ids[row * MAP_W + col]
                if tid == 0:
                    continue
                spr = self.tiles.get(tid)
                if spr is None and tid == 60:
                    spr = self.tiles.get(0x3C)
                if spr is None:
                    continue
                frame.paste(spr, (col * TILE_PX, row * TILE_PX), spr)
        return frame
