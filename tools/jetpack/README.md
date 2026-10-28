# Jetpack (DOS) — phase 1 tooling

Research, extraction, and approval assets for the mmcore port. **No game code** lives here; phase 2 will add `games/jetpack/`.

## Prerequisites

1. Obtain **jetpak15.zip** from [Adept Software](https://www.adeptsoftware.com/jetpack/) or a mirror (see [LICENSE.md](LICENSE.md)).
2. Unzip to `tools/jetpack/_download/extracted/jetpak15/` (gitignored).

```bash
pip install -r tools/jetpack/requirements.txt
```

## Commands

```bash
# Levels → JSON + PNG previews; sprite sheet → out/gfx/
python3 tools/jetpack/scripts/extract_all.py tools/jetpack/_download/extracted/jetpak15

# Gameplay mockups (683×384, 1280×720, 1:1 entity crop — no title/pause UI)
python3 tools/jetpack/scripts/make_mockups.py tools/jetpack/_download/extracted/jetpak15

# TASVideos reference compare (downloads in reference/tasvideos/ if missing)
python3 tools/jetpack/scripts/build_reference_compare.py tools/jetpack/_download/extracted/jetpak15

# DOS ground-truth frames (preferred; also run from extract_all.py when reference PNGs exist)
python3 tools/jetpack/scripts/extract_from_dos.py tools/jetpack/_download/extracted/jetpak15
```

## Outputs

| Path | Description |
|------|-------------|
| `levels/level_XX.json` | Level data (committed after extract) |
| `previews/level_XX.png` | Map preview |
| `gfx/sprites_vga.png` | SCLM LZ77 export (DAT decode **not** colour-accurate; see FORMATS.md) |
| `gfx/frames/` | **In-game** DOS captures → RGBA tiles + `index.json` (`source: dosbox_play_capture`) |
| `reference/` | DOSBox screenshots (`dosbox_*.png`, `level00_play2.png`); `compare.png` vs TAS |
| `gfx/tas_reference.json` | TASVideos ↔ DOS capture mapping from `build_reference_compare.py` |
| `scripts/validate_art.py` | Level 0 playfield MSE vs DOS minimap (threshold 12) |
| `mockups/*.png` | UI / gameplay mockups |

## Docs

- [LICENSE.md](LICENSE.md) — freeware terms and commit guidance
- [FORMATS.md](FORMATS.md) — file formats
- [GAMEPLAY.md](GAMEPLAY.md) — mechanics spec
- [PORT_PLAN.md](PORT_PLAN.md) — mmcore layout and risks
