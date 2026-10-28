"""DAT-based terrain compositor (JETPACK.EXE 0x4730 / 0x48AD rules)."""

from __future__ import annotations

import json
from dataclasses import dataclass
from pathlib import Path

import numpy as np
from PIL import Image

from .atlas_median import ANIMATED_TILE_IDS, temporal_static_mask
from .constants import MAP_H, MAP_W, SPAWN_MARKER_TILE_IDS, TILE_PX

GEM_TILE_IDS = frozenset({4, 5})
from .graphics import DAT_SLOT_COUNTS, TileAtlas
from .levels import Level
from .palette_fit import fit_tile_palettes, fit_vga_palette, slot_to_rgb

PLAYFIELD_W = MAP_W * TILE_PX
PLAYFIELD_H = MAP_H * TILE_PX

BACKDROP_RENDER_IDS = frozenset({0, 60, 0x3C}) | SPAWN_MARKER_TILE_IDS
BLANK_SENTINEL = 0xB000  # word [0x311F] playfield blank sentinel (not a map byte)

# Tube / conveyor tiles (0x4A–0x4F): extra DAT slots after the 120 campaign ids.
TUBE_ANIM_SLOTS: dict[int, tuple[int, ...]] = {
    0x4A: (142, 143, 144),
    0x4B: (143, 144, 145),
    0x4C: (145, 146, 147),
    0x4D: (146, 147, 148),
    0x4E: (148, 149, 150),
    0x4F: (149, 150, 151),
}

_EDGE_JSON = Path(__file__).resolve().parent / "edge_slots.json"

_DIR_OFFSETS = {
    "N": (-1, 0),
    "E": (0, 1),
    "S": (1, 0),
    "W": (0, -1),
}

# Border bands composited when neighbour tile id differs (0x48AD / 0x4730 strip pass).
_EDGE_BANDS: dict[str, tuple[int, int, int, int]] = {
    "N": (0, 0, TILE_PX, 8),
    "S": (0, TILE_PX - 8, TILE_PX, TILE_PX),
    "W": (0, 0, 8, TILE_PX),
    "E": (TILE_PX - 8, 0, TILE_PX, TILE_PX),
}


def _norm_tid(tid: int) -> int:
    return 0x3C if tid == 0 else tid


def _neighbor_tid(tiles: list[int], row: int, col: int, dr: int, dc: int) -> int:
    r, c = row + dr, col + dc
    if 0 <= r < MAP_H and 0 <= c < MAP_W:
        return tiles[r * MAP_W + c]
    return -1


def _needs_edge(center: int, neighbor: int) -> bool:
    if neighbor < 0:
        return False
    if neighbor == center:
        return False
    if neighbor == 0:
        return False
    return True


def edge_dat_slot(center_tid: int, direction: str) -> int | None:
    """SCLM descriptor CL=0x20 uses centre tile id (CH); edge art at 120+tid."""
    key = f"{center_tid}:{direction}"
    if _EDGE_JSON.is_file():
        table = json.loads(_EDGE_JSON.read_text(encoding="utf-8"))
        if key in table:
            return int(table[key])
    slot = 120 + center_tid
    total = sum(DAT_SLOT_COUNTS)
    if slot >= total:
        return None
    return slot


def _apply_edge_band(cell: np.ndarray, overlay_rgba: np.ndarray, rect: tuple[int, int, int, int]) -> None:
    x0, y0, x1, y1 = rect
    sub = cell[y0:y1, x0:x1]
    cell[y0:y1, x0:x1] = _composite_rgba(sub, overlay_rgba[y0:y1, x0:x1])


def _composite_rgba(base_rgb: np.ndarray, overlay: np.ndarray) -> np.ndarray:
    out = base_rgb.astype(np.float32)
    if overlay.shape[-1] == 4:
        rgb = overlay[:, :, :3].astype(np.float32)
        alpha = overlay[:, :, 3:4].astype(np.float32) / 255.0
        out = out * (1 - alpha) + rgb * alpha
    else:
        mask = np.any(overlay != 0, axis=2)
        out[mask] = overlay[mask].astype(np.float32)
    return np.clip(out, 0, 255).astype(np.uint8)


