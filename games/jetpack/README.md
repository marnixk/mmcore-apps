# Jetpack (mmcore port)

**JETPACK — a port for mmcore**

Original game **Jetpack** (1993) by **Adept Software** / **Adam Pedersen**.
Freeware: [adeptsoftware.com/jetpack/](https://www.adeptsoftware.com/jetpack/)

Run from this directory: `RUN "main.bas"`. High scores are kept in `C:/.jetpack`.
The game is silent. Fuel, gems and lives are drawn in the side gutter because
the fuel level is part of the play.

## Art

`art/src/export.py` rebuilds the approved mockups from `art/real/`.
`art/build_assets.py` packs the per-resolution sheets, the title and scores
pages, `data/levels.dat`, `includes/layouts.inc` and `includes/tiles.inc`.

The in-game melt is the original dissolve strip (`art/real/dissolve_12.u8`),
not `generated/brick_melt_*`. Those three cells are only the half-eaten brick
on the title floor.

## Keys

mmcore's held-key list cannot tell Alt from Ctrl, and it does not report
Insert or Delete. Jet is **Space** or **0**. Phaser is **Enter** or **Z**.
Arrows move. Down uses a teleporter or a switch. P or Esc pauses. Q leaves
the pause screen.
