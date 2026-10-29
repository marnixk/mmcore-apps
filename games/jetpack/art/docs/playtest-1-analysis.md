# Jetpack playtest 1 — analysis

Compared with `JETPACK.EXE`
(image base = file offset `0x1C00`, DS = `0x1D03`) and with
`tools/jetpack/reference/dosbox_play_level00.png`. The port sources cited are
`games/jetpack/includes/play.inc` on `27f697a`. The fixes below are in this
branch. Starting fuel is 0, matching the EXE play entry, which the original
write-up had left for a later change. Trackbot conveyors, escalators,
floor switches, and teleports, and steel-ball teleports, are in the
section at the end of this file.

Level 0 in `data/levels.dat` is DOS level 1, “WELCOME TO JETPACK!”. Its enemy
line is `1,12,1 5,1,16`: one trackbot and one spikes. Positions are the
EXE-style bottom-right corner, in tiles. The body occupies the cell just
above and to the left of that corner.

## 1. Fuel gauge

### What the original draws

The status line is the bottom 8 VGA rows, y = 192..199, under the 192-pixel
playfield (rows 0..191). It is not drawn on top of the last tile row. The
DOSBox 640×400 capture is a scaled view of that; the coordinates below are
from the EXE, and the capture agrees on colours and shape.

Drawn every status update, from the play function (file `0xE138` onward):

| Piece | Where | Size | Colour |
| --- | --- | --- | --- |
| Bar background | x = 4, y = 192 | 313×8 | palette 10, RGB (81, 81, 81) |
| Text | x = 4, y = 193 | glyphs 8×7 | see below |
| Gauge frame | x = 42, y = 193 | 91×7 blit | metallic bevel, from the art bitmap at its row 187 |
| Gauge interior | x = 44, y = 195, 196, 197 | 87×3 | filled from the left, remainder black |

The text is the C string at file `0x2016A`:

```
{121}FUEL              LIVES{031}:   {121}SCORE{031}:
```

`{NNN}` sets the font colour (`0x8D70` in the text routine). 121 is
RGB (73, 251, 203). 31 is RGB (255, 255, 255). “FUEL” is four cells starting
at column 0. “LIVES” starts at column 18 (x = 148). “SCORE” starts at column
27. The 91×7 frame sits in the gap after “FUEL”.

The interior is three horizontal runs (`0x13BD6`–`0x13C75`), one per scanline,
filled by `stosb` (`0x4D9F`):

- Width of the coloured part = `fuel * 87 / 10000`, anchored at x = 44.
- The other `87 - width` pixels are palette 0 (black), immediately to the right.
- So the bar empties from the right.
- The three rows use `base`, `base+1`, `base+2`:
  - fuel ≥ 3000: base 100 → (0, 142, 0), (0, 170, 0), (0, 199, 0)
  - fuel ≥ 1500: base 85 → (170, 158, 0), (199, 186, 0), (227, 219, 0)
  - otherwise: base 37 → (170, 0, 0), (199, 0, 0), (227, 0, 0)

Fuel is a word, maximum 10000 (`0xFA94`). A single tank adds 5000, a double
tank 10000 (`0xFA89`). The play function sets that word to 0 at entry
(`0xE015`). 10000 is forced only when three config bytes are set (`0xF2D8`).
The level-0 capture’s black well is that empty state. The port sets
`gm.fuel = FUEL_MAX` in `gm.run`, so the port’s bar starts full.

### What the port draws

`hud.draw` (`play.inc`) runs only when `lay.ox >= 20`. It fills the whole left
gutter with RGB (16, 12, 14), then a vertical box `8 * lay.fsc` wide whose
height is `fuel / 10000` of `(scr.H - 28 * lay.fsc)`, colour RGB (232, 168, 48),
or RGB (210, 48, 36) when fuel < 2000. Under it, the gem count and the life
count. No “FUEL”, no bevel, no score, no green/yellow/red bands. `gm.step`
sets `hud.dirty = TRUE` every logic frame, so this whole gutter is boxed and
marked every tick.

### Fix

Keep the side gutter. On MODE 20 the field is 16×24 = 384 pixels on a 384-high
screen (`lay.oy = 0`), so an 8-pixel bottom bar only fits by covering the last
tile row or by letterboxing every layout. The original bar is chrome; the
gutter already replaced it. A bottom bar is not required to match the widget.

