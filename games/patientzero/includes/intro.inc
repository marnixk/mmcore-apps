' intro.inc - title menu. INKEY$ loop. No credit line.

dim menu.sel
dim string menu.item$(2)

sub intro.init()
  menu.item$(0) = "START GAME"
  menu.item$(1) = "HIGH SCORES"
  menu.item$(2) = "QUIT"
end sub

sub intro.draw()
  local i, y, colr, tx
  page copy PG_TITLE to PG_BACK
  page write PG_BACK
  for i = 0 to 2
    y = (112 + i * 22) * scr.sc
    colr = FB_BODY
    if i = menu.sel then colr = FB_GOLD
    ui.bigC scr.W \ 2, y, menu.item$(i), colr
    if i = menu.sel then
      tx = scr.W \ 2 - len(menu.item$(i)) * 12 * scr.sc \ 2 - 16 * scr.sc
      spr.at F_DROP + 4, tx \ scr.sc, (112 + i * 22) + 3
    end if
  next i
  scr.present
end sub

sub intro.show()
  local k$, k
  menu.leave = FALSE
  if TH_START >= 0 then
    exit sub
  end if
  menu.sel = 0
  on key 0
  key.drain
  intro.draw
  do
    k$ = inkey$
    if k$ = "" then
      pause 5
    else
      k = asc(k$)
      if k >= 97 and k <= 122 then k = k - 32
      if k = K_UP then
        menu.sel = menu.sel - 1
        if menu.sel < 0 then menu.sel = 2
      end if
      if k = K_DOWN then
        menu.sel = menu.sel + 1
        if menu.sel > 2 then menu.sel = 0
      end if
      if k = 81 or k = K_ESC then
        menu.leave = TRUE
        exit do
      end if
      if k = K_ENTER or k = K_LF then
        if menu.sel = 0 then
          exit do
        end if
        if menu.sel = 1 then
          dlg.scores
        end if
        if menu.sel = 2 then
          menu.leave = TRUE
          exit do
        end if
      end if
      intro.draw
    end if
  loop
end sub
