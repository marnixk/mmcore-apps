'
'   CORE CAVING - mmcore edition
'
'   A Boulder Dash style digging game: tunnel through the earth of 16 caves
'   (A to P, plus a bonus cave after every fourth), collect enough diamonds to
'   open the exit and get out before the time runs out. Boulders and diamonds
'   fall and roll, fireflies and butterflies patrol their tunnels, magic walls
'   turn rock into gems and amoeba grows. Completely silent (plays nicely with
'   JUKE).
'
'   Run from its own folder:  RUN "corecaving.bas"  (RUN changes into this
'   folder, so gfx/ resolves relative to it). High scores and the caves
'   reached live in C:/.corecaving. Modules live in includes/; the art in gfx/
'   is generated per screen size.
'
option explicit
option default integer

const TRUE = 1
const FALSE = 0

' ---- key codes (classic CMM2 INKEY$ / KEYDOWN values)
const K_UP = 128, K_DOWN = 129, K_LEFT = 130, K_RIGHT = 131
const K_ENTER = 13, K_LF = 10, K_ESC = 27, K_BKSP = 8, K_DEL = 127, K_SPACE = 32

#include "includes/layouts.inc"

' ---- difficulty (how the numbers are used: cave.inc)
'   cave frame (ms), diamonds needed and time (% of the cave's NORMAL values),
'   amoeba growth chance (%); on EASY enemies rest every 4th frame
const DF_EASY = 0, DF_NORMAL = 1, DF_HARD = 2
dim string df.name$(2) = ("EASY", "NORMAL", "HARD")
dim df.style(2) = (F_GEM, F_GOLD, F_WARM)
dim df.tick(2) = (125, 110, 100)
dim df.need(2) = (80, 100, 120)
dim df.time(2) = (130, 100, 85)
dim df.amoeba(2) = (70, 100, 140)
' lives: 3 to start, one more for each bonus cave cleared (max LIVES_MAX)
const LIVES0 = 3, LIVES_MAX = 6
' score: per diamond (cave values), plus TIME_PTS per second left
const TIME_PTS = 5

#include "includes/screen.inc"
#include "includes/font.inc"
#include "includes/ui.inc"
#include "includes/input.inc"
#include "includes/caves.inc"
#include "includes/cave.inc"
#include "includes/draw.inc"
#include "includes/highscore.inc"
#include "includes/dialogs.inc"
#include "includes/game.inc"
#include "includes/intro.inc"

' ------------------------------------------------------------ main
dim menuPick

randomize timer
on key 0   ' no ON KEY handler in menus and dialogs (they read INKEY$); gm.handlers turns it on during play
scr.init()
scr.loadAssets()
cv.loadAll()
drw.init()
hs.load()
' write the table once at start: finds out early whether C: can be written
' (if not, hsSaveFail turns saving off and the game runs without saving)
hs.boot = TRUE
hs.save()
hsBootDone:
on error
hs.boot = FALSE

mainMenu:
on error
do
  menuPick = intro.run()
  if menuPick = MENU_START then
    game.run(menu.diff, menu.cave)
  elseif menuPick = MENU_SCORES then
    dlg.scores(-1)
  end if
loop until menuPick = MENU_QUIT

scr.restore()
end

' a high-score write failed (no C: drive, read-only card): the game carries on
' without saving. Entering this handler drops the SUB frames, so continue at a
' top-level label: after the start-up save, or back at the menu.
hsSaveFail:
hs.ok = FALSE
if hs.boot then resume hsBootDone
resume mainMenu
