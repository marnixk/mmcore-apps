' dialogs.inc - pause, banners, level clear, out of moves, game over with
' name entry, and the high-score table. The panels (with their fixed labels)
' are PNGs loaded straight onto the dimmed back page; values are drawn at run
' time. All coordinates in UI art units. Dialogs read INKEY$ in a plain loop.
const DLG_RESUME = 0, DLG_RESTART = 1, DLG_QUIT = 2, DLG_NEXT = 3, DLG_MENU = 4, DLG_RETRY = 5

' dark checker over the back page
sub dlg.dim()
  page copy PG_DIM to PG_BACK, B
  page write PG_BACK
end sub

' pause dialog with the controls: DLG_RESUME, DLG_RESTART (R) or DLG_QUIT (Q)
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

' a banner over the dimmed scene: a big gold line and up to two small lines,
' shown for ms (or until a key)
sub dlg.banner(hd$, l1$, s1, l2$, s2, ms)
  local x, cx, y
  dlg.dim()
  x = ui.dialog("banner", BANNER_W, BANNER_H)
  y = ui.py
  cx = x + BANNER_W \ 2
  bigC hd$, cx, y + 16
  if l1$ <> "" then txtC l1$, cx, y + 48, s1
  if l2$ <> "" then txtC l2$, cx, y + 66, s2
  scr.present()
  key.drain()
  dlg.waitFor(ms)
  key.drain()
end sub

' level start: LEVEL nn, its name and the goals
dim dlg.bannerMs = 2200
sub dlg.levelBanner()
  local x, cx, y, k
  dlg.dim()
  x = ui.dialog("banner", BANNER_W, BANNER_H)
  y = ui.py
  cx = x + BANNER_W \ 2
  bigC "LEVEL " + n2$(gm.li + 1), cx, y + 10
  txtC lv.name$(gm.li), cx, y + 34, F_LABEL
  for k = 0 to lv.ng(gm.li) - 1
    txtC gm.goalText$(k), cx, y + 52 + 13 * k, F_WHITE
  next k
  scr.present()
  key.drain()
  dlg.waitFor(dlg.bannerMs)
  key.drain()
  gm.redraw()
end sub

' a framed box as in the art (dark rim, gold edge with a light top / left)
sub dlg.frame(x, y, w, h, fill)
  af x, y, w, h, C_K
  af x + 1, y + 1, w - 2, h - 2, C_AU1
  af x + 1, y + 1, w - 2, 1, C_AU3
  af x + 1, y + 1, 1, h - 2, C_AU3
  af x + 2, y + 2, w - 4, h - 4, fill
end sub

' two choices side by side (sel = 0 left, 1 right): gold underline on the pick
sub dlg.choices(cx, y, a$, b$, sel)
  local xa, xb, w
  w = txtW(a$ + "   " + b$)
  xa = cx - w \ 2
  xb = xa + txtW(a$ + "   ")
  af xa - 4, y - 3, w + 8, 15, C_ST1
  if sel = 0 then
    txt a$, xa, y, F_GOLD
    txt b$, xb, y, F_GREY
    af xa, y + 10, txtW(a$), 1, C_AU2
  else
    txt a$, xa, y, F_GREY
    txt b$, xb, y, F_GOLD
    af xb, y + 10, txtW(b$), 1, C_AU2
  end if
  scr.markA(xa - 4, y - 3, w + 8, 15)
end sub

' Left / Right pick one of two, Enter takes it, Esc takes the right one
function dlg.choose(cx, y, a$, b$)
  local sel, k$
  sel = 0
  dlg.choices(cx, y, a$, b$, sel)
  scr.flush()
  pause 300
  key.drain()
  do
    k$ = inkey$
    if k$ = "" then
      pause 5
    else
      if asc(k$) = K_LEFT or asc(k$) = K_RIGHT then
        sel = 1 - sel
        dlg.choices(cx, y, a$, b$, sel)
        scr.flush()
      elseif key.isEnter(k$) or k$ = " " then
        exit do
      elseif k$ = chr$(K_ESC) or key.isQuit(k$) then
        sel = 1
        exit do
      end if
    end if
  loop
  dlg.choose = sel
end function

