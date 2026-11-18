' Patient Zero - Bless You
' Run from this folder: RUN "main.bas"
' High scores: C:/.patientzero
' Silent.

option explicit
option default integer

#include "includes/testhook.inc"
#include "includes/screen.inc"
#include "includes/atlas.inc"
#include "includes/ui.inc"
#include "includes/input.inc"
#include "includes/highscore.inc"
#include "includes/particles.inc"
#include "includes/levels.inc"
#include "includes/enemies.inc"
#include "includes/dialogs.inc"
#include "includes/intro.inc"
#include "includes/game.inc"

dim menu.leave

randomize timer
on key 0
scr.init
scr.loadAssets
fr.init
lv.init
en.init
intro.init
hs.load
hs.boot = TRUE
on error goto hsSaveFail
hs.save
hsBootDone:
on error
hs.boot = FALSE

if TH_BENCH = 1 then
  gm.bench
  scr.shutdown
  end
end if

do
  intro.show
  if menu.leave then
    exit do
  end if
  gm.run
loop
scr.shutdown
end

hsSaveFail:
  hs.ok = FALSE
  if hs.boot then resume hsBootDone
  resume next
