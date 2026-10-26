' hud.inc - the split panels: LEVEL and MOVES LEFT on the left, GOALS (icon,
' name, count left or a tick) and the PROGRESS bar on the right. The empty
' panels are in the clean scene (PG_CLEAN); a value that changed is redrawn
' by restoring its area from there, drawing it and marking it. Art units.
dim hud.lvl, hud.mv, hud.pg, hud.gl(2)
dim string hud.cname$(5) length 8
hud.cname$(0) = "RUBY" : hud.cname$(1) = "AMBER" : hud.cname$(2) = "CITRINE"
hud.cname$(3) = "EMERALD" : hud.cname$(4) = "SAPPHIRE" : hud.cname$(5) = "AMETHYST"

' the goal boxes (they differ per level) on the clean scene, then all values
sub hud.all()
  local k, x, y
  page write PG_CLEAN
  x = lay.rpx + 6
  for k = 0 to en.ng - 1
    y = GOAL_Y0 + k * GOAL_DY
    ui.spr(0, GB_Y, lay.gw, GOAL_H, x, y)
    ui.spr(hud.icon(k), IC_Y, 20, 20, x + 3, y + 3)
    if lay.gn then txt hud.gname$(k), x + 27, y + 9, F_LABEL
  next k
  page write PG_BACK
  ui.restore(lay.rpx, 0, lay.pw, lay.ah, PG_CLEAN)
  ui.restore(0, 0, lay.pw, lay.ah, PG_CLEAN)
  hud.lvl = -1 : hud.mv = -1 : hud.pg = -1
  for k = 0 to 2
    hud.gl(k) = -1
  next k
  hud.update()
end sub

' the goal's name (as in the mockup: FROST, EMERALD, BOULDER, CHAINS)
function hud.gname$(k)
  select case en.gt(k)
    case G_COL
      hud.gname$ = hud.cname$(en.gcol(k))
    case G_FROST
      hud.gname$ = "FROST"
    case G_BOULDER
      hud.gname$ = "BOULDER"
    case else
      hud.gname$ = "CHAINS"
  end select
end function

function hud.icon(k)
  select case en.gt(k)
    case G_COL
      hud.icon = IC_GEM + 20 * en.gcol(k)
    case G_FROST
      hud.icon = IC_FROST
    case G_BOULDER
      hud.icon = IC_STONE
    case else
      hud.icon = IC_CHAIN
  end select
end function

sub hud.update()
  local k, p
  if hud.lvl <> en.lvl then
    hud.lvl = en.lvl
    hud.levelVal()
  end if
  if hud.mv <> en.moves then
    hud.mv = en.moves
    hud.movesVal()
  end if
  for k = 0 to en.ng - 1
    if hud.gl(k) <> max(0, en.gl(k)) then
      hud.gl(k) = max(0, en.gl(k))
      hud.goalVal(k)
    end if
  next k
  p = en.progress()
  if p <> hud.pg then
    hud.pg = p
    hud.bar()
  end if
end sub

sub hud.levelVal()
  local x, w
  page write PG_BACK
  x = 10
  w = lay.pw - 16
  ui.restore(x + w - 40, lay.lvy - 5, 42, 20, PG_CLEAN)
  bigR n2$(en.lvl + 1), x + w, lay.lvy - 4
  scr.markA(x + w - 40, lay.lvy - 5, 42, 20)
end sub

sub hud.movesVal()
  local w
  page write PG_BACK
  w = lay.pw - 16
  ui.restore(12, lay.mvy + 2, w - 4, 30, PG_CLEAN)
  hugeC str$(max(0, en.moves)), 10 + w \ 2, lay.mvy + 4
  scr.markA(12, lay.mvy + 2, w - 4, 30)
end sub

' count left (big digits) or a tick when done
sub hud.goalVal(k)
  local x, y, w, s$
  page write PG_BACK
  x = lay.rpx + 6
  y = GOAL_Y0 + k * GOAL_DY
  w = lay.gw
  ui.restore(x + w - 54, y + 3, 51, 20, PG_CLEAN)
  if hud.gl(k) <= 0 then
    ui.spr(IC_TICK, IC_Y, 20, 20, x + w - 22, y + 3)
  else
    s$ = str$(hud.gl(k))
    bigR s$, x + w - 5, y + 5
  end if
  scr.markA(x + w - 54, y + 3, 51, 20)
end sub

sub hud.bar()
  local x, y, w, fw
  page write PG_BACK
  x = lay.rpx + 6
  y = lay.pgy
  w = lay.gw
  ui.restore(x, y, w, 12, PG_CLEAN)
  fw = ((w - 6) * hud.pg) \ 1000
  if fw > 0 then
    af x + 3, y + 3, fw, 6, C_G2
    af x + 3, y + 7, fw, 2, C_G1
    af x + 3, y + 3, fw, 1, C_G3
    af x + 3 + (w - 6) \ 3, y + 2, 1, 8, C_K
    af x + 3 + (2 * (w - 6)) \ 3, y + 2, 1, 8, C_K
  end if
  scr.markA(x, y, w, 12)
end sub
