'
'   TETRIS - mmcore edition
'
'   Run from its own folder:  RUN "main.bas"  (RUN changes into this folder,
'   so gfx/ resolves relative to it). High scores live in C:/.tetris.
'
'   Modules live in includes/ and are pulled in with #include below.
'   The art in gfx/ is generated per screen size (see gfx/layout.txt).
'
option explicit
option default integer

const TRUE = 1
const FALSE = 0

' ---- key codes (classic CMM2 INKEY$ / KEYDOWN values)
const K_UP = 128, K_DOWN = 129, K_LEFT = 130, K_RIGHT = 131
const K_ENTER = 13, K_LF = 10, K_ESC = 27, K_BKSP = 8, K_DEL = 127, K_SPACE = 32

' ---- difficulty: changes how many lines it takes to go up a level
const LV_EASY = 0, LV_MEDIUM = 1, LV_HARD = 2
dim string lv.name$(2) = ("EASY", "MEDIUM", "HARD")
dim lv.lines(2) = (15, 10, 5)

#include "includes/layouts.inc"
#include "includes/screen.inc"
#include "includes/font.inc"
#include "includes/ui.inc"
#include "includes/input.inc"
#include "includes/music.inc"
#include "includes/pieces.inc"
#include "includes/board.inc"
#include "includes/highscore.inc"
#include "includes/dialogs.inc"
#include "includes/game.inc"
#include "includes/intro.inc"

' ------------------------------------------------------------ main
dim menuPick

randomize timer
scr.init()
scr.loadAssets()
pc.init()
hs.load()
' write the table back once at start: this moves an old tetris.hi over to C: and
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
    game.run(menu.level)
  elseif menuPick = MENU_SCORES then
    dlg.scores(-1, -1)
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
