' dialogs.inc - pause, game over with name entry, and the high-score table.
' Dialogs are drawn at run time on the back page over the dimmed game scene
' (all coordinates in art units). The game redraws its scene after a pause.

' dark checker over the back page (PG_DIM: dark ink pixels, black = see-through)
sub dlg.dim()
  page copy PG_DIM to PG_BACK, B
  page write PG_BACK
end sub

' pause dialog with the controls. returns TRUE when the player quits (Q)
function dlg.pause()
  local x, y, kx, k, cx
  dlg.dim()
  x = ui.centre(300, 176)
  y = ui.py
  cx = x + 150
  bigC "PAUSED", cx, y + 13, B_GOLD
  y = y + 44
  kx = x + 22
  kx = kx + ui.keycap(kx, y, "SPACE") + 3
  kx = kx + ui.keycap(kx, y, "{") + 3
  kx = kx + ui.keycap(kx, y, "W")
  txt "THRUST", x + 150, y + 3, F_CREAM
  y = y + 24
  kx = x + 22
  kx = kx + ui.keycap(kx, y, "P") + 3
  kx = kx + ui.keycap(kx, y, "ESC")
  txt "PAUSE / RESUME", x + 150, y + 3, F_CREAM
  y = y + 24
  kx = ui.keycap(x + 22, y, "Q")
  txt "QUIT TO MENU", x + 150, y + 3, F_CREAM
  y = y + 26
  ui.rule(x + 18, y, 264)
  txtC "P OR ESC TO CONTINUE", cx, y + 12, F_CYAN
  scr.present()
  key.flush()
  dlg.pause = FALSE
  do
    k = key.get()
    if k = 0 then
      pause 15
    elseif key.isPause(k) then
      exit do
    elseif key.isQuit(k) then
      dlg.pause = TRUE
      exit do
    end if
  loop
  key.flush()
end function

' wait for Enter (or Esc / Space / Q)
sub dlg.waitEnter()
  local k
  key.flush()
  do
    k = key.get()
    if k = 0 then pause 15
  loop until key.isEnter(k) or k = K_ESC or k = K_SPACE or key.isQuit(k)
  key.flush()
end sub

' name slots of the name-entry dialog (current write page)
sub dlg.slots(x, y, n$, cur)
  local i, sx
  for i = 0 to HS_MAXNAME - 1
    sx = x + i * 12
    af sx, y, 10, 14, C_K
    af sx + 1, y + 1, 8, 12, C_WB2
    if i < len(n$) then txt mid$(n$, i + 1, 1), sx + 1, y + 3, F_CREAM
    if i = len(n$) and cur then
      af sx + 2, y + 11, 6, 1, C_C3
    else
      af sx + 2, y + 11, 6, 1, C_WB1
    end if
  next i
end sub

