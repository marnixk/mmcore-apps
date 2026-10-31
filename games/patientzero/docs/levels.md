# Patient Zero - level ideas

The original has one level: an open street, walls along the top and bottom, 7 puddles,
zombies spawning without end. Below are 7 level types that could follow it. Every level uses the
same 32 x 18 tile grid (20 px tiles at base size, so 40 px at 1280x720 and 60 px at 1920x1080).

A level ends when a set number of zombies has been cured (for example 20, then 25, 30 ...), so that
there is a win screen between levels. That is new; the original had no levels and no win.
Score stays hidden until the level-end or game-over screen.

Mockups (with the new enemy types): `mockups/level_hospital_1280x720.png`, `mockups/level_rainy_street_1280x720.png`,
`mockups/level_subway_1280x720.png`.

---

## 1. Street (the original level, cleaned up)
```
################################   building fronts (solid)
................................   pavement
..car....~~.....##........[]....   ~~ puddle, ## broken wall, [] crates
.....~~.........##.......~~.....
- - - - - - - - - - - - - - - -   road marking
..~~........o.......==.....~~...   o manhole, == barrier
################################   rubble / low wall
```
- New: nothing. A few obstacles to walk around.
- Zombies: wander, chase within range, run when you charge (as in the original).
- Puddles: 5-7, fixed.
- Difficulty: the teaching level. Few zombies, slow spawn.

## 2. Hospital corridors  (mockup)
```
####D#####D#####D#####D#####D###   wards along the top, D = open ward door
..bed.......~~.........bed......
.......................~........
##########....########....######   dividing wall with two gaps
......~~.........bed............
####D#####S#####D#####S#####D###   S = shut door
```
- New: zombies do not appear anywhere. They come out of open ward doors, one door at a time,
  with a short warning (the door frame lights up). Shut doors open as the level goes on.
- The dividing wall makes a choke point: a full sneeze in a gap can clear a crowd.
- Puddles: spilled water from leaking pipes, 2-3 only, and far from each other.
- Difficulty: medium. Corridors limit where you can run, so being cornered is the main risk.
- This fits the story: the zombies want to bring Jerry here.

## 3. Rainy street  (mockup)
```
################################
...~~.........~...........~~....   puddles start small
.........~~~~.........~.........
- - - - - - - - - - - - - - - -
....~~~~~.........~~~~~.........   and grow while it rains
################################
```
- New: rain falls in waves. During rain every puddle grows by one size step and new small
  puddles appear. Between waves puddles shrink back.
- Recharging is easy, so the challenge moves to timing: more zombies spawn while it rains.
- Rain particles are drawn as sprites over the field (streaks + small splashes, in fx and tiles_levels).
- Difficulty: medium. A good second level, because it rewards learning the puddle recharge.

## 4. Subway  (mockup)
```
################################   tiled wall
..|.......|....~~.....|.....|...   platform 1, | pillars
================================   yellow edge
###TRAIN########TRAIN###........   track 1 (train passes left to right)
================================
.......|...........|.......~....   platform 2 (centre island)
================================
________________________________   track 2 (train passes right to left)
................^^..........~~..   platform 3, ^^ stairs (zombie entry)
```
- New: two tracks with trains. A light on the platform edge blinks before a train comes.
  A train blocks the track and pushes anything on it off the screen (zombies are removed,
  Jerry loses a life). When no train is near, the tracks can be crossed.
- Zombies: enter by the stairs. They do not avoid trains, so luring them onto the track is a way
  to get rid of them (no score for that, only cures score).
- Puddles: few (leaking roof), only on the platforms.
- Difficulty: medium to hard. Timing the crossings is the skill.

## 5. Park with fountains
```
################################   fence
..TT.....TT.........TT.....TT...   TT trees (solid)
.......(  F  ).......(  F  )....   F fountain with a round basin
..TT.....................TT.....
..........(  F  )...............
################################
```
- New: fountains are big round recharge areas, but they switch off and on in a cycle
  (water visible = recharge works). Small puddles around them dry up over time.
