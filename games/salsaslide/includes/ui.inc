' ui.inc - small drawing helpers. All coordinates are UI units; af() scales
' to screen pixels. Colours C_* come from the art palette in layouts.inc.

' filled rectangle in UI units
sub af(x, y, w, h, c)
  local s
  s = scr.sc
  if w > 0 and h > 0 then box x * s, y * s, w * s, h * s, 1, c, c
end sub

' an icon from the UI block (UI position sx, sy inside the block, size w x h)
' at UI x, y; black = see-through
sub ui.spr(sx, sy, w, h, x, y)
  local s
  s = scr.sc
  blit sx * s, lay.uiy + sy * s, x * s, y * s, w * s, h * s, PG_SHK, 4
end sub

' highlighted row bar (menu and score table): a turquoise glazed tile strip
sub ui.bar(x, y, w)
  af x, y, w, 15, C_PAN0
  af x, y, w, 1, C_TQ
  af x, y + 14, w, 1, C_TQ
end sub

' a dialog panel PNG (gfx/<name>_<res>.png: the w x h panel plus a margin of
' 10 left and right, 2 at the top, 6 at the bottom) centred on the back page;
' alpha-0 pixels are skipped, so the dimmed scene shows round it.
' Returns the panel's left edge, its top in ui.py
dim ui.py
function ui.dialog(n$, w, h)
  local x
  x = (lay.aw - w) \ 2
  ui.py = (lay.ah - h) \ 2
  page write PG_BACK
  load png "gfx/" + n$ + "_" + lay.res$ + ".png", (x - 10) * scr.sc, (ui.py - 2) * scr.sc
  ui.dialog = x
end function

' two-digit number
function n2$(v)
  n2$ = right$("0" + str$(v), 2)
end function
