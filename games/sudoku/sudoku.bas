'
'   SUDOKU GROVE - a calm forest sudoku for mmcore
'
'   Run from its own folder:  RUN "sudoku.bas"  (RUN changes into this folder,
'   so gfx/ and sfx/ resolve relative to it). High scores live in C:/.sudoku.
'
'   Modules live in includes/ and are pulled in with #include below.
'
option explicit
option default integer

const TRUE = 1
const FALSE = 0

' ---- key codes (classic CMM2 INKEY$ values)
const K_UP = 128, K_DOWN = 129, K_LEFT = 130, K_RIGHT = 131
const K_ENTER = 13, K_LF = 10, K_ESC = 27, K_BKSP = 8, K_DEL = 127

' ---- levels
const LV_EASY = 0, LV_MEDIUM = 1, LV_HIGH = 2
dim string lv.name$(2) = ("EASY", "MEDIUM", "HIGH")

#include "includes/screen.inc"
#include "includes/font.inc"
#include "includes/input.inc"
#include "includes/music.inc"
#include "includes/puzzles.inc"
#include "includes/generator.inc"
#include "includes/board.inc"
#include "includes/highscore.inc"
#include "includes/dialogs.inc"
#include "includes/game.inc"
#include "includes/intro.inc"

' ------------------------------------------------------------ main
dim menuPick

randomize
scr.init()
scr.loadAssets()
bank.init()
hs.load()
' write the table back once at start: this moves an old sudoku.hi over to C: and
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
    dlg.scoreBrowser(menu.level)
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
