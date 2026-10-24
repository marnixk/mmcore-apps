' font.inc - pixel font blitted from the sprite sheet (8x8 glyphs, generated).
' Small font: 9-unit cells (glyph + shadow) in 8 colour rows, 8 units apart.
' Big font: 20-unit cells (16 px glyph, outline, shadow) in 4 colour rows, two
' sheet rows of 28 glyphs each, 16 units apart. [ ] { } are arrows L R U D.
const SMALL$ = " !#%'()*+,-./0123456789:<>?ABCDEFGHIJKLMNOPQRSTUVWXYZ_[]{}"
const BIG$ = " !-.0123456789?ABCDEFGHIJKLMNOPQRSTUVWXYZ_"
const F_CREAM = 0, F_GOLD = 1, F_GREEN = 2, F_GREY = 3, F_DIM = 4, F_RED = 5, F_KEY = 6, F_SILVER = 7
const B_GOLD = 0, B_RED = 1, B_WHITE = 2, B_GREEN = 3

' draw s$ at x, y in small-font colour row r on the current write page
sub txt(s$, x, y, r)
  local i, k, cw, u$, n
  cw = 9 * scr.sc
  u$ = ucase$(s$)
  n = len(u$)
  for i = 1 to n
    k = instr(SMALL$, mid$(u$, i, 1)) - 1
    if k < 0 then k = instr(SMALL$, "?") - 1
    if k > 0 then blit k * cw, lay.smallY + r * cw, x + (i - 1) * 8 * scr.sc, y, cw, cw, PG_SHEET, 4
  next i
end sub

function txtW(s$)
  txtW = len(s$) * 8 * scr.sc
end function

sub txtC(s$, cx, y, r)
  txt s$, cx - txtW(s$) \ 2, y, r
end sub

sub txtR(s$, rx, y, r)
  txt s$, rx - txtW(s$), y, r
end sub

' big (double size) font
sub big(s$, x, y, r)
  local i, k, cw, u$, n
  cw = 20 * scr.sc
  u$ = ucase$(s$)
  n = len(u$)
  for i = 1 to n
    k = instr(BIG$, mid$(u$, i, 1)) - 1
    if k < 0 then k = instr(BIG$, "?") - 1
    if k > 0 then blit (k mod 28) * cw, lay.bigY + (r * 2 + k \ 28) * cw, x - scr.sc + (i - 1) * 16 * scr.sc, y - scr.sc, cw, cw, PG_SHEET, 4
  next i
end sub

sub bigC(s$, cx, y, r)
  big s$, cx - len(s$) * 8 * scr.sc, y, r
end sub

' n as at least w digits with leading zeros
function zpad$(n, w)
  zpad$ = str$(n)
  do while len(zpad$) < w
    zpad$ = "0" + zpad$
  loop
end function
