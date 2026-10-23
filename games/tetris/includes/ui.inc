' ui.inc - colours and the lacquer UI drawn at run time (dialog panels,
' ribbons, pills, keycaps). Sizes are art units; s = pixels per art unit.

const C_INK = rgb(14, 10, 20), C_INK2 = rgb(26, 18, 36)
const C_G1 = rgb(138, 90, 24), C_G2 = rgb(200, 138, 32), C_G3 = rgb(240, 192, 48), C_G4 = rgb(255, 230, 128)
const C_R0 = rgb(42, 10, 20), C_R1 = rgb(83, 16, 32), C_R2 = rgb(130, 26, 42), C_R3 = rgb(176, 36, 46), C_R4 = rgb(216, 58, 58)
const C_W0 = rgb(138, 150, 192), C_W1 = rgb(180, 190, 220), C_W3 = rgb(246, 242, 228), C_W4 = rgb(255, 255, 255)

' filled rectangle in pixels
sub rfill(x, y, w, h, c)
  if w > 0 and h > 0 then box x, y, w, h, 1, c, c
end sub

' lacquer panel: ink outline, gold line, red band with gold dots, gold line, maroon
sub ui.panel(x, y, w, h)
  local s = scr.sc, i
  rfill x, y, w, h, C_INK
  rfill x + s, y + s, w - 2 * s, h - 2 * s, C_G3
  rfill x + 2 * s, y + 2 * s, w - 4 * s, h - 4 * s, C_R3
  rfill x + 2 * s, y + 2 * s, w - 4 * s, s, C_R4
  rfill x + 2 * s, y + 2 * s, s, h - 4 * s, C_R4
  rfill x + 2 * s, y + h - 3 * s, w - 4 * s, s, C_R2
  rfill x + w - 3 * s, y + 2 * s, s, h - 4 * s, C_R2
  rfill x + 4 * s, y + 4 * s, w - 8 * s, h - 8 * s, C_G1
  rfill x + 5 * s, y + 5 * s, w - 10 * s, h - 10 * s, C_R0
  rfill x + 5 * s, y + 5 * s, w - 10 * s, s, C_INK
  rfill x + 5 * s, y + 5 * s, s, h - 10 * s, C_INK
  i = 10 * s
  do while i < w - 8 * s
    rfill x + i, y + 3 * s, s, s, C_G4
    rfill x + i, y + h - 4 * s, s, s, C_G3
    i = i + 8 * s
  loop
  i = 10 * s
  do while i < h - 8 * s
    rfill x + 3 * s, y + i, s, s, C_G4
    rfill x + w - 4 * s, y + i, s, s, C_G3
    i = i + 8 * s
  loop
  rfill x + 3 * s, y + 3 * s, s, s, C_G4
  rfill x + w - 4 * s, y + 3 * s, s, s, C_G4
  rfill x + 3 * s, y + h - 4 * s, s, s, C_G4
  rfill x + w - 4 * s, y + h - 4 * s, s, s, C_G4
end sub

' heading ribbon w x h (pixels) with centred small text
sub ui.ribbon(x, y, w, h, t$, r)
  local s = scr.sc
  rfill x, y, w, h, C_INK
  rfill x + s, y + s, w - 2 * s, h - 2 * s, C_G2
  rfill x + 2 * s, y + 2 * s, w - 4 * s, h - 4 * s, C_R3
  rfill x + 2 * s, y + 2 * s, w - 4 * s, s, C_R4
  rfill x + 2 * s, y + h - 3 * s, w - 4 * s, s, C_R2
  txtC t$, x + w \ 2, y + (h - 8 * s) \ 2 + s, r
end sub

' thin gold divider with a centre stud
sub ui.divider(x, y, w)
  local s = scr.sc
  rfill x, y, w, s, C_G1
  rfill x, y + s, w, s, C_INK
  rfill x + (w \ 2) - s, y, 3 * s, s, C_G3
  rfill x + (w \ 2), y - s, s, 3 * s, C_G3
end sub

' difficulty pill
sub ui.pill(x, y, w, h, t$, sel, r)
  local s = scr.sc
  rfill x, y, w, h, C_INK
  if sel then
    rfill x + s, y + s, w - 2 * s, h - 2 * s, C_G3
    rfill x + 2 * s, y + 2 * s, w - 4 * s, h - 4 * s, C_R3
    rfill x + 2 * s, y + 2 * s, w - 4 * s, s, C_R4
    rfill x + 2 * s, y + h - 3 * s, w - 4 * s, s, C_R2
    txtC t$, x + w \ 2, y + 4 * s, F_GOLD
  else
    rfill x + s, y + s, w - 2 * s, h - 2 * s, C_G1
    rfill x + 2 * s, y + 2 * s, w - 4 * s, h - 4 * s, C_INK2
    txtC t$, x + w \ 2, y + 4 * s, r
  end if
end sub

' keycap; label t$ or arrow a (0 L, 1 R, 2 U, 3 D, -1 none). returns width
function ui.keycap(x, y, t$, a)
  local s = scr.sc, w, h = 15 * s
  if a >= 0 then
    w = 15 * s
  else
    w = txtW(t$) + 8 * s
  end if
  if w < 15 * s then w = 15 * s
  rfill x, y, w, h, C_INK
  rfill x + s, y + s, w - 2 * s, h - 3 * s, C_W3
  rfill x + s, y + h - 2 * s, w - 2 * s, s, C_W0
  rfill x + s, y + s, w - 2 * s, s, C_W4
  rfill x + w - 2 * s, y + 2 * s, s, h - 4 * s, C_W1
  if a >= 0 then
    blit lay.arrowX + a * 9 * s, lay.miniY, x + (w - 7 * s) \ 2, y + 3 * s, 7 * s, 7 * s, PG_SHEET, 4
  else
    txt t$, x + (w - txtW(t$)) \ 2 + s, y + 3 * s, F_KEY
  end if
  ui.keycap = w
end function

' a tetromino (kind k, spawn orientation) in mini tiles centred in a box
sub ui.mini(k, x, y, w, h)
  local i, mn = lay.mini, x0, y0, x1, y1, ox, oy
  x0 = 9
  y0 = 9
  x1 = -1
  y1 = -1
  for i = 0 to 3
    x0 = min(x0, pc.cx(k, 0, i))
    x1 = max(x1, pc.cx(k, 0, i))
    y0 = min(y0, pc.cy(k, 0, i))
    y1 = max(y1, pc.cy(k, 0, i))
  next i
  ox = x + (w - (x1 - x0 + 1) * mn) \ 2 - x0 * mn
  oy = y + (h - (y1 - y0 + 1) * mn) \ 2 - y0 * mn
  for i = 0 to 3
    blit k * mn, lay.miniY, ox + pc.cx(k, 0, i) * mn, oy + pc.cy(k, 0, i) * mn, mn, mn, PG_SHEET
  next i
end sub