def _tile_rgba(
    atlas: TileAtlas,
    palette: list[tuple[int, int, int]] | dict[int, tuple[int, int, int]],
    slot: int,
) -> np.ndarray | None:
    px = atlas.tile_pixels(slot)
    if not px:
        return None
    rgb = slot_to_rgb(px, palette)
    alpha = np.zeros((TILE_PX, TILE_PX, 1), dtype=np.uint8)
    for i, idx in enumerate(px):
        if idx != 0:
            x, y = i % TILE_PX, i // TILE_PX
            alpha[y, x, 0] = 255
    return np.concatenate([rgb, alpha], axis=2)


@dataclass
class DatCompositor:
    atlas: TileAtlas
    global_palette: list[tuple[int, int, int]]
    tile_palettes: dict[int, dict[int, tuple[int, int, int]]]
    backdrop: np.ndarray
    anim_phase: int = 0

    @classmethod
    def open(
        cls,
        game_dir: Path,
        native_dir: Path,
        levels: list[Level],
        exclude_level: int | None = None,
        anim_phase: int = 0,
    ) -> DatCompositor:
        atlas = TileAtlas.open(game_dir)
        lvlist = levels if isinstance(levels, list) else list(levels.values())
        global_palette = fit_vga_palette(game_dir, native_dir, lvlist, exclude_level=exclude_level)
        tile_palettes = fit_tile_palettes(game_dir, native_dir, lvlist, exclude_level=exclude_level)
        px = atlas.tile_pixels(0x3C)
        pal = tile_palettes.get(0x3C) or global_palette
        backdrop = slot_to_rgb(px, pal) if px else np.full((TILE_PX, TILE_PX, 3), (0x28, 0x20, 0x38), np.uint8)
        return cls(
            atlas=atlas,
            global_palette=global_palette,
            tile_palettes=tile_palettes,
            backdrop=backdrop,
            anim_phase=anim_phase,
        )

    def _palette_for(self, tid: int) -> list[tuple[int, int, int]] | dict[int, tuple[int, int, int]]:
        tid = _norm_tid(tid)
        if tid in self.tile_palettes:
            return self.tile_palettes[tid]
        return self.global_palette

    def _base_slot(self, tid: int) -> int:
        tid = _norm_tid(tid)
        if tid in TUBE_ANIM_SLOTS:
            frames = TUBE_ANIM_SLOTS[tid]
            return frames[self.anim_phase % len(frames)]
        return tid

    def render_cell(self, tiles: list[int], row: int, col: int, tid: int) -> np.ndarray:
        if tid in BACKDROP_RENDER_IDS or tid == 0:
            return self.backdrop.copy()
        slot = self._base_slot(tid)
        px = self.atlas.tile_pixels(slot)
        if not px:
            return self.backdrop.copy()
        pal = self._palette_for(tid)
        # Start from playfield backdrop; AH=8 blit uses palette index 0 transparent.
        cell = self.backdrop.copy()
        base_rgba = _tile_rgba(self.atlas, pal, slot)
        if base_rgba is not None:
            cell = _composite_rgba(cell, base_rgba)
        center = _norm_tid(tid)
        for dname, (dr, dc) in _DIR_OFFSETS.items():
            nb = _neighbor_tid(tiles, row, col, dr, dc)
            if not _needs_edge(center, _norm_tid(nb) if nb >= 0 else -1):
                continue
            eslot = edge_dat_slot(center, dname)
            if eslot is None:
                continue
            edge = _tile_rgba(self.atlas, self.global_palette, eslot)
            if edge is not None:
                _apply_edge_band(cell, edge, _EDGE_BANDS[dname])
        return cell

    def render_level(self, tile_ids: list[int]) -> Image.Image:
        out = np.zeros((PLAYFIELD_H, PLAYFIELD_W, 3), dtype=np.uint8)
        for row in range(MAP_H):
            for col in range(MAP_W):
                tid = tile_ids[row * MAP_W + col]
                tile = self.render_cell(tile_ids, row, col, tid)
                y0, x0 = row * TILE_PX, col * TILE_PX
                out[y0 : y0 + TILE_PX, x0 : x0 + TILE_PX] = tile
        return Image.fromarray(out, "RGB")


