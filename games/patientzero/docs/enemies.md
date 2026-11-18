# Patient Zero - enemy types

Speeds are in **original pixels per frame at 60 fps** (the LD33 game ran on a 640 px tall field).
For the port, scale by field height / 640: at base size (360 px field) multiply by 0.5625,
on screen at 1280x720 by 1.125, at 1920x1080 by 1.69. Jerry walks at 4.0.

Original zombie rules, used as the reference below:
wander 0.5 in a fixed random direction (bounce off walls), chase 0.8 when Jerry is within 300 px
(not while he blinks after a hit), flee 1.5 when Jerry is charging above 30% within 300 px.
Any zombie inside the sneeze area is cured. Touching Jerry's feet costs him one life.

All frame names below are in `gfx/sheets.json` (sheet `sprites`, cured angels in `angels`).
Every type has walk frames for 4 directions x 4 frames (`<type>_down_0` .. `<type>_left_3`),
`<type>_hit_0/1` (normal, white flash), `<type>_cure_0/1/2` (flash, human, human with sparkles)
and its own angel `<type>_angel_0..3`. The cured human keeps the outfit, with human skin.

---

## 1. Zombie (basic, green, torn shirt) - `zombie`
**Shuffles and chases.** Exactly the original:
- wander 0.5, chase 0.8 within 300 px, flee 1.5 when Jerry charges above 30% within 300 px.
- Cured by any sneeze that reaches it. Score 1337.

## 2. Office worker (comb-over, glasses, red tie) - `office`
**Walks in straight lines like a commuter, and panics early.**
- Moves only along the grid: straight on at 0.7, turns 90 degrees when it reaches a wall or
  obstacle (left or right at random). It does not wander diagonally.
- Notices Jerry only when he is in the same tile row or column, up to 400 px away; then walks
  straight at him at 1.0 along that row or column.
- Flees early and fast: when Jerry charges above 15% (instead of 30%) within 350 px it runs at 1.8.
  So you have to start charging out of sight or catch it in a corridor.
- Score 1337.

## 3. Hoodie (purple hood, grey-green skin) - `hoodie`
**Sneaks and flanks; dashes when Jerry looks away.**
- Wander 0.4. Within 350 px it does not walk straight at Jerry: it circles him at 0.9, keeping
  150-250 px away, and works its way behind him (to the side opposite his last move direction).
- When it is behind Jerry (Jerry last moved away from it), it dashes straight at him at 2.5 for
  30 frames, then rests 60 frames before it can dash again.
- It does not flee when Jerry charges; it freezes on the spot for as long as the charge is above
  30%. A charged sneeze catches it if it is close; turning to face it also stops the dash.
- Score 2000.

## 4. Thriller dancer (red jacket with black V stripes, curly hair, white socks) - `dancer`
**Dances: Thriller shuffle on the spot, then moonwalks at Jerry.**
- Cycle, repeated while Jerry is within 400 px:
  1. Thriller shuffle (`dancer_thriller_0/1`, 12 frames each): stays in place and side-steps
     1 px left and right, 90-150 frames. Easy to hit, but other zombies use this time.
  2. Moonwalk (`dancer_moonwalk_r/l_0..3`, 8 frames each): turns its back to Jerry and slides
     backwards towards him at 1.2 for 120 frames. Faster than a normal chase, and it does not react
     to charging during the moonwalk because it is facing away.
  3. Spin (`dancer_down_0`, `_left_0`, `_up_0`, `_right_0`, 4 frames each) and a toe stand
     (`dancer_toe_r/l`, 30 frames, stands still). The toe stand is the safe moment to sneeze.
- Outside 400 px it wanders at 0.5 like a basic zombie.
- If the moonwalk would end in a wall it stops early and goes straight to the spin.
- Score 2500.

## 5. Zombie paramedic (orange jacket, reflective stripes, navy cap, cross on the back) - `medic`
**Guards the puddles and wears a face mask. The one that wants to take Jerry to hospital.**
- Picks the puddle nearest to Jerry and walks to it at 0.6, then patrols around it within 2 tiles.
  It switches to a new puddle every time Jerry gains a sneeze from a puddle.
- Chases at 0.9 when Jerry is within 200 px of it (shorter range than others, it stays near water).
- When Jerry charges above 30% within 300 px it pulls up a face mask (`medic_mask_down/left/right`)
  for 90 frames. While masked a sneeze does not cure it. After the mask comes down it cannot use
  it again for 180 frames, so the trick is to fake a charge, let go, and sneeze after the mask drops.
- Catching Jerry costs a life and drags him 2 tiles towards the nearest edge ("off to hospital").
- Score 2000.

---

## Notes for tuning
- Speeds, ranges and frame counts above are first guesses; they are meant to be easy to change per
  level (see levels.md).
- At most 2 hoodies dashing and 1 dancer moonwalking at the same time keeps it fair.
- The cure rule stays the original one: a rectangle left and right of Jerry, size set by the charge.
