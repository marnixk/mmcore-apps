' dialogs.inc - pause, game over with name entry, and the high-score table.
' Dialogs are drawn at run time on the back page over the dimmed game scene.

' back page = game scene seen through the dark checker overlay (scene kept on PG_SAVE)
sub dlg.dimScene()
  scr.save()
  page copy PG_DIM to PG_BACK, B
  page write PG_BACK
end sub

' pause dialog with the controls. returns TRUE when the player quits (Q)
function dlg.pause()
  local s = scr.sc, x, y, kx, i, k
  dlg.dimScene()
  x = ui.centre(300, 176)
  y = ui.py
  bigC "PAUSED", x + 150 * s, y + 13 * s, B_GOLD
  y = y + 46 * s
  kx = x + 30 * s
  for i = 1 to 4
    kx = kx + ui.keycap(kx, y, mid$("{}[]", i, 1)) + 3 * s
  next i
  txt "STEER", x + 128 * s, y + 3 * s, F_CREAM
  y = y + 26 * s
  kx = x + 30 * s
  kx = kx + ui.keycap(kx, y, "P") + 3 * s
  kx = kx + ui.keycap(kx, y, "ESC")
  txt "PAUSE / RESUME", x + 128 * s, y + 3 * s, F_CREAM
  y = y + 26 * s
  kx = ui.keycap(x + 30 * s, y, "Q")
  txt "QUIT TO MENU", x + 128 * s, y + 3 * s, F_CREAM
  y = y + 26 * s
  ui.rule(x + 18 * s, y + 2 * s, 264 * s)
  txtC "P OR ESC TO CONTINUE", x + 150 * s, y + 12 * s, F_GREEN
  scr.present()
  key.flush()
  dlg.pause = FALSE
  do
    k = key.get()
    if k = 0 then
      pause 15
    elseif key.isPause(k) then
      exit do
    elseif k = asc("q") or k = asc("Q") then
      dlg.pause = TRUE
      exit do
    end if
  loop
  key.flush()
end function

' wait for Enter (or Esc / Space)
sub dlg.waitEnter()
  local k
  key.flush()
  do
    k = key.get()
    if k = 0 then pause 15
  loop until key.isEnter(k) or k = K_ESC or k = K_SPACE or k = asc("q") or k = asc("Q")
  key.flush()
end sub

' name slots of the name-entry dialog (current write page)
sub dlg.slots(x, y, n$, cur)
  local s = scr.sc, i, sx
  for i = 0 to HS_MAXNAME - 1
    sx = x + i * 12 * s
    rfill sx, y, 10 * s, 14 * s, C_K
    rfill sx + s, y + s, 8 * s, 12 * s, C_K3
    if i < len(n$) then txt mid$(n$, i + 1, 1), sx + s, y + 3 * s, F_CREAM
    if i = len(n$) and cur then
      rfill sx + 2 * s, y + 11 * s, 6 * s, s, C_Y2
    else
      rfill sx + 2 * s, y + 11 * s, 6 * s, s, C_K2
    end if
  next i
end sub

' game over: score summary; a top-10 score asks for a name, saves it and shows the table
sub dlg.gameOver()
  local s = scr.sc, x, y, cx, rk, k, n$, c$, cur = TRUE, tb, bx, by, i
  rk = hs.rank(gm.score)
  dlg.dimScene()
  x = ui.centre(300, 168)
  y = ui.py
  cx = x + 150 * s
  bigC "GAME OVER", cx, y + 13 * s, B_RED
  for i = 0 to 2
    txt choice(i = 0, "SCORE", choice(i = 1, "LEVEL", "FRUIT")), x + 40 * s, y + (44 + 14 * i) * s, F_GOLD
    k = choice(i = 0, gm.score, choice(i = 1, gm.lvn, gm.fruits))
    txtR str$(k), x + 260 * s, y + (44 + 14 * i) * s, F_CREAM
  next i
  ui.rule(x + 18 * s, y + 88 * s, 264 * s)
  bx = x + 102 * s
  by = y + 126 * s
  if rk >= 0 then
    txtC "NEW HIGH SCORE!", cx, y + 98 * s, F_GREEN
    txtC "ENTER YOUR NAME", cx, y + 112 * s, F_GREY
    dlg.slots(bx, by, "", TRUE)
  else
    txtC "NOT IN THE TOP 10 THIS TIME", cx, y + 104 * s, F_GREY
    txtC "PRESS ENTER", cx, y + 128 * s, F_GOLD
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
  hs.insert(rk, n$, gm.score, gm.lvn, gm.diff)
  hs.save()
  dlg.scores(rk)
end sub

' high-score table; hl = row to highlight (-1 none)
sub dlg.scores(hl)
  local s = scr.sc, x = lay.sx, y = lay.sy, w = lay.sw, hy, ry, i, r
  page copy PG_SCORES to PG_BACK
  page write PG_BACK
  ui.panel(x, y, w, lay.sh)
  hy = y + 14 * s
  txt "#", x + 22 * s, hy, F_GREY
  txt "NAME", x + 50 * s, hy, F_GREY
  txt "LEVEL", x + 150 * s, hy, F_GREY
  txt "SKILL", x + 216 * s, hy, F_GREY
  txtR "SCORE", x + w - 22 * s, hy, F_GREY
  ui.rule(x + 14 * s, hy + 12 * s, w - 28 * s)
  for i = 0 to 9
    ry = hy + 20 * s + i * lay.sstep
    if i = hl then rfill x + 12 * s, ry - 5 * s, w - 24 * s, 17 * s, C_K3
    r = F_CREAM
    if i = 0 then r = F_GOLD
    if i = hl then r = F_GREEN
    txtR str$(i + 1), x + 30 * s, ry, choice(i = 0 or i = hl, r, F_GREY)
    if hs.score(i) > 0 then
      txt hs.name$(i), x + 50 * s, ry, r
      ui.tile(T_FRUIT + (hs.level(i) - 1) mod 8, x + 150 * s, ry + 4 * s - lay.cell \ 2)
      txt zpad$(hs.level(i), 2), x + 152 * s + lay.cell, ry, r
      txt df.name$(hs.diff(i)), x + 216 * s, ry, df.row(hs.diff(i))
      txtR zpad$(hs.score(i), 5), x + w - 22 * s, ry, r
    else
      txt "---", x + 50 * s, ry, F_DIM
    end if
  next i
  scr.present()
  dlg.waitEnter()
end sub
