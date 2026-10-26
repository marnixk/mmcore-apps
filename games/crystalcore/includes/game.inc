' game.inc - one game: levels in order, the player's input, the moves and
' their cascades, the level clear / out of moves, the total score.
'
' How a level runs - two interrupt handlers and the main loop:
'  key.onKey (ON KEY, input.inc) queues arrows, Space / Enter and Esc, and
'    flags P for the pause.
'  gm.tick (SETTICK, every TICK_RATE ms) takes the queued keys and moves the
'    cursor, picks up or drops a gem, or asks for a swap (gm.req); a held
'    arrow repeats after REPEAT_DELAY, then every REPEAT_MS (fixed "next
'    step due" times). It notices released keys (key.poll) and only changes
'    the state, it never draws. While the main loop draws (gm.drawing) it
'    waits, so a frame never shows half a step.
'  The main loop (gm.play) does the heavy work: a swap with its animations
'    and cascades (gm.move, the engine runs here), the cursor and pick
'    pulses, the hint after HINT_MS idle, the panels, the pause dialog.
' Both handlers are only on during play; menus and dialogs read INKEY$.
const IDLE_MS = 4, TICK_RATE = 5, REPEAT_DELAY = 260, REPEAT_MS = 95
const HINT_MS = 5000, HINT_FLASH = 330
const GE_NONE = 0, GE_WON = 1, GE_LOST = 2, GE_RESTART = 3, GE_QUIT = 4

dim gm.diff, gm.li, gm.total, gm.cleared, gm.reached, gm.start, gm.seed, gm.attempt
dim gm.res, gm.drawing, gm.busy, gm.req, gm.ra, gm.rb, gm.rdue, gm.idle0
dim gm.ocell = -1, gm.opk = -1, gm.cdue, gm.pdue, gm.hdue
dim gm.dx(3) = (0, 0, -1, 1), gm.dy(3) = (-1, 1, 0, 0)
dim integer gm_now, gm_c, gm_t, gm_x, gm_y, gm_i   ' the tick's scratch (LOCAL is slow)

' ---- the tick (timer interrupt, every TICK_RATE ms)
sub gm.tick()
  key.poll()
  if gm.drawing then
    exit sub
  end if
  gm_now = timer
  gm_c = key.pop()
  if gm_c >= 0 then
    gm.key(gm_c, 0)
    if gm_c <= 3 then gm.rdue = key.t0 + REPEAT_DELAY
  elseif key.held >= 0 then
    if gm_now >= gm.rdue then
      gm.key(key.held, 1)
      gm.rdue = gm.rdue + REPEAT_MS
      if gm.rdue <= gm_now then gm.rdue = gm_now + REPEAT_MS
    end if
  end if
end sub

' one key: c = 0..3 arrow (rep = 1 for a held repeat), KC_PICK, KC_ESC
sub gm.key(c, rep)
  gm.idle0 = gm_now
  if c = KC_PICK then
    if gm.busy or gm.req then
      exit sub
    end if
    gm_i = cur.y * 9 + cur.x
    if cur.pk >= 0 then
      cur.pk = -1
    elseif en.swappable(gm_i) then
      cur.pk = gm_i
    end if
    exit sub
  end if
  if c = KC_ESC then
    if cur.pk >= 0 then
      cur.pk = -1
    else
      key.pause = TRUE
    end if
    exit sub
  end if
  if cur.pk >= 0 then
    ' a picked gem: the arrow swaps it with that neighbour
    if rep or gm.busy or gm.req then
      exit sub
    end if
    gm_x = (cur.pk mod 9) + gm.dx(c)
    gm_y = (cur.pk \ 9) + gm.dy(c)
    if gm_x < 0 or gm_x > 8 or gm_y < 0 or gm_y > 8 then
      exit sub
    end if
    if en.swappable(gm_y * 9 + gm_x) then
      gm.ra = cur.pk
      gm.rb = gm_y * 9 + gm_x
      cur.x = gm_x
      cur.y = gm_y
      cur.pk = -1
      gm.req = 1
    end if
    exit sub
  end if
  ' move the cursor, hopping over cells that are not part of the board
  gm_x = cur.x
  gm_y = cur.y
  do
    gm_x = gm_x + gm.dx(c)
    gm_y = gm_y + gm.dy(c)
    if gm_x < 0 or gm_x > 8 or gm_y < 0 or gm_y > 8 then exit do
    if ac(gm_y * 9 + gm_x) then
      cur.x = gm_x
      cur.y = gm_y
      exit do
    end if
  loop
end sub

