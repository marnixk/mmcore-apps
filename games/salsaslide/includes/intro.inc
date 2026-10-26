' intro.inc - title screen with the main menu
' Up/Down pick a row; Left/Right change the skill or the start round on those
' rows; Enter selects (on SKILL / ROUND it steps to the next value); Esc / Q
' jump to QUIT. The start round can be any round already reached on the
' chosen skill (saved with the high scores).
const MENU_START = 0, MENU_HOWTO = 1, MENU_DIFF = 2, MENU_RD = 3, MENU_SCORES = 4, MENU_QUIT = 5
dim menu.diff = DF_NORMAL
dim menu.rd = 0
dim menu.row = MENU_START

function intro.ty(r)
  intro.ty = lay.my + r * MENU_STEP
end function

' draw one menu row on the back page (selected = glazed bar + plate + turquoise)
sub intro.row(r)
  local x, ty, t$, v$, st, vs, sel
  x = lay.mx
  ty = intro.ty(r)
  sel = (r = menu.row)
  blit (x + 4) * scr.sc, (ty - 4) * scr.sc, (x + 4) * scr.sc, (ty - 4) * scr.sc, (MENU_W - 8) * scr.sc, 16 * scr.sc, PG_SCN
  select case r
    case MENU_START
      t$ = "START GAME"
    case MENU_HOWTO
      t$ = "HOW TO PLAY"
    case MENU_DIFF
      t$ = "SKILL"
      v$ = df.name$(menu.diff)
      vs = df.style(menu.diff)
    case MENU_RD
      t$ = "ROUND"
      v$ = n2$(menu.rd + 1)
      vs = F_GOLD
    case MENU_SCORES
      t$ = "HIGH SCORES"
    case MENU_QUIT
      t$ = "QUIT"
  end select
  st = F_CREAM
  if sel then
    ui.bar(x + 6, ty - 4, MENU_W - 12)
    ui.spr(IC_PLATE, IC_Y, IC_PLATE_W, IC_PLATE_H, x + 10, ty - 2)
    st = F_WHITE
  end if
  txt t$, x + 34, ty, st
  if v$ <> "" then
    if sel then
      txtR "<" + v$ + ">", x + MENU_W - 10, ty, vs
    else
      txtR v$, x + MENU_W - 18, ty, vs
    end if
  end if
  scr.markA(x + 4, ty - 4, MENU_W - 8, 16)
end sub

sub intro.stepRd(dir)
  local n
  n = hs.reach(menu.diff) + 1
  menu.rd = (menu.rd + dir + n) mod n
end sub

function intro.run()
  local k$, old, i, oc, od
  if menu.rd > hs.reach(menu.diff) then menu.rd = hs.reach(menu.diff)
  scr.loadScn("title")
  page copy PG_SCN to PG_BACK
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
      oc = menu.rd
      select case asc(k$)
        case K_UP
          menu.row = (menu.row + MENU_QUIT) mod (MENU_QUIT + 1)
        case K_DOWN
          menu.row = (menu.row + 1) mod (MENU_QUIT + 1)
        case K_LEFT
          if menu.row = MENU_DIFF then menu.diff = max(DF_EASY, menu.diff - 1)
          if menu.row = MENU_RD then intro.stepRd(-1)
        case K_RIGHT
          if menu.row = MENU_DIFF then menu.diff = min(DF_HARD, menu.diff + 1)
          if menu.row = MENU_RD then intro.stepRd(1)
        case K_ESC, asc("q"), asc("Q")
          menu.row = MENU_QUIT
        case K_ENTER, K_LF
          if menu.row = MENU_DIFF then
            menu.diff = (menu.diff + 1) mod 3
          elseif menu.row = MENU_RD then
            intro.stepRd(1)
          else
            intro.run = menu.row
            exit do
          end if
      end select
      if menu.rd > hs.reach(menu.diff) then menu.rd = hs.reach(menu.diff)
      if menu.row <> old then
        intro.row(old)
        intro.row(menu.row)
      end if
      if menu.diff <> od then intro.row(MENU_DIFF)
      if menu.diff <> od or menu.rd <> oc then intro.row(MENU_RD)
      if scr.dirty then scr.flush()
    end if
  loop
end function
