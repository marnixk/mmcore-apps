'
'   JETPACK - a port for mmcore
'
'   Original game by Adept Software / Adam Pedersen.
'   Run from this folder:  RUN "main.bas"
'   High scores: C:/.jetpack
'   Silent. Fuel, gems and lives sit in the gutter (fuel is gameplay).
'
option explicit
option default integer

const TRUE = 1
const FALSE = 0

const K_UP = 128, K_DOWN = 129, K_LEFT = 130, K_RIGHT = 131
const K_ENTER = 10, K_LF = 13, K_ESC = 27, K_BKSP = 8, K_DEL = 127, K_SPACE = 32
const K_INS = 132, K_Z = 122, K_0 = 48

#include "includes/layouts.inc"
#include "includes/testhook.inc"
#include "includes/tiles.inc"
#include "includes/screen.inc"
#include "includes/input.inc"
#include "includes/scores.inc"
#include "includes/play.inc"
#include "includes/title.inc"

dim menu.leave

randomize timer
on key 0
scr.init()
scr.loadAssets()
tf.init()
hs.load()
hs.boot = TRUE
on error goto hsSaveFail
hs.save()
hsBootDone:
on error
hs.boot = FALSE

do
  title.show()
  if menu.leave then
    exit do
  end if
  gm.run()
loop
scr.shutdown()
end

hsSaveFail:
  hs.ok = FALSE
  if hs.boot then resume hsBootDone
  resume next
