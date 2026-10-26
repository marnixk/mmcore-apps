' font.inc - pixel fonts blitted from the UI block of the sheet (Press Start
' 2P 8 px, generated). All coordinates are UI ART units (x scr.sc = px).
'  small: 9 x 9 cells (glyph + shadow) drawn 8 apart; 8 colour styles (F_* in
'         layouts.inc) of two sheet rows each (FS_N glyphs per row)
'  big:   gold 16 px glyphs with a dark rim, 18 x 18 cells drawn 16 apart

' draw s$ at art x, y in style st on the current write page
sub txt(s$, x, y, st)
  local i, k, s, u$, n, cw, dx, sy0
  s = scr.sc
  cw = 9 * s
  u$ = ucase$(s$)
  n = len(u$)
  sy0 = (lay.uy + FS_Y + 18 * st) * s
  for i = 1 to n
    k = instr(SMALL$, mid$(u$, i, 1)) - 1
    if k < 0 then k = instr(SMALL$, "?") - 1
    if k > 0 then
      dx = (x + (i - 1) * 8) * s
      blit (k mod FS_N) * cw, sy0 + (k \ FS_N) * cw, dx, y * s, cw, cw, PG_SHEET, 4
    end if
  next i
end sub

function txtW(s$)
  txtW = len(s$) * 8
end function

sub txtC(s$, cx, y, st)
  txt s$, cx - txtW(s$) \ 2, y, st
end sub

sub txtR(s$, rx, y, st)
  txt s$, rx - txtW(s$), y, st
end sub

' big gold text, centred on art x cx
sub bigC(s$, cx, y)
  local i, k, s, u$, n, cw, x, sy0
  s = scr.sc
  cw = 18 * s
  u$ = ucase$(s$)
  n = len(u$)
  x = cx - n * 8
  sy0 = (lay.uy + BG_Y) * s
  for i = 1 to n
    k = instr(BIG$, mid$(u$, i, 1)) - 1
    if k > 0 then
      blit (k mod BG_N) * cw, sy0 + (k \ BG_N) * cw, (x + (i - 1) * 16) * s, y * s, cw, cw, PG_SHEET, 4
    end if
  next i
end sub
