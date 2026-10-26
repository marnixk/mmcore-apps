' intro.inc - title screen with the main menu
' Up/Down pick a row; Left/Right change the skill or the start level on
' those rows; Enter selects (on SKILL / LEVEL it steps to the next value);
' Esc / Q jump to QUIT. The start level can be any level already reached on
' the chosen skill (saved with the high scores); its stars show beside it.

const MENU_START = 0, MENU_DIFF = 1, MENU_LVL = 2, MENU_SCORES = 3, MENU_QUIT = 4
dim menu.diff = DF_NORMAL
dim menu.lvl = 0
dim menu.row = MENU_START

function intro.ty(r)
  intro.ty = lay.my + r * MENU_STEP
end function

' draw one menu row on the back page (selected = gold bar + ruby + white)
sub intro.row(r)
  local x, ty, t$, v$, st, vs, sel
  x = lay.mx
  ty = intro.ty(r)
  sel = (r = menu.row)
  ui.restore(x + 6, ty - 6, MENU_W - 12, 19, PG_MENU)
  select case r
    case MENU_START
      t$ = "START GAME"
    case MENU_DIFF
      t$ = "SKILL"
      v$ = df.name$(menu.diff)
      vs = df.style(menu.diff)
    case MENU_LVL
      t$ = "LEVEL"
      v$ = n2$(menu.lvl + 1) + " " + hs.stars$(menu.diff, menu.lvl)
      vs = F_GOLD
    case MENU_SCORES
      t$ = "HIGH SCORES"
    case MENU_QUIT
      t$ = "QUIT"
  end select
  st = F_GOLD
  if sel then
    ui.bar(x + 6, ty - 5, MENU_W - 12)
    ui.spr(IC_GEM, IC_Y, 20, 20, x + 8, ty - 7)
    st = F_WHITE
  end if
  txt t$, x + 34, ty, st
  if v$ <> "" then
    if sel then
      txtR "<" + v$ + ">", x + MENU_W - 12, ty, vs
    else
      txtR v$, x + MENU_W - 20, ty, vs
    end if
  end if
  scr.markA(x + 6, ty - 7, MENU_W - 12, 21)
end sub

sub intro.stepLvl(dir)
  local n
  n = hs.reach(menu.diff) + 1
  menu.lvl = (menu.lvl + dir + n) mod n
end sub

function intro.run()
  local k$, old, i, oc, od
  if menu.lvl > hs.reach(menu.diff) then menu.lvl = hs.reach(menu.diff)
  scr.menu("title")
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
        case K_ENTER, K_LF, 32
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
