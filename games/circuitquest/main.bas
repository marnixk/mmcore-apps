'
'   CIRCUIT QUEST - mmcore edition
'
'   A Chip's Challenge style puzzle game set in a silicon lab. Collect every
'   chip on the board to open the socket, then reach the exit. 20 levels, each
'   one teaching a new idea: keycards and doors, blocks and coolant, boots,
'   ice, conveyors, toggle walls, traps, teleports, tanks, bombs, hidden walls
'   and a whole zoo of monsters. Unlimited retries. Completely silent.
'
'   Run from its own folder:  RUN "main.bas"  (RUN changes into this
'   folder, so gfx/ resolves relative to it). High scores and the levels
'   reached live in C:/.circuitquest. Modules live in includes/; the art in
'   gfx/ is generated per screen size (art/gen_game_assets.py), the levels in
'   includes/levels.inc by tools/mklevels.py.
'
option explicit
option default integer

const TRUE = 1
const FALSE = 0

' ---- key codes (classic CMM2 INKEY$ / KEYDOWN values)
const K_UP = 128, K_DOWN = 129, K_LEFT = 130, K_RIGHT = 131
const K_ENTER = 13, K_LF = 10, K_ESC = 27, K_BKSP = 8, K_DEL = 127

#include "includes/layouts.inc"

' ---- difficulty: the time limit comes per level from levels.inc; the
' monsters step every df.mp ticks (50 ms each), teeth and blobs half as often
const DF_EASY = 0, DF_NORMAL = 1, DF_HARD = 2
dim string df.name$(2) = ("EASY", "NORMAL", "HARD")
dim df.style(2) = (F_CYAN, F_GOLD, F_RED)
dim df.mp(2) = (6, 4, 3)
' score per level: 10 points a second left, minus 50 per retry (never below 0)
const PTS_SEC = 10, PTS_RETRY = 50

#include "includes/screen.inc"
#include "includes/font.inc"
#include "includes/ui.inc"
#include "includes/input.inc"
#include "includes/levels.inc"
#include "includes/engine.inc"
#include "includes/view.inc"
#include "includes/hud.inc"
#include "includes/highscore.inc"
#include "includes/dialogs.inc"
#include "includes/game.inc"
#include "includes/intro.inc"

' ------------------------------------------------------------ main
dim menuPick

on key 0   ' no ON KEY handler in menus and dialogs (they read INKEY$); gm.handlers turns it on during play
scr.init()
scr.loadAssets()
lv.loadAll()
en.init()
vw.init()
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
    game.run(menu.diff, menu.lvl)
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
