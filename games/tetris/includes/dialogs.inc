' dialogs.inc - pause, game over, name entry and the high-score table.
' Dialogs are drawn at run time with the lacquer UI primitives over the
' dimmed game scene (PG_DIM: the well is hidden while a dialog is up).

' GAME OVER banner across the well (current write page)
sub dlg.gameOver()
  local s = scr.sc, y, w, cx
  w = 10 * lay.cell
  cx = lay.wx + w \ 2
  y = lay.wy + 10 * lay.cell - 31 * s
  rfill lay.wx, y, w, 62 * s, C_INK
  rfill lay.wx, y + s, w, s, C_G3
  rfill lay.wx, y + 60 * s, w, s, C_G3
  rfill lay.wx, y + 3 * s, w, s, C_R1
  rfill lay.wx, y + 58 * s, w, s, C_R1
  bigC "GAME", cx, y + 6 * s, B_RED
  bigC "OVER", cx, y + 25 * s, B_RED
  txtC "PRESS ENTER", cx, y + 46 * s, F_GOLD
end sub

' pause dialog with the controls. returns TRUE when the player quits (Q)
function dlg.pause()
  local s = scr.sc, w, h, x, y, ry, tx, k$, kw
  w = 300 * s
  h = 232 * s
  x = (scr.W - w) \ 2
  y = (scr.H - h) \ 2
  page copy PG_DIM to PG_BACK
  page write PG_BACK
  ui.panel(x, y, w, h)
  ui.ribbon(x + 80 * s, y + 12 * s, w - 160 * s, 14 * s, "PAUSED", F_GOLD)
  ry = y + 36 * s
  tx = x + 110 * s
  ' move
  kw = ui.keycap(x + 28 * s, ry, "", 0)
  kw = ui.keycap(x + 46 * s, ry, "", 1)
  txt "MOVE", tx, ry + 4 * s, F_CREAM
  ry = ry + 20 * s
  kw = ui.keycap(x + 28 * s, ry, "", 3)
  txt "SOFT DROP", tx, ry + 4 * s, F_CREAM
  ry = ry + 20 * s
  kw = ui.keycap(x + 28 * s, ry, "SPACE", -1)
  txt "HARD DROP", tx, ry + 4 * s, F_CREAM
  ry = ry + 20 * s
  kw = ui.keycap(x + 28 * s, ry, "X", -1)
  kw = ui.keycap(x + 28 * s + kw + 3 * s, ry, "", 2)
  txt "ROTATE RIGHT", tx, ry + 4 * s, F_CREAM
  ry = ry + 20 * s
  kw = ui.keycap(x + 28 * s, ry, "Z", -1)
  txt "ROTATE LEFT", tx, ry + 4 * s, F_CREAM
  ry = ry + 20 * s
  kw = ui.keycap(x + 28 * s, ry, "C", -1)
  txt "HOLD PIECE", tx, ry + 4 * s, F_CREAM
  ry = ry + 20 * s
  kw = ui.keycap(x + 28 * s, ry, "ESC", -1)
  txt "PAUSE / RESUME", tx, ry + 4 * s, F_CREAM
  ry = ry + 20 * s
  kw = ui.keycap(x + 28 * s, ry, "Q", -1)
  txt "QUIT TO MENU", tx, ry + 4 * s, F_CREAM
  ui.divider(x + 20 * s, y + h - 34 * s, w - 40 * s)
  txtC "ESC TO RESUME  -  Q TO QUIT", x + w \ 2, y + h - 22 * s, F_GOLD
  scr.present()
  dlg.pause = FALSE
  do
    k$ = inkey$
    if k$ = "" then
      pause 5
    elseif k$ = chr$(K_ESC) then
      exit do
    elseif key.isQuit(k$) then
      dlg.pause = TRUE
      exit do
    end if
  loop
end function

' name slots of the name-entry dialog (current write page = PG_BACK)
sub dlg.nameSlots(x, y, n$, cur)
  local s = scr.sc, i, sx, c$
  for i = 0 to HS_MAXNAME - 1
    sx = x + i * 24 * s
    rfill sx, y, 20 * s, 24 * s, C_INK
    rfill sx + s, y + s, 18 * s, 22 * s, C_INK2
    if i < len(n$) then
      c$ = mid$(n$, i + 1, 1)
      big c$, sx + s, y + 3 * s, B_WHITE
    end if
    if i = len(n$) and cur then
      rfill sx + 3 * s, y + 20 * s, 14 * s, 2 * s, C_G3
    else
      rfill sx + 3 * s, y + 20 * s, 14 * s, 2 * s, C_G1
    end if
  next i
end sub

