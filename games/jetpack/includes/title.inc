' title.inc - START GAME, level select, high scores, quit.
' The picture is the approved title. The words are drawn on the empty panel.

dim menu.sel, menu.level
dim string menu.k$
dim string menu.item$(3) = ("START GAME", "LEVEL", "HIGH SCORES", "QUIT")

function title.two$(n)
  if n < 10 then
    title.two$ = "0" + num$(n)
  else
    title.two$ = num$(n)
  end if
end function

sub title.draw()
  local i, y, style
  page copy PG_TITLE to PG_BACK
  for i = 0 to 3
    y = lay.item0 + i * lay.mlh
    style = 0
    if i = menu.sel then
      style = 1
      box lay.barX, y - 3 * lay.fsc, lay.barW, lay.barH, 1, rgb(110, 28, 22), rgb(110, 28, 22)
    end if
    ui.text(lay.textX, y, menu.item$(i), style)
  next i
  ui.textR(lay.valR, lay.item0 + lay.mlh, "< " + title.two$(menu.level + 1) + " >", 2)
  ui.textC(lay.mx + lay.mw \ 2, lay.nameY, lv.name$, 3)
  scr.present()
end sub

sub title.show()
  local k
  menu.leave = FALSE
  menu.sel = 0
  if TH_START >= 0 then
    menu.level = TH_START
    if menu.level < 0 then menu.level = 0
    if menu.level > 99 then menu.level = 99
    exit sub
  end if
  if menu.level < 0 then menu.level = 0
  if menu.level > 99 then menu.level = 99
  lv.peek(menu.level)
  key.drain()
  title.draw()
  do
    menu.k$ = inkey$
    if menu.k$ = "" then
      pause 5
    else
      k = asc(menu.k$)
      if k >= 97 and k <= 122 then k = k - 32
      if k = K_UP then
        menu.sel = menu.sel - 1
        if menu.sel < 0 then menu.sel = 3
      end if
      if k = K_DOWN then
        menu.sel = menu.sel + 1
        if menu.sel > 3 then menu.sel = 0
      end if
      if menu.sel = 1 and k = K_LEFT then
        menu.level = menu.level - 1
        if menu.level < 0 then menu.level = 99
        lv.peek(menu.level)
      end if
      if menu.sel = 1 and k = K_RIGHT then
        menu.level = menu.level + 1
        if menu.level > 99 then menu.level = 0
        lv.peek(menu.level)
      end if
      if k = 81 then
        menu.leave = TRUE
        exit do
      end if
      if k = K_ENTER or k = K_LF then
        if menu.sel = 0 or menu.sel = 1 then
          exit do
        end if
        if menu.sel = 2 then
          hs.show(-1)
        end if
        if menu.sel = 3 then
          menu.leave = TRUE
          exit do
        end if
      end if
      title.draw()
    end if
  loop
end sub
