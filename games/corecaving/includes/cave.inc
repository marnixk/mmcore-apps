' cave.inc - the cave: loading, and one cave frame (the classic scan)
'
' The cave is 40 x 22 cells in g(), index i = y * 40 + x. Row 0, row 21 and
' the side columns are always steel, so the scan runs over i = 41 .. 838 and
' never looks outside the array.
'
' One cave frame (cv.step, run from the timer tick in game.inc) scans the
' cells top to bottom, left to right, like the original. An object that moves
' down or right lands on a cell the scan has not reached yet; it is tagged
' with E_FLAG so it does not move twice in one frame (the scan strips the tag
' when it gets there). Every changed cell is queued for drawing (cv.mark);
' the main loop draws the queue (draw.inc).
'
' Rules (classic): boulders and diamonds fall, and roll off boulders,
' diamonds and brick walls when the cells beside and below-beside are empty;
' a falling one that lands on the miner or an enemy explodes it; fireflies
' follow the left wall and explode to empty space, butterflies the right wall
' and explode into diamonds; an enemy next to the miner or amoeba explodes;
' a falling object hitting the magic wall wakes it for cv.magic frames, and
' while awake it turns boulders into diamonds and back below it; amoeba grows
' slowly, then fast after cv.aslow frames, turns into diamonds when it can no
' longer grow and into boulders when it reaches cv.amax cells.
' The Python mirror of this file is tools/cavesim.py (used to design, check
' and solve the caves).

const E_SPACE = 0, E_DIRT = 1, E_BRICK = 2, E_MAGIC = 3, E_STEEL = 4, E_EXITC = 5, E_EXITO = 6
const E_BOULDER = 7, E_BOULDERF = 8, E_DIAMOND = 9, E_DIAMONDF = 10, E_AMOEBA = 11, E_PLAYER = 12
const E_FF = 13, E_BF = 17, E_EXS = 21, E_EXD = 25, E_LAST = 28, E_FLAG = 64
const CV_W = 40, CV_H = 22, CV_N = 880
' a push needs PUSH_N frames in a row against the boulder
const PUSH_N = 2

' ---- the cave set (from caves.inc)
dim string cvd.id$(CAVE_N - 1) length 2, cvd.title$(CAVE_N - 1) length 20
dim cvd.bonus(CAVE_N - 1), cvd.need(CAVE_N - 1), cvd.time(CAVE_N - 1), cvd.magic(CAVE_N - 1)
dim cvd.aslow(CAVE_N - 1), cvd.ap1(CAVE_N - 1), cvd.ap2(CAVE_N - 1), cvd.amax(CAVE_N - 1)
dim cvd.dval(CAVE_N - 1), cvd.xval(CAVE_N - 1)
dim string cvd.row$(CAVE_N * CV_H - 1) length CV_W

' ---- the running cave
dim integer g(CV_N - 1)
dim integer cv.off(3) = (-40, 1, 40, -1)   ' up right down left (clockwise)
dim cv.idx, cv.need, cv.time, cv.tick, cv.diff, cv.fr, cv.afr, cv.ms, cv.left
dim cv.got, cv.pi, cv.push, cv.pd, cv.snap, cv.dead, cv.won, cv.timeout
dim cv.mw, cv.magic, cv.mwChg            ' magic wall: 0 asleep, 1 awake, 2 spent
dim cv.amN, cv.amGrow, cv.aslow, cv.amax, cv.ap1, cv.ap2
dim cv.exitI, cv.score, cv.face, cv.spr, cv.enRest
' the amoeba's random numbers: a small LCG (the same as tools/cavesim.py)
dim integer cv.rs
' magic wall cells (for the shimmer)
const MG_MAX = 120
dim integer mg.i(MG_MAX - 1), mg.n
' changed cells waiting to be drawn (dm() = already queued)
dim integer dq(CV_N), dm(CV_N - 1), dqn
' scan scratch (globals: LOCAL is slow in mmcore, see screen.inc)
dim integer cv_i, cv_c, cv_b, cv_j, cv_k, cv_d, cv_n0, cv_n1, cv_n2, cv_n3, cv_toB, cv_toD, cv_cnt, cv_can
dim integer cv_x, cv_m, cv_t, cv_en, cv_sp, cv_ey, cv_ex, cv_et
' the active cells (see cv.step): one "1" / "0" per cell, row by row
dim string ac$(CV_H - 1) length CV_W
dim integer cv_y, cv_yb, cv_p, cv_at, cv_v, cv_wy, cv_wx, cv_q, cv_tk
' lookup tables: row and column (+ 0, the start of its 3-cell wake block in
' ac$) of each cell; per code (0 .. 127, tag included): cv.kind = what the
' scan does with it, cv.round = boulder, diamond or brick (things roll off),
' cv.foe = miner or amoeba (an enemy next to one explodes), cv.isd = diamond
dim integer cv.ry(CV_N - 1), cv.rx(CV_N - 1)
dim integer cv.kind(127), cv.round(127), cv.foe(127), cv.isd(127), cv.soft(127), cv_am4
dim integer cv.twk(3) = (0, 21, 22, 23)

