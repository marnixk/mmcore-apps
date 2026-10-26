' hud.inc - the side panel on the right (from screen x lay.px): logo, LEVEL,
' TIME, CHIPS LEFT, KEYS, BOOTS and the HINT box. The empty panel (labels,
' boxes, dimmed key and boot slots) is on PG_UI at x 0; only values that
' changed are redrawn (restore the box from PG_UI, draw the value, note the
' rectangle for scr.flush). All coordinates in UI art units.
dim hud.ax                         ' the panel's left edge (art x)
dim hud.lvl, hud.tl, hud.cl, hud.hint
dim hud.k(3), hud.b(3)
dim integer hud_i, hud_s

' the whole panel on PG_BACK (values forced to redraw)
sub hud.all()
  local i
  page write PG_BACK
  hud.ax = lay.px \ scr.sc
  blit 0, 0, lay.px, 0, PANEL_W * scr.sc, lay.H, PG_UI
  hud.lvl = -1 : hud.tl = -1 : hud.cl = -1 : hud.hint = -1
  for i = 0 to 3
    hud.k(i) = 0
    hud.b(i) = 0
  next i
  hud.update()
end sub

' redraw whatever changed in the engine since the last call
sub hud.update()
  if hud.lvl <> en.lvl then
    hud.lvl = en.lvl
    hud.val(P_LEVEL, hud.n2$(en.lvl + 1))
  end if
  if hud.tl <> en.tl then
    hud.tl = en.tl
    hud.val(P_TIME, str$(max(0, en.tl)))
  end if
  if hud.cl <> en.cl then
    hud.cl = en.cl
    hud.val(P_CHIPS, str$(en.cl))
  end if
  for hud_i = 0 to 3
    if hud.k(hud_i) <> en.keys(hud_i) then hud.key(hud_i)
    if hud.b(hud_i) <> en.boots(hud_i) then hud.boot(hud_i)
  next hud_i
  hud_s = (lt(en.pi) = T_HINT)
  if hud.hint <> hud_s then hud.showHint(hud_s)
end sub

function hud.n2$(n)
  hud.n2$ = right$("0" + str$(n), 2)
end function

' a value box (label at art y y0): big gold digits right-aligned
sub hud.val(y0, s$)
  local s, x
  s = scr.sc
  page write PG_BACK
  x = hud.ax + P_X
  blit P_X * s, (y0 + 10) * s, x * s, (y0 + 10) * s, P_W * s, 24 * s, PG_UI
  big s$, x + P_W - 6 - 16 * len(s$), y0 + 14
  scr.markA(x, y0 + 10, P_W, 24)
end sub

' key slot k: bright key while held, with a count when more than one
sub hud.key(k)
  local s, x
  hud.k(k) = en.keys(k)
  s = scr.sc
  page write PG_BACK
  x = hud.ax + P_X + k * P_SLOTDX
  blit (P_X + k * P_SLOTDX) * s, P_KEYS * s, x * s, P_KEYS * s, P_SLOT * s, P_SLOT * s, PG_UI
  if en.keys(k) > 0 then ui.spr(IC_KEY + 16 * k, IC_Y, 16, 16, x + 4, P_KEYS + 4)
  if en.keys(k) > 1 then txt str$(min(9, en.keys(k))), x + P_SLOT - 9, P_KEYS + P_SLOT - 9, F_WHITE
  scr.markA(x, P_KEYS, P_SLOT, P_SLOT)
end sub

sub hud.boot(k)
  local s, x
  hud.b(k) = en.boots(k)
  s = scr.sc
  page write PG_BACK
  x = hud.ax + P_X + k * P_SLOTDX
  blit (P_X + k * P_SLOTDX) * s, P_BOOTS * s, x * s, P_BOOTS * s, P_SLOT * s, P_SLOT * s, PG_UI
  if en.boots(k) then ui.spr(IC_BOOT + 16 * k, IC_Y, 16, 16, x + 4, P_BOOTS + 4)
  scr.markA(x, P_BOOTS, P_SLOT, P_SLOT)
end sub

' the level's hint while the player stands on a hint tile (lines split by |)
sub hud.showHint(sh)
  local s, x, y, h$, p
  hud.hint = sh
  s = scr.sc
  page write PG_BACK
  x = hud.ax + P_X
  blit (P_X + 2) * s, (P_HINT + 22) * s, (x + 2) * s, (P_HINT + 22) * s, (P_W - 4) * s, (P_HINTH - 24) * s, PG_UI
  if sh then
    h$ = lv.hint$(en.lvl) + "|"
    y = P_HINT + 26
    do
      p = instr(h$, "|")
      if p = 0 then exit do
      txt left$(h$, p - 1), x + 5, y, F_CYAN
      h$ = mid$(h$, p + 1)
      y = y + 11
    loop
  end if
  scr.markA(x + 2, P_HINT + 22, P_W - 4, P_HINTH - 24)
end sub
