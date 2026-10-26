' ui.inc - small drawing helpers. All coordinates are UI ART units; af()
' scales to screen pixels. Colours C_* come from the art palette in layouts.inc.

' filled rectangle in art units
sub af(x, y, w, h, c)
  local s
  s = scr.sc
  if w > 0 and h > 0 then box x * s, y * s, w * s, h * s, 1, c, c
end sub

' an image from the UI block (art position sx, sy, size w x h) at art x, y;
' black = see-through
sub ui.spr(sx, sy, w, h, x, y)
  local s
  s = scr.sc
  blit sx * s, sy * s, x * s, y * s, w * s, h * s, PG_UI, 4
end sub

' copy an area (art units) from page pg to the same place on the back page
sub ui.restore(x, y, w, h, pg)
  local s
  s = scr.sc
  blit x * s, y * s, x * s, y * s, w * s, h * s, pg
end sub

' highlighted row bar (menus and the score table): a gold-lit slot
sub ui.bar(x, y, w)
  af x, y, w, 17, C_AU1
  af x, y, w, 1, C_AU3
  af x, y + 16, w, 1, C_AU0
end sub

' a dialog panel PNG (gfx/<name>_<res>.png: the w x h panel plus a margin of
' 10 px left and right, 2 px at the top, 6 px at the bottom) centred on the
' back page; alpha-0 pixels are skipped, so the dimmed scene shows round it.
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

function n2$(n)
  n2$ = right$("0" + str$(n), 2)
end function
