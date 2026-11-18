# Patient Zero (LD33, by Ireal / Marnix Kok) - notes from the original source

Source fetched from https://zombies.marnixkok.nl/ into `original/` (index.html, js/*.js,
assets/tileset.png, assets/sprites/snot.png, assets/fonts/zombie-gradient.*, assets/audio/*.mp3).
Engine: PIXI 3.0.7 (WebGL), jQuery. All timings below are per frame of `PIXI.ticker` (~60 fps).

Subtitle in the page title: "Patient Zero - The end of a species". The hero is called **Jerry**.

## Screens
- **Menu**: story text in red ("The game plays in a zombie-apocalyptic world in which no man survived ..
  Jerry, our lovable zombie, is visiting friends, but he has the sniffles!! They want to send him home,
  after all you can't zombie proper with a cold! Jerry isn't having any of it!"), instructions in cyan,
  pulsing "START". 1000 drifting snot particles behind. Enter or mouse click starts.
- **Arena**: the one level.
- **Game over**: "Game Over / Thanks for playing!", "Your score: N", "(refresh to play again)".
  No high score table, no name entry, no restart key.
- No pause. Esc was commented out. Background music loops, plus voice samples
  (achoo, "bless you", "brains", "excuse me"/sniff, "ew", inhale 1-3).

## Playfield
- Canvas 832x640, but the game uses `width = window width` and `height = 640`.
- Tiles are 64x64 in `tileset.png` (320x640, magenta-free alpha PNG).
- Background: the dark camo tile `block-bg` (#161616/#252525) tiled and rotated 0.1 rad.
- Walls: one row of brown camo blocks (#644837/#a27948, yellow edge line) along the top and bottom
  only. No side walls, no obstacles inside the field. Player is clamped to y 92..height-96 and x 0..width-128.
- **Puddles**: 7 pools, one per 1/7 of the screen width, each built from 20 random overlapping water
  circles (scale 0.1-2.1 of a 64px circle, alpha 0.1-0.35, multiply blend, slow alpha pulse).
  Puddle test = player position inside any of the 140 circles.

## Player (Jerry, the blue sick zombie)
- Arrows move, 8 directions (diagonals allowed), 4 px/frame. Sprite flips left/right only
  (side view; there are no up/down sprites). 2-frame walk.
- 5 lives (hearts, bottom left). Touching a zombie's feet rectangle costs a life, then 100 frames of
  blinking invulnerability. 0 lives = game over.
- **Sneeze ammo ("flems")**: starts at 5, max 5, shown as green blobs bottom right. Each sneeze uses one.
  With 0 flems, Z and V do nothing.
- **Charging (Z)**: Z must be *held*. `flemFactor` +1 per frame up to 100 (~1.7 s).
  Held at 100 for more than 10 frames, it drops by 8 per frame ("Don't inhale too long!").
  Released, it falls 5 per frame. So the trick is to press V while Z is still held near the top.
- **Sneeze (V)**: only if flems > 0 and not already sneezing. Reach = max(10, 300 * flemFactor/100) px.
  Hit area is a **rectangle**, not particle collision: x +/- reach around the player, y -50..+80.
  Every zombie whose position is in that rectangle while the sneeze plays is cured. So the sneeze
  hits both left and right at once.
- Sneeze visual: 400 particles (16x16 green/cyan "achoo" sprite, random scale 0..2), each waits 1-10
  frames, flies in a 70 px high arc out to its own random distance, then slides along the ground and
  fades. Screen shakes and the player squash-stretches while it plays (~1 s).
- **Recharging in puddles**: while standing in a puddle `chillFactor` +0.6/frame; out of it -0.3/frame.
  Over 100 (~2.8 s in the puddle) gives +1 flem and resets to 0 (if already 5 flems it stays at 100).
  A cyan particle band across the top of the screen shows chillFactor; the text "GOT TO SNEEZE"
  wobbles in over 50%.
- A green band along the bottom shows the current charge (flemFactor).

## Zombies (green)
- Pool of 50; 21 active at start; one more spawns every 100 frames (~1.7 s), at least 200 px from the player.
- Each has a random fixed wander direction and bounces off the screen edges.
- Speed: wander 0.5 px/frame; within 300 px of the player they **chase** at 0.8 (not while the player
  is blinking); within 300 px while the player is charging with flemFactor > 30 they **run away** at 1.5.
  So charging scares them off and you must release at the right moment.
- Cured: score +1337, "ew" sound, zombie replaced by the cured human sprite (brown hair, tan robe,
  2 frames: eyes open / blink). It stands still for 100 frames, then fades out over 100 frames.
  **No wings and no floating up in the original**; that is new in the key art.

## Scoring / end
- Score only from cures: 1337 each. Shown live at the bottom (to be moved to the end screen for mmcore).
- No win condition and no levels: zombies keep spawning until lives run out.

## Sprites and palette (tileset.png, 64x64 cells, art drawn with ~2-3 px chunky pixels)
| name | colours |
|---|---|
| zombie (side view, arm forward, X eyes, open mouth) | skin #90bc61, clothes #5a6d46, feet #447411, mouth #721426 |
| player (same shape, blue) | skin #81cad4, clothes #328a97, mouth #14383d |
| cured human | hair #ae8405, skin #d4b881, robe #977532, belt #e1d875 |
| sneeze particle "achoo" | #6aec47 green + #1fc5e3 cyan |
| flem (ammo) | #6aec47 with #2c3028 outline |
| heart / lost heart | #a70000, #ec1212, #3f1919 |
| walls | #644837 / #a27948, edge #ffd400-ish |
| floor | #161616 / #252525 |
| water | pale cyan #b1d3d4 with blue #4877a2 |

## Differences from the brief
1. The cure is not per-particle: the sneeze has a rectangular hit zone left and right of Jerry whose
   width depends on how well you charged. Particles are decoration.
2. Z is hold-to-charge, with an over-charge penalty; charging also makes nearby zombies run away.
3. Sneezes are limited ammo (5). Puddles give ammo back one at a time; they do not top up the charge.
4. Cured humans stand still and fade out; the wings and light beam come from the new key art.
5. Original sneeze particles are green/cyan; the key art makes them blue. The new art follows the key art.
6. There are lives (5 hearts) and a short invulnerability after a hit.
7. No pause, no high scores, no win condition, single endless level.