Draw a horizontal gauge in the gutter, scaled to fit. Narrowest HUD gutter is
`lay.ox = 29` (683×384), which cannot hold 91 pixels, so scale the 91×7 frame
and the 87×3 well down to the gutter width. Keep:

- left-to-right fill, `width = fuel * well / 10000`, remainder black
- the three threshold bands and the three shade rows (the RGB triples above)
- a one-pixel light/dark bevel around the well
- the word FUEL in (73, 251, 203) when the gutter is wide enough (1280 and 1920)

Leave the starting fuel at 10000 unless a later change asks for the original
empty start. That is a gameplay change (no jet until the first tank), not part
of matching the widget.

Effort: `hud.draw` only. Risk: low. Faithfulness: shape, colours, direction,
and thresholds match the EXE. Position stays in the gutter, so it is not a
copy of the y = 192 strip.

## 2. The level-0 enemy should ping-pong

### Which enemy

Type 5, spikes, record `5,1,16`. Init frame in the EXE is 16 (file `0xDD87`).
It is the small spiked ball, not the trackbot and not the steel ball (type 2,
which is not on this level). The port places it with its bottom-right corner
on column 1, row 16, so the body sits in column 0, row 15, the open cell at
the bottom left.

`GAMEPLAY.md` already had the TAS summary: speed 20 on both axes, diagonal,
bounces, initial direction random. The EXE agrees.

### What the EXE does

Per-enemy thinker, file `0x161C7`–`0x16443`. No gravity.

If the direction byte `DS:[slot+0x3536]` is 0, it becomes `rand() % 4 + 1`
(`0x161DE`, `rand` is the far call at `0x1581`). The four diagonals, stored
as ±1 in the velocity words and then multiplied by 20 (`0x16422`):

| State | vx (`0x3226`) | vy (`0x31FE`) | Direction (y grows downward) |
| --- | --- | --- | --- |
| 1 | −20 | −20 | up-left |
| 2 | +20 | −20 | up-right |
| 3 | +20 | +20 | down-right |
| 4 | −20 | +20 | down-left |

Before that, the thinker samples tile ids across the 12×12 footprint
(`0x160CE`–`0x161AE`, map stride 28). A sample ≥ `0x28` (40) is a wall.
Tiles below 40, including ladders (id 1) and the early solid ids 24..39, do
not bounce this enemy. Each state has an ordered list of those samples. The
first wall hit picks a new state: both forward samples solid means reverse
(1↔3, 2↔4); one solid sample means a 90° turn onto one of the other two
diagonals. The open path rewrites the words to ±1 and then multiplies by 20,
so a frame in open air steps by exactly ±20.

The blocked branch jumps to that same multiply without writing ±1 first
(`0x1627D` and the matching jumps in states 2..4). If the words still hold
±20 from the previous frame, that one frame becomes ±400. Do not copy that.
A 400-unit step is a full tile, and the same function then zeroes both
velocities if the center cell is ≥ 40 (`0x165D8`), which would freeze a ball
that stepped into a wall. The behaviour to port is a constant ±20 with the
sign reflected.

The integrator (`0x127EF`) adds those words to the position once. There is no
per-pixel slide.

### What the port does

`en.think` for `p_k = 5` forces `vx = 20` when it is 0 and `vy = 20` when it
is 0, then calls `en.move(i, TRUE)`. `en.move` reflects x through `en.bounceX`
(negate `vx`), but on a y hit it calls `en.land`, which sets `vy = 0`. The
next think sees `vy = 0` and writes `+20` again. The ball never travels up.
It slides along the floor and only turns around in x.

`lv.oneEn` starts type 5 at `vy = 20` and `vx = ±20` from `en.n mod 2`. On
this level that is the second slot, so `vx = -20`, `vy = +20`. The EXE rolls
1..4 when the direction byte is still 0.

### Fix

In the type-5 branch, do not call `en.land`. On an x hit negate `vx` and keep
`abs(vx) = 20`. On a y hit negate `vy` and keep `abs(vy) = 20`. No gravity.
Roll the initial state once, 1..4, instead of `en.n mod 2`. Treat a tile as a
wall for this bounce only when its id is ≥ 40, matching the EXE probe rather
than `fld.solid` (which also blocks ids 24..39, escalators, and conveyors).

Effort: the type-5 branch and a y-reflect helper. Risk: low. The steel ball
(type 2) should keep `en.land`; it really does have gravity and only bounces
in x. Faithfulness: this restores the original. It does not add a new pattern.

