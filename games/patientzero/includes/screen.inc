' screen.inc - mode, pages, colours, dirty rectangles, sprite blit.
' Pages: 0 display, 1 back, 2 packed sprites, 3 atlas, 4 font, 5 field, 6 title.
' Nothing is drawn on page 0. A full PAGE COPY is only used when the scene changes.

const TRUE = 1
const FALSE = 0

const PG_BACK = 1
const PG_SPR = 2
const PG_ATLAS = 3
const PG_FONT = 4
const PG_FIELD = 5
const PG_TITLE = 6

const BASE_W = 640
const BASE_H = 360
const TILE = 20
const MAP_W = 32
const MAP_H = 18

const EN_LAST = 27
const PT_LAST = 39
const PD_LAST = 9
const DR_MAX = 96
const IDLE_MS = 8

const SP_JERRY = 576
const SP_W05 = 72
const SP_C08 = 115
const SP_F15 = 216
const SP_O07 = 101
const SP_O10 = 144
const SP_O18 = 259
const SP_H04 = 58
const SP_H09 = 130
const SP_H25 = 360
const SP_D12 = 173
const SP_M06 = 86
const SP_M09 = 130

const R84 = 7056
const R112 = 12544
const R113 = 12769
const R141 = 19881
const R169 = 28561
const R197 = 38809
const R225 = 50625

const KZ_ZOMBIE = 0
const KZ_OFFICE = 1
const KZ_HOODIE = 2
const KZ_DANCER = 3
const KZ_MEDIC = 4

const ST_WALK = 1
const ST_CHASE = 2
const ST_FLEE = 3
const ST_CURE = 4
const ST_ANGEL = 5
const ST_MASK = 6
const ST_DASH = 7
const ST_REST = 8
const ST_THRILL = 9
const ST_MOON = 10
const ST_SPIN = 11
const ST_TOE = 12
const ST_GOTO = 13
const ST_PATROL = 14

const DR_MAX1 = 95

dim dr.x(DR_MAX1), dr.y(DR_MAX1), dr.w(DR_MAX1), dr.h(DR_MAX1)
dim dr.n, scr.full, scr.dirty
dim float scr.oldMode
dim scr.W, scr.H, scr.sc
dim string scr.res$
dim c.bg, c.panel, c.edge, c.hi, c.gold, c.warn, c.bar
dim b_sx, b_sy, b_dx, b_dy, b_w, b_h, b_ori
dim drx, dry, drw, drh
dim scr_i, scr_bits
dim rng

sub scr.init()
  local w, h, bits, cm
  scr.oldMode = mm.info(mode)
  cm = int(scr.oldMode)
  bits = cint((scr.oldMode - cm) * 100)
  if bits < 16 then bits = 16
  scr_bits = bits
  w = mm.hres
  h = mm.vres
  ' This interpreter's 1280x720 is MODE 11 and 1920x1080 is MODE 16.
  ' The art notes call those MODE 19 and MODE 20; those numbers are the
  ' 1366x768 panel and its half-resolution twin, which do not match the sheets.
  if w >= 1600 or h >= 1000 then
    scr.sc = 3
    scr.res$ = "1080"
    mode 16, bits
  else
    scr.sc = 2
    scr.res$ = "720"
    mode 11, bits
  end if
  locate , , 0
  scr.W = mm.hres
  scr.H = mm.vres
  c.bg = rgb(6, 7, 12)
  c.panel = rgb(14, 16, 26)
  c.edge = rgb(56, 64, 86)
  c.hi = rgb(84, 94, 118)
  c.gold = rgb(255, 210, 70)
  c.warn = rgb(255, 210, 70)
  c.bar = rgb(14, 30, 78)
  scr.dirty = FALSE
  scr.full = FALSE
  dr.n = 0
  rng = timer
  if rng < 1 then rng = 1
end sub