sub cv.loadAll()
  local i, r
  cv.tables()
  restore caveData
  for i = 0 to CAVE_N - 1
    read cvd.id$(i), cvd.title$(i), cvd.bonus(i), cvd.need(i), cvd.time(i), cvd.magic(i)
    read cvd.aslow(i), cvd.ap1(i), cvd.ap2(i), cvd.amax(i), cvd.dval(i), cvd.xval(i)
    for r = 0 to CV_H - 1
      read cvd.row$(i * CV_H + r)
    next r
  next i
end sub

' diamonds needed / seconds for cave k on difficulty d
function cv.needFor(k, d)
  cv.needFor = max(1, (cvd.need(k) * df.need(d) + 99) \ 100)
end function

function cv.timeFor(k, d)
  cv.timeFor = (cvd.time(k) * df.time(d)) \ 100
end function

' set up cave k on difficulty d
sub cv.load(k, d)
  local x, y, i, r$, ch$
  cv.idx = k
  cv.diff = d
  cv.tick = df.tick(d)
  cv.need = cv.needFor(k, d)
  cv.time = cv.timeFor(k, d)
  cv.magic = (cvd.magic(k) * 1000) \ cv.tick
  cv.aslow = (cvd.aslow(k) * 1000) \ cv.tick
  cv.amax = cvd.amax(k)
  cv.ap1 = (cvd.ap1(k) * df.amoeba(d)) \ 100
  cv.ap2 = (cvd.ap2(k) * df.amoeba(d)) \ 100
  cv.mw = 0
  cv.mwChg = 0
  cv.amN = 0
  cv.rs = int(rnd * 2147483647)
  cv.amGrow = 0
  cv.fr = 0
  cv.afr = 0
  cv.ms = 0
  cv.left = cv.time
  cv.got = 0
  cv.push = 0
  cv.pd = -1
  cv.snap = FALSE
  cv.dead = FALSE
  cv.won = FALSE
  cv.timeout = FALSE
  cv.score = 0
  cv.face = 1
  cv.spr = T_DIG_IDLE
  cv.exitI = -1
  mg.n = 0
  dqn = 0
  for y = 0 to CV_H - 1
    r$ = cvd.row$(k * CV_H + y)
    for x = 0 to CV_W - 1
      i = y * CV_W + x
      ch$ = mid$(r$, x + 1, 1)
      select case ch$
        case "."
          g(i) = E_DIRT
        case " "
          g(i) = E_SPACE
        case "W"
          g(i) = E_STEEL
        case "w"
          g(i) = E_BRICK
        case "M"
          g(i) = E_MAGIC
          if mg.n < MG_MAX then
            mg.i(mg.n) = i
            mg.n = mg.n + 1
          end if
        case "o"
          g(i) = E_BOULDER
        case "*"
          g(i) = E_DIAMOND
        case "F"
          g(i) = E_FF + 3
        case "B"
          g(i) = E_BF + 2
        case "a"
          g(i) = E_AMOEBA
        case "P"
          g(i) = E_PLAYER
          cv.pi = i
        case "X"
          g(i) = E_EXITC
          cv.exitI = i
        case else
          g(i) = E_STEEL
      end select
      dm(i) = 0
    next x
  next y
  ' only cells that may do something start awake (see cv.step)
  for y = 0 to CV_H - 1
    r$ = string$(CV_W, "0")
    for x = 0 to CV_W - 1
      if g(y * CV_W + x) > 4 then mid$(r$, x + 1, 1) = "1"
    next x
    ac$(y) = r$
  next y