' The handlers are off while a move runs (engine, animations, cascades):
' mmcore df2cb82 forgets how many nested block IFs it is skipping when an
' interrupt runs in the middle of a skipped IF arm (ticket
' interrupt-if-shadow), and then runs lines it should skip. Keys typed during
' a move wait in the INKEY$ buffer and are taken by gm.resume. The code that
' runs with the handlers on (gm.play loop, gm.cursor, hud.update, bv.paint)
' has no block IF nested inside a block IF (tools/nestif.py checks).
sub gm.handlers(hon)
  if hon then
    key.reset()
    on key key.onKey
    settick TICK_RATE, gm.tick
  else
    settick 0, gm.tick
    on key 0
  end if
end sub

' redraw what the tick changed: the cursor, the picked gem; and the pulses
' (cursor, picked gem) and the hint flash when idle. No flush.
sub gm.cursor()
  gm_t = timer
  gm_i = cur.y * 9 + cur.x
  if gm_i <> gm.ocell then
    if gm.ocell >= 0 then bv.cell(gm.ocell)
    gm.ocell = gm_i
    bv.cf = 0
    gm.cdue = gm_t + CUR_MS
    bv.cell(gm_i)
  end if
  if cur.pk <> gm.opk then
    if gm.opk >= 0 then bv.cell(gm.opk)
    gm.opk = cur.pk
    bv.pf = 0
    gm.pdue = gm_t + PULSE_MS
    if cur.pk >= 0 then bv.cell(cur.pk)
  end if
  if gm.busy then
    exit sub
  end if
  if gm_t >= gm.cdue then
    bv.cf = 1 - bv.cf
    gm.cdue = gm_t + CUR_MS
    bv.cell(gm_i)
  end if
  if cur.pk >= 0 and gm_t >= gm.pdue then
    bv.pf = 1 - bv.pf
    gm.pdue = gm_t + PULSE_MS
    bv.cell(cur.pk)
  end if
  ' (no block IF nested in a block IF on this path: see gm.handlers)
  if en.ha >= 0 and cur.pk < 0 and gm_t - gm.idle0 >= HINT_MS then
    if gm_t >= gm.hdue then gm.hintFlash()
  elseif bv.hon then
    gm.hintOff()
  end if
end sub

sub gm.hintFlash()
  bv.hon = 1 - bv.hon
  gm.hdue = gm_t + HINT_FLASH
  bv.cell(en.ha)
  bv.cell(en.hb)
end sub

sub gm.hintOff()
  if bv.hon = 0 or en.ha < 0 then
    bv.hon = 0
    exit sub
  end if
  bv.hon = 0
  bv.cell(en.ha)
  bv.cell(en.hb)
end sub

' a swap and everything it sets off (runs in the main loop; the tick only
' moves the cursor meanwhile)
sub gm.move(a, b)
  local r, s0
  gm.handlers(FALSE)
  gm.hintOff()
  if en.moveOk(a, b) = 0 then
    bv.swap(a, b, 1)
    gm.drawing = TRUE
    bv.cell(a)
    bv.cell(b)
    bv.frameEnd()
    gm.resume()
    exit sub
  end if
  bv.swap(a, b, 0)
  r = en.swap(a, b)
  gm.drawing = TRUE
  bv.cell(a)
  bv.cell(b)
  hud.update()
  bv.frameEnd()
  s0 = en.shuf
  do
    if en.clear() = 0 then exit do
    bv.clearAnim()
    gm.drawing = TRUE
    hud.update()
    bv.frameEnd()
    en.fall()
    bv.fallAnim()
  loop
  r = en.settle()
  if en.shuf <> s0 then gm.shuffled()
  if r = 1 then gm.res = GE_WON
  if r = 2 then gm.res = GE_LOST
  if gm.res = GE_NONE then gm.resume()
end sub

' the handlers back on after a move, keeping what was typed meanwhile (it
' waited in the INKEY$ buffer) and the arrows still held
sub gm.resume()
  key.onKey()
  gm_now = timer
  gm.rdue = gm_now + REPEAT_DELAY
  on key key.onKey
  settick TICK_RATE, gm.tick
end sub

' no move was left and the gems were mixed: say so, then show the new board
sub gm.shuffled()
  dlg.banner("NO MOVES LEFT", "SHUFFLING THE GEMS", F_LABEL, "", F_GREY, 1200)
  gm.redraw()
end sub

' the whole play scene again (after a dialog)
sub gm.redraw()
  page copy PG_CLEAN to PG_BACK
  page write PG_BACK
  bv.all()
  hud.all()
  gm.ocell = -1
  gm.opk = -1
  scr.present()
end sub

function gm.pauseGame()
  gm.handlers(FALSE)
  gm.pauseGame = dlg.pause()
  gm.redraw()
  if gm.pauseGame = DLG_RESUME then gm.handlers(TRUE)
end function

