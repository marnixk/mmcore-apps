' font.inc - pixel font blitted from gfx/font_xN.png (5x7 glyphs in 6x8 cells)
const FONT_CHARS$ = " !'()*+,-./0123456789:<>?ABCDEFGHIJKLMNOPQRSTUVWXYZ_~"
const F_CREAM = 0, F_GOLD = 1, F_BROWN = 2, F_RED = 3, F_GREEN = 4

' draw s$ at x, y in colour row r on the current write page
sub txt(s$, x, y, r)
  local i, k, cw = 6 * scr.sc, ch = 8 * scr.sc, u$
  u$ = ucase$(s$)
  if len(u$) = 0 then exit sub
  for i = 1 to len(u$)
    k = instr(FONT_CHARS$, mid$(u$, i, 1)) - 1
    if k < 0 then k = 24
    if k > 0 then blit k * cw, r * ch, x + (i - 1) * cw, y, cw, ch, PG_FONT, 4
  next i
end sub

function txtW(s$)
  txtW = len(s$) * 6 * scr.sc
end function

' zero padded number
function pad2$(n)
  pad2$ = right$("0" + str$(n), 2)
end function

function fmtTime$(secs)
  local a$, b$
  a$ = right$("0" + str$(secs \ 60), 2)
  b$ = right$("0" + str$(secs mod 60), 2)
  fmtTime$ = a$ + ":" + b$
end function

' in-game timer: whole minutes only ("<1 MIN", "3 MIN")
function fmtMins$(mins)
  if mins < 1 then
    fmtMins$ = "<1 MIN"
  else
    fmtMins$ = str$(mins) + " MIN"
  end if
end function

' exact time for the win screen: m:ss, or h:mm:ss from one hour on
function fmtClock$(secs)
  local h, m, a$, b$
  h = secs \ 3600
  m = (secs mod 3600) \ 60
  b$ = right$("0" + str$(secs mod 60), 2)
  if h > 0 then
    a$ = str$(h) + ":" + right$("0" + str$(m), 2)
  else
    a$ = str$(m)
  end if
  fmtClock$ = a$ + ":" + b$
end function

' right-align n in a field of w characters
function rjust$(s$, w)
  if len(s$) >= w then rjust$ = s$ else rjust$ = space$(w - len(s$)) + s$
end function

function ljust$(s$, w)
  if len(s$) >= w then ljust$ = left$(s$, w) else ljust$ = s$ + space$(w - len(s$))
end function
