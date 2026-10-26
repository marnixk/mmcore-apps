' engine.inc - the rules: four counters, the cook, customers, plates, tips.
' Pure game state, no drawing. Same integer rules as tools/salsasim.py (the
' parity tests compare the two tick by tick).
'
' Positions along a counter: units 0 (door end) .. 1024 (pump end = the cook's
' home). One en.step = one 20 ms tick. Input en.inp per tick (bits):
'   1 up, 2 down, 4 left, 8 right, 16 serve
' A customer catches the first full plate that reaches them, slides back
' SLIDE_D units while holding it (out of the door = served), else eats, sends
' the empty plate back to the right and comes forward again. A life is lost
' when a customer reaches X_END, a full plate falls off the left end or an
' empty one off the right end (en.lost = L_*); the game layer then plays the
' animation and calls en.lifeLost().
const HOME = 1024, X_MIN = 48, X_END = 960
const NC = 16, NP = 20, NO = 4
const CS_WALK = 0, CS_PAUSE = 1, CS_SLIDE = 2, CS_EAT = 3, CS_LEAVE = 4
' after MEALS plates a customer has had enough and strolls out (served): no
' endless rounds, no farming the last customer
const MEALS = 3, LEAVE_V = 4
const L_REACH = 1, L_FULL = 2, L_EMPTY = 3
const SLIDE_D = 256, SLIDE_V = 8, SERVE_T = 8, SERVE_AT = 4, WALK_T = 4, WALK_V = 8
const JUMP_T = 4, SHOW_T = 75, COIN_T = 400
const PTS_CATCH = 50, PTS_OUT = 100, PTS_EMPTY = 100, PTS_TIP = 500
const MAX_LIVES = 5, NEXTRA = 3
const IN_UP = 1, IN_DOWN = 2, IN_LEFT = 4, IN_RIGHT = 8, IN_SERVE = 16
' customer types (slow -> fast): move speed, move ticks, pause ticks
dim ty.v(5) = (2, 2, 2, 3, 3, 3)
dim ty.m(5) = (16, 20, 24, 16, 20, 24)
dim ty.p(5) = (40, 34, 28, 30, 22, 16)
dim en.extra(NEXTRA - 1) = (20000, 50000, 100000)

dim cu.on(NC - 1), cu.l(NC - 1), cu.x(NC - 1), cu.t(NC - 1), cu.s(NC - 1), cu.n(NC - 1), cu.an(NC - 1), cu.sl(NC - 1), cu.m(NC - 1)
dim pl.on(NP - 1), pl.l(NP - 1), pl.x(NP - 1), pl.f(NP - 1)
dim co.on(NO - 1), co.l(NO - 1), co.x(NO - 1), co.n(NO - 1)
' cook: lane, x, walk ticks left / direction, serve timer, jump timer / direction, catch pose timer
dim ck.l, ck.x, ck.wn, ck.wd, ck.sv, ck.jn, ck.jd, ck.cn
dim en.rng, en.diff, en.lives, en.score, en.xi, en.inp, en.tk
dim en.round, en.total, en.interval, en.ntypes, en.maxlane, en.eatT, en.psf, en.pse, en.padj
dim en.eff
dim en.served, en.plates, en.tips, en.toSpawn, en.spT, en.show, en.lost, en.ll, en.lx, en.li, en.won
' scratch (globals: LOCAL is slow in mmcore)
dim integer en_i, en_j, en_k, en_b, en_bx, en_n, en_mn, en_l, en_t, en_r

function en.rnd(n)
  en.rng = (en.rng * 1103515245 + 12345) mod 2147483648
  en.rnd = (en.rng \ 65536) mod n
end function

sub en.add(p)
  en.score = en.score + p
  do while en.xi < NEXTRA
    if en.score < en.extra(en.xi) then exit do
    en.xi = en.xi + 1
    if en.lives < MAX_LIVES then en.lives = en.lives + 1
  loop
end sub

' a new game: seed, skill, lives, score
sub en.newGame(seed, dif, lv, sc)
  en.rng = seed
  en.diff = dif
  en.lives = lv
  en.score = sc
  en.xi = 0
  do while en.xi < NEXTRA
    if sc < en.extra(en.xi) then exit do
    en.xi = en.xi + 1
  loop
end sub

' round r (0-based): the ramp of count, speed and spawn rate
sub en.start(r)
  en.round = r
  ' every knob is a monotone function of e = round + 2 * skill, so a later round or
  ' a harder skill is never easier in any respect (checked: tools/difficulty.py).
  ' Full plates keep one speed (faster ones made later rounds easier); spawns stay
  ' >= 40 ticks apart (packed doors let the cook push customers straight out).
  en.eff = r + 2 * en.diff
  en.total = min(40, 8 + 2 * en.eff)
  en.interval = max(40, 120 - 5 * en.eff)
  en.ntypes = min(6, 2 + en.eff \ 2)
  en.maxlane = min(4, 2 + en.eff \ 3)
  en.eatT = max(24, 70 - 2 * en.eff)
  en.psf = 8
  en.pse = min(8, 5 + en.eff \ 6)
  en.padj = 2 * en.eff - 8
  en.served = 0
  en.plates = 0
  en.tips = 0
  en.tk = 0
  ck.l = 0
  en.resetBoard()