end sub

' the scan's lookup tables (once)
sub cv.tables()
  local i, c
  for i = 0 to CV_N - 1
    cv.ry(i) = i \ CV_W
    cv.rx(i) = i mod CV_W
  next i
  for i = 0 to 127
    c = i
    if c >= E_FLAG then c = c - E_FLAG
    cv.kind(i) = 0
    if i >= E_FLAG then
      cv.kind(i) = 1
    elseif c = E_BOULDER or c = E_DIAMOND then
      cv.kind(i) = 2
    elseif c = E_BOULDERF or c = E_DIAMONDF then
      cv.kind(i) = 3
    elseif c >= E_FF and c < E_EXS then
      cv.kind(i) = 4
    elseif c = E_AMOEBA then
      cv.kind(i) = 5
    elseif c = E_PLAYER then
      cv.kind(i) = 6
    elseif c >= E_EXS and c <= E_LAST then
      cv.kind(i) = 7
    elseif c = E_EXITC then
      cv.kind(i) = 8
    end if
    cv.round(i) = (i = E_BOULDER or i = E_DIAMOND or i = E_BRICK)
    cv.foe(i) = (i = E_PLAYER or i = E_AMOEBA)
    cv.isd(i) = (i = E_DIAMOND)
    cv.soft(i) = (i <= E_DIRT)
  next i
end sub

' queue cell m for drawing
sub cv.mark(m)
  dq(dqn) = m : dqn = dqn + 1 - dm(m) : dm(m) = 1
end sub

' ---- writing a cell. Every change to g() during the scan goes through cv.w
' (cell cv_at := cv_v) or cv.wm (the same, and queue it for drawing). Both
' wake the cells that look at it (see the active cells, below).
sub cv.w()
  g(cv_at) = cv_v
  cv_wy = cv.ry(cv_at) : cv_wx = cv.rx(cv_at)
  mid$(ac$(cv_wy), cv_wx, 3) = "111"
  cv_wy = cv_wy - 1
  mid$(ac$(cv_wy), cv_wx, 3) = "111"
end sub

sub cv.wm()
  g(cv_at) = cv_v
  cv_wy = cv.ry(cv_at) : cv_wx = cv.rx(cv_at)
  mid$(ac$(cv_wy), cv_wx, 3) = "111"
  cv_wy = cv_wy - 1
  mid$(ac$(cv_wy), cv_wx, 3) = "111"
  dq(dqn) = cv_at : dqn = dqn + 1 - dm(cv_at) : dm(cv_at) = 1
end sub

' explode the 3 x 3 cells round c into kind (E_EXS or E_EXD); cells after the
' scan position get the tag. Steel and the exit survive.
sub cv.explode(c, kind)
  for cv_ey = -40 to 40 step 40
    for cv_ex = -1 to 1
      cv_at = c + cv_ey + cv_ex
      cv_et = g(cv_at) and 63
      if cv_et <> E_STEEL and cv_et <> E_EXITC and cv_et <> E_EXITO then
        if cv_et = E_PLAYER then cv.dead = TRUE
        cv_v = kind
        if cv_at > cv_i then cv_v = kind + E_FLAG
        cv.wm()
      end if
    next cv_ex
  next cv_ey
end sub

