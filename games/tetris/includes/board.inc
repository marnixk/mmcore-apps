' board.inc - the well: 10 x 22 cells (rows 0-1 are hidden above the top),
' collision tests, locking, line detection and drawing.
'
' bd.c(row * 10 + col): 0 empty, 1..7 = kind + 1, 8 = grey (game over).
' The locked stack is drawn into PG_STACK (on lock / after a clear); the
' falling piece and its ghost are composed on PG_BACK and only the well
' rectangle is copied to the display.

const BD_W = 10, BD_H = 22, BD_HID = 2
dim bd.c(BD_W * BD_H - 1)
dim bd.full(3), bd.nFull = 0

sub bd.clear()
  local i
  for i = 0 to BD_W * BD_H - 1
    bd.c(i) = 0
  next i
end sub

' TRUE when piece k in rotation r fits with its box at (x, y)
function bd.fits(k, r, x, y)
  local i, cx, cy
  bd.fits = FALSE
  for i = 0 to 3
    cx = x + pc.cx(k, r, i)
    cy = y + pc.cy(k, r, i)
    if cx < 0 or cx >= BD_W or cy >= BD_H then exit function
    if cy >= 0 then
      if bd.c(cy * BD_W + cx) <> 0 then exit function
    end if
  next i
  bd.fits = TRUE
end function

' lowest y the piece can drop to from y
function bd.dropY(k, r, x, y)
  local yy = y
  do while bd.fits(k, r, x, yy + 1)
    yy = yy + 1
  loop
  bd.dropY = yy
end function

' write the piece into the board; returns TRUE when all of it is above the
' visible well (lock out = game over)
function bd.lock(k, r, x, y)
  local i, cx, cy, above = TRUE
  for i = 0 to 3
    cx = x + pc.cx(k, r, i)
    cy = y + pc.cy(k, r, i)
    if cy >= 0 then bd.c(cy * BD_W + cx) = k + 1
    if cy >= BD_HID then above = FALSE
  next i
  bd.lock = above
end function

' find full rows (bd.full(), bd.nFull)
sub bd.findFull()
  local r, c, n
  bd.nFull = 0
  for r = 0 to BD_H - 1
    n = 0
    for c = 0 to BD_W - 1
      if bd.c(r * BD_W + c) <> 0 then n = n + 1
    next c
    if n = BD_W and bd.nFull < 4 then
      bd.full(bd.nFull) = r
      bd.nFull = bd.nFull + 1
    end if
  next r
end sub

' remove the full rows, everything above falls down
sub bd.removeFull()
  local i, r, rr, c
  for i = 0 to bd.nFull - 1
    r = bd.full(i)
    for rr = r to 1 step -1
      for c = 0 to BD_W - 1
        bd.c(rr * BD_W + c) = bd.c((rr - 1) * BD_W + c)
      next c
    next rr
    for c = 0 to BD_W - 1
      bd.c(c) = 0
    next c
  next i
  bd.nFull = 0
end sub

function bd.isFull(r)
  local i
  bd.isFull = FALSE
  for i = 0 to bd.nFull - 1
    if bd.full(i) = r then bd.isFull = TRUE
  next i
end function

' ---- drawing (current write page)
' tile t (0..6 kinds, 7 grey) at board cell (c, r)
sub bd.tile(t, c, r)
  local s = lay.cell
  if r < BD_HID then exit sub
  blit t * s, lay.tileY, lay.wx + c * s, lay.wy + (r - BD_HID) * s, s, s, PG_SHEET
end sub

sub bd.ghostTile(t, c, r)
  local s = lay.cell
  if r < BD_HID then exit sub
  blit t * s, lay.tileY + s, lay.wx + c * s, lay.wy + (r - BD_HID) * s, s, s, PG_SHEET, 4
end sub

' empty well cell, then optional flash frame f (0..3) on top
sub bd.flashTile(f, c, r)
  local s = lay.cell, x, y
  x = lay.wx + c * s
  y = lay.wy + (r - BD_HID) * s
  blit x, y, x, y, s, s, PG_GAME
  if f >= 0 and f <= 3 then blit (f + 1) * s, lay.tileY + 2 * s, x, y, s, s, PG_SHEET, 4
end sub

' redraw the whole locked stack on PG_STACK (grey = game-over look)
sub bd.drawStack(grey)
  local r, c, v
  page write PG_STACK
  blit lay.wx, lay.wy, lay.wx, lay.wy, 10 * lay.cell, 20 * lay.cell, PG_GAME
  for r = BD_HID to BD_H - 1
    for c = 0 to BD_W - 1
      v = bd.c(r * BD_W + c)
      if v > 0 then
        if grey then
          bd.tile(7, c, r)
        else
          bd.tile(v - 1, c, r)
        end if
      end if
    next c
  next r
end sub

' draw a piece's tiles (ghost or solid) on the current write page
sub bd.piece(k, r, x, y, isGhost)
  local i
  for i = 0 to 3
    if isGhost then
      bd.ghostTile(k, x + pc.cx(k, r, i), y + pc.cy(k, r, i))
    else
      bd.tile(k, x + pc.cx(k, r, i), y + pc.cy(k, r, i))
    end if
  next i
end sub