' level clear: the level's score (moves left x PTS_MOVE included), its stars
' and the best so far; NEXT LEVEL (FINISH after the last) or MENU
function dlg.clear(pts, st, best)
  local x, y, cx, ty, i, n$
  pause 400
  dlg.dim()
  x = ui.dialog("clear", CLEAR_W, CLEAR_H)
  y = ui.py
  cx = x + CLEAR_W \ 2
  ty = y + CLEAR_TY
  txtC "LEVEL " + n2$(gm.li + 1) + " CLEAR!", cx, ty, F_TITLE
  txt "GOALS", x + 14, ty + 14, F_LABEL
  txtR "DONE", x + CLEAR_W - 14, ty + 14, F_GOLD
  txt "MOVES LEFT", x + 14, ty + 26, F_LABEL
  txtR str$(en.moves) + " X " + str$(PTS_MOVE), x + CLEAR_W - 14, ty + 26, F_GOLD
  txt "SPECIALS", x + 14, ty + 38, F_LABEL
  txtR str$(en.spc), x + CLEAR_W - 14, ty + 38, F_GOLD
  txt "BEST", x + 14, ty + 50, F_LABEL
  txtR str$(best), x + CLEAR_W - 14, ty + 50, F_GOLD
  for i = 0 to 2
    if i < st then
      ui.spr(IC_STAR, IC2_Y, 24, 24, cx - 42 + 30 * i, ty + 64)
    else
      ui.spr(IC_STAROFF, IC2_Y, 24, 24, cx - 42 + 30 * i, ty + 64)
    end if
  next i
  dlg.frame(x + 12, ty + 94, CLEAR_W - 24, 26, C_ST0)
  txt "SCORE", x + 20, ty + 103, F_LABEL
  bigR str$(pts), x + CLEAR_W - 20, ty + 99
  n$ = "NEXT LEVEL"
  if gm.li + 1 >= LV_N then n$ = "FINISH"
  scr.present()
  if dlg.choose(cx, ty + 130, n$, "MENU") = 0 then
    dlg.clear = DLG_NEXT
  else
    dlg.clear = DLG_MENU
  end if
end function

' out of moves: what was left, RETRY or MENU
function dlg.fail()
  local x, y, cx, k, gx
  pause 400
  dlg.dim()
  x = ui.dialog("fail", FAIL_W, FAIL_H)
  y = ui.py
  cx = x + FAIL_W \ 2
  gx = cx - en.ng * 30
  for k = 0 to en.ng - 1
    ui.spr(hud.icon(k), IC_Y, 20, 20, gx + 60 * k, y + 54)
    if en.gl(k) <= 0 then
      ui.spr(IC_TICK, IC_Y, 20, 20, gx + 60 * k + 22, y + 54)
    else
      txt str$(en.gl(k)), gx + 60 * k + 24, y + 60, F_WHITE
    end if
  next k
  scr.present()
  if dlg.choose(cx, y + 96, "RETRY", "MENU") = 0 then
    dlg.fail = DLG_RETRY
  else
    dlg.fail = DLG_MENU
  end if
end function

' name entry: frame, typed letters, block cursor, underlines (8 slots)
sub dlg.slots(bx, by, n$, cur)
  local i, sx
  ui.spr(NB_X, IC2_Y, NB_W, NB_H, bx, by)
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
      af sx + 1, by + 17, 9, 1, C_ST3
    end if
  next i
end sub

sub dlg.nameShow(bx, by, n$, cur)
  dlg.slots(bx, by, n$, cur)
  scr.markA(bx, by, NB_W, NB_H)
  scr.flush()
end sub

' the end of a game (all levels cleared, or back to the menu): SCORE (the
' total), LEVELS cleared, BEST; a top-10 score asks for a name, saves it and
' shows the table
sub dlg.gameOver(won, qt)
  local x, y, w, h, cx, rk, k, k$, n$, c$, cur, tb, bx, by, best, rx
  cur = TRUE
  rk = hs.rank(gm.total)
  best = max(hs.score(0), gm.total)
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
  txtR str$(gm.total), rx, y + 44, F_WHITE
  txtR str$(gm.cleared), rx, y + 59, F_WHITE
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
  ' a short guard, then drop the keys still buffered from play
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
  if n$ = "" then n$ = "MINER"
  hs.insert(rk, n$, gm.total, gm.reached, gm.diff)
  hs.save()
  dlg.scores(rk)
end sub

' high-score table: #, NAME, SKILL, LEVEL, SCORE; hl = row to highlight (-1 none)
sub dlg.scores(hl)
  local x, y, w, hy, ry, i, st, rs, d
  x = lay.sx
  y = lay.sy
  w = SC_W
  scr.menu("scores")
  hy = y + 9
  for i = 0 to 9
    ry = hy + 20 + i * lay.sstep
    if i = hl then ui.bar(x + 10, ry - 5, w - 20)
    st = F_WHITE
    if i = 0 then st = F_GOLD
    if i = hl then st = F_WHITE
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
