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
  local k$, sc = scr.sc, y, i
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
  do
    music.poll()
    k$ = inkey$
    if k$ = "" then
      pause 15
    elseif k$ = chr$(K_ESC) then
      dlg.pause = FALSE
      exit do
    elseif key.isQuit(k$) then
      dlg.pause = TRUE
      exit do
    end if
  loop
end function

' ---- win summary; returns when a key is pressed
sub dlg.win(lv, secs, misses, score, qualifies)
  local sc = scr.sc, x, y, k$, t = timer
  game.compose()
  dlg.frame()
  dlg.header(4, "WELL DONE!", F_GREEN)
  x = dlg.x + 24 * sc
  y = dlg.y + 32 * sc
  txt "LEVEL", x, y, F_BROWN
  txt lv.name$(lv), x + 54 * sc, y, F_GREEN
  txt "TIME", x, y + 14 * sc, F_BROWN
  txt fmtClock$(secs), x + 54 * sc, y + 14 * sc, F_BROWN
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
  ' ignore keys for a moment so a held digit doesn't skip the dialog, then
  ' drop whatever was typed meanwhile
  do while timer - t < 700
  loop
  key.drain()
  do
    k$ = inkey$
    if k$ = "" then pause 15
  loop until key.isEnter(k$) or k$ = chr$(K_ESC) or k$ = " "
end sub

' one table line: rank, name, score, time
function dlg.row$(i, n$, score, secs)
  local c$, d$
  if score > 0 then
    c$ = rjust$(str$(score), 6)
    d$ = fmtTime$(secs)
  else
    c$ = "     -"
    d$ = ""
  end if
  dlg.row$ = rjust$(str$(i + 1), 2) + " " + ljust$(n$, HS_MAXNAME + 1) + c$ + " " + d$
end function

' ---- high score table on the current write page; editRow = row being typed (-1 none)
sub dlg.table(lv, hilite, editRow, nm$, blink)
  local i
  dlg.frame()
  if editRow >= 0 then
    dlg.header(3, "NEW HIGH SCORE!", F_RED)
  else
    dlg.header(3, "BEST  < " + lv.name$(lv) + " >", F_GREEN)
  end if
  for i = 0 to 9
    dlg.tableRow(lv, i, hilite, editRow, nm$, blink)
  next i
end sub

' one line of the table (drawn over the dialog frame)
sub dlg.tableRow(lv, i, hilite, editRow, nm$, blink)
  local sc = scr.sc, y, r, n$, s$
  y = dlg.y + 25 * sc + i * 11 * sc
  if i = editRow then
    n$ = nm$
    if blink then n$ = n$ + "_"
    s$ = dlg.row$(i, n$, hs.score(lv, i), hs.secs(lv, i))
    r = F_RED
  elseif hs.score(lv, i) > 0 then
    s$ = dlg.row$(i, hs.name$(lv, i), hs.score(lv, i), hs.secs(lv, i))
    if i = hilite then r = F_RED else r = F_BROWN
  else
    s$ = dlg.row$(i, "---", 0, 0)
    r = F_BROWN
  end if
  txt s$, dlg.x + 14 * sc, y, r
end sub

' redraw table line i on the back page over a clean slice of the parchment and
' copy just that line to the screen
sub dlg.tableRowShow(lv, i, editRow, nm$, blink)
  local sc = scr.sc, x, y, w, h
  x = dlg.x + 8 * sc
  y = dlg.y + (24 + i * 11) * sc
  w = lay.dlgW - 16 * sc
  h = 11 * sc
  page write PG_BACK
  blit 8 * sc, (24 + i * 11) * sc, x, y, w, h, PG_PANELS
  dlg.tableRow(lv, i, -1, editRow, nm$, blink)
  page write 0
  blit x, y, x, y, w, h, PG_BACK
end sub

' ---- browse the tables from the menu; left/right switches level (only the
' dialog is redrawn and copied to the screen)
sub dlg.scoreBrowser(lv)
  local k$
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
    k$ = inkey$
    if k$ = "" then
      pause 15
    elseif k$ = chr$(K_LEFT) or k$ = chr$(K_RIGHT) then
      if k$ = chr$(K_LEFT) then lv = (lv + 2) mod 3 else lv = (lv + 1) mod 3
      page write PG_BACK
      blit dlg.x, dlg.y, dlg.x, dlg.y, lay.dlgW, lay.dlgH, PG_CLEAN
      dlg.table(lv, -1, -1, "", FALSE)
      page write 0
      blit dlg.x, dlg.y, dlg.x, dlg.y, lay.dlgW, lay.dlgH, PG_BACK
    end if
  loop until k$ = chr$(K_ESC) or key.isEnter(k$)
end sub

' ---- type a name for table row rk (already inserted); returns the name
' (the whole dialog is drawn once; a key or the cursor blink redraws only the
' row being typed)
function dlg.enterName$(lv, rk)
  local n$ = "", k, k$, blink = TRUE, t = timer, c$
  page write PG_BACK
  page copy PG_GAME to PG_BACK
  bd.drawAll()
  dlg.table(lv, -1, rk, n$, blink)
  scr.present()
  do
    k$ = inkey$
    if k$ = "" then
      if timer - t > 400 then
        blink = not blink
        t = timer
        dlg.tableRowShow(lv, rk, rk, n$, blink)
      end if
      pause 15
    else
      k = asc(k$)
      if key.isEnter(k$) or k = K_ESC then exit do
      if k = K_BKSP or k = K_DEL then
        if len(n$) > 0 then n$ = left$(n$, len(n$) - 1)
      elseif k >= 32 and k < 127 then
        c$ = ucase$(k$)
        if instr(FONT_CHARS$, c$) > 0 and len(n$) < HS_MAXNAME then n$ = n$ + c$
      end if
      blink = TRUE
      t = timer
      dlg.tableRowShow(lv, rk, rk, n$, blink)
    end if
  loop
  n$ = trim$(n$)
  if n$ = "" then n$ = "PLAYER"
  dlg.enterName$ = n$
end function