' ---- one cave frame; cv.pd = the miner's direction (-1 none), cv.snap = grab
'
' The active cells. The classic scan visits all 798 inner cells, but most of
' them do nothing: dirt, space, walls, and boulders and diamonds lying still.
' Only these are visited here: ac$(y) has a "1" at column x + 1 for every
' cell that may do something. A boulder or diamond that stays where it is
' goes to sleep (its "1" becomes "0"), and so does any cell of code <= 4 or 6;
' everything else stays awake. Writing a cell (cv.w / cv.wm) wakes the cell
' itself and the five that look at it (a resting boulder or diamond looks at
' the cells below, left, right, below-left and below-right), so a sleeping
' cell is always one that the classic scan would also have left alone, and
' the cells are visited in the same order as before (INSTR finds the next
' "1" in the row, so a cell woken ahead of the scan is still reached this
' frame). The result is the same as the full scan, frame for frame.
' A resting diamond's twinkle (draw only) is done after the scan, from the
' cells whose twinkle is due this frame (cv.twinkle).
'
' Written for mmcore's fast path: "x = arithmetic" and "IF a <op> b THEN
' x = ..." on their own, with plain-variable array subscripts, run several
' times faster than anything else; so the hot paths read neighbours through
' an index variable, use lookup tables (cv.kind, cv.round) instead of AND /
' OR tests, and jump with GOTO instead of block IFs and calls.
' Codes: 0 space, 1 dirt, 2 brick, 3 magic, 4 steel, 5 exit closed, 6 exit
' open, 7 boulder, 8 falling boulder, 9 diamond, 10 falling diamond, 11
' amoeba, 12 miner, 13-16 firefly, 17-20 butterfly, 21-24 explosion to space,
' 25-28 explosion to diamonds, +64 scanned tag.
sub cv.step()
  if cv.dead or cv.won or cv.timeout then cv.pd = -1
  cv.afr = cv.fr
  cv_toB = (cv.amN > 0 and cv.amN >= cv.amax)
  cv_toD = (cv.amN > 0 and cv.amGrow = 0)
  cv_cnt = 0
  cv_can = 0
  cv_en = (cv.diff <> DF_EASY or (cv.fr mod 4) <> 3)
  cv_am4 = ((cv.afr mod 4) = 0)
  for cv_y = 1 to CV_H - 2
    cv_yb = cv_y * CV_W - 1
    cv_p = instr(ac$(cv_y), "1")
    do while cv_p > 0
      cv_i = cv_yb + cv_p
      cv_c = g(cv_i)
      cv_k = cv.kind(cv_c)
      if cv_k = 2 then goto cvRest
      if cv_k = 1 then goto cvStrip
      if cv_k = 0 then goto cvSleep
      if cv_k = 3 then goto cvFall
      if cv_k = 4 then goto cvEnemy
      if cv_k = 7 then goto cvBoom
      if cv_k = 5 then goto cvAmoeba
      if cv_k = 6 then goto cvPlayer
      ' 8: closed exit
      if cv.got >= cv.need then cv_at = cv_i : cv_v = 6 : cv.wm()
      goto cvNext
cvStrip:
      g(cv_i) = cv_c - 64
      goto cvNext
cvRest:
      ' a boulder or diamond lying still: falls into space below, or rolls
      ' off a round thing or brick wall (left first)
      cv_j = cv_i + 40 : cv_b = g(cv_j)
      if cv_b = 0 then goto cvRDrop
      cv_t = cv.round(cv_b)
      if cv_t = 0 then goto cvSleep
      cv_j = cv_i - 1 : cv_n0 = g(cv_j) : cv_j = cv_i + 39 : cv_n1 = g(cv_j) : cv_t = cv_n0 + cv_n1
      if cv_t = 0 then goto cvRLeft
      cv_j = cv_i + 1 : cv_n0 = g(cv_j) : cv_j = cv_i + 41 : cv_n1 = g(cv_j) : cv_t = cv_n0 + cv_n1
      if cv_t = 0 then goto cvRRight