## 3. Trackbot stuck in the ice

### What is on level 0

Trackbot record `1,12,1`: bottom-right corner on column 12, row 1, so the body
is in column 11, row 0. Under it, row 1 columns 10..14 are solid (id ≥ 40).
Column 15 is a ladder (id 1) from row 1 downward. Tile `0x5E` (94), which is
kind 1 in `tiles.inc` and reads as the pale ice strip, is row 4, columns
15..21. The player corner is column 17, row 16, so the chase is to the right
and down: off that shelf, toward the ladder, and onto the ice.

The EXE treats id ≥ 40 as solid for this enemy too, so the ice is a floor in
both programs. Kind 2 (ladder, id 1) is not solid in `fld.solid`, which matches
the EXE treating ids 1..3 as ladder/escalator rather than wall.

### What the EXE does

Same slot layout as the spikes, different thinker: file `0x14800`–`0x15230`.
The direction byte `0x3536` holds one of 1, 2, 4, 8 (not a combination):

| Value | Motion | Word written |
| --- | --- | --- |
| 1 | left | `0x3226 = -43` (`0xFFD5`) |
| 2 | right | `0x3226 = +43` (`0x2B`) |
| 4 | climb up | `0x31FE = -33` (`0xFFDF`) |
| 8 | climb down | `0x31FE = +33` (`0x21`) |

While the byte is 4 or 8, horizontal speed is forced to ±32 so the body
centers on the ladder column: `(x >> 5) mod 12` (`0x14D5E`). ≥ 6 writes +32,
1..5 writes −32. Climb frames are `(y >> 5)` bit 0 plus 10 (`0x14E58`). Walk
frames are the `(x >> 5) mod 3 + 4` group, plus 3 when vx is negative
(`0x14E83`).

It keeps the current direction while that way is still open. Open/blocked
comes from the same kind of tile probe (id ≥ 40 blocks; ids 1..3 are
ladder/escalator). Toward the player it will take a ladder: the signed
deltas are `[bp-8]` (horizontal) and `[bp-0xA]` (vertical). At a junction,
`rand() % 3 == 0` reconsiders, and `rand() % 20 == 0` takes a side branch
(`0x14904`, `0x14919`, and the matching tests further down). That is the
“random at junctions” line in `GAMEPLAY.md`.

If the feet are not on a support, it does not keep walking. `0x14D9D` clears
the direction byte, sets vx to 0, and sets vy to `0x40` (64). It drops
straight down at terminal speed. It does not carry the ±43 off the edge.
Conveyor ids `0x72`..`0x77` then bias vx (`0x14F58`, `0x14FC2`). The
integrator adds the words once (`0x127EF`).

So the original does climb ladders, does turn instead of walking into a wall,
and falls only as a straight drop after it has lost the floor. It does not
roll off a shelf and then grind in the pit.

### What the port does

`en.think` for `p_k = 1`:

- If the single cell under the body center is a ladder and the player is more
  than 200 units away in y, it zeroes vx and sets vy to ±33 toward the player.
- Otherwise it adds 5 to vy, caps at 64, and sets vx to ±43 toward the player
  every frame.

There is no edge test, no direction memory, and no junction roll. It walks
off the row-1 shelf because vx stays ±43 in the air. `en.move` then
`en.land` zeroes vy on the ice. The next think aims vx at the player again,
so `en.bounceX`’s negated vx is thrown away. Against the ice wall it steps,
hits, undoes the whole step, and repeats. It never climbs column 15, because
the ladder test requires the center cell to already be a ladder.

It is not stuck inside the collision routine. `en.move` undoes a whole step;
it does not use the player’s 16-unit retreat. A body that starts a frame
clear ends the frame clear. The ice is an ordinary solid. The failure is the
missing ledge test and the missing ladder choice, so it falls into the pit
and jitters on the wall.

### Fix

Replace the type-1 branch with the director above. `en.st` is unused for type
1 and can hold 1/2/4/8.

- One direction at a time. Horizontal ±43, ladder ±33, ladder-centering ±32.
- Probe the footprint. Id ≥ 40 blocks. Ids 1..3 are climbable.
- Keep the current direction while it is open.
- At a junction, prefer the axis toward the player; `rnd() mod 3` and
  `rnd() mod 20` as above.
- If the feet are not supported, vx = 0 and vy = 64. Do not call `en.land`
  to cancel a climb, and do not recompute vx toward the player every frame.
