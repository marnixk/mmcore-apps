' ui.inc - small drawing helpers. All coordinates are ART units; af() scales
' to screen pixels. Colours C_* come from the art palette in layouts.inc.

' filled rectangle in art units
sub af(x, y, w, h, c)
  local s
  s = scr.sc
  if w > 0 and h > 0 then box x * s, y * s, w * s, h * s, 1, c, c
end sub

' a sheet image (art sheet position sx, sy, size w x h) at art x, y; black = see-through
sub ui.spr(sx, sy, w, h, x, y)
  local s
  s = scr.sc
  blit sx * s, sy * s, x * s, y * s, w * s, h * s, PG_SHEET, 4
end sub

' highlighted row bar (menu and score table)
sub ui.bar(x, y, w)
  af x, y, w, 17, C_N3
  af x, y, w, 1, C_N4
  af x, y, 2, 17, C_C2
end sub

' difficulty pill i (sel = highlighted)
sub ui.pill(x, y, i, sel)
  ui.spr(SH_PILL_X + 62 * sel, SH_PILL_Y + 16 * i, 60, 15, x, y)
end sub

' a dialog panel PNG (gfx/<name>_<res>.png, w x h panel + 2 px glow) centred on
' the back page; alpha-0 pixels are skipped, so the dimmed scene shows round
' it. Returns the panel's left edge, its top in ui.py
dim ui.py
function ui.dialog(n$, w, h)
  local x
  x = (lay.aw - w) \ 2
  ui.py = (lay.ah - h) \ 2
  page write PG_BACK
  load png "gfx/" + n$ + "_" + lay.res$ + ".png", (x - 2) * scr.sc, (ui.py - 2) * scr.sc
  ui.dialog = x
end function
