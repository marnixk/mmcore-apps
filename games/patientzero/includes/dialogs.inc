' dialogs.inc - banners and menus. INKEY$ only. Nothing here runs in the enemy tick.

sub dlg.waitEnter()
  local k$
  key.drain
  do
    k$ = inkey$
    if k$ = "" then
      pause 5
    else
      if key.enter(k$) then
        exit do
      end if
    end if
  loop
end sub

sub dlg.banner()
  local k$, t0
  if TH_NOBANNER = 1 then
    exit sub
  end if
  on key 0
  page write PG_BACK
  ui.panel 160, 120, 320, 110
  ui.bigC scr.W \ 2, 136 * scr.sc, lv.b1$, FB_GOLD
  ui.textC scr.W \ 2, 162 * scr.sc, lv.b2$, F_CYAN
  ui.textC scr.W \ 2, 178 * scr.sc, "CURE " + num$(lv.cure), F_BODY
  ui.textC scr.W \ 2, 196 * scr.sc, "ENTER", F_HINT
  scr.present
  key.drain
  t0 = timer
  do
    k$ = inkey$
    if k$ <> "" then
      exit do
    end if
    if timer - t0 >= 1600 then
      exit do
    end if
    pause 5
  loop
end sub

sub dlg.pause()
  local k$, k
  on key 0
  page write PG_BACK
  ui.panel 175, 78, 290, 170
  ui.bigC scr.W \ 2, 92 * scr.sc, "PAUSED", FB_GOLD
  ui.textC scr.W \ 2, 118 * scr.sc, "ARROWS MOVE", F_BODY
  ui.textC scr.W \ 2, 132 * scr.sc, "Z (HOLD) CHARGE SNEEZE", F_BODY
  ui.textC scr.W \ 2, 146 * scr.sc, "V SNEEZE", F_BODY
  ui.textC scr.W \ 2, 160 * scr.sc, "ESC / P RESUME", F_CYAN
  ui.textC scr.W \ 2, 174 * scr.sc, "Q QUIT TO TITLE", F_CYAN
  ui.textC scr.W \ 2, 196 * scr.sc, "STAND IN PUDDLES TO REFILL", F_HINT
  scr.present
  key.drain
  do
    k$ = inkey$
    if k$ = "" then
      pause 5
    else
      k = asc(k$)
      if k >= 97 and k <= 122 then k = k - 32
      if k = K_ESC or k = 80 then
        exit do
      end if
      if k = 81 then
        gm.leave = 1
        exit do
      end if
    end if
  loop
end sub

sub dlg.levelWin()
  on key 0
  page write PG_BACK
  ui.heading 1
  ui.panel 170, 70, 300, 130
  ui.textC scr.W \ 2, 88 * scr.sc, lv.b1$, F_GOLD
  ui.textC scr.W \ 2, 108 * scr.sc, "CURED " + num$(gm.lcured), F_BODY
  ui.textC scr.W \ 2, 124 * scr.sc, "SCORE " + num$(gm.score), F_GOLD
  ui.textC scr.W \ 2, 148 * scr.sc, "ENTER", F_CYAN
  scr.present
  dlg.waitEnter
end sub

' hilite is the row being typed, or -1 for a finished table.
sub dlg.overBody(name$, hilite)
  local i, y, colr
  page copy PG_FIELD to PG_BACK
  page write PG_BACK
  ui.heading 0
  ui.textC scr.W \ 2, 48 * scr.sc, "JERRY WAS TAKEN TO HOSPITAL", F_BODY
  ui.textC scr.W \ 2, 62 * scr.sc, "ZOMBIES CURED " + num$(gm.cured), F_DIM
  ui.textC scr.W \ 2, 74 * scr.sc, "SCORE " + num$(gm.score), F_GOLD
  ui.panel 150, 90, 340, 210
  ui.textC scr.W \ 2, 100 * scr.sc, "HIGH SCORES", F_CYAN
  for i = 0 to 9
    y = (116 + i * 14) * scr.sc
    colr = F_BODY
    if i = hilite then colr = F_GOLD
    if hs.score(i) > 0 or i = hilite then
      ui.text 168 * scr.sc, y, num$(i + 1), F_DIM
    if i = hilite then
      ui.text 200 * scr.sc, y, name$, colr
      ui.text 400 * scr.sc, y, num$(gm.score), colr
    else
      ui.text 200 * scr.sc, y, hs.name$(i), colr
      ui.text 400 * scr.sc, y, num$(hs.score(i)), colr
    end if
    end if
  next i
  if hilite >= 0 then
    ui.textC scr.W \ 2, 268 * scr.sc, "ENTER TO SAVE", F_HINT
  else
    ui.textC scr.W \ 2, 268 * scr.sc, "ENTER", F_HINT
  end if
  scr.present
end sub

sub dlg.gameOver()
  local rk
  on key 0
  rk = hs.rank(gm.score)
  if rk >= 0 then
    hs.ask
    hs.insert rk, hs.entered$, gm.score, gm.cured, gm.level + 1 + gm.round * LV_N
    hs.save
    dlg.overBody hs.entered$, -1
    dlg.waitEnter
  else
    dlg.overBody "", -1
    dlg.waitEnter
  end if
end sub

sub dlg.scores()
  local i, y, k$, any
  on key 0
  page copy PG_TITLE to PG_BACK
  page write PG_BACK
  ui.panel 150, 36, 340, 270
  ui.bigC scr.W \ 2, 48 * scr.sc, "HIGH SCORES", FB_GOLD
  any = 0
  for i = 0 to 9
    if hs.score(i) > 0 then
      any = 1
      y = (84 + i * 16) * scr.sc
      ui.text 176 * scr.sc, y, num$(i + 1), F_DIM
      ui.text 210 * scr.sc, y, hs.name$(i), F_BODY
      ui.text 400 * scr.sc, y, num$(hs.score(i)), F_GOLD
    end if
  next i
  if any = 0 then ui.textC scr.W \ 2, 160 * scr.sc, "NO SCORES YET", F_DIM
  ui.textC scr.W \ 2, 270 * scr.sc, "ANY KEY", F_HINT
  scr.present
  key.drain
  do
    k$ = inkey$
    if k$ <> "" then
      exit do
    end if
    pause 5
  loop
end sub
