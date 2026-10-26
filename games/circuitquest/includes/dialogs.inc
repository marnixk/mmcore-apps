' dialogs.inc - pause, banners, game over with name entry, and the
' high-score table. The panels (with their fixed labels) are PNGs loaded
' straight onto the dimmed back page; values are drawn at run time.
' All coordinates in UI art units.

' dark checker over the back page (keeps a copy of the scene in PG_SAVE)
sub dlg.dim()
  page copy PG_BACK to PG_SAVE
  page copy PG_DIM to PG_BACK, B
  page write PG_BACK
end sub

' put the scene back after a dialog (not shown yet)
sub dlg.undim()
  page copy PG_SAVE to PG_BACK
  page write PG_BACK
end sub

' pause dialog with the controls: DLG_RESUME, DLG_RESTART (R) or DLG_QUIT (Q)
const DLG_RESUME = 0, DLG_RESTART = 1, DLG_QUIT = 2
function dlg.pause()
  local x, k$
  dlg.dim()
  x = ui.dialog("pause", PAUSE_W, PAUSE_H)
  scr.present()
  key.drain()
  dlg.pause = DLG_RESUME
  do
    k$ = inkey$
    if k$ = "" then
      pause 5
    elseif key.isPause(k$) then
      exit do
    elseif key.isRestart(k$) then
      dlg.pause = DLG_RESTART
      exit do
    elseif key.isQuit(k$) then
      dlg.pause = DLG_QUIT
      exit do
    end if
  loop
  dlg.undim()
end function

' wait for Enter (or Esc / Space / Q)
sub dlg.waitEnter()
  local k$
  do
    k$ = inkey$
    if k$ = "" then pause 5
  loop until key.isEnter(k$) or k$ = chr$(K_ESC) or k$ = " " or key.isQuit(k$)
end sub

' wait up to ms, or until Enter / Space / Esc
sub dlg.waitFor(ms)
  local t, k$
  t = timer + ms
  do while timer < t
    k$ = inkey$
    if key.isEnter(k$) or k$ = " " or k$ = chr$(K_ESC) then exit do
    pause 5
  loop
end sub

' a banner over the dimmed scene: a big gold line and up to two small lines.
' Shown for ms (or until a key); the scene comes back afterwards.
sub dlg.banner(hd$, l1$, s1, l2$, s2, ms)
  local x, cx, y
  dlg.dim()
  x = ui.dialog("banner", BANNER_W, BANNER_H)
  y = ui.py
  cx = x + BANNER_W \ 2
  bigC hd$, cx, y + 12
  if l1$ <> "" then txtC l1$, cx, y + 42, s1
  if l2$ <> "" then txtC l2$, cx, y + 60, s2
  scr.present()
  key.drain()
  dlg.waitFor(ms)
  key.drain()
  dlg.undim()
  scr.present()
end sub

' name entry: frame, typed letters, block cursor, underlines (8 slots)
sub dlg.slots(bx, by, n$, cur)
  local i, sx
  ui.spr(NB_X, IC_Y, NB_W, NB_H, bx, by)
  for i = 0 to HS_MAXNAME - 1
    sx = bx + 8 + i * 12
    if i < len(n$) then
      txt mid$(n$, i + 1, 1), sx + 2, by + 6, F_GOLD
    elseif i = len(n$) and cur then
      af sx + 1, by + 5, 9, 11, C_CY1
      af sx + 1, by + 5, 9, 1, C_CY2
    end if
    if i <= len(n$) then
      af sx + 1, by + 17, 9, 1, C_CY1
    else
      af sx + 1, by + 17, 9, 1, C_M1
    end if
  next i
end sub

' redraw the name box and copy just that strip to the screen
sub dlg.nameShow(bx, by, n$, cur)
  dlg.slots(bx, by, n$, cur)
  scr.markA(bx, by, NB_W, NB_H)
  scr.flush()
end sub

' the end of a game (won = all levels cleared; also after quitting): SCORE,
' LEVELS cleared, BEST; a top-10 score asks for a name, saves it and shows
' the table
sub dlg.gameOver(won, qt)
  local x, y, w, h, cx, rk, k, k$, n$, c$, cur, tb, bx, by, best, rx
  cur = TRUE
  rk = hs.rank(gm.score)
  best = max(hs.score(0), gm.score)
  dlg.dim()
  w = OVER_W
  h = OVER_H
  if won then
    x = ui.dialog("win", w, h)
  else
    x = ui.dialog("over", w, h)
  end if
  y = ui.py
  cx = x + w \ 2
  rx = x + w - 36
  txtR str$(gm.score), rx, y + 44, F_WHITE
  txtR str$(gm.li - gm.start) + " OF " + str$(LV_N - gm.start), rx, y + 59, F_WHITE
  txtR str$(best), rx, y + 74, F_GOLD
  bx = x + (w - NB_W) \ 2
  by = y + 140
  if rk >= 0 then
    txtC "NEW HIGH SCORE! RANK " + str$(rk + 1), cx, y + 104, F_GOLD
    txtC "ENTER YOUR NAME", cx, y + 122, F_GREY
    dlg.slots(bx, by, "", TRUE)
  else
    txtC "NO HIGH SCORE THIS TIME", cx, y + 112, F_GREY
    txtC "PRESS ENTER", cx, y + 140, F_DIM
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
  if n$ = "" then n$ = "CHIP"
  hs.insert(rk, n$, gm.score, gm.reached, gm.diff)
  hs.save()
  dlg.scores(rk)
end sub

' high-score table: #, NAME, SKILL, LEVEL, SCORE; hl = row to highlight (-1 none)
sub dlg.scores(hl)
  local x, y, w, hy, ry, i, st, rs, d
  x = lay.sx
  y = lay.sy
  w = SC_W
  page copy PG_SCORES to PG_BACK
  page write PG_BACK
  hy = y + 10
  for i = 0 to 9
    ry = hy + 20 + i * lay.sstep
    if i = hl then ui.bar(x + 10, ry - 5, w - 20)
    st = F_WHITE
    if i = 0 then st = F_GOLD
    if i = hl then st = F_CYAN
    rs = F_GREY
    if i = 0 or i = hl then rs = st
    txtR str$(i + 1), x + 30, ry, rs
    if hs.score(i) > 0 then
      txt hs.name$(i), x + 44, ry, st
      d = hs.diff(i)
      txt df.name$(d), x + 132, ry, df.style(d)
      txt hs.lvlName$(hs.lvl(i)), x + 228, ry, rs
      txtR str$(hs.score(i)), x + w - 16, ry, st
    else
      txt "---", x + 44, ry, F_DIM
    end if
  next i
  scr.present()
  key.drain()
  dlg.waitEnter()
end sub
