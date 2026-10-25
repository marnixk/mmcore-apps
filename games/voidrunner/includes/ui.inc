' ui.inc - the steel-and-glass UI drawn at run time (panels, pills, keycaps,
' sheet sprites). All coordinates are ART units; af() scales to screen pixels.
' Colours C_* come from the art palette in layouts.inc.

' filled rectangle in art units
sub af(x, y, w, h, c)
  local s
  s = scr.sc
  if w > 0 and h > 0 then box x * s, y * s, w * s, h * s, 1, c, c
end sub

' steel panel: ink outline, bevelled steel frame with rivets, dark glass inside
sub ui.panel(x, y, w, h)
  local xx
  af x - 1, y - 1, w + 2, h + 2, C_K
  af x, y, w, h, C_M2
  af x, y, w, 1, C_M4
  af x, y, 1, h, C_M3
  af x, y + h - 1, w, 1, C_M0
  af x + w - 1, y, 1, h, C_M0
  af x + 1, y + 1, w - 2, 1, C_M3
  for xx = x + 14 to x + w - 11 step 20
    af xx, y + 2, 6, 2, C_M1
    af xx, y + h - 4, 6, 2, C_M1
  next xx
  af x + 5, y + 5, w - 10, h - 10, C_K
  af x + 6, y + 6, w - 12, h - 12, C_WB1
  af x + 6, y + 6, w - 12, 1, C_WB3
  af x + 6, y + 6, 1, h - 12, C_WB2
  ui.rivet(x + 2, y + 2)
  ui.rivet(x + w - 4, y + 2)
  ui.rivet(x + 2, y + h - 4)
  ui.rivet(x + w - 4, y + h - 4)
end sub

sub ui.rivet(x, y)
  af x, y, 2, 2, C_M4
  af x, y, 1, 1, C_M5
  af x + 1, y + 1, 1, 1, C_M1
end sub

' a panel w x h art units centred on the screen; returns its left edge, top in ui.py
dim ui.py
function ui.centre(w, h)
  local x
  x = (lay.aw - w) \ 2
  ui.py = (lay.ah - h) \ 2
  ui.panel(x, ui.py, w, h)
  ui.centre = x
end function

' thin divider line
sub ui.rule(x, y, w)
  af x, y, w, 1, C_WB3
end sub

' highlighted row bar
sub ui.bar(x, y, w)
  af x, y, w, 17, C_WB3
  af x, y, w, 1, C_WB4
end sub

' keycap with label t$ (arrows are [ ] { }); returns its width
function ui.keycap(x, y, t$)
  local w
  w = max(14, len(t$) * 8 + 6)
  af x, y, w, 14, C_K
  af x + 1, y + 1, w - 2, 12, C_M3
  af x + 1, y + 1, w - 2, 10, C_M4
  af x + 2, y + 2, w - 4, 1, C_M5
  txt t$, x + (w - txtW(t$)) \ 2 + 1, y + 3, F_KEY
  ui.keycap = w
end function

' difficulty pill
sub ui.pill(x, y, w, t$, sel, r)
  af x, y, w, 15, C_K
  if sel then
    af x + 1, y + 1, w - 2, 13, C_C2
    af x + 2, y + 2, w - 4, 11, C_WB3
    txtC t$, x + w \ 2, y + 4, r
  else
    af x + 1, y + 1, w - 2, 13, C_M1
    af x + 2, y + 2, w - 4, 11, C_WB1
    txtC t$, x + w \ 2, y + 4, F_DIM
  end if
end sub

' a sheet sprite (art sheet position sx, sy, size w x h) at art x, y; black = see-through
sub ui.spr(sx, sy, w, h, x, y)
  local s
  s = scr.sc
  bl sx * s, sy * s, x * s, y * s, w * s, h * s, PG_SHEET, 1
end sub

' opaque variant: only for art drawn onto black (black stays transparent later)
sub ui.sprO(sx, sy, w, h, x, y)
  local s
  s = scr.sc
  bl(sx * s, sy * s, x * s, y * s, w * s, h * s, PG_SHEET, 0)
end sub

sub ui.gem(x, y)
  ui.spr(SP_CRYSTAL_0_X, SP_CRYSTAL_0_Y, 16, 16, x, y)
end sub
