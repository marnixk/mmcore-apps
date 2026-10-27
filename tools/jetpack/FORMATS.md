# Jetpack (DOS v1.5) data formats

Sources: `MANUAL.TXT`, [TASVideos GameResources](https://tasvideos.org/GameResources/DOS/Jetpack), [downerj level layout gist](https://gist.github.com/downerj/0bd0664cd407da0eb4d438531176fd37), reverse engineering of `jetpak15.zip`, and `SCLM.DAT` (Adept shared library).

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

| File | Size (v1.5) | Slots (512-byte blobs) |
|------|-------------|-------------------------|
| JETPACK0.DAT | 43 559 | 83 |
| JETPACK1.DAT | 30 487 | 58 |
| JETPACK2.DAT | 33 739 | 64 |
| JETPACK3.DAT | 38 840 | 74 |
| JETPACK4.DAT | 21 048 | 39 |
| JETPACK5.DAT | 17 686 | 33 |

**351** compressed slots total; campaign **tile IDs `0x00`–`0x77` (120)** map to the first 120 slots across `JETPACK0` + `JETPACK1` (tile `N` → file `0` slot `N` for `N < 83`, else file `1` slot `N − 83`, etc.).

### Per-file layout

1. **512-byte header**: little-endian `uint16` values `0, 514, 1028, …` (byte pairs `00 00`, `02 02`, `04 04`, …) — one entry per slot, pointing at compressed blob starts (stride **514** = 512 data + 2 pad).
2. **Slot `i`** at offset `512 + i × 514`, length **512** bytes compressed payload (2 trailing bytes in the stride are padding; not part of the bitstream).
3. Decompress each 512-byte payload to **576** bytes via **SCLM LZ77** (see below), then use the first **576** bytes as a **24×24** indexed bitmap (**index 0 = transparent** when compositing).

### SCLM LZ77 (from `SCLM.DAT` offset `0x87`)

Bitstream: LSB-first within each control byte; **0 = literal** (next source byte), **1 = LZ** (next `uint16` LE):

- `length = ((high_byte >> 4) & 0x0F) + 2`
- `offset = word & 0x0FFF` (copy from `output_len - offset`)

After decompress, only the first `24×24 = 576` bytes are the visible tile; remaining bytes in the 576-byte buffer are unused padding from the original blitter.

`jetpack/graphics.py` implements this decoder and exports `gfx/sprites_vga.png` + `sprites_index.json` (`tile_id_map` + per-slot metadata).

### Rendering notes

- Empty map cells (`tile_id == 0`) are drawn as **cobblestone** tile `0x3C` in previews/mockups (matches DOS level preview backdrop).
- **Enemy / player sprites** live in later slots and animation frames; mockups still overlay simple entity markers until those indices are catalogued.

## Other files

| File | Notes |
|------|--------|
| `JETDEMO.DAT` | Demo replay data |
| `JETSOUND.DAT` | Digitized SFX |
| `SCLM.DAT` | Adept VGA/sound library (mode 13h setup, LZ77 above, blit helpers) |
| `_JETP_*.DAT` | Graphics module switcher variants (Christmas, Junior, etc.) |
| `JSWITCH.EXE` | Module switcher tool |

## Playfield geometry (original)

- **26 × 16** tiles, **24 px** per tile → **624 × 384** px playfield (single screen, no scroll).
