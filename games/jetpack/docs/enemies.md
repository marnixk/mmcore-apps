# Jetpack enemies

Static reading of freeware `JETPACK.EXE` (141152 bytes, Adept 1993).
File offsets below are into that image. The enemy dispatcher is at file
`0x14493`: every tick it clears vx and vy, sets `si = sar(x, 5)` and
`[bp-2] = sar(y, 5)`, then jumps through the table at file `0x176CF`.
Spawn (`0xDE17` / `0xDE42`) stores `tile * 0x180` and does not give a
steel ball a starting velocity. The direction byte is cleared with the
other enemies (`0xE4BE` area; the port's `lv.oneEn` now leaves it 0).

Positions in `data/levels.dat` are the same bottom-right corner, in
tiles. Probes below are pixels from the sprite's top-left, which is
`pos - 384`. A tile id with bit `0x80` is read as empty. Id `>= 0x28`
(40) is a wall. Ids 1..3 are ladder / escalator.

Speeds are sub-pixels per original IRQ0 tick (about 55 ms).

## 1 Trackbot

File `0x14504`. Walks at vx ±43, climbs at vy ±33, and centres on a
ladder at vx ±32 while `(x >> 5) mod 12` is not 0. With nothing under
either foot it falls at vy 64 and vx 0. Escalator id 2 (climbing) uses
−61 / +16; id 3 uses +61 / −16. Both feet on a left belt (`0x72`–`0x74`)
set vx to −55 / +20 / −40 (against / with / standing). Both feet on a
right belt (`0x75`–`0x77`) set −20 / +55 / +40. Both feet on ice
`0x61`–`0x63` scale vx by 0.6. Floor switches `0x21`–`0x23` and
teleports 6..8 are in the same thinker; teleport alignment is
`(x >> 5) mod 12 < 2` and the same for y. Moss `0x5E`–`0x60` only
changes the frame.

The port's `en.track` follows that probe set, those speeds, and the
ladder / belt / escalator / switch / teleport tests.

## 2 Steel ball

File `0x1527A`. Its probes are not the trackbot's:

| Address | Pixel from top-left | Use |
| --- | --- | --- |
| `0x354B` → `0x354A` | (6, 11) | pad and switch id |
| `0x354C` | (12, 11) | right side |
| `0x354D` | (1, 12) | support, ice, belt |
| `0x354E` | (10, 12) | support |
| `0x354F` | (−1, 11) | left side |

If both support probes are neither a wall nor a ladder, vy = 64 and vx
stays 0 (straight fall). Otherwise the direction byte increments. When
`(dir mod 64) = 63` it wraps to 0 if `dir < 0x40`, else to `0x40`.
A wall on the left forces `dir = 0x40`. A wall on the right, with the
left clear, forces `dir = 0`. Then vx is −59 when `dir < 0x40` and the
left is clear, else +59 when the right is clear, else 0. Frame is 3
while falling; on the ground it is `(dir >> 1) mod 3`, subtracted from
2 when facing left.

Ice and the belt read `0x354D` only. Left belt: −62 / +27 / −40.
Right belt: −27 / +62 / +40. Floor switches and teleports use `0x354A`.
Teleport alignment is wider than the trackbot: y mod 12 < 2, and x mod
12 < 2 or ≥ 11 (`0x156B9`).

Level 2 ("THE MARBLE WORM") places every ball at tile x 26, over an
empty column, with the belt on row 14. The old port forced `dir = 64`
and vx 59, and it tested the trackbot's right-edge probes. Those land
off the map and count as walls, so the ball thought it was supported,
pushed right, and bounced back every tick. It never fell. `en.ball`
uses the probes above, starts at dir 0, and clamps a fall onto the
tile under the centre column instead of undoing the whole 64-unit step.

## 3 Spring

File `0x157CA`. vy is +50 (down, dir 0) or −50 (up, dir 1). Probes:
(6, 4) copied to `0x354A`, (6, −1) ceiling, (6, 8) floor. A floor wall
sets dir 10 and frame 12. A ceiling wall sets dir 20 and frame 15.
While `dir / 10 >= 1`, the spring waits: dir increments until
`dir mod 10 = 4`, then becomes `2 - (dir / 10)` (10..14 → 1, 20..24 → 0).
Other dir values leave vy at 0. It also tests the floor switch and the
pad on `0x354A`. The port used to reverse instantly and start upward.
`en.spring` starts at dir 0 and uses this pause.

## 4 Missile

File `0x15B77`. The speed immediate is 64 (`0x15DCB`). The heading is a
4-way state (up, right, down, left). The port keeps speed 64 and turns
clockwise when a solid stops that axis. Wall-probe details of the turn
are not fully decoded; the visible behaviour is a constant-speed
orthogonal patrol that turns when blocked.

## 5 Spikes

File `0x1601B`. Unit signs on both axes, then `imul 20` at `0x16422`.
A wall is id `>= 40`. No gravity. Each axis reverses on its own when
that axis hits a wall, so the result is a diagonal ping-pong. The port
already does this in the type 5 branch (`en.blocked`, negate the axis
that hit). Speed stays ±20.

## 6 Flitzer

File `0x16638`. After the probe block, speed is 11 (`mov dx, 0xB` at
`0x16AF6`) on both axes. `call 0x0:0x1581` is the rand helper (`0x16A49`).
When the probe flag `[bp-6]` is set, `rand mod 10 = 0` clears the
direction byte; otherwise the byte steps by ±1 and is wrapped into
1..8. The port used a fixed 8-frame cycle. It now keeps speed 11 and
steps that 1..8 heading with the same `rand mod 10` / ±1 rule, once
per eight ticks so a single heading is visible, and still reverses a
step that enters a solid.

## 7 Homer

File `0x16D07`. vx and vy are ±4 toward the player, one axis at a time.
The move is not tested against walls. The port matches that (speed 4,
`en.move` with solids off).

## 8 Batbot

File `0x16DFA`. Horizontal immediates are 10 (`0x1730A`) and the
vertical cap is 17 (`0x17320`). The port moves at vx ±10 toward the
player, adds 5 to vy up to 17 while airborne, and hops (vy −50) every
48 ticks. The −50 hop is not a single immediate in this function; the
48-tick hop is the port's reading of the jump. Conveyors, ladders and
teleports are not in this function.

## What a belt carries

Only the player (`0x10582`), the trackbot (both feet, `0x14F58`) and
the steel ball (`0x354D`, `0x1555D`) have belt code. Spikes, springs,
missiles, flitzers, homers and bats do not.
