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

## `JETPACK.EXE` (v1.5 freeware) — RE notes

Binary: **141 152** bytes, plain **MZ** (no PKLITE/LZEXE wrapper). **MZ header** = **448** paragraphs (**7168** bytes, `e_cparhdr`); code/data follow at file offset **0x1C00**. Filename strings sit in the tail of the image (e.g. `JETPACK0.DAT` @ file **0x1F125**, `SCLM.DAT` @ **0x1F1BF**, `JETLEV.DAT` @ **0x1F208**).

| File offset | Role |
|-------------|------|
| **0x31A7** | DOS **read** wrapper — `int 21h` **`AH=3Fh`** @ **0x31BE** (`CX` = length, `DS:DX` = buffer, `BX` = handle). |
| **0x4CAD** | Open path used by stdio-style I/O (`int 21h` **`AH=3Dh`** inside **0x5A00** @ **0x5A26** for low-level open). |
| **0x5891** | `creat` / open helper (`AH=3Ch` @ **0x5864** for create; **`AH=3Dh`** via **0x5A00**). |
| **0x3B42** | VGA dispatch (sets **`DS=0x40`**, **`int 10h`** for mode **13h** when `AL=40h`; checks **`byte [0x84] == 0x18`** (24) before accepting mode 13h @ **0x3BC1**). |
| **0x4730** | Tile/sprite **blit** loop — resolves map coordinates via **0x46C4**, fetches pixels through **0x3B42** with **`AH=8` / `AH=9`** (SCLM blitter entry points). |
| **0x3896** | VRAM **rep mov** to port **0x3DA** (VGA latch), used on the blit path when `word [0x311C] != 0`. |
| **0x48AD** | Palette / fill helper (invoked from gameplay draw setup). |

**`SCLM.DAT`** (loaded at runtime, not linked at fixed addresses in the EXE):

| SCLM file offset | Role |
|------------------|------|
| **0x0000** | Mode **13h** init, table-driven blit to **0xA000:0000** (`call` through offset table @ **0x22A**). |
| **0x0087** | **LZ77 decompressor** — `shr`/`jb` bit test on control byte, literal `movsb`, match `rep movsb` with `length = ((AH>>4)&0xF)+2`, `offset = AX & 0xFFF` (same as Python `sclm_lz77_decompress`). Called from **0x002A** inside SCLM with arguments on the stack. |

The game opens `JETPACKn.DAT` through the **0x5891 / 0x4CAD** open layer, reads **512-byte** compressed blobs with the **0x31A7** reader, and decompresses each blob through the **SCLM `0x87`** routine into a **576-byte** (**24×24**) scratch buffer before **0x4730** draws with index **0** transparent.

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

1. **512-byte header**: repeating **3-byte** stride tags `(0,0,0)`, `(2,2,2)`, `(4,4,4)`, … (i.e. even steps `0,2,4,…` tripled). Runtime and `jetpack/graphics.py` use fixed **`512 + slot_index × 514`** (514 = 512-byte LZ payload + **2** pad bytes between slots).
2. **Slot `i`** at offset `512 + i × 514`, length **512** bytes compressed payload (2 trailing bytes in the stride are padding; not part of the bitstream).
3. Decompress each 512-byte payload to **576** bytes via **SCLM LZ77** (see below), then use the first **576** bytes as a **24×24** indexed bitmap (**index 0 = transparent** when compositing).

### SCLM LZ77 (from `SCLM.DAT` offset `0x87`)

Bitstream: LSB-first within each control byte; **0 = literal** (next source byte), **1 = LZ** (next `uint16` LE):

- `length = ((high_byte >> 4) & 0x0F) + 2`
- `offset = word & 0x0FFF` (copy from `output_len - offset`)

After decompress, only the first `24×24 = 576` bytes are the visible tile; remaining bytes in the 576-byte buffer are unused padding from the original blitter.

`jetpack/graphics.py` implements the LZ77 step and can export `gfx/sprites_vga.png` + `sprites_index.json`, but **indexed DAT → RGBA using `JETPAL.DAT` alone does not reproduce in-game colours** (sparse/nonzero bytes after decompress, ~16 brown gradient entries in `JETPAL.DAT`). Bit-plane order, runtime DAC reprogramming, and/or a different palette source remain **unverified** — treat DAT bitmap decode as **broken** until matched to DOS captures.

### Art pipeline in this repo (v1.5 phase 1)

**Ground truth:** DOSBox captures under `reference/` (`level00_play2.png`, `dosbox_title.png`, …). Level-0 minimap lives at **(175, 95)** size **290×270** inside `level00_play2.png` (same pixels as `level00_preview_crop2.png`).

**Export route:** `jetpack/dos_art.py` + `scripts/extract_from_dos.py` crop **416** minimap cells to `gfx/frames/level00_cells/cell_CC_RR.png`, dedupe per tile ID for `tile_*.png`, title crops for `player_fly_*.png` / emerald pickups. `gfx/frames/index.json` sets `"source": "dosbox_capture"`.

**Validation:**

- `scripts/validate_art.py` — level 0 render MSE vs DOS minimap (threshold **12** at 624×384).
- `scripts/build_reference_compare.py` — TASVideos silhouettes vs DOS frame atlas (`reference/compare.png`).

TAS wiki PNGs use a **light/dark matte background**; comparisons use **mask MSE** on opaque pixels (RGB differs from DOS VGA but shapes should align).

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