cvSleep:
      mid$(ac$(cv_y), cv_p, 1) = "0"
      goto cvNext
cvRDrop:
      cv_at = cv_i : cv_v = 0 : cv.wm()
      cv_at = cv_i + 40 : cv_v = cv_c + 65 : cv.wm()
      goto cvNext
cvRLeft:
      cv_at = cv_i : cv_v = 0 : cv.wm()
      cv_at = cv_i - 1 : cv_v = cv_c + 1 : cv.wm()
      goto cvNext
cvRRight:
      cv_at = cv_i : cv_v = 0 : cv.wm()
      cv_at = cv_i + 1 : cv_v = cv_c + 65 : cv.wm()
      goto cvNext
cvFall:
      cv.falling()
      goto cvNext
cvEnemy:
      cv.enemy()
      goto cvNext
cvBoom:
      ' explosion frames: 21-24 end as space, 25-28 as a diamond
      cv_at = cv_i : cv_v = cv_c + 1
      if cv_c = 24 then cv_v = 0
      if cv_c = 28 then cv_v = 9
      cv.wm()
      goto cvNext
cvAmoeba:
      cv.amoeba()
      goto cvNext
cvPlayer:
      cv.player()
cvNext:
      cv_p = instr(cv_p + 1, ac$(cv_y), "1")
    loop
  next cv_y
  cv.amN = cv_cnt
  cv.amGrow = cv_can
  cv.twinkle()
  ' the magic wall: shimmer while awake, back to sleep-look when spent
  if cv.mw = 1 then
    cv.magic = cv.magic - 1
    if cv.magic <= 0 then
      cv.mw = 2
      cv.mwChg = TRUE
    end if
  end if
  if cv.mwChg or (cv.mw = 1 and (cv.afr mod 2) = 0) then
    for cv_k = 0 to mg.n - 1
      if g(mg.i(cv_k)) = 3 then cv.mark(mg.i(cv_k))
    next cv_k
    cv.mwChg = FALSE
  end if
  ' the open exit flickers
  if cv.exitI >= 0 then
    if g(cv.exitI) = 6 and (cv.afr mod 3) = 0 then cv.mark(cv.exitI)
  end if
  cv.fr = cv.fr + 1
  if not cv.won and not cv.dead then
    cv.ms = cv.ms + cv.tick
    cv_t = cv.time - cv.ms \ 1000
    if cv_t <> cv.left then
      cv.left = cv_t
      hud.dirty = TRUE
    end if
    if cv.left <= 0 then cv.timeout = TRUE
  end if
end sub

' a diamond twinkles (is redrawn) on the frames where (afr + 7 i) mod 24 is 0
' or 21 .. 23. 7 x 7 = 49 = 1 (mod 24), so those are the cells with
' i = 7 (k - afr) (mod 24) for k = 0, 21, 22, 23: four strided passes.
sub cv.twinkle()
  for cv_tk = 0 to 3
    cv_t = (7 * (cv.twk(cv_tk) - cv.afr)) mod 24
    if cv_t < 0 then cv_t = cv_t + 24
    cv_t = 24 + cv_t
    if cv_t < 41 then cv_t = cv_t + 24
    for cv_j = cv_t to 838 step 24
      cv_v = g(cv_j) : cv_q = cv.isd(cv_v)
      dq(dqn) = cv_j : dqn = dqn + cv_q - cv_q * dm(cv_j) : dm(cv_j) = dm(cv_j) + cv_q - cv_q * dm(cv_j)
    next cv_j
  next cv_tk
end sub

