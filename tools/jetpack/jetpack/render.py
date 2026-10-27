from __future__ import annotations

from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

from .constants import MAP_H, MAP_W, TILE_PX
from .frame_atlas import FrameAtlas
from .graphics import TileAtlas, render_level_tiles
from .levels import Level

_FRAME_ATLAS: FrameAtlas | None = None


def _frames() -> FrameAtlas | None:
    global _FRAME_ATLAS
    if _FRAME_ATLAS is not None:
        return _FRAME_ATLAS
    try:
        _FRAME_ATLAS = FrameAtlas.load()
        return _FRAME_ATLAS
    except FileNotFoundError:
        return None
from .palette import (
    DOOR_COLOR,
    ENEMY_COLORS,
    PLAYER_COLOR,
    tile_rgb,
)


def _try_font(size: int) -> ImageFont.FreeTypeFont | ImageFont.ImageFont:
    for name in ("DejaVuSansMono.ttf", "DejaVuSans.ttf", "LiberationMono-Regular.ttf"):
        try:
            return ImageFont.truetype(name, size)
        except OSError:
            continue
    return ImageFont.load_default()


def render_level_preview(
    level: Level,
    scale: int = 1,
    show_entities: bool = True,
    atlas: TileAtlas | None = None,
) -> Image.Image:
    frames = _frames()
    if frames is not None:
        field = frames.render_level(level.tiles, use_level0_cells=(level.index == 0))
        if show_entities and frames.player_frames:
            s = TILE_PX
            px = (level.player_x - 1) * s
            py = (level.player_y - 1) * s
            field.paste(frames.player_frames[0], (px, py), frames.player_frames[0])
        if scale != 1:
            w, h = field.size
            field = field.resize((w * scale, h * scale), Image.Resampling.NEAREST)
        return field.convert("RGB")
    if atlas is not None:
        field = render_level_tiles(level.tiles, atlas)
        if scale != 1:
            w, h = field.size
            field = field.resize((w * scale, h * scale), Image.Resampling.NEAREST)
        return field.convert("RGB")
    else:
        w, h = MAP_W * TILE_PX * scale, MAP_H * TILE_PX * scale
        img = Image.new("RGB", (w, h), (0x18, 0x18, 0x20))
        px = img.load()
        for y in range(MAP_H):
            for x in range(MAP_W):
                tid = level.tiles[y * MAP_W + x]
                color = tile_rgb(tid)
                for dy in range(TILE_PX * scale):
                    for dx in range(TILE_PX * scale):
                        px[x * TILE_PX * scale + dx, y * TILE_PX * scale + dy] = color

    if not show_entities:
        return img

    draw = ImageDraw.Draw(img)
    s = TILE_PX * scale
    dx, dy = level.door_x - 1, level.door_y - 1
    draw.rectangle(
        [dx * s, dy * s, (dx + 2) * s - 1, (dy + 2) * s - 1],
        outline=DOOR_COLOR,
        width=max(1, scale),
    )
    px0 = (level.player_x - 1) * s + s // 2
    py0 = (level.player_y - 1) * s + s // 2
    draw.ellipse([px0 - s // 3, py0 - s // 3, px0 + s // 3, py0 + s // 3], fill=PLAYER_COLOR)
    for e in level.enemies:
        ex = (e.x - 1) * s + s // 2
        ey = (e.y - 1) * s + s // 2
        col = ENEMY_COLORS.get(e.type_id, (0xFF, 0x80, 0x80))
        draw.rectangle([ex - s // 4, ey - s // 4, ex + s // 4, ey + s // 4], fill=col)
    return img


def composite_gameplay_frame(
    level: Level,
    out_w: int,
    out_h: int,
    level_label: str = "LEVEL 01",
    atlas: TileAtlas | None = None,
) -> Image.Image:
    """Full-screen gameplay mockup: scaled playfield + HUD gutters."""
    field_w, field_h = MAP_W * TILE_PX, MAP_H * TILE_PX
    scale = min(out_w / field_w, out_h / field_h)
    tile_sc = max(1, int(scale))
    if tile_sc * field_w < out_w * 0.92:
        tile_sc += 1
    preview = render_level_preview(level, scale=tile_sc, show_entities=True, atlas=atlas)
    fw, fh = preview.size
    ox = (out_w - fw) // 2
    oy = (out_h - fh) // 2

    bg = (0x12, 0x0A, 0x1A)
    frame = Image.new("RGB", (out_w, out_h), bg)
    frame.paste(preview, (ox, oy))
    draw = ImageDraw.Draw(frame)
    font = _try_font(14 if out_w < 800 else 18)
    small = _try_font(12 if out_w < 800 else 14)

    bar_y = 8
    draw.text((12, bar_y), level_label, fill=(0xE8, 0xD8, 0xA8), font=font)
    draw.text((out_w - 120, bar_y), "GEMS 3/3", fill=(0x60, 0xF0, 0x60), font=small)
    bx, by, bw, bh = 12, out_h - 28, min(200, out_w // 3), 14
    draw.rectangle([bx, by, bx + bw, by + bh], outline=(0x60, 0x50, 0x70))
    draw.rectangle([bx + 2, by + 2, bx + bw - 30, by + bh - 2], fill=(0x30, 0xA0, 0xE0))
    draw.text((bx + bw + 8, by), "FUEL", fill=(0xC0, 0xC0, 0xD0), font=small)
    return frame


def render_entity_showcase_crop(level: Level, atlas: TileAtlas, margin_tiles: int = 4) -> Image.Image:
    """1:1 crop around player start showing tiles, gems, fuel, door, and entity markers."""
    px = (level.player_x - 1) * TILE_PX
    py = (level.player_y - 1) * TILE_PX
    s = TILE_PX
    m = margin_tiles * TILE_PX
    frames = _frames()
    if frames is not None:
        field = frames.render_level(level.tiles, use_level0_cells=(level.index == 0))
        if frames.player_frames:
            field.paste(frames.player_frames[0], (px, py), frames.player_frames[0])
    else:
        field = render_level_tiles(level.tiles, atlas)
        s = TILE_PX
        draw = ImageDraw.Draw(field)
        draw.ellipse(
            [px + s // 2 - 8, py + s // 2 - 8, px + s // 2 + 8, py + s // 2 + 8],
            fill=PLAYER_COLOR,
        )
    x0 = max(0, px - m)
    y0 = max(0, py - m)
    x1 = min(field.width, px + s + m)
    y1 = min(field.height, py + s + m)
    return field.crop((x0, y0, x1, y1)).convert("RGB")


# House-style UI colors (aligned with voidrunner / salsaslide)
C_WB0 = (14, 7, 22)
C_WB1 = (23, 11, 34)
C_WB2 = (34, 16, 47)
C_GOLD = (232, 200, 120)
C_CREAM = (240, 228, 200)
C_CYAN = (120, 220, 255)


def draw_panel(draw: ImageDraw.ImageDraw, x: int, y: int, w: int, h: int) -> None:
    draw.rectangle([x, y, x + w, y + h], fill=C_WB1, outline=C_WB2)
    draw.rectangle([x + 4, y + 4, x + w - 4, y + h - 4], outline=(50, 22, 61))


def render_title_screen(w: int = 1280, h: int = 720) -> Image.Image:
    img = Image.new("RGB", (w, h), C_WB0)
    draw = ImageDraw.Draw(img)
    title_font = _try_font(56)
    sub_font = _try_font(22)
    menu_font = _try_font(20)
    small = _try_font(16)

    draw.text((w // 2, 72), "JETPACK", fill=C_GOLD, font=title_font, anchor="mm")
    draw.text(
        (w // 2, 118),
        "a port for mmcore",
        fill=C_CREAM,
        font=sub_font,
        anchor="mm",
    )
    draw.text(
        (w // 2, 148),
        "Original game by Adept Software / Adam Pedersen",
        fill=(0xA0, 0x90, 0xB0),
        font=small,
        anchor="mm",
    )

    pw, ph = 360, 200
    px, py = (w - pw) // 2, 200
    draw_panel(draw, px, py, pw, ph)
    items = ["START GAME", "HIGH SCORES", "QUIT"]
    for i, label in enumerate(items):
        ty = py + 36 + i * 44
        if i == 0:
            draw.rectangle([px + 24, ty - 6, px + pw - 24, ty + 26], fill=C_WB2)
        draw.text((px + 48, ty), label, fill=C_GOLD if i == 0 else C_CREAM, font=menu_font)

    draw.text(
        (w // 2, h - 36),
        "Collect every gem. Reach the exit. Mind your fuel.",
        fill=(0x90, 0x80, 0xA0),
        font=small,
        anchor="mm",
    )
    return img


def render_pause_dialog(level: Level, w: int = 1280, h: int = 720, atlas: TileAtlas | None = None) -> Image.Image:
    label = f"LEVEL {level.index:02d}"
    bg = composite_gameplay_frame(level, w, h, label, atlas=atlas)
    overlay = Image.new("RGB", (w, h), (0, 0, 0))
    img = Image.blend(bg, overlay, 0.55)
    draw = ImageDraw.Draw(img)
    pw, ph = 520, 280
    px, py = (w - pw) // 2, (h - ph) // 2
    draw_panel(draw, px, py, pw, ph)
    head = _try_font(28)
    row = _try_font(18)
    draw.text((w // 2, py + 28), "PAUSED", fill=C_GOLD, font=head, anchor="mm")
    controls = [
        ("ARROWS", "MOVE / CLIMB"),
        ("ALT / INS", "JETPACK"),
        ("CTRL / DEL", "PHASE SHIFT"),
        ("DOWN", "USE / ENTER"),
        ("P / ESC", "RESUME"),
        ("Q", "QUIT TO TITLE"),
    ]
    y0 = py + 64
    for i, (key, action) in enumerate(controls):
        y = y0 + i * 32
        draw.text((px + 32, y), key, fill=C_CYAN, font=row)
        draw.text((px + 220, y), action, fill=C_CREAM, font=row)
    draw.text((w // 2, py + ph - 24), "JETPACK", fill=C_GOLD, font=row, anchor="mm")
    return img
