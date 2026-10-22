' dialogs.inc - wood-framed parchment dialogs: pause/help, win, high scores and
' name entry. Each one is drawn over the current back-buffer contents.

dim dlg.x, dlg.y

' blit the dialog frame centred on the current write page
sub dlg.frame()
  dlg.x = (scr.W - lay.dlgW) \ 2
  dlg.y = (scr.H - lay.dlgH) \ 2
  blit 0, 0, dlg.x, dlg.y, lay.dlgW, lay.dlgH, PG_PANELS, 4
end sub

sub dlg.icon(n, x, y)
  local s = 8 * scr.sc
  blit n * s, lay.iconY, x, y, s, s, PG_SHEET, 4
end sub

sub dlg.header(icon, t$, r)
  local sc = scr.sc
  dlg.icon(icon, dlg.x + 12 * sc, dlg.y + 11 * sc)
  txt t$, dlg.x + 23 * sc, dlg.y + 11 * sc, r
end sub

' ---- pause / help: returns TRUE when the player wants to quit to the menu
function dlg.pause()
  local k, sc = scr.sc, y, i
  local string keys$(4) = ("ARROWS", "1-9", "0 DEL BKSP", "ESC", "Q")
  local string acts$(4) = ("MOVE", "PLACE DIGIT", "CLEAR CELL", "RESUME", "QUIT TO MENU")
  game.compose()
  dlg.frame()
  dlg.header(2, "PAUSED - CONTROLS", F_GREEN)
  for i = 0 to 4
    y = dlg.y + 29 * sc + i * 13 * sc
    txt keys$(i), dlg.x + 14 * sc, y, F_RED
    txt acts$(i), dlg.x + 80 * sc, y, F_BROWN
  next i
  txt "GIVENS ARE LOCKED", dlg.x + 14 * sc, dlg.y + 29 * sc + 5 * 13 * sc + 6 * sc, F_GREEN
  scr.present()
  key.flush()
  do
    music.poll()
    k = key.get()
    if k = K_ESC then
      dlg.pause = FALSE
      exit do
    end if
    if k = asc("q") or k = asc("Q") then
      dlg.pause = TRUE
      exit do
    end if
    pause 15
  loop
  key.flush()
end function

' ---- win summary; returns when a key is pressed
sub dlg.win(lv, secs, misses, score, qualifies)
  local sc = scr.sc, x, y, k, t = timer, tm$
  game.compose()
  dlg.frame()
  dlg.header(4, "WELL DONE!", F_GREEN)
  x = dlg.x + 24 * sc
  y = dlg.y + 32 * sc
  txt "LEVEL", x, y, F_BROWN
  txt lv.name$(lv), x + 54 * sc, y, F_GREEN
  txt "TIME", x, y + 14 * sc, F_BROWN
  tm$ = fmtClock$(secs)
  txt tm$, x + 54 * sc, y + 14 * sc, F_BROWN
  txt "MISSES", x, y + 28 * sc, F_BROWN
  txt str$(misses), x + 54 * sc, y + 28 * sc, F_BROWN
  txt "SCORE", x, y + 42 * sc, F_BROWN
  txt str$(score), x + 54 * sc, y + 42 * sc, F_GREEN
  if qualifies then
    txt "NEW HIGH SCORE!", dlg.x + (lay.dlgW - len("NEW HIGH SCORE!") * 6 * sc) \ 2, y + 64 * sc, F_RED
  else
    txt "THE FOREST IS AT PEACE", dlg.x + (lay.dlgW - len("THE FOREST IS AT PEACE") * 6 * sc) \ 2, y + 64 * sc, F_GREEN
  end if
  scr.present()
  ' ignore keys for a moment so a held digit doesn't skip the dialog
  do while timer - t < 700
  loop
  key.flush()
  do
    k = key.get()
    pause 15
  loop until key.isEnter(k) or k = K_ESC or k = 32
  key.flush()
