'
'   SALSA SLIDE - mmcore edition
'
'   A lane game in a sunny adobe cantina. Four counters, hungry customers
'   walking in from the doors on the left, and one cook at the pumps on the
'   right. Slide full plates of nachos down the counters, catch the empty
'   plates coming back, grab the tips. Every 4th round: the Salsa Shuffle.
'   Completely silent.
'
'   Run from its own folder:  RUN "salsaslide.bas"  (RUN changes into this
'   folder, so gfx/ resolves relative to it). High scores and the rounds
'   reached live in C:/.salsaslide. Modules live in includes/; the art in
'   gfx/ is generated per screen size (art/gen_game_assets.py).
'
option explicit
option default integer

const TRUE = 1
const FALSE = 0

' ---- key codes (classic CMM2 INKEY$ / KEYDOWN values)
const K_UP = 128, K_DOWN = 129, K_LEFT = 130, K_RIGHT = 131
const K_ENTER = 13, K_LF = 10, K_ESC = 27, K_BKSP = 8, K_DEL = 127

#include "includes/layouts.inc"

' ---- difficulty: rounds ramp up from the skill's own start (engine.inc)
const DF_EASY = 0, DF_NORMAL = 1, DF_HARD = 2
dim string df.name$(2) = ("EASY", "NORMAL", "HARD")
dim df.style(2) = (F_TQ, F_GOLD, F_RED)

#include "includes/screen.inc"
#include "includes/font.inc"
#include "includes/ui.inc"
#include "includes/input.inc"
#include "includes/engine.inc"
#include "includes/view.inc"
#include "includes/hud.inc"
#include "includes/highscore.inc"
#include "includes/dialogs.inc"
#include "includes/bonus.inc"
#include "includes/game.inc"
#include "includes/intro.inc"

' ------------------------------------------------------------ main
dim menuPick

on key 0   ' no ON KEY handler in menus and dialogs (they read INKEY$); gm.handlers turns it on during play
scr.init()
scr.loadAssets()
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
    game.run(menu.diff, menu.rd)
  elseif menuPick = MENU_SCORES then
    dlg.scores(-1)
  end if
loop until menuPick = MENU_QUIT

vw.clear()
scr.restore()
end

' a high-score write failed (no C: drive, read-only card): the game carries on
' without saving. Entering this handler drops the SUB frames, so continue at a
' top-level label: after the start-up save, or back at the menu.
hsSaveFail:
hs.ok = FALSE
if hs.boot then resume hsBootDone
resume mainMenu
