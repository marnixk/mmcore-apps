' font.inc - pixel font blitted from the sprite sheet (Press Start 2P 8 px, generated).
' All coordinates are ART units (x scr.sc = screen pixels).
' Glyph cells are 9 x 9 (glyph + shadow), drawn 8 apart; 9 colour styles (F_*
' in layouts.inc) of two sheet rows each (FS_N glyphs per row) from sheet y FS_Y.
' [ ] { } are not used as arrows here.

' draw s$ at art x, y in style st on the current write page
sub txt(s$, x, y, st)
  local i, k, s, u$, n, cw, dx, sy0
  s = scr.sc
  cw = 9 * s
  u$ = ucase$(s$)
  n = len(u$)
  sy0 = (FS_Y + 18 * st) * s
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
