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
dim integer dq(CV_N - 1), dm(CV_N - 1), dqn
' scan scratch (globals: LOCAL is slow in mmcore, see screen.inc)
dim integer cv_i, cv_c, cv_b, cv_j, cv_k, cv_d, cv_n0, cv_n1, cv_n2, cv_n3, cv_toB, cv_toD, cv_cnt, cv_can
dim integer cv_x, cv_m, cv_t, cv_en

sub cv.loadAll()
  local i, r
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
end sub

' queue cell m for drawing
sub cv.mark(m)
  if dm(m) = 0 then
    dm(m) = 1
    dq(dqn) = m
    dqn = dqn + 1
  end if
end sub

' explode the 3 x 3 cells round c into kind (E_EXS or E_EXD); cells after the
' scan position get the tag. Steel and the exit survive.
sub cv.explode(c, kind)
  local j, t, dy, dx
  for dy = -40 to 40 step 40
    for dx = -1 to 1
      j = c + dy + dx
      t = g(j) and 63
      if t <> E_STEEL and t <> E_EXITC and t <> E_EXITO then
        if t = E_PLAYER then cv.dead = TRUE
        if j > cv_i then
          g(j) = kind + E_FLAG
        else
          g(j) = kind
        end if
        cv.mark(j)
      end if
    next dx
  next dy
end sub

' one cave frame; cv.pd = the miner's direction (-1 none), cv.snap = grab
' The scan subs below use the plain numbers of the E_ codes (CONST lookups
' cost time in this hot loop): 0 space, 1 dirt, 2 brick, 3 magic, 4 steel,
' 5 exit closed, 6 exit open, 7 boulder, 8 falling boulder, 9 diamond,
' 10 falling diamond, 11 amoeba, 12 miner, 13-16 firefly, 17-20 butterfly,
' 21-24 explosion to space, 25-28 explosion to diamonds, +64 scanned tag.
sub cv.step()
  if cv.dead or cv.won or cv.timeout then cv.pd = -1
  cv.afr = cv.fr
  cv_toB = (cv.amN > 0 and cv.amN >= cv.amax)
  cv_toD = (cv.amN > 0 and cv.amGrow = 0)
  cv_cnt = 0
  cv_can = 0
  cv_en = (cv.diff <> DF_EASY or (cv.fr mod 4) <> 3)
  ' (most cells are dirt, space or walls: one short test each)
  for cv_i = 41 to 838
    cv_c = g(cv_i)
    if cv_c > 4 then cv.cell()
  next cv_i
  cv.amN = cv_cnt
  cv.amGrow = cv_can
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

