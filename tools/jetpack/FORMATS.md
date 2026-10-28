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
| **0x3871** | **Map index** — 1-based `(col, row)` → byte offset into level map at `DS:[0x311F]` using width **`byte [0x311A]`** (26) and base **`word [0x311D]`** (`index = (col−1)×26 + (row−1)`, then `×2` for word-sized strip state). |
| **0x46C4** | Pixel/tile coordinate → `(row, col)` on the 26-wide grid (uses **`[0x311A]`**). |
| **0x47FA** | **Per-cell terrain draw** — gameplay path calls **0x4730** (SCLM blit); editor/VRAM path calls **0x3896**. |
| **0x4730** | **Strip blit** — walks cells; when the **map tile id** at the leading edge differs from the trailing edge (**0x46C4** + compare), fetches a new **24×24** via SCLM **`AH=8`** (base) and composites an edge with **`AH=9`** (transparent index 0). **`word [0x311F]`** is the player/blank sentinel tile id for “no edge”. |
| **0x48AD** | **Edge/shadow rectangle pass** (gameplay, `[bp+4]==1`) — for each border pixel runs **0x460C** (Bresenham over the map) and **0x43D1** / **0x4427** to stamp edge strips; **0x488B** writes SCLM slot words with **`CH = byte [0x3116]`** (current centre **tile id**) and **`CL = 0x20`**. Neighbour cells are read with **0x3871** at offsets around the rectangle (see branches @ **0x4979** vs **0x48E8** for corner case **`[bp+0xE]==6`**). |
| **0x3BC0** | Playfield setup — sets **`[0x311A]=26`**, **`[0x3119]=height`**, **`[0x311F]`** map segment, **`[0x311B]`** editor vs play flag. **No extra tileset byte** in the 506-byte level record after the 416-byte map (door @ **0x1A0**, player, enemies, title only). |

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

Bitstream: LSB-first within each control byte; **1 = literal** (next source byte), **0 = LZ** (next `uint16` LE). `SCLM.DAT` at `0x87`: `shr` / `jc` takes the literal when CF=1.

- `length = ((high_byte >> 4) & 0x0F) + 2`
- `offset = word & 0x0FFF` (copy from `output_len - offset`)

After decompress, only the first `24×24 = 576` bytes are the visible tile; remaining bytes in the 576-byte buffer are unused padding from the original blitter.

`jetpack/graphics.py` implements the LZ77 step and can export `gfx/sprites_vga.png` + `sprites_index.json`, but **indexed DAT → RGBA using `JETPAL.DAT` alone does not reproduce in-game colours** (sparse/nonzero bytes after decompress, ~16 brown gradient entries in `JETPAL.DAT`). Bit-plane order, runtime DAC reprogramming, and/or a different palette source remain **unverified** — treat DAT bitmap decode as **broken** until matched to DOS captures.

### Art pipeline in this repo (v1.5 phase 1)

**Ground truth:** native VGA PNGs `reference/native/level_XX_fN.png` (**640×400**, `scaler=none`, no resample). Legacy window captures are rejected.

**Playfield:** VGA mode 13h is 320×200, chain-4 (sequencer index 4 = `0x0E`). The playfield is **26×16 tiles of 12×12** indices at VGA origin **(4, 0)** (312×192). Doubled 640×400 captures crop **(9, 0)–(633, 384)** = **26×16×24 px** (`reference/playfield_geometry.json`). Column 8 of those PNGs is a black gutter; the doubled pixels start at x=9.

**Capture:** `scripts/dosbox_capture_slot0.py` — per level, copy campaign record into **slot 0** of a writable `JETLEV.DAT` (`jetpack/jetlev_swap.py`), fresh Xvfb + DOSBox per level, grab **640×400** root framebuffer. Legacy `scripts/dosbox_capture.py` (TAB+2+S skip) is deprecated after mis-label bugs.

**Map layout:** `scripts/find_map_layout.py` scores 416-byte map candidates (offsets 0–32, row/column major, flips); locked layout in `reference/map_layout.json` (**`off00|row_major`**, bytes 0–415 — door at 0x1A0). Raw within-ID variance favours `off26|row_major` on level 0 but misaligns the level record; see `reference/layout_search.json`.

**Capture verify:** `scripts/verify_capture_labels_fingerprint.py` — each `level_XX_f0.png` must best-match JETLEV level **XX** via DAT compositor render MSE (expect **100/100** when DAC calibration matches DOS).

**Playfield draw (`jetpack/vga_index.py`):** each map cell is one 12×12 cell from the in-memory sheet. There is no VGA edge compositor. `0x4730` / `0x48AD` are the text-mode editor blit (`int 10h` AH=8/9). The VGA path copies a 12×12 sheet cell with the stride-320 blitter at file `0x1A3CE` (segment `0x187B` offset `0x1E`).

**Index ground truth:** `scripts/dump_vga_level.py` boots slot 0 under DOSBox-X and the `vga_dump.asm` TSR writes a `JPVG` dump (BIOS mode, sequencer/CRTC/graphics registers, 768-byte 6-bit DAC, then four 64KiB plane reads). Mode is **13h**, sequencer index 4 is `0x0E` (chain-4), and the four planes are identical, so the framebuffer is the linear A000 image. `dac6.bin` is the level-0 DAC. Indices 0–246 match on levels 5, 7, 37, 59 and 99; indices 247–254 are the spark cycle. `sheet_12x12.u8` is the 120-tile sheet at linear `0x4AED5` (stride 320, 20 columns, pitch 13, separator index `0x66`). The same 120 cells are at that address in the level-5 memory dump. Tile id `N` is cell `N`. RGB uses `(v<<2)|(v>>4)` on the 6-bit DAC, with no palette fitting.

Pixel index match of sheet[tile id] against the dumped playfield (sparks, door, player and enemies included): level 0 **98.691%**, 5 **97.529%**, 7 **98.391%**, 37 **97.047%**, 59 **97.546%**, 99 **97.421%**. Interior 3×3-uniform cells on level 0 are 34/37 exact; the three misses are spark pixels on tile 0.

`sclm_lz77_decompress` matches `SCLM.DAT` `0x87` (bit 1 = literal), but those decompressed `JETPACK*.DAT` blobs do **not** contain the 12×12 indices (the brick row `07 26 27 27 …` never appears). The on-screen indices are the memory sheet, not a raw LZ bitmap.

**Validation:** `scripts/validate_art.py` renders the sheet through the dumped DAC and compares to native `f0`. The sheet and DAC do not depend on the held-out level. MSE skips backdrop, gems, enemies, the player cell, the door sprite, and animated tube/teleport ids. Side-by-sides for levels **0, 5, 7, 37, 59, 99**.

## Other files

| File | Notes |
|------|--------|
| `JETDEMO.DAT` | Demo replay data |
| `JETSOUND.DAT` | Digitized SFX |
| `SCLM.DAT` | Adept VGA/sound library (mode 13h setup, LZ77 above, blit helpers) |
| `_JETP_*.DAT` | Graphics module switcher variants (Christmas, Junior, etc.) |
| `JSWITCH.EXE` | Module switcher tool |

## Playfield geometry (original)

- **26 × 16** tiles, **12 VGA px** per tile → **312 × 192** at A000 (x = 4). Doubled captures are **624 × 384**.
