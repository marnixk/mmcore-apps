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

# Approval mockups (2 gameplay + title + pause)
python3 tools/jetpack/scripts/make_mockups.py tools/jetpack/_download/extracted/jetpak15
```

## Outputs

| Path | Description |
|------|-------------|
| `levels/level_XX.json` | Level data (committed after extract) |
| `previews/level_XX.png` | Map preview |
| `gfx/sprites_vga.png` | Decoded sprite sheet (best-effort) |
| `mockups/*.png` | UI / gameplay mockups |

## Docs

- [LICENSE.md](LICENSE.md) — freeware terms and commit guidance
- [FORMATS.md](FORMATS.md) — file formats
- [GAMEPLAY.md](GAMEPLAY.md) — mechanics spec
- [PORT_PLAN.md](PORT_PLAN.md) — mmcore layout and risks