' ---- one attempt at the level: GE_WON, GE_LOST, GE_RESTART or GE_QUIT
function gm.play()
  local r
  gm.res = GE_NONE
  gm.req = 0
  gm.busy = 0
  gm.drawing = FALSE
  gm.idle0 = timer
  gm.ocell = -1
  gm.opk = -1
  bv.hon = 0
  gm.handlers(TRUE)
  do
    if key.cmd = 1 then gm.res = GE_RESTART
    if key.cmd = 2 then gm.res = GE_QUIT
    if gm.res <> GE_NONE then exit do
    if key.pause then
      key.pause = FALSE
      r = gm.pauseGame()
      if r = DLG_QUIT then gm.res = GE_QUIT
      if r = DLG_RESTART then gm.res = GE_RESTART
      if gm.res <> GE_NONE then exit do
    end if
    if gm.req then
      gm.busy = 1
      gm.req = 0
      gm.move(gm.ra, gm.rb)
      gm.busy = 0
      gm.idle0 = timer
      if gm.res <> GE_NONE then exit do
    end if
    gm.drawing = TRUE
    gm.cursor()
    hud.update()
    if scr.dirty then scr.flush()
    gm.drawing = FALSE
    pause IDLE_MS
  loop
  gm.handlers(FALSE)
  gm.play = gm.res
end function

' set up level gm.li with seed gm.seed and show it
sub gm.setup()
  en.load(gm.li, lv.mv(gm.li, gm.diff), gm.seed)
  cur.x = 4
  cur.y = 4
  if ac(40) = 0 then gm.firstCell()
  cur.pk = -1
  bv.start()
  hud.all()
  scr.present()
end sub

' the cursor on the active cell nearest the middle
sub gm.firstCell()
  local i, d, dbest
  dbest = 99
  for i = 0 to 80
    if ac(i) then
      d = abs((i mod 9) - 4) + abs((i \ 9) - 4)
      if d < dbest then
        dbest = d
        cur.x = i mod 9
        cur.y = i \ 9
      end if
    end if
  next i
end sub

' a new seed for every attempt (the board and the refills follow from it)
function gm.newSeed()
  gm.attempt = gm.attempt + 1
  gm.newSeed = (int(timer) * 7919 + gm.li * 104729 + gm.attempt * 1013) and &H7FFFFFFF
end function

' the goals as text for the level banner
function gm.goalText$(k)
  select case lv.gt(gm.li, k)
    case G_COL
      gm.goalText$ = "COLLECT " + str$(lv.gn(gm.li, k)) + " " + hud.cname$(lv.gc(gm.li, k))
    case G_FROST
      gm.goalText$ = "CLEAR ALL THE FROST"
    case G_BOULDER
      gm.goalText$ = "BREAK " + str$(lv.gn(gm.li, k)) + " BOULDERS"
    case else
      gm.goalText$ = "FREE " + str$(lv.gn(gm.li, k)) + " CHAINED GEMS"
  end select
end function

function gm.stars(pts)
  gm.stars = 1
  if pts >= lv.s2(gm.li) then gm.stars = 2
  if pts >= lv.s3(gm.li) then gm.stars = 3
end function

sub game.run(dif, start)
  local r, pts, st, gQuit, gWon, first, pick, best
  gm.diff = dif
  gm.li = start
  gm.start = start
  gm.total = 0
  gm.cleared = 0
  gm.reached = start
  gm.attempt = 0
  gQuit = FALSE
  gWon = FALSE
  do
    hs.reach(dif) = max(hs.reach(dif), gm.li)
    gm.reached = max(gm.reached, gm.li)
    first = TRUE
    do
      gm.seed = gm.newSeed()
      gm.setup()
      if first then dlg.levelBanner()
      first = FALSE
      r = gm.play()
      if r = GE_LOST then
        if dlg.fail() = DLG_MENU then r = GE_QUIT
      end if
      if r = GE_WON or r = GE_QUIT then exit do
    loop
    if r = GE_QUIT then
      gQuit = TRUE
      exit do
    end if
    pts = en.levelScore()
    st = gm.stars(pts)
    best = hs.best(dif, gm.li)
    gm.total = gm.total + pts
    gm.cleared = gm.cleared + 1
    hs.best(dif, gm.li) = max(best, pts)
    hs.st(dif, gm.li) = max(hs.st(dif, gm.li), st)
    if gm.li + 1 < LV_N then hs.reach(dif) = max(hs.reach(dif), gm.li + 1)
    hs.save()
    pick = dlg.clear(pts, st, max(best, pts))
    gm.li = gm.li + 1
    if gm.li >= LV_N then
      gWon = TRUE
      gm.reached = LV_N
      exit do
    end if
    if pick = DLG_MENU then
      gQuit = TRUE
      exit do
    end if
    gm.reached = max(gm.reached, gm.li)
  loop
  hs.save()
  dlg.gameOver(gWon, gQuit)
end sub
