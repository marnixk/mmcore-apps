' font.inc - pixel fonts blitted from the UI block of the sheet (Press Start
' 2P 8 px, generated). All coordinates are UI ART units (x scr.sc = px).
'  small: 9 x 9 cells (glyph + shadow) drawn 8 apart; 8 colour styles (F_* in
'         layouts.inc) of two sheet rows each (FS_N glyphs per row)
'  big:   gold 16 px glyphs with a dark rim, 18 x 18 cells drawn 16 apart

' draw s$ at art x, y in style st on the current write page
' scratch for the sub below (globals: LOCAL clears mmcore's fast-path cache)
dim tx_i, tx_k, tx_s, tx_n, tx_cw, tx_dx, tx_sy0
dim string tx_u$
sub txt(s$, x, y, st)
  tx_s = scr.sc
  tx_cw = 9 * tx_s
  tx_u$ = ucase$(s$)
  tx_n = len(tx_u$)
  tx_sy0 = (lay.uy + FS_Y + 18 * st) * tx_s
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

' big gold text, centred on art x cx
' scratch for the sub below (globals: LOCAL clears mmcore's fast-path cache)
dim bc_i, bc_k, bc_s, bc_n, bc_cw, bc_x, bc_sy0
dim string bg_u$
sub bigC(s$, cx, y)
  bc_s = scr.sc
  bc_cw = 18 * bc_s
  bg_u$ = ucase$(s$)
  bc_n = len(bg_u$)
  bc_x = cx - bc_n * 8
  bc_sy0 = (lay.uy + BG_Y) * bc_s
  for bc_i = 1 to bc_n
    bc_k = instr(BIG$, mid$(bg_u$, bc_i, 1)) - 1
    if bc_k > 0 then
      blit (bc_k mod BG_N) * bc_cw, bc_sy0 + (bc_k \ BG_N) * bc_cw, (bc_x + (bc_i - 1) * 16) * bc_s, y * bc_s, bc_cw, bc_cw, PG_SHEET, 4
    end if
  next bc_i
end sub
