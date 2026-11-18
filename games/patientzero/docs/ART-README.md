# Patient Zero - phase 1 art (for approval)

## Tile size: 20 px base, stored pre-scaled per resolution
- 1280x720 (MODE 19) = 32 x 18 tiles of 40 px (base x2)
- 1920x1080 (MODE 20) = 32 x 18 tiles of 60 px (base x3)
Both modes get the same 32 x 18 grid with integer scaling, so levels, collision and layout are
identical and the screen is filled edge to edge with no border. 16 px would give 2.5x / 3.75x
(not whole numbers, or a 40 x 22.5 grid), and 32 px would give 1.25x at 720p. Characters are
20 x 30 (1 x 1.5 tiles), so 40 x 60 on screen at 720p, close to the 64 px sprites of the original.

## Files
- `gfx/base/`  art at 1x (the master pixels)
- `gfx/720/`   the same sheets scaled 2x with nearest neighbour, for MODE 19
- `gfx/1080/`  scaled 3x, for MODE 20
- `gfx/sheets.json`  name -> [x, y, w, h] for every frame in every sheet (base pixels; multiply by 2 or 3)
- `art/*.py`   the Python/Pillow generator. `python3 build.py`, `python3 mockups.py`,
  `python3 levels_mock.py`, `python3 preview.py` rebuild everything.

Sheets: sprites (20x30 cells, 16 per row, 13 rows): Jerry (variant A, hospital gown) walk 4
directions x 4 frames, idle x2, charge x2, sneeze front x2, sneeze left/right, hurt; then for each
enemy type (zombie, office, hoodie, dancer, medic) walk 4 x 4, hit x2, cure x3, plus dancer Thriller
shuffle x2, toe stand L/R, moonwalk 4 frames each way, and paramedic face mask (down/right/left).
Old names cure_0..2 and angel_0..3 are kept as aliases of the basic zombie.
angels (30x32, 4 wing frames, one row per enemy type), tiles (20x20 street), props (car, lamp, barrel, large puddles 4 frames each),
fx (particles in 4 blues x 6 sizes, cloud puffs 16/24/32, sparkles, light beam + glow,
charge meter 0-10 + overcharge, ammo drops 0-4 fill, hearts), logo, title_bg, tiles_levels, props_levels.

All sheets: 48-colour palette (art/pal.py), alpha is only 0 or 255 (no soft edges).
If mmcore needs a key colour instead of alpha, the build can fill the background with one colour.

## Choices to confirm
1. In-play HUD: a small charge bar above Jerry while Z is held, and the ammo drops (5) above
   him briefly after a sneeze or while refilling in a puddle. Nothing else on screen. This goes
   against "no live status" but is core to the game. Lives (hearts) are drawn but not placed:
   suggestion is to show them only for a moment after a hit.
2. Sneeze direction: the original hits left and right of Jerry at once. The gameplay mockup
   keeps that; the title art shows a one-way sneeze as in the key art.
3. Sneeze colour: blue/white (key art) instead of the original green/cyan.
4. Cured humans now float up with wings in a gold light beam (key art); in the original they stood
   still and faded out.
