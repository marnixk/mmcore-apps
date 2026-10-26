' intro.inc - title screen with the main menu
' Up/Down pick a row; Left/Right change the difficulty or the start cave on
' those rows; Enter selects (on DIFFICULTY / START CAVE it steps to the next
' value); Esc / Q jump to QUIT. The start cave can be any cave A-P already
' reached on the chosen difficulty (saved with the high scores).

const MENU_START = 0, MENU_DIFF = 1, MENU_CAVE = 2, MENU_SCORES = 3, MENU_QUIT = 4
dim menu.diff = DF_NORMAL
dim menu.cave = 0
dim menu.row = MENU_START

' art y of menu row r (text row)
function intro.ty(r)
  intro.ty = lay.my + 14 + r * MENU_STEP
end function

' draw one menu row on the back page (selected = warm bar + gem bullet + gold)
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
      t$ = "DIFFICULTY"
      v$ = df.name$(menu.diff)
      vs = df.style(menu.diff)
    case MENU_CAVE
      t$ = "START CAVE"
      v$ = cvd.id$(menu.cave)
      vs = F_GOLD
    case MENU_SCORES
      t$ = "HIGH SCORES"
    case MENU_QUIT
      t$ = "QUIT"
  end select
  st = F_WHITE
  if sel then
    ui.bar(x + 8, ty - 5, MENU_W - 16)
    ui.spr(IC_BULLET, IC_Y, 8, 8, x + 16, ty)
    st = F_GOLD
  end if
  txt t$, x + 32, ty, st
  if v$ <> "" then
    if sel then
      txtR "<" + v$ + ">", x + MENU_W - 14, ty, vs
    else
      txtR v$, x + MENU_W - 22, ty, vs
    end if
  end if
  scr.markA(x + 6, ty - 5, MENU_W - 12, 17)
end sub

' the start caves offered: A-P (not the bonus caves) up to the furthest reached
function intro.caveOk(k)
  intro.caveOk = (k >= 0 and k < CAVE_N and k <= hs.reach(menu.diff))
  if intro.caveOk then
    if cvd.bonus(k) then intro.caveOk = FALSE
  end if
end function

' step the start cave by dir (+1 / -1), wrapping round the caves offered
sub intro.stepCave(dir)
  local k, n
  k = menu.cave
  for n = 1 to CAVE_N
    k = (k + dir + CAVE_N) mod CAVE_N
    if intro.caveOk(k) then
      menu.cave = k
      exit sub
    end if
  next n
  menu.cave = 0
end sub

function intro.run()
  local k$, old, i, oc, od
  if not intro.caveOk(menu.cave) then menu.cave = 0
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
      oc = menu.cave
      select case asc(k$)
        case K_UP
          menu.row = (menu.row + MENU_QUIT) mod (MENU_QUIT + 1)
        case K_DOWN
          menu.row = (menu.row + 1) mod (MENU_QUIT + 1)
        case K_LEFT
          if menu.row = MENU_DIFF then menu.diff = max(DF_EASY, menu.diff - 1)
          if menu.row = MENU_CAVE then intro.stepCave(-1)
        case K_RIGHT
          if menu.row = MENU_DIFF then menu.diff = min(DF_HARD, menu.diff + 1)
          if menu.row = MENU_CAVE then intro.stepCave(1)
        case K_ESC, asc("q"), asc("Q")
          menu.row = MENU_QUIT
        case K_ENTER, K_LF
          if menu.row = MENU_DIFF then
            menu.diff = (menu.diff + 1) mod 3
          elseif menu.row = MENU_CAVE then
            intro.stepCave(1)
          else
            intro.run = menu.row
            exit do
          end if
      end select
      if menu.diff <> od then
        if not intro.caveOk(menu.cave) then intro.stepCave(-1)
      end if
      if menu.row <> old then
        intro.row(old)
        intro.row(menu.row)
      end if
      if menu.diff <> od then intro.row(MENU_DIFF)
      if menu.diff <> od or menu.cave <> oc then intro.row(MENU_CAVE)
      if scr.dirty then scr.flush()
    end if
  loop
end function
