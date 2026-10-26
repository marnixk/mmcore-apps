'
'   TRONIX - mmcore edition
'
'   Xonix on the Grid: ride your light cycle off the claimed land, lay a wall
'   across the open grid and close it to claim every region the orbs are not
'   in. Claim the target share of the grid to clear the level. Orbs bounce
'   through the open grid and cut your wall; sentinels patrol the claimed land.
'   Completely silent (plays nicely with JUKE).
'
'   Run from its own folder:  RUN "tronix.bas"  (RUN changes into this folder,
'   so gfx/ resolves relative to it). High scores live in C:/.tronix.
'   Modules live in includes/; the art in gfx/ is generated per screen size.
'
option explicit
option default integer

const TRUE = 1
const FALSE = 0

' ---- key codes (classic CMM2 INKEY$ / KEYDOWN values)
const K_UP = 128, K_DOWN = 129, K_LEFT = 130, K_RIGHT = 131
const K_ENTER = 13, K_LF = 10, K_ESC = 27, K_BKSP = 8, K_DEL = 127, K_SPACE = 32

#include "includes/layouts.inc"

' ---- difficulty (see game.inc for how the numbers are used)
'   orbs on level 1, one more orb every n levels (max ORB_MAX), the levels the
'   2nd and 3rd sentinel join, orb step time (ms per cell) on level 1, orbs
'   speed up by ORB_FASTER ms every n levels down to a minimum
const DF_EASY = 0, DF_NORMAL = 1, DF_HARD = 2
dim string df.name$(2) = ("EASY", "NORMAL", "HARD")
dim df.style(2) = (F_CYAN, F_BLUE, F_ORANGE)
dim df.orbs0(2) = (1, 2, 3)
dim df.orbEvery(2) = (2, 1, 1)
dim df.en2(2) = (5, 3, 2)
dim df.en3(2) = (9, 6, 4)
dim df.orbMs(2) = (95, 80, 68)
dim df.fastEvery(2) = (4, 3, 2)
dim df.orbMin(2) = (68, 56, 46)
const ORB_MAX = 8, ORB_FASTER = 6
' sentinels step this much slower than the orbs
const EN_SLOWER = 20
' player: ms per cell; on land a held arrow repeats after LAND_DELAY
const PLAYER_MS = 50, LAND_DELAY = 150
' claim target (%), raised from level TARGET2_LEVEL on
const TARGET1 = 75, TARGET2 = 80, TARGET2_LEVEL = 8
' score: PTS_PER_PCT per 1% of the grid claimed, OVER_BONUS per % over the
' target and LIFE_BONUS per life left at the level clear
const PTS_PER_PCT = 30, OVER_BONUS = 100, LIFE_BONUS = 100
' lives: 3 to start, one more every EXTRA_EVERY levels cleared (max LIVES_MAX)
const LIVES0 = 3, EXTRA_EVERY = 3, LIVES_MAX = 6

#include "includes/screen.inc"
#include "includes/font.inc"
#include "includes/ui.inc"
#include "includes/input.inc"
#include "includes/music.inc"
#include "includes/field.inc"
#include "includes/actors.inc"
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
fld.planes()
hs.load()
' write the table once at start: finds out early whether C: can be written
' (if not, hsSaveFail turns saving off and the game runs without saving)
hs.boot = TRUE
hs.save()
hsBootDone:
on error
hs.boot = FALSE
music.start()

mainMenu:
on error
do
  menuPick = intro.run()
  if menuPick = MENU_START then
    game.run(menu.diff)
  elseif menuPick = MENU_SCORES then
    dlg.scores(-1)
  end if
loop until menuPick = MENU_QUIT

music.stop()
scr.restore()
end

' a high-score write failed (no C: drive, read-only card): the game carries on
' without saving. Entering this handler drops the SUB frames, so continue at a
' top-level label: after the start-up save, or back at the menu.
hsSaveFail:
hs.ok = FALSE
if hs.boot then resume hsBootDone
resume mainMenu