- Paths between trees are narrow; zombies can come from any edge of the park.
- Difficulty: medium. A breather level with open space but fewer safe spots.

## 6. Rooftops
```
....######..........#######.....   ###### = roof blocks (walkable)
....######====......#######.....   ==== = plank bridge
....######..........#######.....   .... = drop (not walkable)
............######..............
............######====#######...
```
- New: the field is split into roof blocks joined by narrow planks. Falling is not possible
  (edges are solid), but planks are one tile wide, so a zombie on a plank blocks the way.
- Zombies climb up from fire escapes at the roof corners.
- Puddles: rain water on the flat roofs, one per roof.
- Difficulty: hard. Little room to dodge. A full-reach sneeze over a gap hits zombies on the
  next roof (the sneeze hit area ignores walls, as in the original).

## 7. Night blackout (limited vision)
```
the street layout from level 1, but only a circle of about 5 tiles
around Jerry is lit; burning barrels and street lamps light small
areas around them
```
- New: the screen is dark outside a radius around Jerry and around light sources.
  Zombies outside the light are shown only as a pair of yellow eyes.
- Charging makes Jerry's light circle shrink (he closes his eyes), which makes the trade-off of
  charging long clearer.
- Puddles: only visible when lit, or when they catch light (one bright reflection pixel).
- Difficulty: hard. Best placed late. On the Pi 400 the dark mask has to be done with cheap
  means (pre-drawn dark tiles around a lit window, updated per tile), not per-pixel blending.

## 8. Shopping mall with escalators (optional)
```
######SHOP####SHOP####SHOP######
........>>>>>>......<<<<<<......   >>>> escalator moving right
.....~~............~~...........
........<<<<<<......>>>>>>......
######SHOP####SHOP####SHOP######
```
- New: escalator strips move anything on them (Jerry and zombies) by 1 px per frame in their
  direction. Useful to escape, or to carry zombies into the sneeze.
- Zombies come out of shop doors. Puddles near a broken fountain in the middle.
- Difficulty: medium to hard.

---

## Enemy introduction and mix per level
Behaviour of each type is in `enemies.md`. One new type per level for the first five levels,
and the new type is shown on the level-start banner.

| # | Level | New type | Mix (share of spawns) | Cure target |
|---|---|---|---|---|
| 1 | Street | zombie (basic) | 100% zombie | 20 |
| 2 | Rainy street | office worker | 70% zombie, 30% office | 25 |
| 3 | Hospital | paramedic | 50% zombie, 25% office, 25% paramedic (they come out of the wards) | 25 |
| 4 | Park | hoodie | 40% zombie, 20% office, 15% paramedic, 25% hoodie (hides behind trees) | 30 |
| 5 | Subway | Thriller dancer | 30% zombie, 20% office, 15% paramedic, 20% hoodie, 15% dancer (moonwalks along the platforms) | 30 |
| 6 | Rooftops | - | 30% zombie, 15% office, 15% paramedic, 25% hoodie, 15% dancer | 35 |
| 7 | Blackout | - | 25% zombie, 15% office, 20% paramedic, 30% hoodie, 10% dancer | 35 |
| 8 | Mall (optional) | - | even mix, 20% each | 40 |

Then repeat the list with faster spawns and a higher cure target.

Why this order: the office worker is the easiest new type (straight lines, easy to line up).
The paramedic fits the hospital and teaches that puddles can be guarded. The hoodie needs hiding
places, so the park. The dancer is the most showy and comes last, on the long subway platforms
where a moonwalk has room.

Per level the knobs are: number of zombies at the start, spawn interval (original: one every
100 frames), chase range (original 300 px), chase speed (original 0.8 px/frame vs Jerry's 4),
number of puddles, puddle recharge time (original about 2.8 s for one sneeze), and the cure target.

Phase 1 has art for levels 1-4 (street, hospital, subway and rain tiles in `tiles_levels.png` /
`props_levels.png`). Park, rooftop, blackout and mall would need their own tiles once picked.