' new high score: ask for a name (Enter or Esc saves; empty = PLAYER)
function dlg.enterName$(lv, rk)
  local s = scr.sc, w, h, x, y, k, k$, n$, sx, sy, cur = TRUE, tb, r, c$
  w = 290 * s
  h = 196 * s
  x = (scr.W - w) \ 2
  y = (scr.H - h) \ 2
  if lv = LV_EASY then r = F_GREEN
  if lv = LV_MEDIUM then r = F_GOLD
  if lv = LV_HARD then r = F_RED
  page copy PG_DIM to PG_BACK
  page write PG_BACK
  ui.panel(x, y, w, h)
  ui.ribbon(x + 60 * s, y + 12 * s, w - 120 * s, 14 * s, "NEW HIGH SCORE!", F_GOLD)
  txtC lv.name$(lv) + "  -  RANK " + str$(rk + 1), x + w \ 2, y + 34 * s, r
  txt "SCORE", x + 40 * s, y + 52 * s, F_GREY
  txtR str$(gm.score), x + w - 40 * s, y + 52 * s, F_CREAM
  txt "LEVEL", x + 40 * s, y + 66 * s, F_GREY
  txtR str$(gm.level), x + w - 40 * s, y + 66 * s, F_CREAM
  txt "LINES", x + 40 * s, y + 80 * s, F_GREY
  txtR str$(gm.lines), x + w - 40 * s, y + 80 * s, F_CREAM
  ui.divider(x + 20 * s, y + 96 * s, w - 40 * s)
  txtC "ENTER YOUR NAME", x + w \ 2, y + 106 * s, F_GOLD3
  txtC "ENTER SAVES", x + w \ 2, y + h - 24 * s, F_GOLD
  sx = x + (w - (HS_MAXNAME * 24 - 4) * s) \ 2
  sy = y + 122 * s
  n$ = ""
  dlg.nameSlots(sx, sy, n$, cur)
  scr.present()
  tb = timer
  do
    k$ = inkey$
    if k$ = "" then
      if timer - tb > 400 then
        cur = not cur
        tb = timer
        page write PG_BACK
        dlg.nameSlots(sx, sy, n$, cur)
        scr.show(sx, sy, HS_MAXNAME * 24 * s, 24 * s)
      end if
      pause 5
    else
      k = asc(k$)
      if key.isEnter(k$) or k = K_ESC then exit do
      if (k = K_BKSP or k = K_DEL or k = K_LEFT) and len(n$) > 0 then
        n$ = left$(n$, len(n$) - 1)
      elseif k >= 32 and k < 127 and len(n$) < HS_MAXNAME then
        c$ = ucase$(k$)
        if instr(BIG$, c$) > 0 and c$ <> "_" and c$ <> "?" then
          if not (c$ = " " and len(n$) = 0) then n$ = n$ + c$
        end if
      end if
      cur = TRUE
      tb = timer
      page write PG_BACK
      dlg.nameSlots(sx, sy, n$, cur)
      scr.show(sx, sy, HS_MAXNAME * 24 * s, 24 * s)
    end if
  loop
  n$ = rtrim$(n$)
  if n$ = "" then n$ = "PLAYER"
  dlg.enterName$ = n$
end function

function rtrim$(a$)
  local n = len(a$)
  do while n > 0
    if mid$(a$, n, 1) <> " " then exit do
    n = n - 1
  loop
  rtrim$ = left$(a$, n)
end function

' high-score table, three columns. hlLv/hlRank highlight a new entry (-1 none)
sub dlg.scores(hlLv, hlRank)
  local s = scr.sc, c, i, x, y, r, sr, k$, t$
  page copy PG_SCORES to PG_BACK
  page write PG_BACK
  for c = 0 to 2
    x = lay.sx0 + c * (lay.spw + lay.sgap)
    if lay.slv then
      sr = x + 154 * s
    else
      sr = x + lay.spw - 12 * s
    end if
    for i = 0 to 9
      y = lay.sy0 + i * lay.sstep
      if c = hlLv and i = hlRank then
        rfill x + 6 * s, y - 3 * s, lay.spw - 12 * s, 14 * s, C_G1
        rfill x + 7 * s, y - 2 * s, lay.spw - 14 * s, 12 * s, C_R1
      end if
      r = F_CREAM
      if i = 0 then r = F_GOLD
      if i = 1 then r = F_SILVER
      if i = 2 then r = F_RED
      t$ = str$(i + 1)
      if i < 9 then
        txt t$, x + 20 * s, y, r
      else
        txt t$, x + 12 * s, y, r
      end if
      if hs.score(c, i) > 0 then
        txt hs.name$(c, i), x + 38 * s, y, r
        txtR str$(hs.score(c, i)), sr, y, r
        if lay.slv then txt "L" + str$(hs.level(c, i)), x + lay.spw - 36 * s, y, F_GREY
      else
        txt "---", x + 38 * s, y, F_DIM
      end if
    next i
  next c
  scr.present()
  do
    k$ = inkey$
    if k$ = "" then pause 5
  loop until k$ = chr$(K_ESC) or key.isEnter(k$) or key.isQuit(k$) or k$ = " "
end sub
