' board.inc - board state, rule checking and drawing

const T_NORMAL = 0, T_GIVEN = 1, T_PEER = 2, T_SAME = 3, T_ZONE = 4, T_CLASH = 5

dim bd.g(80)        ' digit in each cell, 0 = empty
dim bd.giv(80)      ' TRUE for the locked starting clues
dim bd.zone(80)     ' cell sits in a row/column/box that breaks the rules
dim bd.clash(80)    ' cell holds a digit that is duplicated in one of its units
dim bd.cnt(9)      ' how often each digit 1..9 occurs in the unit being checked
dim bd.r = 4, bd.c = 4        ' cursor
dim bd.nClash
dim bd.tShown(80)   ' tile * 16 + digit look last drawn on the back buffer, -1 = unknown

' mark one unit (9 cells given by start + step pattern)
sub bd.checkUnit(kind, u)
  local k, cell, d, bad
  for d = 1 to 9
    bd.cnt(d) = 0
  next d
  for k = 0 to 8
    cell = bd.unitCell(kind, u, k)
    d = bd.g(cell)
    if d then bd.cnt(d) = bd.cnt(d) + 1
  next k
  bad = FALSE
  for d = 1 to 9
    if bd.cnt(d) > 1 then bad = TRUE
  next d
  if bad then
    for k = 0 to 8
      cell = bd.unitCell(kind, u, k)
      bd.zone(cell) = TRUE
      d = bd.g(cell)
      if d then
        if bd.cnt(d) > 1 then bd.clash(cell) = TRUE
      end if
    next k
  end if
end sub

' kind 0 = row, 1 = column, 2 = box; k = 0..8 inside the unit
function bd.unitCell(kind, u, k)
  if kind = 0 then
    bd.unitCell = u * 9 + k
  elseif kind = 1 then
    bd.unitCell = k * 9 + u
  else
    bd.unitCell = ((u \ 3) * 3 + k \ 3) * 9 + (u mod 3) * 3 + (k mod 3)
  end if
end function

' recompute every conflict; returns the number of clashing cells
function bd.check()
  local i, u
  for i = 0 to 80
    bd.zone(i) = FALSE
    bd.clash(i) = FALSE
  next i
  for u = 0 to 8
    bd.checkUnit(0, u)
    bd.checkUnit(1, u)
    bd.checkUnit(2, u)
  next u
  bd.nClash = 0
  for i = 0 to 80
    if bd.clash(i) then bd.nClash = bd.nClash + 1
  next i
  bd.check = bd.nClash
end function

function bd.empties()
  local i, n = 0
  for i = 0 to 80
    if bd.g(i) = 0 then n = n + 1
  next i
  bd.empties = n
end function

' tile to use for a cell, in priority order clash > zone > same > peer > given
function bd.tileFor(r, c)
  local cell = r * 9 + c, cd = bd.g(bd.r * 9 + bd.c)
  if bd.clash(cell) then
    bd.tileFor = T_CLASH
  elseif bd.zone(cell) then
    bd.tileFor = T_ZONE
  elseif cd > 0 and bd.g(cell) = cd then
    bd.tileFor = T_SAME
  elseif r = bd.r or c = bd.c or (r \ 3 = bd.r \ 3 and c \ 3 = bd.c \ 3) then
    bd.tileFor = T_PEER
  elseif bd.giv(cell) then
    bd.tileFor = T_GIVEN
  else
    bd.tileFor = T_NORMAL
  end if
end function

' digit look of a cell: 0 empty, else digit * 4 + style (0 given, 1 player, 2 clash)
function bd.lookFor(cell)
  local d = bd.g(cell)
  if d = 0 then
    bd.lookFor = 0
  elseif bd.giv(cell) then
    bd.lookFor = d * 4
  elseif bd.clash(cell) then
    bd.lookFor = d * 4 + 2
  else
    bd.lookFor = d * 4 + 1
  end if
end function

sub bd.drawCell(r, c)
  local cs = lay.cs, x, y, t, d, lk, cell = r * 9 + c
  x = lay.bx + lay.off(c)
  y = lay.by + lay.off(r)
  t = bd.tileFor(r, c)
  blit t * cs, 0, x, y, cs, cs, PG_SHEET
  lk = bd.lookFor(cell)
  bd.tShown(cell) = t * 64 + lk
  if lk then
    d = lk \ 4
    blit (d - 1) * cs, cs + (lk mod 4) * cs, x, y, cs, cs, PG_SHEET, 4
  end if
end sub

' redraw (on the current write page) only the cells whose tile or digit
' changed since they were last drawn; returns how many were redrawn.
' all = FALSE after a pure cursor move from (r0, c0): then only cells in the
' old or new row, column or box, or holding the old or new cursor digit, can
' change, so the others are skipped without the full tile test.
function bd.refresh(all, r0, c0)
  local r, c, cell, t, n = 0, b0, b1, d0, d1, cand
  b0 = (r0 \ 3) * 3 + c0 \ 3
  b1 = (bd.r \ 3) * 3 + bd.c \ 3
  d0 = bd.g(r0 * 9 + c0)
  d1 = bd.g(bd.r * 9 + bd.c)
  if d0 = 0 then d0 = -1
  if d1 = 0 then d1 = -1
  for r = 0 to 8
    for c = 0 to 8
      cell = r * 9 + c
      cand = all or r = r0 or r = bd.r or c = c0 or c = bd.c
      if not cand then cand = ((r \ 3) * 3 + c \ 3 = b0) or ((r \ 3) * 3 + c \ 3 = b1)
      if not cand then cand = (bd.g(cell) = d0) or (bd.g(cell) = d1) or (bd.tShown(cell) < 0)
      if cand then
        t = bd.tileFor(r, c)
        t = t * 64 + bd.lookFor(cell)
        if t <> bd.tShown(cell) then n = n + 1 : bd.drawCell(r, c)
      end if
    next c
  next r
  bd.refresh = n
end function

sub bd.drawAll()
  local r, c
  for r = 0 to 8
    for c = 0 to 8
      bd.drawCell(r, c)
    next c
  next r
end sub

sub bd.drawCursor(fr)
  local x, y
  x = lay.bx + lay.off(bd.c) - scr.sc
  y = lay.by + lay.off(bd.r) - scr.sc
  blit fr * lay.cur, lay.curY, x, y, lay.cur, lay.cur, PG_SHEET, 4
end sub
