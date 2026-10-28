"""Render using RGBA frames exported under gfx/frames/ (DOS capture or DAT)."""

from __future__ import annotations

import json
import pickle
from dataclasses import dataclass
from pathlib import Path

from PIL import Image

from .constants import MAP_H, MAP_W, SPAWN_MARKER_TILE_IDS, TILE_PX

_PKG = Path(__file__).resolve().parents[1]
_FRAMES = _PKG / "gfx" / "frames"
_INDEX = _FRAMES / "index.json"
_CONTEXT_PKL = _PKG / "reference" / "context_atlas.pkl"


@dataclass
class FrameAtlas:
    tiles: dict[int, Image.Image]
    player_frames: list[Image.Image]
    source: str
    context_stats: object | None = None

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
        ctx = None
        pkl = _CONTEXT_PKL
        if pkl.is_file():
            with pkl.open("rb") as f:
                ctx = pickle.load(f)
        return cls(
            tiles=tiles,
            player_frames=player,
            source=meta.get("source", "unknown"),
            context_stats=ctx,
        )

    def tile_image(self, tile_id: int) -> Image.Image | None:
        return self.tiles.get(tile_id)

    def render_level(
        self,
        tile_ids: list[int],
        background_tile_id: int = 0x3C,
        use_level0_cells: bool = False,
    ) -> Image.Image:
        _ = (background_tile_id, use_level0_cells)
        if self.context_stats is not None:
            from .atlas_median import render_level_from_context_atlas

            rgb = render_level_from_context_atlas(tile_ids, self.context_stats)
            return rgb.convert("RGBA")
        from .atlas_median import TileAtlasStats, render_level_from_atlas
        import numpy as np

        tiles_np: dict[int, np.ndarray] = {}
        for tid, im in self.tiles.items():
            tiles_np[tid] = np.array(im.convert("RGB"))
        stats = TileAtlasStats(tiles=tiles_np, counts={}, disagreement={})
        rgb = render_level_from_atlas(tile_ids, stats)
        return rgb.convert("RGBA")
