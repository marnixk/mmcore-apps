' font.inc - pixel font blitted from the sprite sheet (Press Start 2P 8 px, generated).
' All coordinates are ART units (x scr.sc = screen pixels).
' Small font: 9-unit cells (glyph + shadow), 8 units apart, 10 colour rows of two
' sheet rows each (FS_N glyphs per sheet row) starting at sheet x FS_X.
' Big font: 20-unit cells (16 px glyph, outline, shadow), 16 units apart, 4 colours
' placed at fb.x/fb.y with fb.n glyphs per row. [ ] { } are arrows L R U D.
' Text goes through bl(), so it honours the clip rectangle.
const SMALL$ = " !#%'()*+,-./0123456789:<>?ABCDEFGHIJKLMNOPQRSTUVWXYZ_[]{}"
const BIG$ = " !-.0123456789?ABCDEFGHIJKLMNOPQRSTUVWXYZ_"
const F_CREAM = 0, F_GOLD = 1, F_GREEN = 2, F_GREY = 3, F_DIM = 4, F_RED = 5, F_KEY = 6, F_CYAN = 7, F_PINK = 8, F_SILVER = 9
const B_GOLD = 0, B_RED = 1, B_CYAN = 2, B_GREEN = 3

' draw s$ at art x, y in small-font colour row r on the current write page
sub txt(s$, x, y, r)
  local i, k, s, u$, n, cw
  s = scr.sc
  cw = 9 * s
  u$ = ucase$(s$)
  n = len(u$)
  for i = 1 to n
    k = instr(SMALL$, mid$(u$, i, 1)) - 1
    if k < 0 then k = instr(SMALL$, "?") - 1
    if k > 0 then bl((FS_X + (k mod FS_N) * 9) * s, (2 * r + k \ FS_N) * cw, (x + (i - 1) * 8) * s, y * s, cw, cw, PG_SHEET, 1)
  next i
end sub

function txtW(s$)
  txtW = len(s$) * 8
end function

sub txtC(s$, cx, y, r)
  txt s$, cx - txtW(s$) \ 2, y, r
end sub

sub txtR(s$, rx, y, r)
  txt s$, rx - txtW(s$), y, r
end sub

' big (double size) font
sub big(s$, x, y, r)
  local i, k, s, u$, n, cw, bn
  s = scr.sc
  cw = 20 * s
  bn = fb.n(r)
  u$ = ucase$(s$)
  n = len(u$)
  for i = 1 to n
    k = instr(BIG$, mid$(u$, i, 1)) - 1
    if k < 0 then k = instr(BIG$, "?") - 1
    if k > 0 then bl((fb.x(r) + (k mod bn) * 20) * s, (fb.y(r) + (k \ bn) * 20) * s, (x - 1 + (i - 1) * 16) * s, (y - 1) * s, cw, cw, PG_SHEET, 1)
  next i
end sub

sub bigC(s$, cx, y, r)
  big s$, cx - len(s$) * 8, y, r
end sub

' n as at least w digits with leading zeros
function zpad$(n, w)
  zpad$ = str$(n)
  do while len(zpad$) < w
    zpad$ = "0" + zpad$
  loop
end function