- Conveyor bias (± the `0x14F58` adjustment) was not part of that fix. It
  is in the element section at the end. It is not what pins the bot on
  level 0.

Effort: one enemy routine, plus a shared “is this tile a wall / a ladder”
probe. It wants a pass on a level that is mostly trackbots (level 37 in the
old captures has five). Risk: medium, because the junction rolls are easy to
get subtly wrong, and every trackbot level changes. Faithfulness: this is the
original, including ladders. The port’s chase-and-fall is the invention.

## 4. It feels slow, and tile collision

### What a frame actually costs

`OPTION FRAMEPROFILE` is in the mmcore this tree builds (0.293.0). A profile
frame is one present (a blit onto the display page), not one logic tick.
`tools/jetpack/playtest1_frameprofile.py` runs a copy of the game, capped at
48 logic frames, MODE 20, level 0, `TH_FAST = 0` so `pause IDLE_MS` and
`SETTICK` are live. It does not edit `games/jetpack`.

Host numbers, SDL dummy, not a Pi. Interpreter time here is about 1.2 µs per
BASIC statement. A Pi is slower; the statement counts are the part that
transfers.

| Bucket | Count | Statements (median) | Interpreter time here |
| --- | --- | --- | --- |
| Asset load (first present) | 1 | 19223 | 91 s |
| Draw with no logic tick | 218 | 1255 | 1.6 ms |
| Draw that also ran `gm.step` | 48 | 1790 | 1.8 ms |
| Extra presents inside one draw | 7 per draw | ~2 each | ~0 |

The logic tick adds about 535 statements on top of the 1255. Draws ran about
4.5 times per logic tick. Each draw presents 8 rects: the heavy frame holds
the BASIC, and the next seven are the `scr.flush` loop.

That 1255 is the size of `fld.flush`. It walks all 16×26 cells every draw
and blits only the dirty ones. On a settled screen nothing is dirty, and the
scan is still the whole cost. HUD text, sprite moves, input, physics, both
enemies, and collision together are the other ~535, and only on the logic tick.

`bod.hit` is already a cell test. The body is one tile (384 units). It loops
the cells from `(x-344)\384` to `(x-40)\384` and the same in y, which is at
most a 2×2, and each cell is one `fld.solid` / `tf.k` lookup. It does not
walk pixels and it does not read the framebuffer.

The only stepped walk is `pl.moveX` / `pl.moveY`: after an overlap they step
back 16 units, up to 24 times, calling `bod.hit` each time. That runs when
the player is jammed into a surface. `en.move` does not do it. The stuck
trackbot is two `bod.hit` calls per logic frame, the same as a trackbot that
is walking. Nothing in that state scans more cells, retries more, or dirties
more of the map. The hitch Marnix feels there is the bot not getting out, on
top of a draw loop that is already spending ~1255 statements to discover that
no tile changed.

`hud.draw` is not the statement spike (the gutter `box` and the glyph blits
are C, and present time was ~10 µs here). It does mark the whole gutter every
logic frame, so one of the 8 rects is always the full gutter. On a Pi that
blit is worth dropping; it is not the 1255.

### Tile collision, as suggested

Moving movement checks to “which cell am I entering” matches the EXE, which
probes tile ids and then adds velocity once. In this port the enemy and the
clear-air player path are already that, plus a 2×2 overlap instead of a
handful of footprint points.

Replacing the player retreat loop with one cell test removes up to 24
`bod.hit` calls on a grind. At roughly 25 statements each, that is on the
order of 600 statements, about one extra logic tick, and only on frames that
are already overlapping. It does not run in the stuck-robot case. Against a
draw scan of 1255 statements, several times per tick, it is a small fraction
of the frame. It is not the speed fix for this playtest.

### Fix

Two separate changes:

1. Stop the full-map scan. Give `fld.flush` a count or a short list of dirty
   cells, and skip the 16×26 walk when it is zero. That is the ~1255. Set
   `hud.dirty` only when fuel, gems, or lives change, so the gutter is not
   marked every tick. Risk: low. No gameplay change. This is the speed work.
2. Leave `bod.hit` as the overlap test until the trackbot and spikes fixes
   land. If movement is later switched to the EXE’s footprint probes (issues
   2 and 3), the player retreat loop can go at the same time, because the
   probe decides the step before it is added. Doing that only for speed,
   while the enemies still chase with `en.land`, risks new stuck cases.
   Estimated saving if it is done: a few hundred statements on a grinding
   frame, and about none on the stuck trackbot.