def pick_anim_phase(
    level: Level,
    native_dir: Path,
    geom,
    game_dir: Path,
    exclude_level: int | None = None,
) -> int:
    """Choose f0/f1/f2 phase for animated tiles (e.g. tube 0x4B / decimal 75)."""
    from .atlas_median import ANIMATED_TILE_IDS

    if not any(t in ANIMATED_TILE_IDS for t in level.tiles):
        return 0
    paths = [native_dir / f"level_{level.index:02d}_f{n}.png" for n in range(3)]
    paths = [p for p in paths if p.is_file()]
    if len(paths) < 2:
        return 0
    from .native_vga import playfield_from_native

    caps = [np.array(playfield_from_native(Image.open(p).convert("RGB"), geom)) for p in paths]
    best_phase, best_mse = 0, float("inf")
    levels = [level]
    for phase in range(3):
        comp = DatCompositor.open(game_dir, native_dir, levels, exclude_level=exclude_level, anim_phase=phase)
        rend = np.array(comp.render_level(level.tiles))
        mask = np.zeros((MAP_H, MAP_W), dtype=bool)
        for row in range(MAP_H):
            for col in range(MAP_W):
                tid = level.tiles[row * MAP_W + col]
                if tid in ANIMATED_TILE_IDS:
                    mask[row, col] = True
        if not np.any(mask):
            return 0
        m = mask.repeat(TILE_PX, 0).repeat(TILE_PX, 1)
        diff = (caps[0].astype(np.float32) - rend.astype(np.float32)) ** 2
        mse = float(np.mean(diff[m]))
        if mse < best_mse:
            best_mse, best_phase = mse, phase
    return best_phase


def validation_eval_mask(level: Level, static_mask: np.ndarray) -> np.ndarray:
    m = np.zeros((MAP_H, MAP_W), dtype=bool)
    tiles = level.tiles
    enemy_cells = {(e.x - 1, e.y - 1) for e in level.enemies}
    px, py = level.player_x - 1, level.player_y - 1
    for row in range(MAP_H):
        for col in range(MAP_W):
            tid = tiles[row * MAP_W + col]
            if tid in BACKDROP_RENDER_IDS or tid == 0:
                continue
            if tid in GEM_TILE_IDS:
                continue
            if (col, row) in enemy_cells or (col, row) == (px, py):
                continue
            if not static_mask[row, col]:
                continue
            m[row, col] = True
    return m


def coverage_mask(tile_ids: list[int], compositor: DatCompositor) -> np.ndarray:
    """Cells where base DAT art exists for the map tile id."""
    m = np.zeros((MAP_H, MAP_W), dtype=bool)
    for row in range(MAP_H):
        for col in range(MAP_W):
            tid = tile_ids[row * MAP_W + col]
            if tid in BACKDROP_RENDER_IDS or tid == 0:
                continue
            slot = compositor._base_slot(tid)
            if compositor.atlas.tile_pixels(slot) is not None:
                m[row, col] = True
    return m


# Legacy no-op types for scripts still importing CompositeAtlas
@dataclass
class CompositeAtlas:
    backdrop: np.ndarray

    def render_level(self, tile_ids: list[int]) -> Image.Image:
        raise RuntimeError("Use DatCompositor instead of CompositeAtlas pickle")


def build_composite_atlas(*_args, **_kwargs):
    raise RuntimeError("composite_atlas.pkl removed; use DatCompositor")
