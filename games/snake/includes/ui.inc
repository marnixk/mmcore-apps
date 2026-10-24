' ui.inc - colours and the stone UI drawn at run time (panels, pills,
' keycaps). Sizes are in screen pixels; s = pixels per art unit.

const C_K = rgb(22, 18, 30), C_K2 = rgb(34, 27, 44), C_K3 = rgb(47, 38, 57)
const C_S1 = rgb(62, 53, 70), C_S2 = rgb(95, 85, 102), C_S4 = rgb(179, 168, 180), C_S5 = rgb(216, 207, 212)
const C_Y0 = rgb(107, 74, 18), C_Y1 = rgb(185, 137, 27), C_Y2 = rgb(241, 200, 58), C_Y3 = rgb(255, 240, 140)
const C_M1 = rgb(199, 154, 34), C_N3 = rgb(100, 191, 69), C_N5 = rgb(233, 248, 166)

' filled rectangle in pixels
sub rfill(x, y, w, h, c)
  if w > 0 and h > 0 then box x, y, w, h, 1, c, c
end sub

' stone panel: dark outline, bevelled stone rim with mortar ticks, gold rivets, ink inside
sub ui.panel(x, y, w, h)
  local s = scr.sc, i
  rfill x - s, y - s, w + 2 * s, h + 2 * s, C_K
  rfill x, y, w, h, C_S2
  rfill x, y, w, s, C_S5
  rfill x, y, s, h, C_S4
  rfill x, y + h - s, w, s, C_S1
  rfill x + w - s, y, s, h, C_S1
  i = 12 * s
  do while i < w - 6 * s
    rfill x + i, y + s, s, 4 * s, C_S1
    rfill x + i + 9 * s, y + h - 5 * s, s, 4 * s, C_S1
    i = i + 18 * s
  loop
  i = 12 * s
  do while i < h - 6 * s
    rfill x + s, y + i, 4 * s, s, C_S1
    rfill x + w - 5 * s, y + i + 8 * s, 4 * s, s, C_S1
    i = i + 16 * s
  loop
  rfill x + 5 * s, y + 5 * s, w - 10 * s, h - 10 * s, C_K
  rfill x + 6 * s, y + 6 * s, w - 12 * s, h - 12 * s, C_K2
  rfill x + 6 * s, y + 6 * s, w - 12 * s, s, C_K3
  ui.rivet(x + 2 * s, y + 2 * s)
  ui.rivet(x + w - 4 * s, y + 2 * s)
  ui.rivet(x + 2 * s, y + h - 4 * s)
  ui.rivet(x + w - 4 * s, y + h - 4 * s)
end sub

sub ui.rivet(x, y)
  rfill x, y, 2 * scr.sc, 2 * scr.sc, C_Y1
  rfill x, y, scr.sc, scr.sc, C_Y3
end sub

' a panel w x h art units centred on the screen; returns its left edge, top in ui.py
dim ui.py
function ui.centre(w, h)
  local x
  x = (scr.W - w * scr.sc) \ 2
  ui.py = (scr.H - h * scr.sc) \ 2
  ui.panel(x, ui.py, w * scr.sc, h * scr.sc)
  ui.centre = x
end function

' thin divider line
sub ui.rule(x, y, w)
  rfill x, y, w, scr.sc, C_K3
end sub

' keycap with label t$ (arrows are [ ] { }); returns its width
function ui.keycap(x, y, t$)
  local s = scr.sc, w, h = 14 * scr.sc
  w = max(14, len(t$) * 8 + 6) * s
  rfill x, y, w, h, C_K
  rfill x + s, y + s, w - 2 * s, h - 2 * s, C_S1
  rfill x + s, y + s, w - 2 * s, h - 4 * s, C_S4
  rfill x + 2 * s, y + 2 * s, w - 4 * s, h - 6 * s, C_S5
  txt t$, x + (w - txtW(t$)) \ 2 + s, y + 3 * s, F_KEY
  ui.keycap = w
end function

' difficulty pill, w in pixels
sub ui.pill(x, y, w, t$, sel, r)
  local s = scr.sc
  rfill x, y, w, 15 * s, C_K
  if sel then
    rfill x + s, y + s, w - 2 * s, 13 * s, C_M1
    rfill x + 2 * s, y + 2 * s, w - 4 * s, 11 * s, C_K3
    txtC t$, x + w \ 2, y + 4 * s, r
  else
    rfill x + s, y + s, w - 2 * s, 13 * s, C_S1
    rfill x + 2 * s, y + 2 * s, w - 4 * s, 11 * s, C_K2
    txtC t$, x + w \ 2, y + 4 * s, F_DIM
  end if
end sub

' one sheet tile (index t) with its top-left at x, y; black pixels are skipped
sub ui.tile(t, x, y)
  local c = lay.cell
  blit (t mod 16) * c, (t \ 16) * c, x, y, c, c, PG_SHEET, 4
end sub