Faithfulness: skipping a clean flush does not change the game. Footprint
probes would move collision toward the EXE; they are not what makes the
stuck robot expensive.

## Summary

| Issue | Root cause | Fix | Faithfulness |
| --- | --- | --- | --- |
| Fuel gauge | `hud.draw` is a vertical orange gutter bar. The EXE draws an 87×3 well in an 8-pixel strip at y = 192, filled from the left, green / yellow / red at 3000 and 1500. | Scaled horizontal well in the gutter, same fraction, bands, and bevel. | Widget matches. Position stays in the gutter. |
| Ping-pong enemy | Level 0 type 5 (spikes). `en.land` zeroes vy, then the think writes +20. The EXE steps ±20 on both axes and reflects off tile id ≥ 40. | Negate both components. No gravity. Random initial diagonal. | Restores the original. |
| Stuck trackbot | Type 1 chases with vx ±43 and gravity, so it walks off the shelf onto tile `0x5E` and never takes the ladder at column 15. The EXE climbs at ±33, turns at walls, and drops straight (vx 0, vy 64) only after it loses the floor. | Replace the type-1 branch with that director. | Restores the original, including ladders. |
| Slow | Every draw spends ~1255 statements in `fld.flush` scanning 416 clean cells. The stuck bot adds no extra scan. `bod.hit` is already per cell. | Dirty-cell list, and stop marking the gutter every tick. Tile probes are a later, smaller saving. | The scan removal does not change play. |

## Trackbot elements and steel-ball teleports

Read from `JETPACK.EXE` (file offsets). The trackbot thinker is
`0x14800`–`0x15277`, then a jump to the shared epilogue at `0x17577`.
Enemy words: vx `[bx+0x3226]`, vy `[bx+0x31FE]`, x `[bx+0x3276]`,
y `[bx+0x324E]`, direction `[bx+0x3536]`, flags `[bx+0x3522]` (low
nibble teleport cooldown, bit `0x10` switch latch). The integrator at
`0x127EF` adds vx and vy once, after this thinker. Probes are VGA
pixels from the sprite top-left. Feet are `0x354F` and `0x3551`.
Center is `0x354A`. Id ≥ 128 is empty. Id ≥ 40 (`0x28`) is a wall.

### Conveyors

Both feet must be on the belt. One foot does nothing.

| Belt | Ids | vx &lt; 0 | vx &gt; 0 | vx = 0 |
| --- | --- | --- | --- | --- |
| Left (`0x14F58`) | `0x72`–`0x74` | −55 | 20 | −40 |
| Right (`0x14FC2`) | `0x75`–`0x77` | −20 | 55 | 40 |

Frames are chosen from the pre-belt velocity. A later test (`0x1520E`)
zeroes vx and vy if the center is a wall, so a belt cannot carry a bot
whose center is inside a solid. The port does that zero after the belt
and sets the stuck frame to 10, which is what `0x1523A` writes.

### Escalators

Only on the supported path. The unsupported drop (`0x14D9D`, vy = 64)
skips this. Id 2 (`0x14C6B`): vy ≠ 0, center or either foot is 2, and
the up bit of the open mask is set. Then vy = −61 going up, or +16
going down. Id 3 (`0x14CDF`) is the same test with the down bit, and
overwrites: +61 going down, or −16 going up. The mask bit still
requires the x alignment the director already uses (`(x >> 5) mod 12`
&lt; 2). A bot whose center is an escalator but which is not lined up
does not take the escalator speed.

Clamps run after the belt (`0x15033`, `0x1508E`) and apply to every
trackbot, not only escalators:

- vy &lt; −32: if the inner-bottom probe `0x3555` is not a ladder or
  escalator (1..3), vy = −32. If it is, and the cell two pixels above
  the sprite is a wall, vy = −32. Otherwise −33 and −61 are kept.
- vy &gt; 32: if the cell just under the sprite (`0x3554`) is a wall,
  vy = 32. Else if the center is 1..3, keep it. Else if `0x3554` is
  1..3, vy = 32. Else keep it, including an open-air fall of 64 and a
  non-wall tile under the feet (gem, belt, pad).

### Floor switches