' game over: SCORE / BEST / GEMS; a top-10 score asks for a name, saves it and
' shows the table
sub dlg.gameOver()
  local s, x, y, cx, rk, k, n$, c$, cur, tb, bx, by, best, i
  s = scr.sc
  cur = TRUE
  rk = hs.rank(gm.score)
  best = max(hs.score(0), gm.score)
  dlg.dim()
  ' the wreck stays bright on top of the dim
  bl(SP_A_HIT_X * s, SP_A_HIT_Y * s, SHX * s, sh.dy * s, 64 * s, 48 * s, PG_SHEET, 1)
  for i = 0 to 2
    if bm.f(i) >= 0 then bl((SP_BOOM_0_X + 32 * bm.f(i)) * s, SP_BOOM_0_Y * s, (SHX + bm.dx(i)) * s, (sh.dy + bm.dy(i)) * s, 32 * s, 32 * s, PG_SHEET, 1)
  next i
  x = ui.centre(300, 176)
  y = ui.py
  cx = x + 150
  bigC "GAME OVER", cx, y + 13, B_RED
  txt "SCORE", x + 40, y + 44, F_GOLD
  txtR str$(gm.score), x + 260, y + 44, F_CREAM
  txt "BEST", x + 40, y + 59, F_GOLD
  txtR str$(best), x + 260, y + 59, F_GOLD
  txt "GEMS", x + 40, y + 74, F_PINK
  txtR str$(gm.gems), x + 260, y + 74, F_PINK
  ui.gem(x + 260 - txtW(str$(gm.gems)) - 18, y + 70)
  ui.rule(x + 18, y + 92, 264)
  bx = x + (300 - 8 * 12) \ 2
  by = y + 132
  if rk >= 0 then
    txtC "TOP 10! RANK " + str$(rk + 1), cx, y + 102, F_GREEN
    txtC "ENTER YOUR NAME", cx, y + 116, F_GREY
    dlg.slots(bx, by, "", TRUE)
  else
    txtC "NOT IN THE TOP 10 THIS TIME", cx, y + 106, F_GREY
    txtC "PRESS ENTER", cx, y + 130, F_GOLD
  end if
  scr.present()
  pause 600
  if rk < 0 then
    dlg.waitEnter()
    exit sub
  end if
  key.flush()
  n$ = ""
  tb = timer
  do
    k = key.get()
    if k = 0 then
      if timer - tb > 400 then
        cur = not cur
        tb = timer
        dlg.slots(bx, by, n$, cur)
        scr.present()
      end if
      pause 15
    else
      if key.isEnter(k) or k = K_ESC then exit do
      if (k = K_BKSP or k = K_DEL or k = K_LEFT) and len(n$) > 0 then
        n$ = left$(n$, len(n$) - 1)
      elseif k >= 32 and k < 127 and len(n$) < HS_MAXNAME then
        c$ = ucase$(chr$(k))
        if instr("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 -!.", c$) > 0 then
          if not (c$ = " " and len(n$) = 0) then n$ = n$ + c$
        end if
      end if
      cur = TRUE
      tb = timer
      dlg.slots(bx, by, n$, cur)
      scr.present()
    end if
  loop
  do while right$(n$, 1) = " "
    n$ = left$(n$, len(n$) - 1)
  loop
  if n$ = "" then n$ = "PLAYER"
  hs.insert(rk, n$, gm.score, gm.gems, gm.diff)
  hs.save()
  dlg.scores(rk)
end sub

' high-score table; hl = row to highlight (-1 none)
sub dlg.scores(hl)
  local x, y, w, hy, ry, i, r, rn, d
  x = lay.sx
  y = lay.sy
  w = lay.sw
  page copy PG_SCORES to PG_BACK
  page write PG_BACK
  ui.panel(x, y, w, lay.sh)
  hy = y + 14
  txt "#", x + 22, hy, F_GREY
  txt "NAME", x + 50, hy, F_GREY
  txt "SKILL", x + 150, hy, F_GREY
  txt "GEMS", x + 228, hy, F_GREY
  txtR "SCORE", x + w - 22, hy, F_GREY
  ui.rule(x + 14, hy + 12, w - 28)
  for i = 0 to 9
    ry = hy + 20 + i * lay.sstep
    if i = hl then ui.bar(x + 12, ry - 5, w - 24)
    r = F_CREAM
    if i = 0 then r = F_GOLD
    if i = hl then r = F_CYAN
    rn = F_GREY
    if i = 0 or i = hl then rn = r
    txtR str$(i + 1), x + 30, ry, rn
    if hs.score(i) > 0 then
      txt hs.name$(i), x + 50, ry, r
      d = hs.diff(i)
      txt df.name$(d), x + 150, ry, df.row(d)
      ui.gem(x + 226, ry - 4)
      txtR str$(hs.gems(i)), x + 262, ry, rn
      txtR str$(hs.score(i)), x + w - 22, ry, r
    else
      txt "---", x + 50, ry, F_DIM
    end if
  next i
  scr.present()
  dlg.waitEnter()
end sub
