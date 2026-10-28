"""Render using composite atlas from native captures."""

from __future__ import annotations

import json
import pickle
from dataclasses import dataclass
from pathlib import Path

from PIL import Image

_PKG = Path(__file__).resolve().parents[1]
_FRAMES = _PKG / "gfx" / "frames"
_COMPOSITE_PKL = _PKG / "reference" / "composite_atlas.pkl"


@dataclass
class FrameAtlas:
    tiles: dict[int, Image.Image]
    player_frames: list[Image.Image]
    source: str
    composite: object | None = None

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
        composite = None
        if _COMPOSITE_PKL.is_file():
            with _COMPOSITE_PKL.open("rb") as f:
                composite = pickle.load(f)
        return cls(
            tiles=tiles,
            player_frames=player,
            source=meta.get("source", "unknown"),
            composite=composite,
        )

    def tile_image(self, tile_id: int) -> Image.Image | None:
        return self.tiles.get(tile_id)

    def render_level(self, tile_ids: list[int], **kwargs) -> Image.Image:
        if self.composite is not None:
            return self.composite.render_level(tile_ids).convert("RGBA")
        from .tile_composite import CompositeAtlas
        import numpy as np
        from .atlas_median import TileAtlasStats, render_level_from_atlas

        tiles_np = {tid: np.array(im.convert("RGB")) for tid, im in self.tiles.items()}
        stats = TileAtlasStats(tiles=tiles_np, counts={}, disagreement={})
        return render_level_from_atlas(tile_ids, stats).convert("RGBA")
