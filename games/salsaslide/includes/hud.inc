' hud.inc - the slim status bar: chef-hat lives, ROUND n, customers left.
' No live score (the score shows at the end). UI units; the bar is BAR_H
' tall and is redrawn (from the clean backdrop) only when a value changes.
const BAR_H = 12
dim hud.lv = -1, hud.left = -1, hud.rd = -1
dim string hud.mid$

' redraw the whole bar: lives, the middle text, and the count (left < 0 = none)
sub hud.draw(lv, md$, left)
  local i, x, n
  blit 0, 0, 0, 0, scr.W, BAR_H * scr.sc, PG_BG
  n = max(3, lv)
  for i = 0 to n - 1
    if i < lv then
      ui.spr(IC_HAT, IC_Y, IC_HAT_W, IC_HAT_H, 4 + i * 10, 1)
    else
      ui.spr(IC_HATD, IC_Y, IC_HATD_W, IC_HATD_H, 4 + i * 10, 1)
    end if
  next i
  txt md$, lay.aw \ 2 - txtW(md$) \ 2, 1, F_CREAM
  if left >= 0 then
    x = lay.aw - 6 - 24
    txt "X" + n2$(left), x, 1, F_CREAM
    ui.spr(IC_HEAD, IC_Y, IC_HEAD_W, IC_HEAD_H, x - 11, 1)
  end if
  scr.mark(0, 0, scr.W, BAR_H * scr.sc)
  hud.lv = lv
  hud.left = left
  hud.mid$ = md$
end sub

' during play: only when lives, the round or the count changed
sub hud.update()
  if en.lives <> hud.lv or en.total - en.served <> hud.left or en.round <> hud.rd then
    hud.rd = en.round
    hud.draw(en.lives, "ROUND " + str$(en.round + 1), en.total - en.served)
  end if
end sub

sub hud.all()
  hud.lv = -1
  hud.update()
end sub
