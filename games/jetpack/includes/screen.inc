' screen.inc - mode, pages, font, dirty rectangles.
' Pages: 0 display  1 back  2 sheet  3 ui  4 title  5 scores  6 hold  7 dim tiles
const PG_BACK = 1, PG_SHEET = 2, PG_UI = 3, PG_TITLE = 4, PG_SCORES = 5, PG_HOLD = 6, PG_DIM = 7

dim integer scr_i, scr_x, scr_y, scr_w, scr_h, scr_c, scr_a, scr_sx, scr_sy
dim float scr.oldMode
dim scr.W, scr.H, scr.lay, scr.dirty, scr.full
dim string lay.res$
dim lay.mode, lay.W, lay.H, lay.sc, lay.tile, lay.ox, lay.oy, lay.fsc
dim lay.syTile, lay.syPlayer, lay.syEnemy, lay.syDoor, lay.syMelt
dim lay.mx, lay.my, lay.mw, lay.mh, lay.mlh, lay.item0, lay.nameY
dim lay.textX, lay.valR, lay.barX, lay.barW, lay.barH
dim lay.sx, lay.sy, lay.sw, lay.sh, lay.srow, lay.s0, lay.snum, lay.sname, lay.slv, lay.ssc
dim lay.fontY, lay.p0x, lay.p0y, lay.p1x, lay.p1y, lay.pw, lay.ph, lay.iconX, lay.iconY, lay.iconS

const DR_MAX = 48
dim dr.x(DR_MAX - 1), dr.y(DR_MAX - 1), dr.w(DR_MAX - 1), dr.h(DR_MAX - 1)
dim dr.n

function scr.pick(w, h)
  local i, best, bw
  best = 0
  for i = 0 to LAY_N - 1
    if lay.ws(i) = w and lay.hs(i) = h then
      scr.pick = i
      exit function
    end if
  next i
  best = 0
  bw = 99999
  for i = 0 to LAY_N - 1
    if lay.ws(i) >= w and lay.hs(i) >= h and lay.ws(i) < bw then
      best = i
      bw = lay.ws(i)
    end if
  next i
  if bw = 99999 then best = 3
  scr.pick = best
end function

sub scr.init()
  local bits, cm
  scr.oldMode = MM.INFO(MODE)
  cm = int(scr.oldMode)
  bits = cint((scr.oldMode - cm) * 100)
  if bits < 16 then bits = 16
  scr.lay = scr.pick(MM.HRES, MM.VRES)
  lay.set(scr.lay)
  mode lay.mode, bits
  locate , , 0
  scr.W = MM.HRES
  scr.H = MM.VRES
  scr.dirty = FALSE
  scr.full = FALSE
  dr.n = 0
end sub

sub scr.loadAssets()
  page write PG_SHEET
  cls rgb(0, 0, 0)
  load png "gfx/sheet_" + lay.res$ + ".png", 0, 0
  page write PG_UI
  cls rgb(0, 0, 0)
  load png "gfx/ui_" + lay.res$ + ".png", 0, 0
  page write PG_TITLE
  load png "gfx/title_" + lay.res$ + ".png", 0, 0
  page write PG_SCORES
  load png "gfx/scores_" + lay.res$ + ".png", 0, 0
  page write PG_DIM
  cls rgb(0, 0, 0)
  load png "gfx/dim_" + lay.res$ + ".png", 0, 0
  page write PG_BACK
end sub

sub scr.shutdown()
  sprite close all
  mode int(scr.oldMode), 16
end sub

sub scr.present()
  page copy PG_BACK to 0
  page write PG_BACK
  scr.dirty = FALSE
  scr.full = FALSE
  dr.n = 0
end sub

sub scr.mark(x0, y0, w0, h0)
  scr_x = x0
  scr_y = y0
  scr_w = w0
  scr_h = h0
  if scr_x < 0 then
    scr_w = scr_w + scr_x
    scr_x = 0
  end if
  if scr_y < 0 then
    scr_h = scr_h + scr_y
    scr_y = 0
  end if
  if scr_x + scr_w > scr.W then scr_w = scr.W - scr_x
  if scr_y + scr_h > scr.H then scr_h = scr.H - scr_y
  if scr_w <= 0 or scr_h <= 0 then
    exit sub
  end if
  scr.dirty = TRUE
  if scr.full then
    exit sub
  end if
  if dr.n >= DR_MAX then
    scr.full = TRUE
    exit sub
  end if
  dr.x(dr.n) = scr_x
  dr.y(dr.n) = scr_y
  dr.w(dr.n) = scr_w
  dr.h(dr.n) = scr_h
  dr.n = dr.n + 1
end sub

sub scr.flush()
  if not scr.dirty then
    exit sub
  end if
  if scr.full then
    scr.present()
    exit sub
  end if
  page write 0
  for scr_i = 0 to dr.n - 1
    blit dr.x(scr_i), dr.y(scr_i), dr.x(scr_i), dr.y(scr_i), dr.w(scr_i), dr.h(scr_i), PG_BACK
  next scr_i
  page write PG_BACK
  dr.n = 0
  scr.dirty = FALSE
end sub

' style 0 menu 1 select 2 gold 3 dim 4 head 5 legal. Advance is 8*fsc; cell is 9*fsc.
sub ui.glyph(ch, x, y, style)
  if ch < 32 or ch > 95 then ch = 32
  scr_c = ch - 32
  scr_sx = (scr_c mod 32) * 9 * lay.fsc
  scr_sy = (style * 2 + scr_c \ 32) * 9 * lay.fsc
  scr_a = 9 * lay.fsc
  blit scr_sx, scr_sy + lay.fontY, x, y, scr_a, scr_a, PG_UI, 4
end sub

sub ui.text(x, y, s$, style)
  local i, n, ch
  n = len(s$)
  for i = 1 to n
    ch = asc(mid$(s$, i, 1))
    ui.glyph(ch, x + (i - 1) * 8 * lay.fsc, y, style)
  next i
end sub

sub ui.textR(right, y, s$, style)
  ui.text(right - len(s$) * 8 * lay.fsc, y, s$, style)
end sub

sub ui.textC(cx, y, s$, style)
  ui.text(cx - len(s$) * 4 * lay.fsc, y, s$, style)
end sub

function num$(n)
  local s$
  s$ = str$(n)
  if left$(s$, 1) = " " then s$ = mid$(s$, 2)
  num$ = s$
end function
