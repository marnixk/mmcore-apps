' intro.inc - title screen with the main menu
' Up/Down pick a row; Left/Right change the skill or the start level on
' those rows; Enter selects (on SKILL / LEVEL it steps to the next value);
' Esc / Q jump to QUIT. The start level can be any level already reached on
' the chosen skill (saved with the high scores).

const MENU_START = 0, MENU_DIFF = 1, MENU_LVL = 2, MENU_SCORES = 3, MENU_QUIT = 4
dim menu.diff = DF_NORMAL
dim menu.lvl = 0
dim menu.row = MENU_START

' art y of menu row r (text row)
function intro.ty(r)
  intro.ty = lay.my + r * lay.mstep
end function

' draw one menu row on the back page (selected = lit trace bar + chip + cyan)
sub intro.row(r)
  local x, ty, t$, v$, st, vs, sel
  x = lay.mx
  ty = intro.ty(r)
  sel = (r = menu.row)
  ' clear the row from the clean title page
  blit (x + 6) * scr.sc, (ty - 5) * scr.sc, (x + 6) * scr.sc, (ty - 5) * scr.sc, (MENU_W - 12) * scr.sc, 17 * scr.sc, PG_TITLE
  select case r
    case MENU_START
      t$ = "START GAME"
    case MENU_DIFF
      t$ = "SKILL"
      v$ = df.name$(menu.diff)
      vs = df.style(menu.diff)
    case MENU_LVL
      t$ = "LEVEL"
      v$ = right$("0" + str$(menu.lvl + 1), 2)
      vs = F_GOLD
    case MENU_SCORES
      t$ = "HIGH SCORES"
    case MENU_QUIT
      t$ = "QUIT"
  end select
  st = F_GREY
  if sel then
    ui.bar(x + 8, ty - 5, MENU_W - 16)
    ui.spr(IC_CHIP, IC_Y, 16, 16, x + 12, ty - 4)
    st = F_CYAN
  end if
  txt t$, x + 34, ty, st
  if v$ <> "" then
    if sel then
      txtR "<" + v$ + ">", x + MENU_W - 14, ty, vs
    else
      txtR v$, x + MENU_W - 22, ty, vs
    end if
  end if
  scr.markA(x + 6, ty - 5, MENU_W - 12, 17)
end sub

' step the start level by dir (+1 / -1), wrapping round the levels reached
sub intro.stepLvl(dir)
  local n
  n = hs.reach(menu.diff) + 1
  menu.lvl = (menu.lvl + dir + n) mod n
end sub

function intro.run()
  local k$, old, i, oc, od
  if menu.lvl > hs.reach(menu.diff) then menu.lvl = hs.reach(menu.diff)
  page copy PG_TITLE to PG_BACK
  page write PG_BACK
  for i = 0 to MENU_QUIT
    intro.row(i)
  next i
  scr.present()
  key.drain()
  do
    k$ = inkey$
    if k$ = "" then
      pause 5
    else
      old = menu.row
      od = menu.diff
      oc = menu.lvl
      select case asc(k$)
        case K_UP
          menu.row = (menu.row + MENU_QUIT) mod (MENU_QUIT + 1)
        case K_DOWN
          menu.row = (menu.row + 1) mod (MENU_QUIT + 1)
        case K_LEFT
          if menu.row = MENU_DIFF then menu.diff = max(DF_EASY, menu.diff - 1)
          if menu.row = MENU_LVL then intro.stepLvl(-1)
        case K_RIGHT
          if menu.row = MENU_DIFF then menu.diff = min(DF_HARD, menu.diff + 1)
          if menu.row = MENU_LVL then intro.stepLvl(1)
        case K_ESC, asc("q"), asc("Q")
          menu.row = MENU_QUIT
        case K_ENTER, K_LF
          if menu.row = MENU_DIFF then
            menu.diff = (menu.diff + 1) mod 3
          elseif menu.row = MENU_LVL then
            intro.stepLvl(1)
          else
            intro.run = menu.row
            exit do
          end if
      end select
      if menu.lvl > hs.reach(menu.diff) then menu.lvl = hs.reach(menu.diff)
      if menu.row <> old then
        intro.row(old)
        intro.row(menu.row)
      end if
      if menu.diff <> od then intro.row(MENU_DIFF)
      if menu.diff <> od or menu.lvl <> oc then intro.row(MENU_LVL)
      if scr.dirty then scr.flush()
    end if
  loop
end function