sub cv.falling()
  cv_j = cv_i + 40 : cv_b = g(cv_j)
  if cv_b = 0 then
    cv_at = cv_i : cv_v = 0 : cv.wm()
    cv_at = cv_i + 40 : cv_v = cv_c + 64 : cv.wm()
  elseif cv_b = 3 then
    if cv.mw = 0 then
      cv.mw = 1
      cv.mwChg = TRUE
    end if
    if cv.mw = 1 then
      cv_at = cv_i : cv_v = 0 : cv.wm()
      if g(cv_i + 80) = 0 then
        cv_at = cv_i + 80
        if cv_c = 8 then
          cv_v = 74
        else
          cv_v = 72
        end if
        cv.wm()
      end if
    else
      cv_at = cv_i : cv_v = cv_c - 1 : cv.w()
    end if
  elseif cv_b = 12 or (cv_b >= 13 and cv_b < 17) then
    cv.explode(cv_i + 40, 21)
  elseif cv_b >= 17 and cv_b < 21 then
    cv.explode(cv_i + 40, 25)
  elseif cv_b = 7 or cv_b = 9 or cv_b = 2 then
    if g(cv_i - 1) = 0 and g(cv_i + 39) = 0 then
      cv_at = cv_i : cv_v = 0 : cv.wm()
      cv_at = cv_i - 1 : cv_v = cv_c : cv.wm()
    elseif g(cv_i + 1) = 0 and g(cv_i + 41) = 0 then
      cv_at = cv_i : cv_v = 0 : cv.wm()
      cv_at = cv_i + 1 : cv_v = cv_c + 64 : cv.wm()
    else
      cv_at = cv_i : cv_v = cv_c - 1 : cv.w()
    end if
  else
    cv_at = cv_i : cv_v = cv_c - 1 : cv.w()
  end if
end sub

' fireflies (E_FF + dir) and butterflies (E_BF + dir)
sub cv.enemy()
  cv_j = cv_i - 40 : cv_n0 = g(cv_j)
  cv_j = cv_i + 1 : cv_n1 = g(cv_j)
  cv_j = cv_i + 40 : cv_n2 = g(cv_j)
  cv_j = cv_i - 1 : cv_n3 = g(cv_j)
  cv_t = cv.foe(cv_n0) + cv.foe(cv_n1) + cv.foe(cv_n2) + cv.foe(cv_n3)
  if cv_t > 0 then
    if cv_c >= 17 then
      cv.explode(cv_i, 25)
    else
      cv.explode(cv_i, 21)
    end if
    exit sub
  end if
  cv_at = cv_i : dq(dqn) = cv_at : dqn = dqn + 1 - dm(cv_at) : dm(cv_at) = 1
  if not cv_en then exit sub
  if cv_c >= 17 then
    cv_x = 17
    cv_d = cv_c - 17
    cv_k = (cv_d + 1) mod 4
  else
    cv_x = 13
    cv_d = cv_c - 13
    cv_k = (cv_d + 3) mod 4
  end if
  cv_j = cv_i + cv.off(cv_k)
  if g(cv_j) = 0 then
    cv_at = cv_i : cv_v = 0 : cv.w()
    cv_at = cv_j : cv_v = cv_x + cv_k
    if cv_j > cv_i then cv_v = cv_v + 64
    cv.wm()
  else
    cv_j = cv_i + cv.off(cv_d)
    if g(cv_j) = 0 then
      cv_at = cv_i : cv_v = 0 : cv.w()
      cv_at = cv_j : cv_v = cv_c
      if cv_j > cv_i then cv_v = cv_c + 64
      cv.wm()
    elseif cv_x = 17 then
      cv_at = cv_i : cv_v = cv_x + (cv_d + 3) mod 4 : cv.w()
    else
      cv_at = cv_i : cv_v = cv_x + (cv_d + 1) mod 4 : cv.w()
    end if
  end if
end sub

