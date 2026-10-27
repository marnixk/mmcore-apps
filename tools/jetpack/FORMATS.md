# Jetpack (DOS v1.5) data formats

Sources: `MANUAL.TXT`, [TASVideos GameResources](https://tasvideos.org/GameResources/DOS/Jetpack), [downerj level layout gist](https://gist.github.com/downerj/0bd0664cd407da0eb4d438531176fd37), and reverse engineering of `jetpak15.zip` in this repo.

## Campaign levels — `JETLEV.DAT`

| Field | Value |
|-------|--------|
| Size | 50 602 bytes |
| Header | **2 bytes** (`0x86 0xE6` in v1.5 freeware) then level records |
| Record count | **100** × 506 bytes |
| Custom levels | Separate `*.JET` files (same 506-byte layout), up to 10 000+ in editor |

### `.JET` / level record (506 bytes, `0x000`–`0x1F9`)

| Offset | Size | Content |
|--------|------|---------|
| `0x000`–`0x19F` | 416 | Map tiles, **column-major** 26×16, 1 byte tile ID (`0x00`–`0x77`) |
| `0x1A0`–`0x1A1` | 2 | Exit door top-left (X `1`–`25`, Y `1`–`15`; door is 2×2 tiles) |
| `0x1A2`–`0x1A3` | 2 | Player start (X `1`–`26`, Y `1`–`16`) |
| `0x1A4`–`0x1DF` | 60 | 20 × (enemy_id, x, y); `enemy_id == 0` = empty slot |
| `0x1E0`–`0x1F9` | 26 | Level title (JetSCII subset of ASCII) |

### Enemy IDs

| ID | Name |
|----|------|
| 1 | Trackbot |
| 2 | Steel ball |
| 3 | Spring |
| 4 | Missile |
| 5 | Spikes (spinner) |
| 6 | Flitzer |
| 7 | Homer |
| 8 | Batbot |

Full tile ID table: see `jetpack/constants.py` and the gist linked above.

## Palette — `JETPAL.DAT`

785 bytes. Repeating records:

- Optional separator `0x55`
- `index`, `0x01`, `0x00`, then **VGA 6-bit** R/G/B (each ×4 for 8-bit RGB)

Used at runtime for blitted sprites and UI.

## Graphics archives — `JETPACK0.DAT` … `JETPACK5.DAT`

| File | Size (v1.5) | Role |
|------|-------------|------|
| JETPACK0.DAT | 43 559 | Tiles / objects (module 0) |
| JETPACK1.DAT | 30 487 | Sprites |
| JETPACK2.DAT | 33 739 | Sprites |
| JETPACK3.DAT | 38 840 | Sprites |
| JETPACK4.DAT | 21 048 | Sprites |
| JETPACK5.DAT | 17 686 | Sprites |

### Container layout (reverse-engineered)

1. **Offset table** at file start: little-endian `uint16` offsets, strictly increasing, until an offset ≥ file size.
2. **Sprite blobs** between consecutive offsets (skipping 2-byte “meta” gaps in some entries).
3. Each sprite decodes to **24×24** indexed pixels (576 bytes) via **RLE**:

```
while output < 576:
  b = read_byte()
  if b == 0:
    count = read_byte(); value = read_byte()
    emit value repeated count times
  else:
    emit b
```

**Uncertainty:** Mapping from **map tile ID** → sprite index in these files is not fully verified; level **editor preview** uses tile ID as a colour index (see `jetpack/palette.py`). Runtime drawing may use additional animation frames from other indices.

`scripts/extract_all.py` exports a best-effort `out/gfx/sprites_vga.png` + `sprites_index.json` using `JETPAL.DAT`.

## Other files

| File | Notes |
|------|--------|
| `JETDEMO.DAT` | Demo replay data |
| `JETSOUND.DAT` | Digitized SFX |
| `SCLM.DAT` | x86 helper (sound/driver), not level data |
| `_JETP_*.DAT` | Graphics module switcher variants (Christmas, Junior, etc.) |
| `JSWITCH.EXE` | Module switcher tool |

## Playfield geometry (original)

- **26 × 16** tiles, **24 px** per tile → **624 × 384** px playfield (single screen, no scroll).