end sub

sub en.resetBoard()
  for en_i = 0 to NC - 1
    cu.on(en_i) = 0
  next en_i
  for en_i = 0 to NP - 1
    pl.on(en_i) = 0
  next en_i
  for en_i = 0 to NO - 1
    co.on(en_i) = 0
  next en_i
  en.toSpawn = en.total - en.served
  en.spT = 1
  ck.x = HOME
  ck.wn = 0
  ck.wd = 0
  ck.sv = 0
  ck.jn = 0
  ck.jd = 0
  ck.cn = 0
  en.show = 0
  en.lost = 0
  en.ll = 0
  en.lx = 0
  en.li = -1
  en.won = 0
end sub

sub en.lifeLost()
  en.lives = en.lives - 1
  en.resetBoard()
  en.spT = 50
end sub

function en.freePlate()
  en.freePlate = -1
  for en_j = 0 to NP - 1
    if pl.on(en_j) = 0 then
      en.freePlate = en_j
      exit function
    end if
  next en_j
end function

function en.freeCoin()
  en.freeCoin = -1
  for en_j = 0 to NO - 1
    if co.on(en_j) = 0 then
      en.freeCoin = en_j
      exit function
    end if
  next en_j
end function

' ---- one tick
sub en.step()
  en.tk = en.tk + 1
  if en.show > 0 then
    en.show = en.show - 1
    exit sub
  end if
  en.cook()
  for en_i = 0 to NP - 1
    if pl.on(en_i) then
      if pl.f(en_i) then
        pl.x(en_i) = pl.x(en_i) - en.psf
      else
        pl.x(en_i) = pl.x(en_i) + en.pse
      end if
    end if
  next en_i
  for en_i = 0 to NC - 1
    if cu.on(en_i) then en.cust(en_i)
  next en_i
  en.collide()
  en.spawn()
  if en.served >= en.total and en.lost = 0 then en.won = 1
end sub

sub en.cook()
  if ck.jn > 0 then ck.jn = ck.jn - 1
  if ck.cn > 0 then ck.cn = ck.cn - 1
  if (en.inp and 3) <> 0 and ck.sv = 0 then
    if en.inp and IN_UP then
      ck.l = (ck.l + 3) mod 4
      ck.jd = 1
    else
      ck.l = (ck.l + 1) mod 4
      ck.jd = 2
    end if
    ck.x = HOME
    ck.wn = 0
    ck.jn = JUMP_T
  elseif (en.inp and 12) <> 0 and ck.wn = 0 and ck.sv = 0 then
    if en.inp and IN_LEFT then
      ck.wd = -1
    else
      ck.wd = 1
    end if
    ck.wn = WALK_T
  elseif (en.inp and IN_SERVE) <> 0 and ck.x = HOME and ck.sv = 0 and ck.wn = 0 then
    ck.sv = SERVE_T
  end if
  if ck.wn > 0 then
    ck.x = min(HOME, max(X_MIN, ck.x + ck.wd * WALK_V))
    ck.wn = ck.wn - 1
  end if
  if ck.sv > 0 then
    ck.sv = ck.sv - 1
    if ck.sv = SERVE_AT then
      en_j = en.freePlate()
      if en_j >= 0 then
        pl.on(en_j) = 1
        pl.l(en_j) = ck.l
        pl.x(en_j) = HOME - 24
        pl.f(en_j) = 1
      end if
    end if
  end if
end sub

sub en.cust(i)
  select case cu.s(i)
    case CS_WALK
      cu.x(i) = cu.x(i) + ty.v(cu.t(i))
      cu.n(i) = cu.n(i) - 1
      cu.an(i) = cu.an(i) + 1
      if cu.n(i) <= 0 then
        cu.s(i) = CS_PAUSE
        cu.n(i) = max(6, ty.p(cu.t(i)) - en.padj)
      end if
    case CS_PAUSE
      cu.n(i) = cu.n(i) - 1
      if cu.n(i) <= 0 then
        cu.s(i) = CS_WALK
        cu.n(i) = ty.m(cu.t(i))
      end if
    case CS_SLIDE
      cu.x(i) = cu.x(i) - SLIDE_V
      cu.sl(i) = cu.sl(i) - SLIDE_V
      if cu.x(i) <= -32 then
        cu.on(i) = 0
        en.served = en.served + 1
        en.add(PTS_OUT)
      elseif cu.sl(i) <= 0 then
        cu.s(i) = CS_EAT
        cu.n(i) = en.eatT
      end if
    case CS_EAT
      cu.n(i) = cu.n(i) - 1
      cu.an(i) = cu.an(i) + 1
      if cu.n(i) <= 0 then
        en_j = en.freePlate()
        if en_j >= 0 then
          pl.on(en_j) = 1
          pl.l(en_j) = cu.l(i)
          pl.x(en_j) = cu.x(i) + 16
          pl.f(en_j) = 0
        end if
        cu.m(i) = cu.m(i) + 1
        if cu.m(i) >= MEALS then
          cu.s(i) = CS_LEAVE
        else
          cu.s(i) = CS_PAUSE
          cu.n(i) = 30
        end if
      end if
    case CS_LEAVE
      cu.x(i) = cu.x(i) - LEAVE_V
      cu.an(i) = cu.an(i) + 1
      if cu.x(i) <= -32 then
        cu.on(i) = 0
        en.served = en.served + 1
        en.add(PTS_OUT)
      end if
  end select
