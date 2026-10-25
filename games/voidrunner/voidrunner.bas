'
'   VOID RUNNER - mmcore edition
'
'   Thread the void: one tap of Space (or Up / W) fires the thruster, gravity
'   does the rest. Fly through the gaps between the asteroid pillars and the
'   laser gates, grab the optional crystals, and the scroll speeds up every 10
'   gaps. Completely silent (plays nicely with JUKE).
'
'   Run from its own folder:  RUN "voidrunner.bas"  (RUN changes into this
'   folder, so gfx/ resolves relative to it). High scores live in C:/.voidrunner.
'   Modules live in includes/; the art in gfx/ is generated per screen size.
'
option explicit
option default integer

const TRUE = 1
const FALSE = 0

' ---- key codes (classic CMM2 INKEY$ / KEYDOWN values)
const K_UP = 128, K_DOWN = 129, K_LEFT = 130, K_RIGHT = 131
const K_ENTER = 13, K_LF = 10, K_ESC = 27, K_BKSP = 8, K_DEL = 127, K_SPACE = 32

' ---- difficulty (art pixels per 20 ms tick; see game.inc)
'   start / step / max scroll speed, gap height, column spacing, max gap shift
const DF_EASY = 0, DF_NORMAL = 1, DF_HARD = 2
dim string df.name$(2) = ("EASY", "NORMAL", "HARD")
dim df.row(2) = (2, 1, 5)
dim float df.v0(2) = (1.6, 2.0, 2.4)
dim float df.dv(2) = (0.15, 0.2, 0.2)
dim float df.vmax(2) = (2.8, 3.4, 4.0)
dim df.gap(2) = (116, 100, 88)
dim df.space(2) = (220, 200, 190)
dim df.shift(2) = (110, 130, 150)
' the scroll speeds up every RAMP_GAPS gaps passed, until df.vmax
const RAMP_GAPS = 10

#include "includes/layouts.inc"
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
on key 0   ' no ON KEY handler: every loop reads INKEY$ itself
scr.init()
scr.loadAssets()
gm.sprites()
hs.load()
' write the table back once at start: this moves an old voidrunner.hi over to C: and
' finds out early whether C: can be written (if not, hsSaveFail turns saving off)
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

sprite close all
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
