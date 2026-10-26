' font.inc - pixel font blitted from the sprite sheet (Press Start 2P 8 px, generated).
' All coordinates are ART units (x scr.sc = screen pixels).
' Glyph cells are 9 x 9 (glyph + shadow), drawn 8 apart; 9 colour styles (F_*
' in layouts.inc) of two sheet rows each (FS_N glyphs per row) from sheet y FS_Y.
' [ ] { } are not used as arrows here.

' draw s$ at art x, y in style st on the current write page
' scratch (LOCAL in mmcore clears its fast-path cache)
dim integer tx_i, tx_k, tx_s, tx_n, tx_cw, tx_dx, tx_sy0
dim string tx_u$
sub txt(s$, x, y, st)
  tx_s = scr.sc
  tx_cw = 9 * tx_s
  tx_u$ = ucase$(s$)
  tx_n = len(tx_u$)
  tx_sy0 = (FS_Y + 18 * st) * tx_s
  for tx_i = 1 to tx_n
    tx_k = instr(SMALL$, mid$(tx_u$, tx_i, 1)) - 1
    if tx_k < 0 then tx_k = instr(SMALL$, "?") - 1
    if tx_k > 0 then
      tx_dx = (x + (tx_i - 1) * 8) * tx_s
      blit (tx_k mod FS_N) * tx_cw, tx_sy0 + (tx_k \ FS_N) * tx_cw, tx_dx, y * tx_s, tx_cw, tx_cw, PG_SHEET, 4
    end if
  next tx_i
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
