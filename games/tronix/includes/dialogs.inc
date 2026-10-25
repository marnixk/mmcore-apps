' dialogs.inc - pause, level clear, game over with name entry, and the
' high-score table. The panels (with their fixed labels) are PNGs loaded
' straight onto the dimmed back page; values are drawn at run time.
' All coordinates in art units.

' dark checker over the back page (PG_P7 holds the dim overlay during a game)
sub dlg.dim()
  page copy PG_P7 to PG_BACK, B
  page write PG_BACK
end sub

' pause dialog with the controls. returns TRUE when the player quits (Q)
function dlg.pause()
  local x, k$
  dlg.dim()
  x = ui.dialog("pause", PAUSE_W, PAUSE_H)
  scr.present()
  dlg.pause = FALSE
  do
    k$ = inkey$
    if k$ = "" then
      pause 5
    elseif key.isPause(k$) then
      exit do
    elseif key.isQuit(k$) then
      dlg.pause = TRUE
      exit do
    end if
  loop
end function

' wait for Enter (or Esc / Space / Q)
sub dlg.waitEnter()
  local k$
  do
    k$ = inkey$
    if k$ = "" then pause 5
  loop until key.isEnter(k$) or k$ = chr$(K_ESC) or k$ = " " or key.isQuit(k$)
end sub

' level clear: claimed, target, over-target bonus, lives bonus, score, next level
sub dlg.levelClear(p, over, lb, extra)
  local x, y, w, h, rx
  dlg.dim()
  w = CLEAR_W
  h = CLEAR_H
  x = ui.dialog("clear", w, h)
  y = ui.py
  rx = x + w - 28
  txtR str$(p) + "%", rx, y + 42, F_HOT
  txtR str$(gm.target) + "%", rx, y + 57, F_ORANGE
  txtR "+" + str$(over), rx, y + 72, F_CYAN
  txtR "+" + str$(lb), rx, y + 87, F_CYAN
  txtR str$(gm.score), rx, y + 108, F_WHITE
  if extra then txtC "EXTRA LIFE!", x + w \ 2, y + h - 38, F_ORANGE
  txtC "NEXT: LEVEL " + str$(gm.level + 1), x + w \ 2, y + h - 24, F_BLUE
  scr.present()
  pause 400
  key.drain()
  dlg.waitEnter()
end sub

' name entry: frame, typed letters, block cursor, underlines (8 slots)
sub dlg.slots(bx, by, n$, cur)
  local i, sx
  ui.spr(SH_NB_X, SH_NB_Y, NB_W, NB_H, bx, by)
  for i = 0 to HS_MAXNAME - 1
    sx = bx + 8 + i * 12
    if i < len(n$) then
      txt mid$(n$, i + 1, 1), sx + 2, by + 6, F_HOT
    elseif i = len(n$) and cur then
      af sx + 1, by + 5, 9, 11, C_C2
      af sx + 1, by + 5, 9, 1, C_C3
    end if
    if i <= len(n$) then
      af sx + 1, by + 17, 9, 1, C_C1
    else
      af sx + 1, by + 17, 9, 1, C_S2
    end if
  next i
end sub

' redraw the name box and copy just that strip to the screen
sub dlg.nameShow(bx, by, n$, cur)
  dlg.slots(bx, by, n$, cur)
  scr.markA(bx, by, NB_W, NB_H)
  scr.flush()
end sub

' game over: SCORE / LEVEL / BEST; a top-10 score asks for a name, saves it
' and shows the table
sub dlg.gameOver()
  local x, y, w, h, cx, rk, k, k$, n$, c$, cur, tb, bx, by, best, rx
  cur = TRUE
  rk = hs.rank(gm.score)
  best = max(hs.score(0), gm.score)
  dlg.dim()
  w = OVER_W
  h = OVER_H
  x = ui.dialog("over", w, h)
  y = ui.py
  cx = x + w \ 2
  rx = x + w - 36
  txtR str$(gm.score), rx, y + 42, F_WHITE
  txtR str$(gm.level), rx, y + 57, F_WHITE
  txtR str$(best), rx, y + 72, F_GOLD
  bx = x + (w - NB_W) \ 2
  by = y + 138
  if rk >= 0 then
    txtC "NEW HIGH SCORE! RANK " + str$(rk + 1), cx, y + 102, F_ORANGE
    txtC "ENTER YOUR NAME", cx, y + 120, F_GREY
    dlg.slots(bx, by, "", TRUE)
  else
    txtC "NO HIGH SCORE THIS TIME", cx, y + 110, F_GREY
  end if
  scr.present()
  ' a short guard, then drop the arrows still buffered from play
  pause 600
  key.drain()
  if rk < 0 then
    dlg.waitEnter()
    exit sub
  end if
  n$ = ""
  tb = timer
  do
    k$ = inkey$
    if k$ = "" then
      if timer - tb > 400 then
        cur = not cur
        tb = timer
        dlg.nameShow(bx, by, n$, cur)
      end if
      pause 5
    else
      k = asc(k$)
      if key.isEnter(k$) or k = K_ESC then exit do
      if (k = K_BKSP or k = K_DEL or k = K_LEFT) and len(n$) > 0 then
        n$ = left$(n$, len(n$) - 1)
      elseif k >= 32 and k < 127 and len(n$) < HS_MAXNAME then
        c$ = ucase$(k$)
        if instr("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 -!.", c$) > 0 then
          if not (c$ = " " and len(n$) = 0) then n$ = n$ + c$
        end if
      end if
      cur = TRUE
      tb = timer
      dlg.nameShow(bx, by, n$, cur)
    end if
  loop
  do while right$(n$, 1) = " "
    n$ = left$(n$, len(n$) - 1)
  loop
  if n$ = "" then n$ = "PLAYER"
  hs.insert(rk, n$, gm.score, gm.level, gm.diff)
  hs.save()
  dlg.scores(rk)
end sub

' high-score table: #, NAME, SKILL, LVL, SCORE; hl = row to highlight (-1 none)
sub dlg.scores(hl)
  local x, y, w, hy, ry, i, st, rs, d
  scr.useScores()
  x = lay.sx
  y = lay.sy
  w = lay.sw
  page copy PG_P7 to PG_BACK
  page write PG_BACK
  hy = y + 12
  for i = 0 to 9
    ry = hy + 18 + i * lay.sstep
    if i = hl then ui.bar(x + 10, ry - 5, w - 20)
    st = F_WHITE
    if i = 0 then st = F_GOLD
    if i = hl then st = F_HOT
    rs = F_GREY
    if i = 0 or i = hl then rs = st
    txtR str$(i + 1), x + 26, ry, rs
    if hs.score(i) > 0 then
      txt hs.name$(i), x + 48, ry, st
      d = hs.diff(i)
      txt df.name$(d), x + 140, ry, df.style(d)
      txtR str$(hs.level(i)), x + 244, ry, rs
      txtR str$(hs.score(i)), x + w - 18, ry, st
    else
      txt "---", x + 48, ry, F_DIM
    end if
  next i
  scr.present()
  key.drain()
  dlg.waitEnter()
end sub