end sub

' one table line: rank, name, score, time. mmcore (v0.273) garbles string
' expressions that contain more than one user function call, so every
' piece is built in its own statement.
function dlg.row$(i, n$, score, secs)
  local a$, b$, c$, d$
  a$ = rjust$(str$(i + 1), 2)
  b$ = ljust$(n$, HS_MAXNAME + 1)
  if score > 0 then
    c$ = rjust$(str$(score), 6)
    d$ = fmtTime$(secs)
  else
    c$ = "     -"
    d$ = ""
  end if
  dlg.row$ = a$ + " " + b$ + c$ + " " + d$
end function

' ---- high score table on the current write page; editRow = row being typed (-1 none)
sub dlg.table(lv, hilite, editRow, nm$, blink)
  local sc = scr.sc, i, y, r, n$, s$, sc2, sx2
  dlg.frame()
  if editRow >= 0 then
    dlg.header(3, "NEW HIGH SCORE!", F_RED)
  else
    dlg.header(3, "BEST  < " + lv.name$(lv) + " >", F_GREEN)
  end if
  for i = 0 to 9
    y = dlg.y + 25 * sc + i * 11 * sc
    if i = editRow then
      n$ = nm$
      if blink then n$ = n$ + "_"
      sc2 = hs.score(lv, i)
      sx2 = hs.secs(lv, i)
      s$ = dlg.row$(i, n$, sc2, sx2)
      r = F_RED
    elseif hs.score(lv, i) > 0 then
      n$ = hs.name$(lv, i)
      sc2 = hs.score(lv, i)
      sx2 = hs.secs(lv, i)
      s$ = dlg.row$(i, n$, sc2, sx2)
      if i = hilite then r = F_RED else r = F_BROWN
    else
      s$ = dlg.row$(i, "---", 0, 0)
      r = F_BROWN
    end if
    txt s$, dlg.x + 14 * sc, y, r
  next i
end sub

' ---- browse the tables from the menu; left/right switches level
sub dlg.scoreBrowser(lv)
  local k
  key.flush()
  do
    page write PG_BACK
    page copy PG_CLEAN to PG_BACK
    dlg.table(lv, -1, -1, "", FALSE)
    fly.ax = dlg.x
    fly.ay = dlg.y
    fly.aw = lay.dlgW
    fly.ah = lay.dlgH
    scr.present()
    do
      music.poll()
      intro.fireflies()
      k = key.get()
      pause 15
    loop until k <> 0
    if k = K_LEFT then lv = (lv + 2) mod 3
    if k = K_RIGHT then lv = (lv + 1) mod 3
  loop until k = K_ESC or key.isEnter(k)
  key.flush()
end sub

' ---- type a name for table row rk (already inserted); returns the name
function dlg.enterName$(lv, rk)
  local n$ = "", k, blink = TRUE, t = timer, redraw = TRUE, c$
  key.flush()
  do
    if timer - t > 400 then
      blink = not blink
      t = timer
      redraw = TRUE
    end if
    if redraw then
      page write PG_BACK
      page copy PG_GAME to PG_BACK
      bd.drawAll()
      dlg.table(lv, -1, rk, n$, blink)
      scr.present()
      redraw = FALSE
    end if
    k = key.get()
    if k = K_BKSP or k = K_DEL then
      if len(n$) > 0 then n$ = left$(n$, len(n$) - 1)
      redraw = TRUE
    elseif k >= 32 and k < 127 then
      c$ = ucase$(chr$(k))
      if instr(FONT_CHARS$, c$) > 0 and len(n$) < HS_MAXNAME then n$ = n$ + c$
      redraw = TRUE
    end if
    pause 15
  loop until key.isEnter(k) or k = K_ESC
  n$ = trim$(n$)
  if n$ = "" then n$ = "PLAYER"
  dlg.enterName$ = n$
  key.flush()
end function