sub cv.amoeba()
  if cv_toB <> 0 then goto cvAmB
  if cv_toD <> 0 then goto cvAmD
  cv_cnt = cv_cnt + 1
  ' (redrawn every 4th frame: cv_am4 = 1 then)
  dq(dqn) = cv_i : dqn = dqn + cv_am4 - cv_am4 * dm(cv_i) : dm(cv_i) = dm(cv_i) + cv_am4 - cv_am4 * dm(cv_i)
  ' room to grow: space or dirt next to it
  cv_j = cv_i - 40 : cv_n0 = g(cv_j) : cv_j = cv_i + 1 : cv_n1 = g(cv_j)
  cv_j = cv_i + 40 : cv_n2 = g(cv_j) : cv_j = cv_i - 1 : cv_n3 = g(cv_j)
  cv_t = cv.soft(cv_n0) + cv.soft(cv_n1) + cv.soft(cv_n2) + cv.soft(cv_n3)
  if cv_t = 0 then exit sub
  cv_can = 1
  cv_k = cv.ap1
  if cv.afr >= cv.aslow then cv_k = cv.ap2
  cv.rs = (cv.rs * 1103515245 + 12345) and &H7FFFFFFF
  if ((cv.rs \ 65536) * 1000) \ 32768 >= cv_k then exit sub
  cv.rs = (cv.rs * 1103515245 + 12345) and &H7FFFFFFF
  cv_j = cv_i + cv.off((cv.rs \ 65536) mod 4)
  if g(cv_j) > 1 then exit sub
  cv_at = cv_j : cv_v = 11
  if cv_j > cv_i then cv_v = 75
  cv.wm()
  exit sub
cvAmB:
  cv_at = cv_i : cv_v = 7 : cv.wm()
  exit sub
cvAmD:
  cv_at = cv_i : cv_v = 9 : cv.wm()
end sub

' the miner: move / dig / collect / push in direction cv.pd; grab (cv.snap)
' takes the dirt or diamond next to him without moving
sub cv.player()
  if cv.pd < 0 then
    cv.push = 0
    cv_sp = T_DIG_IDLE
    if (cv.afr mod 50) >= 47 then cv_sp = T_DIG_BLINK
  else
    cv_j = cv_i + cv.off(cv.pd)
    cv_t = g(cv_j)
    if cv.pd = 1 then cv.face = 1
    if cv.pd = 3 then cv.face = 0
    cv_sp = T_DIG_WL_0 + 2 * cv.face + (cv.afr mod 2)
    if cv_t = E_SPACE or cv_t = E_DIRT then
      cv.push = 0
      if cv.snap then
        cv_at = cv_j : cv_v = E_SPACE : cv.wm()
        cv_sp = T_DIG_PL + cv.face
      else
        cv.move()
      end if
    elseif cv_t = E_DIAMOND then
      cv.push = 0
      cv.got = cv.got + 1
      if cv.got <= cv.need then
        cv.score = cv.score + cvd.dval(cv.idx)
      else
        cv.score = cv.score + cvd.xval(cv.idx)
      end if
      hud.dirty = TRUE
      if cv.snap then
        cv_at = cv_j : cv_v = E_SPACE : cv.wm()
        cv_sp = T_DIG_PL + cv.face
      else
        cv.move()
      end if
    elseif cv_t = E_EXITO and not cv.snap then
      cv.move()
      cv.won = TRUE
    elseif cv_t = E_BOULDER and (cv.pd = 1 or cv.pd = 3) and not cv.snap then
      cv_sp = T_DIG_PL + cv.face
      cv_k = cv_j + cv.off(cv.pd)
      if g(cv_k) = E_SPACE then
        cv.push = cv.push + 1
        if cv.push >= PUSH_N then
          cv_at = cv_k
          if cv_k > cv_i then
            cv_v = E_BOULDER + E_FLAG
          else
            cv_v = E_BOULDER
          end if
          cv.wm()
          cv.move()
          cv.push = 0
        end if
      else
        cv.push = 0
      end if
    else
      cv.push = 0
    end if
  end if
  if cv_sp <> cv.spr then
    cv.spr = cv_sp
    cv.mark(cv.pi)
  end if
end sub

sub cv.move()
  cv_at = cv_i : cv_v = E_SPACE : cv.wm()
  cv_at = cv_j : cv_v = E_PLAYER
  if cv_j > cv_i then cv_v = E_PLAYER + E_FLAG
  cv.wm()
  cv.pi = cv_j
end sub
