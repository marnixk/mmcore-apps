'
'   SNAKE - mmcore edition
'
'   Run from its own folder:  RUN "snake.bas"  (RUN changes into this folder,
'   so gfx/ and the high score file snake.hi resolve relative to it).
'
'   Steer with the arrow keys, eat 10 fruits to clear a level, 10 layouts,
'   then they loop faster. Completely silent (plays nicely with JUKE).
'   Modules live in includes/; the art in gfx/ is generated per screen size.
'
option explicit
option default integer

const TRUE = 1
const FALSE = 0

' ---- key codes (classic CMM2 INKEY$ / KEYDOWN values)
const K_UP = 128, K_DOWN = 129, K_LEFT = 130, K_RIGHT = 131
const K_ENTER = 13, K_LF = 10, K_ESC = 27, K_BKSP = 8, K_DEL = 127, K_SPACE = 32

' ---- difficulty: starting step time (ms) and how much faster each level gets
const DF_EASY = 0, DF_NORMAL = 1, DF_HARD = 2
dim string df.name$(2) = ("EASY", "NORMAL", "HARD")
dim df.row(2) = (2, 1, 5)
dim df.start(2) = (160, 125, 100)
dim df.faster(2) = (6, 7, 6)
const SPEED_MIN = 45

#include "includes/layouts.inc"
#include "includes/levels.inc"
#include "includes/screen.inc"
#include "includes/font.inc"
#include "includes/ui.inc"
#include "includes/input.inc"
#include "includes/music.inc"
#include "includes/field.inc"
#include "includes/highscore.inc"
#include "includes/dialogs.inc"
#include "includes/game.inc"
#include "includes/intro.inc"

' ------------------------------------------------------------ main
dim menuPick

randomize timer
scr.init()
scr.loadAssets()
fld.readLevels()
hs.load()
music.start()
key.start()

do
  menuPick = intro.run()
  if menuPick = MENU_START then
    game.run(menu.diff)
  elseif menuPick = MENU_SCORES then
    dlg.scores(-1)
  end if
loop until menuPick = MENU_QUIT

key.stop()
music.stop()
scr.restore()
end
