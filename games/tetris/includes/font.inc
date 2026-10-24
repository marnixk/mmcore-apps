' font.inc - Press Start 2P pixel font blitted from the sprite sheet.
' Small font: 8px cells (x scale) in 9 colour rows; big font: 16px cells in 4
' colour rows, two sheet rows of 28 glyphs each. Glyphs carry their shadow.
const SMALL$ = " !#%'()*+,-./0123456789:<>?ABCDEFGHIJKLMNOPQRSTUVWXYZ_"
const BIG$ = " !-.0123456789?ABCDEFGHIJKLMNOPQRSTUVWXYZ_"
const F_CREAM = 0, F_GOLD = 1, F_GOLD3 = 2, F_GREY = 3, F_DIM = 4, F_RED = 5, F_GREEN = 6, F_KEY = 7, F_SILVER = 8
const B_CREAM = 0, B_GOLD = 1, B_RED = 2, B_WHITE = 3

' draw s$ at x, y in small-font colour row r on the current write page
sub txt(s$, x, y, r)
  local i, k, cw, u$, n
  cw = 9 * scr.sc
  u$ = ucase$(s$)
  n = len(u$)
  if n = 0 then exit sub
  for i = 1 to n
    k = instr(SMALL$, mid$(u$, i, 1)) - 1
    if k < 0 then k = instr(SMALL$, "?") - 1
    if k > 0 then blit k * cw, lay.fontY + r * cw, x + (i - 1) * 8 * scr.sc, y, cw, cw, PG_SHEET, 4
  next i
end sub

function txtW(s$)
  txtW = len(s$) * 8 * scr.sc
end function

' centred / right-aligned small text
sub txtC(s$, cx, y, r)
  txt s$, cx - txtW(s$) \ 2, y, r
end sub

sub txtR(s$, rx, y, r)
  txt s$, rx - txtW(s$), y, r
end sub

' big (double size) font
sub big(s$, x, y, r)
  local i, k, cw, u$, n
  cw = 18 * scr.sc
  u$ = ucase$(s$)
  n = len(u$)
  if n = 0 then exit sub
  for i = 1 to n
    k = instr(BIG$, mid$(u$, i, 1)) - 1
    if k < 0 then k = instr(BIG$, "?") - 1
    if k > 0 then blit (k mod 28) * cw, lay.bigY + (r * 2 + k \ 28) * cw, x + (i - 1) * 16 * scr.sc, y, cw, cw, PG_SHEET, 4
  next i
end sub

function bigW(s$)
  bigW = len(s$) * 16 * scr.sc
end function

sub bigC(s$, cx, y, r)
  big s$, cx - bigW(s$) \ 2, y, r
end sub

sub bigR(s$, rx, y, r)
  big s$, rx - bigW(s$), y, r
end sub

function rjust$(s$, w)
  if len(s$) >= w then
    rjust$ = s$
  else
    rjust$ = space$(w - len(s$)) + s$
  end if
end function

function ljust$(s$, w)
  if len(s$) >= w then
    ljust$ = left$(s$, w)
  else
    ljust$ = s$ + space$(w - len(s$))
  end if
end function
