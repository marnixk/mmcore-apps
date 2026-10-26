'
'   CRYSTAL CORE - mmcore edition
'
'   A match-3 puzzle in a crystal mine. Move the gold cursor with the arrows,
'   pick up a gem with Space or Enter and swap it with a neighbour (arrow).
'   Lines of 3 or more vanish; 4 make a striped gem, an L or T a charged gem,
'   5 a rainbow prism, and specials swapped together make bigger blasts.
'   30 levels with frost, boulders and chained gems; unlimited retries; any
'   level reached can be the start. Completely silent.
'
'   Run from its own folder:  RUN "crystalcore.bas"  (RUN changes into this
'   folder, so gfx/ resolves relative to it). High scores, the levels reached
'   and the stars live in C:/.crystalcore. Modules live in includes/; the art
'   in gfx/ and includes/layouts.inc come from art/gen_game_assets.py, the
'   levels in includes/levels.inc from tools/mklevels.py (bot-tested).
'
option explicit
option default integer

const TRUE = 1
const FALSE = 0

' ---- key codes (classic CMM2 INKEY$ / KEYDOWN values)
const K_UP = 128, K_DOWN = 129, K_LEFT = 130, K_RIGHT = 131
const K_ENTER = 13, K_LF = 10, K_ESC = 27, K_BKSP = 8, K_DEL = 127

#include "includes/layouts.inc"

' ---- skill: the moves per level come from levels.inc (EASY +30 %, HARD -15 %)
const DF_EASY = 0, DF_NORMAL = 1, DF_HARD = 2
dim string df.name$(2) = ("EASY", "NORMAL", "HARD")
dim df.style(2) = (F_GREEN, F_GOLD, F_RED)

#include "includes/screen.inc"
#include "includes/font.inc"
#include "includes/ui.inc"
#include "includes/input.inc"
#include "includes/levels.inc"
#include "includes/engine.inc"
#include "includes/board.inc"
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
bv.init()
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
