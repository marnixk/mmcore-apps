# Jetpack — gameplay specification (DOS v1.5)

Numbers marked **(TAS)** come from [TASVideos Jetpack resources](https://tasvideos.org/GameResources/DOS/Jetpack). Manual quotes come from `MANUAL.TXT`. Items without a solid source are marked **uncertain**.

## Objective

Collect **all emerald gems** on a level, then enter the **exit door** (2×2 tiles; opens after the last gem). One life lost on enemy/hazard contact; **5 lives** at start (v1.5, `HISTORY.TXT`).

## Screen and camera

| Property | Value |
|----------|--------|
| Map size | 26 × 16 tiles **(TAS)** |
| Tile size | 24 × 24 pixels **(TAS)** |
| Scrolling | **None** — entire level visible (624×384 px) |
| Level order | Campaign **0–99** in `JETLEV.DAT` |

## Movement and physics **(TAS)**

Subpixel unit: 1 pixel = 16 subpixels; 1 tile = 384 subpixels.

| Quantity | Value (subpixels/frame) |
|----------|-------------------------|
| Walk | 55 |
| Walk on moss | 32 |
| Walk with conveyor | 60 |
| Walk against conveyor | 25 |
| Jump initial vertical | 68 up |
| Jump initial horizontal | 62 |
| Jump gravity | 5 |
| Jetpack thrust accel | 4 vertical, 6 horizontal |
| Jetpack terminal up/down | 64 |
| Jetpack terminal horizontal | 64 |
| Fall terminal vertical | 64 |
| Fall terminal horizontal | 20 |
| Ladder climb | 45 (with motion 64, against 20) |
| Phase align to tile center | 32 |

**Fuel (TAS):** tank capacity 10 000 units; half tank 5 000; jetpack burn **10 units/frame**; charger **+30/frame**; drain **−20/frame**.

**Phasing times (frames):** crate/purple 5; light brick 26; dark brick 56; purple return 20; light return 94; dark return 226 **(TAS)**.

**Other timings (TAS):** teleport 20 frames; exit door opens **38 frames** after last gem.

## Controls (original → planned mmcore)

| Action | DOS keyboard | DOS joystick |
|--------|--------------|--------------|
| Move | Arrow keys | Stick |
| Jetpack | Alt / Ins / Keypad 0 | Button A |
| Phase | Ctrl / Del / Enter | Button B |
| Use (ladder, teleporter, buttons) | Down | — |
| Pause | Pause / F10 | — |
| Quit | Q | — |

Port: **ON KEY + SETTICK** held keys in-game; **INKEY$** menus; pause **P/Esc**, quit from pause **Q** (house standard).

## Items on map (tile IDs)

| Tile / item | Points (manual) | Notes |
|-------------|-----------------|-------|
| Emerald gem | 5 | Required to finish |
| Gold coin | 10 | |
| Pile o' gold | 25 | |
| Golden vase | 50 | |
| Extra life | 100 | Also +1 life |
| Single fuel tank | 10 | **5000** fuel **(TAS)** |
| Double fuel tank | 20 | **10000** fuel **(TAS)** |
| Stunner | 25 | Paralyses monsters **(duration uncertain)** |
| Shield | 25 | **(duration uncertain)** |

**Extra life:** every **1 000** points (manual).

**Random bonus items:** stunner/shield/fuel can appear during play (manual); spawn rules **uncertain** (TAS notes “busyness” and level number).

## Enemies **(TAS speeds)**

| Enemy | Speed (subpixels/frame) | Behaviour summary |
|-------|-------------------------|-------------------|
| Trackbot | H 43, ladder V 33 | Gravity; pursues player; random at junctions |
| Steel ball | H 59 | Gravity; bounces off walls |
| Spring | V 50 | Bounces vertically; 5-frame turn delay |
| Missile | 64 | Turns 90° clockwise on wall hit |
| Spikes | 20 diagonal | Bounces; initial direction random |
| Flitzer | 11 | Random 8-way; changes every 8 frames |
| Batbot | H 10, V 17 | Homes on player after landing |
| Homer | 4 | Diagonal until aligned, then straight; passes through walls |

Enemies with gravity: terminal fall **64** **(TAS)**. Most enemies can toggle **floor switches**.

## Switches, barriers, teleporters

- **Buttons** (wall) and **floor switches**: red / blue / cyan (green); toggle matching **barriers**.
- **Teleporters**: yellow / pink / green; **Down** to use; random destination if multiple pads of same colour **(TAS)**.
- **Mirage** tiles: hidden until phased or walked through.

## Scoring and timer

- Per-item scores: see table above (manual).
- **Level timer / bonus for speed:** not documented in manual; treat as **uncertain** for port unless verified in phase 2.
- **Victory dance** length: **uncertain** **(TAS TODO)**.

## Debug keys (not for port)

TAB+2+L/F/I/S — extra life, fuel, invincibility, skip level (manual).