`0x150C7`. Center id `0x21`..`0x23`, and `((y >> 5) - 1) mod 12` &gt; 10
(bottom pixel of that tile), and the latch bit is clear. Store
`id + 0xE0` into byte `[0x373E]` (low byte 1, 2, or 3) and set the
latch. Leave the tile, or fail the alignment, and the latch clears.
Wall buttons 9–14 are not tested. There is no extra “react to the
switch” choice in the director. The next frame’s probes see whatever
the signal changed.

The consumer at `0x11F28` is the same signal the player’s wall buttons
send (`0x1076D`: ids 9–10, 11–12, 13–14 → signals 1, 2, 3). It swaps
those button ids and, on one polarity, ORs `0x80` onto tube tiles
`0x4A`–`0x4F` whose pair matches the color (`0x12283`); the other
polarity writes the tube id back without that bit. The port’s switch
is `lv.flip`, which swaps 9↔10, 11↔12, 13↔14 and does not touch tubes.
Those button ids are below 40, so the open mask does not change.
`en.tid` would treat a mirage bit as empty, so the EXE’s tube toggle
would change pathing. That toggle is not in the port’s switch model,
for the player or the trackbot. The trackbot calls `lv.flip(17/18/19)`
for tiles 33/34/35, which is the signal the port already has.

### Teleports

`0x15126`, shared epilogue `0x17577`. Center id 6, 7, or 8. The stun
word `[0x373C]` must be 0; if it is not, the cooldown nibble is
cleared and the pad is skipped. The port has no enemy-stun state
(`pl.shield` is the player), so that gate is idle.

Cooldown is the low nibble of the flags byte. Non-zero ticks once per
frame while the center is still a pad: 1..14 increment, 15 stores 0.
Off the pad, the nibble is cleared and the high nibble (latch) is
kept. On a pad with cooldown 0, the bot teleports only when both
`(x >> 5) mod 12` &lt; 2 and `(y >> 5) mod 12` &lt; 2. That zeroes the
direction byte, sets the nibble to 1, and passes color `id - 5` to
the epilogue. Mis-aligned on a pad with cooldown 0 does not tick and
does not clear.

The epilogue rolls up to 10 times: `rand() % count[color]`, using the
pad table built at `0xE0A5` (each map cell 6..8 stores column+1 and
row+1). Destination is that pair times `0x180` (384). It prefers a
pad whose cell differs from `sar(x+192,5)/12` and `sar(y+352,5)/12`.
If all 10 rolls are the same cell, it still moves there. Then vx = 0,
vy = 0, x and y become the destination. The sparkle list at `0x35F0`
and the sound flag `[0x1538]` are not ported.

### What the trackbot ignores

- Moss `0x5E`–`0x60`, both feet (`0x14ECC`): animation only. Frame 4
  if vx ≥ 0, else 7. Velocity is unchanged. Not implemented.
- Ids `0x61`–`0x63`, both feet (`0x14F11`): a Borland FPU multiply of
  vx by the qword at `ds:0x15E4` (the `int 0x3B` / `int 0x38` bytes are
  the emulator prefix, not real interrupts). Not one of the four
  elements. Not implemented. The constant was not decoded.
- Enemy stun `[0x373C]`: zeroes vx, and vy unless vy is already 64,
  and suppresses teleport. Not implemented.
- Wall buttons 9–14: the thinker never reads them.

### Steel balls

Type 2 does teleport. Same pad table, same epilogue, same velocity
clear. This is original behaviour, not a port addition. Alignment is
wider than the trackbot (`0x156B9`): `(y >> 5) mod 12` &lt; 2, and
`(x >> 5) mod 12` &lt; 2 or ≥ 11. The direction byte is not cleared.
The ball thinker rewrites vx from that byte every frame (`0x1541E`):
below `0x40` and the foot probe clear → −59 (`0xFFC5`); otherwise the
upper probe clear → +59 (`0x3B`); both blocked → 0. The teleport frame
ends at vx = vy = 0, and the next think restores ±59 from the facing
byte.

The port’s ball has no direction byte. `en.st` is the bounce flash.
Bit 16 of `en.cd` holds the sign of vx across the zeroed frame, and
the next think writes −59 or +59 from it. A ball that stops for any
other reason still starts right, which is the port’s existing rule.

Also in the ball thinker, and not added here: its own belt
(`0x1555D`, probe `0x354D` only; left −62 / 27 / −40, right −27 / 62 /
40), the same `0x21`–`0x23` floor switch, and the same `0x61`–`0x63`
float scale. The player still does not trigger floor tiles
`0x21`–`0x23`; only the trackbot was wired to `lv.flip`.