end sub

sub en.collide()
  for en_i = 0 to NP - 1
    if pl.on(en_i) then
      if pl.f(en_i) then
        en.full(en_i)
      else
        en.empty(en_i)
      end if
    end if
  next en_i
  for en_i = 0 to NO - 1
    if co.on(en_i) then
      co.n(en_i) = co.n(en_i) - 1
      if co.l(en_i) = ck.l and abs(co.x(en_i) - ck.x) <= 24 then
        co.on(en_i) = 0
        en.tips = en.tips + 1
        en.add(PTS_TIP)
        en.show = SHOW_T
      elseif co.n(en_i) <= 0 then
        co.on(en_i) = 0
      end if
    end if
  next en_i
  for en_i = 0 to NC - 1
    if cu.on(en_i) then
      if cu.x(en_i) >= X_END and en.lost = 0 then
        en.lost = L_REACH
        en.ll = cu.l(en_i)
        en.lx = cu.x(en_i)
        en.li = en_i
      end if
    end if
  next en_i
end sub

' a full plate: caught by the front-most walking customer it has reached
sub en.full(p)
  en_b = -1
  en_bx = -9999
  for en_k = 0 to NC - 1
    if cu.on(en_k) then
      if cu.l(en_k) = pl.l(p) and cu.s(en_k) <= CS_PAUSE and cu.x(en_k) + 16 >= pl.x(p) and cu.x(en_k) > en_bx then
        en_b = en_k
        en_bx = cu.x(en_k)
      end if
    end if
  next en_k
  if en_b >= 0 then
    pl.on(p) = 0
    cu.s(en_b) = CS_SLIDE
    cu.sl(en_b) = SLIDE_D
    cu.n(en_b) = 0
    en.plates = en.plates + 1
    en.add(PTS_CATCH)
    if en.rnd(5) = 0 then
      en_j = en.freeCoin()
      if en_j >= 0 then
        co.on(en_j) = 1
        co.l(en_j) = cu.l(en_b)
        co.x(en_j) = max(0, cu.x(en_b))
        co.n(en_j) = COIN_T
      end if
    end if
  elseif pl.x(p) < -16 and en.lost = 0 then
    en.lost = L_FULL
    en.ll = pl.l(p)
    en.lx = pl.x(p)
  end if
end sub

sub en.empty(p)
  if pl.l(p) = ck.l and pl.x(p) >= ck.x - 16 then
    pl.on(p) = 0
    en.add(PTS_EMPTY)
    ck.cn = 6
  elseif pl.x(p) > HOME + 40 and en.lost = 0 then
    en.lost = L_EMPTY
    en.ll = pl.l(p)
    en.lx = pl.x(p)
  end if
end sub

sub en.spawn()
  if en.toSpawn <= 0 then exit sub
  en.spT = en.spT - 1
  if en.spT > 0 then exit sub
  en_r = en.rnd(4)
  for en_j = 0 to 3
    en_l = (en_r + en_j) mod 4
    en_n = 0
    en_mn = 9999
    for en_k = 0 to NC - 1
      if cu.on(en_k) then
        if cu.l(en_k) = en_l then
          en_n = en_n + 1
          en_mn = min(en_mn, cu.x(en_k))
        end if
      end if
    next en_k
    if en_n < en.maxlane and en_mn >= 80 then
      en_k = -1
      for en_i = 0 to NC - 1
        if cu.on(en_i) = 0 then
          en_k = en_i
          exit for
        end if
      next en_i
      if en_k < 0 then exit for
      en_t = en.rnd(en.ntypes)
      cu.on(en_k) = 1
      cu.l(en_k) = en_l
      cu.x(en_k) = 0
      cu.t(en_k) = en_t
      cu.s(en_k) = CS_WALK
      cu.n(en_k) = ty.m(en_t)
      cu.an(en_k) = 0
      cu.sl(en_k) = 0
      cu.m(en_k) = 0
      en.toSpawn = en.toSpawn - 1
      en.spT = en.interval + en.rnd(en.interval \ 2 + 1)
      exit sub
    end if
  next en_j
  en.spT = 10
end sub
