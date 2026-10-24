' field.inc - the 42 x 23 playfield: level layouts, the cell grid and drawing
' single cells. Field cell (fx, fy) sits at screen x = lay.ox + fx * cell,
' y = (fy + 1) * cell (the top cell row is the HUD bar). Every screen size uses
' the same grid, so one set of layouts fits all of them.

const FW = 42, FH = 23
' grid values
const G_FREE = 0, G_WALL = 1, G_SNAKE = 2
dim fg(FW - 1, FH - 1)

' sheet tile indices (16 per sheet row, see ASSETS.txt)
const T_HEAD = 0, T_DEAD = 4, T_BODYH = 8, T_BODYV = 9, T_LU = 10, T_UR = 11, T_RD = 12, T_DL = 13
const T_TAIL = 14, T_LIFE = 18, T_GLOW = 19, T_FRUIT = 21, T_WALL = 29, T_FLOOR = 33
const T_SHA = 37, T_SHB = 41, T_DITHER = 45

' level layouts, read once from levels.inc
const LP_MAX = 128
dim string lv.name$(LV_COUNT - 1)
dim lv.hx(LV_COUNT - 1), lv.hy(LV_COUNT - 1), lv.hd(LV_COUNT - 1), lv.p0(LV_COUNT), lv.used
dim lp.x(LP_MAX - 1), lp.y(LP_MAX - 1), lp.w(LP_MAX - 1), lp.h(LP_MAX - 1)

' direction steps: 0 right, 1 down, 2 left, 3 up
dim dx(3) = (1, 0, -1, 0)
dim dy(3) = (0, 1, 0, -1)

sub fld.readLevels()
  local i, j, n, a$
  restore LvData
  lv.used = 0
  for i = 0 to LV_COUNT - 1
    read a$, lv.hx(i), lv.hy(i), lv.hd(i), n
    lv.name$(i) = a$
    lv.p0(i) = lv.used
    for j = 1 to n
      read lp.x(lv.used), lp.y(lv.used), lp.w(lv.used), lp.h(lv.used)
      lv.used = lv.used + 1
    next j
  next i
  lv.p0(LV_COUNT) = lv.used
end sub

' cheap 0..99 hash for the floor / wall variations (same as the art generator).
' Only + * MOD: in mmcore AND / XOR are logical, not bitwise.
function fld.hash(x, y, s)
  local h
  h = (x + 11) * 7919 + (y + 17) * 104729 + s * 31337
  h = (h * h) mod 2147483647
  h = (h * 48271 + 12345) mod 2147483647
  h = (h * h) mod 2147483647
  fld.hash = (h \ 7) mod 100
end function

function fld.wallTile(x, y)
  local h = fld.hash(x, y, 77)
  fld.wallTile = T_WALL
  if h >= 66 then fld.wallTile = T_WALL + 1
  if h >= 86 then fld.wallTile = T_WALL + 2
  if h >= 93 then fld.wallTile = T_WALL + 3
end function

function fld.isWall(x, y)
  fld.isWall = FALSE
  if x >= 0 and y >= 0 then fld.isWall = (fg(x, y) = G_WALL)
end function

' floor tile: two checker tones, wall shadows from the top / left, a few grass tufts
function fld.floorTile(x, y)
  local t, l, b, sh = -1
  t = fld.isWall(x, y - 1)
  l = fld.isWall(x - 1, y)
  if t and l then
    sh = 2
  elseif t then
    sh = 0
  elseif l then
    sh = 1
  elseif fld.isWall(x - 1, y - 1) then
    sh = 3
  end if
  b = (x + y) mod 2
  if sh >= 0 then
    fld.floorTile = T_SHA + 4 * b + sh
  else
    fld.floorTile = T_FLOOR + 2 * b + (fld.hash(x, y, 5) < 12)
  end if
end function

' grid for layout li (0..9): border + the layout's wall pieces
sub fld.setup(li)
  local x, y, i, k, px, py
  ' (subscripts are plain variables: an array element inside the subscript of an
  ' assignment target trips ?ARRAY in mmcore)
  for y = 0 to FH - 1
    for x = 0 to FW - 1
      fg(x, y) = G_FREE
      if x = 0 or y = 0 or x = FW - 1 or y = FH - 1 then fg(x, y) = G_WALL
    next x
  next y
  for i = lv.p0(li) to lv.p0(li + 1) - 1
    px = lp.x(i)
    py = lp.y(i)
    if lp.h(i) < 0 then
      for k = 0 to lp.w(i) - 1
        x = px + k
        y = py + k
        if lp.h(i) = -2 then y = py - k
        fg(x, y) = G_WALL
      next k
    else
      for y = lp.y(i) to lp.y(i) + lp.h(i) - 1
        for x = lp.x(i) to lp.x(i) + lp.w(i) - 1
          fg(x, y) = G_WALL
        next x
      next y
    end if
  next i
end sub

' the clean level picture on PG_FIELD: empty HUD bar, floor and walls, and the
' side gaps (screen width is not a whole number of cells) as partial wall
sub fld.build()
  local x, y, t, c = lay.cell, gr, rx
  page write PG_FIELD
  cls C_K
  for y = 0 to FH - 1
    for x = 0 to FW - 1
      if fg(x, y) = G_WALL then
        t = fld.wallTile(x, y)
      else
        t = fld.floorTile(x, y)
      end if
      blit (t mod 16) * c, (t \ 16) * c, lay.ox + x * c, (y + 1) * c, c, c, PG_SHEET
    next x
    rx = lay.ox + FW * c
    gr = scr.W - rx
    if lay.ox > 0 then
      t = fld.wallTile(-1, y)
      blit (t mod 16) * c + c - lay.ox, (t \ 16) * c, 0, (y + 1) * c, lay.ox, c, PG_SHEET
    end if
    if gr > 0 then
      t = fld.wallTile(FW, y)
      blit (t mod 16) * c, (t \ 16) * c, rx, (y + 1) * c, gr, c, PG_SHEET
    end if
  next y
end sub

' draw field cell fx, fy on the current write page: floor/wall from PG_FIELD,
' then up to two sprites (-1 = none)
sub cel.draw(fx, fy, t1, t2)
  local c = lay.cell, x, y
  x = lay.ox + fx * c
  y = (fy + 1) * c
  blit x, y, x, y, c, c, PG_FIELD
  if t1 >= 0 then blit (t1 mod 16) * c, (t1 \ 16) * c, x, y, c, c, PG_SHEET, 4
  if t2 >= 0 then blit (t2 mod 16) * c, (t2 \ 16) * c, x, y, c, c, PG_SHEET, 4
end sub

' redraw a cell on the live scene and show just that cell (no flicker)
sub cel.put(fx, fy, t1, t2)
  local c = lay.cell, x, y
  page write PG_LIVE
  cel.draw(fx, fy, t1, t2)
  x = lay.ox + fx * c
  y = (fy + 1) * c
  page write 0
  blit x, y, x, y, c, c, PG_LIVE
end sub
