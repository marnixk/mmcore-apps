"""Render playfield using DAT compositor (no capture context atlas)."""

from __future__ import annotations

import json
from dataclasses import dataclass
from pathlib import Path

from PIL import Image

from .levels import Level
from .tile_composite import DatCompositor

_PKG = Path(__file__).resolve().parents[1]
_FRAMES = _PKG / "gfx" / "frames"


@dataclass
class FrameAtlas:
    tiles: dict[int, Image.Image]
    player_frames: list[Image.Image]
    source: str
    compositor: DatCompositor | None = None

    @classmethod
    def load(
        cls,
        frames_dir: Path | None = None,
        game_dir: Path | None = None,
        native_dir: Path | None = None,
        levels: list[Level] | None = None,
        exclude_level: int | None = None,
        anim_phase: int = 0,
    ) -> FrameAtlas:
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
        compositor = None
        if game_dir and native_dir and levels:
            compositor = DatCompositor.open(
                game_dir, native_dir, levels, exclude_level=exclude_level, anim_phase=anim_phase
            )
        return cls(
            tiles=tiles,
            player_frames=player,
            source=meta.get("source", "unknown"),
            compositor=compositor,
        )

    def tile_image(self, tile_id: int) -> Image.Image | None:
        return self.tiles.get(tile_id)

    def render_level(self, tile_ids: list[int], **kwargs) -> Image.Image:
        if self.compositor is not None:
            return self.compositor.render_level(tile_ids).convert("RGBA")
        from .atlas_median import TileAtlasStats, render_level_from_atlas
        import numpy as np

        tiles_np = {tid: np.array(im.convert("RGB")) for tid, im in self.tiles.items()}
        stats = TileAtlasStats(tiles=tiles_np, counts={}, disagreement={})
        return render_level_from_atlas(tile_ids, stats).convert("RGBA")