' one cell that may do something (code cv_c > 4 at cv_i).
' Short IF blocks that end in EXIT SUB: skipping a long IF / ELSEIF chain is
' slow in mmcore. EXIT SUB stays on its own line: after a sub call on the same
' line it returns to the start of the caller's line (ticket draft
' exit-sub-after-call-same-line.md).
sub cv.cell()
  if cv_c >= 64 then
    g(cv_i) = cv_c - 64
    exit sub
  end if
  if cv_c = 7 or cv_c = 9 then
    cv.resting()
    exit sub
  end if
  if cv_c = 8 or cv_c = 10 then
    cv.falling()
    exit sub
  end if
  if cv_c >= 13 and cv_c < 21 then
    cv.enemy()
    exit sub
  end if
  if cv_c = 11 then
    cv.amoeba()
    exit sub
  end if
  if cv_c = 12 then
    cv.player()
    exit sub
  end if
  if cv_c >= 21 then
    cv.boom()
    exit sub
  end if
  if cv_c = 5 and cv.got >= cv.need then
    g(cv_i) = 6
    cv.mark(cv_i)
  end if
end sub

' a boulder or diamond lying still: falls into space below, or rolls off a
' round thing or brick wall (left first); a diamond twinkles now and then
sub cv.resting()
  cv_b = g(cv_i + 40)
  if cv_b = 0 then
    g(cv_i) = 0
    g(cv_i + 40) = cv_c + 65
    cv.mark(cv_i)
    cv.mark(cv_i + 40)
    exit sub
  end if
  if cv_b = 7 or cv_b = 9 or cv_b = 2 then
    if g(cv_i - 1) = 0 and g(cv_i + 39) = 0 then
      g(cv_i) = 0
      g(cv_i - 1) = cv_c + 1
      cv.mark(cv_i)
      cv.mark(cv_i - 1)
      exit sub
    end if
    if g(cv_i + 1) = 0 and g(cv_i + 41) = 0 then
      g(cv_i) = 0
      g(cv_i + 1) = cv_c + 65
      cv.mark(cv_i)
      cv.mark(cv_i + 1)
      exit sub
    end if
  end if
  if cv_c = 9 then
    cv_k = (cv.afr + cv_i * 7) mod 24
    if cv_k = 0 or cv_k >= 21 then cv.mark(cv_i)
  end if
end sub

' explosion frames: 21-24 end as space, 25-28 as a diamond
sub cv.boom()
  if cv_c = 24 then
    g(cv_i) = 0
  elseif cv_c = 28 then
    g(cv_i) = 9
  else
    g(cv_i) = cv_c + 1
  end if
  cv.mark(cv_i)
end sub

sub cv.falling()
  cv_b = g(cv_i + 40)
  if cv_b = 0 then
    g(cv_i) = 0
    g(cv_i + 40) = cv_c + 64
    cv.mark(cv_i)
    cv.mark(cv_i + 40)
  elseif cv_b = 3 then
    if cv.mw = 0 then
      cv.mw = 1
      cv.mwChg = TRUE
    end if
    if cv.mw = 1 then
      g(cv_i) = 0
      cv.mark(cv_i)
      if g(cv_i + 80) = 0 then
        if cv_c = 8 then
          g(cv_i + 80) = 74
        else
          g(cv_i + 80) = 72
        end if
        cv.mark(cv_i + 80)
      end if
    else
      g(cv_i) = cv_c - 1
    end if
  elseif cv_b = 12 or (cv_b >= 13 and cv_b < 17) then
    cv.explode(cv_i + 40, 21)
  elseif cv_b >= 17 and cv_b < 21 then
    cv.explode(cv_i + 40, 25)
  elseif cv_b = 7 or cv_b = 9 or cv_b = 2 then
    if g(cv_i - 1) = 0 and g(cv_i + 39) = 0 then
      g(cv_i) = 0
      g(cv_i - 1) = cv_c
      cv.mark(cv_i)
      cv.mark(cv_i - 1)
    elseif g(cv_i + 1) = 0 and g(cv_i + 41) = 0 then
      g(cv_i) = 0
      g(cv_i + 1) = cv_c + 64
      cv.mark(cv_i)
      cv.mark(cv_i + 1)
    else
      g(cv_i) = cv_c - 1
    end if
  else
    g(cv_i) = cv_c - 1
  end if
end sub

' fireflies (E_FF + dir) and butterflies (E_BF + dir)
sub cv.enemy()
  cv_n0 = g(cv_i - 40)
  cv_n1 = g(cv_i + 1)
  cv_n2 = g(cv_i + 40)
  cv_n3 = g(cv_i - 1)
  if cv_n0 = 12 or cv_n1 = 12 or cv_n2 = 12 or cv_n3 = 12 or cv_n0 = 11 or cv_n1 = 11 or cv_n2 = 11 or cv_n3 = 11 then
    if cv_c >= 17 then
      cv.explode(cv_i, 25)
    else
      cv.explode(cv_i, 21)
    end if
    exit sub
  end if
  cv.mark(cv_i)
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
    g(cv_i) = 0
    if cv_j > cv_i then
      g(cv_j) = cv_x + cv_k + 64
    else
      g(cv_j) = cv_x + cv_k
    end if
    cv.mark(cv_j)
  else
    cv_j = cv_i + cv.off(cv_d)
    if g(cv_j) = 0 then
      g(cv_i) = 0
      if cv_j > cv_i then
        g(cv_j) = cv_c + 64
      else
        g(cv_j) = cv_c
      end if
      cv.mark(cv_j)
    elseif cv_x = 17 then
      g(cv_i) = cv_x + (cv_d + 3) mod 4
    else
      g(cv_i) = cv_x + (cv_d + 1) mod 4
    end if
  end if
end sub

sub cv.amoeba()
  if cv_toB then
    g(cv_i) = 7
    cv.mark(cv_i)
  elseif cv_toD then
    g(cv_i) = 9
    cv.mark(cv_i)
  else
    cv_cnt = cv_cnt + 1
    if (cv.afr mod 4) = 0 then cv.mark(cv_i)
    if g(cv_i - 40) <= 1 or g(cv_i + 1) <= 1 or g(cv_i + 40) <= 1 or g(cv_i - 1) <= 1 then
      cv_can = 1
      if cv.afr < cv.aslow then
        cv_k = cv.ap1
      else
        cv_k = cv.ap2
      end if
      cv.rs = (cv.rs * 1103515245 + 12345) and &H7FFFFFFF
      if ((cv.rs \ 65536) * 1000) \ 32768 < cv_k then
        cv.rs = (cv.rs * 1103515245 + 12345) and &H7FFFFFFF
        cv_j = cv_i + cv.off((cv.rs \ 65536) mod 4)
        if g(cv_j) <= 1 then
          if cv_j > cv_i then
            g(cv_j) = 75
          else
            g(cv_j) = 11
          end if
          cv.mark(cv_j)
        end if
      end if
    end if
  end if
end sub

' the miner: move / dig / collect / push in direction cv.pd; grab (cv.snap)
' takes the dirt or diamond next to him without moving
sub cv.player()
  local sp
  if cv.pd < 0 then
    cv.push = 0
    sp = T_DIG_IDLE
    if (cv.afr mod 50) >= 47 then sp = T_DIG_BLINK
  else
    cv_j = cv_i + cv.off(cv.pd)
    cv_t = g(cv_j)
    if cv.pd = 1 then cv.face = 1
    if cv.pd = 3 then cv.face = 0
    sp = T_DIG_WL_0 + 2 * cv.face + (cv.afr mod 2)
    if cv_t = E_SPACE or cv_t = E_DIRT then
      cv.push = 0
      if cv.snap then
        g(cv_j) = E_SPACE
        cv.mark(cv_j)
        sp = T_DIG_PL + cv.face
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
        g(cv_j) = E_SPACE
        cv.mark(cv_j)
        sp = T_DIG_PL + cv.face
      else
        cv.move()
      end if
    elseif cv_t = E_EXITO and not cv.snap then
      cv.move()
      cv.won = TRUE
    elseif cv_t = E_BOULDER and (cv.pd = 1 or cv.pd = 3) and not cv.snap then
      sp = T_DIG_PL + cv.face
      cv_k = cv_j + cv.off(cv.pd)
      if g(cv_k) = E_SPACE then
        cv.push = cv.push + 1
        if cv.push >= PUSH_N then
          if cv_k > cv_i then
            g(cv_k) = E_BOULDER + E_FLAG
          else
            g(cv_k) = E_BOULDER
          end if
          cv.mark(cv_k)
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
  if sp <> cv.spr then
    cv.spr = sp
    cv.mark(cv.pi)
  end if
end sub

sub cv.move()
  g(cv_i) = E_SPACE
  if cv_j > cv_i then
    g(cv_j) = E_PLAYER + E_FLAG
  else
    g(cv_j) = E_PLAYER
  end if
  cv.pi = cv_j
  cv.mark(cv_i)
  cv.mark(cv_j)
end sub