sub scr.loadAssets()
  local p$
  p$ = "gfx/" + scr.res$ + "/"
  page write PG_SPR
  cls c.bg
  load png p$ + "pack_spr.png", 0, 0
  page write PG_ATLAS
  cls c.bg
  load png p$ + "angels.png", 0, 0
  load png p$ + "fx.png", 128 * scr.sc, 0
  load png p$ + "logo.png", 128 * scr.sc, 56 * scr.sc
  load png p$ + "tiles.png", 0, 168 * scr.sc
  load png p$ + "tiles_levels.png", 220 * scr.sc, 168 * scr.sc
  load png p$ + "props.png", 0, 260 * scr.sc
  load png p$ + "props_levels.png", 260 * scr.sc, 260 * scr.sc
  page write PG_FONT
  cls c.bg
  load png p$ + "font.png", 0, 0
  page write PG_TITLE
  cls c.bg
  load png p$ + "title_scene.png", 0, 0
  page write PG_FIELD
  cls c.bg
  page write PG_BACK
  cls c.bg
end sub

sub scr.shutdown()
  local m, b
  page write 0
  m = int(scr.oldMode)
  b = cint((scr.oldMode - m) * 100)
  if b = 8 or b = 12 or b = 16 or b = 32 then
    mode m, b
  else
    mode m
  end if
end sub

sub scr.present()
  page copy PG_BACK to 0
  page write PG_BACK
  scr.dirty = FALSE
  scr.full = FALSE
  dr.n = 0
end sub

sub scr.mark(x0, y0, w0, h0)
  if w0 < 1 or h0 < 1 then
    exit sub
  end if
  if x0 < 0 then
    w0 = w0 + x0
    x0 = 0
  end if
  if y0 < 0 then
    h0 = h0 + y0
    y0 = 0
  end if
  if x0 + w0 > scr.W then w0 = scr.W - x0
  if y0 + h0 > scr.H then h0 = scr.H - y0
  if w0 < 1 or h0 < 1 then
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
  dr.x(dr.n) = x0
  dr.y(dr.n) = y0
  dr.w(dr.n) = w0
  dr.h(dr.n) = h0
  dr.n = dr.n + 1
end sub

sub scr.markAll()
  scr.dirty = TRUE
  scr.full = TRUE
end sub

sub scr.flush()
  if scr.dirty = FALSE then
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

' Draw frame id at a base-pixel top-left. Sets drx,dry,drw,drh (drw = 0 if clipped away).
sub spr.at(id, bx, by)
  drw = 0
  b_w = fr.w(id) * scr.sc
  b_h = fr.h(id) * scr.sc
  b_dx = bx * scr.sc
  b_dy = by * scr.sc
  b_sx = fr.x(id) * scr.sc
  b_sy = fr.y(id) * scr.sc
  if b_dx < 0 then
    b_sx = b_sx - b_dx
    b_w = b_w + b_dx
    b_dx = 0
  end if
  if b_dy < 0 then
    b_sy = b_sy - b_dy
    b_h = b_h + b_dy
    b_dy = 0
  end if
  if b_dx + b_w > scr.W then b_w = scr.W - b_dx
  if b_dy + b_h > scr.H then b_h = scr.H - b_dy
  if b_w < 1 or b_h < 1 then
    exit sub
  end if
  blit b_sx, b_sy, b_dx, b_dy, b_w, b_h, fr.pg(id), 4
  drx = b_dx
  dry = b_dy
  drw = b_w
  drh = b_h
end sub

sub spr.back(rx, ry, rw, rh)
  if rw < 1 or rh < 1 then
    exit sub
  end if
  blit rx, ry, rx, ry, rw, rh, PG_FIELD
end sub

function rndn(n)
  rng = (rng * 1103515245 + 12345) mod 2147483648
  if rng < 0 then rng = 0 - rng
  rndn = (rng \ 65536) mod n
end function

function num$(n)
  local s$
  s$ = str$(n)
  if left$(s$, 1) = " " then s$ = mid$(s$, 2)
  num$ = s$
end function
