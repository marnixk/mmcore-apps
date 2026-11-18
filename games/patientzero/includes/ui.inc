' ui.inc - pixel font and dialog chrome. Strings stay out of the play loop.

const F_BODY = 0
const F_GOLD = 1
const F_CYAN = 2
const F_DIM = 3
const F_RED = 4
const F_HINT = 5
const FB_GOLD = 0
const FB_BODY = 1
const FB_DIM = 2

dim fnt_i, fnt_n, fnt_ch, fnt_sx, fnt_sy
dim string fnt_s$

sub ui.glyph(ch, x, y, colr, big)
  if big = 1 then
    fnt_sx = (FNT_BIG_X + (ch mod 16) * 12) * scr.sc
    fnt_sy = ((colr * 4) + (ch \ 16)) * 16 * scr.sc
    blit fnt_sx, fnt_sy, x, y, 12 * scr.sc, 16 * scr.sc, PG_FONT, 4
  else
    fnt_sx = (ch mod 16) * 6 * scr.sc
    fnt_sy = ((colr * 4) + (ch \ 16)) * 8 * scr.sc
    blit fnt_sx, fnt_sy, x, y, 6 * scr.sc, 8 * scr.sc, PG_FONT, 4
  end if
end sub

sub ui.put(x, y, s$, colr, big, adv)
  fnt_s$ = s$
  fnt_n = len(fnt_s$)
  for fnt_i = 1 to fnt_n
    fnt_ch = asc(mid$(fnt_s$, fnt_i, 1))
    if fnt_ch >= 97 and fnt_ch <= 122 then fnt_ch = fnt_ch - 32
    if fnt_ch < 32 or fnt_ch > 95 then fnt_ch = 63
    ui.glyph(fnt_ch - 32, x + (fnt_i - 1) * adv, y, colr, big)
  next fnt_i
end sub

sub ui.text(x, y, s$, colr)
  ui.put x, y, s$, colr, 0, 6 * scr.sc
end sub

sub ui.textC(cx, y, s$, colr)
  ui.text cx - len(s$) * 6 * scr.sc \ 2, y, s$, colr
end sub

sub ui.big(x, y, s$, colr)
  ui.put x, y, s$, colr, 1, 12 * scr.sc
end sub

sub ui.bigC(cx, y, s$, colr)
  ui.big cx - len(s$) * 12 * scr.sc \ 2, y, s$, colr
end sub

sub ui.heading(which)
  if which = 0 then
    b_w = HD_OVER_W * scr.sc
    b_h = HD_OVER_H * scr.sc
    b_sy = HD_OVER_Y * scr.sc
  else
    b_w = HD_CLEAR_W * scr.sc
    b_h = HD_CLEAR_H * scr.sc
    b_sy = HD_CLEAR_Y * scr.sc
  end if
  b_sx = 0
  b_dx = (scr.W - b_w) \ 2
  b_dy = 14 * scr.sc
  blit b_sx, b_sy, b_dx, b_dy, b_w, b_h, PG_FONT, 4
end sub

' Panel in base pixels.
sub ui.panel(x, y, w, h)
  box x * scr.sc, y * scr.sc, w * scr.sc, h * scr.sc, 1, c.edge, c.edge
  box (x + 1) * scr.sc, (y + 1) * scr.sc, (w - 2) * scr.sc, (h - 2) * scr.sc, 1, c.hi, c.hi
  box (x + 3) * scr.sc, (y + 3) * scr.sc, (w - 6) * scr.sc, (h - 6) * scr.sc, 1, c.panel, c.panel
end sub

sub ui.rule(x, y, w)
  box x * scr.sc, y * scr.sc, w * scr.sc, scr.sc, 1, c.edge, c.edge
end sub
