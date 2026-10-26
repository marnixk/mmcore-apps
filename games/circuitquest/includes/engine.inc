' engine.inc - the rules: one tick of the board (a mirror of tools/cqsim.py)
'
' Keep this file and tools/cqsim.py in step: same codes, same order of
' operations, same random numbers. The level solver and the time limits come
' from the Python copy, and test/ checks that both give the same boards.
'
' One tick = TICK_MS (50 ms). Per tick: the player phase (a move once the
' step cooldown is over, or a slide step on ice / conveyors), then every
' en.mp ticks the monsters (teeth and blobs every 2 * en.mp), then every
' SEC_T ticks one second off the clock.
'
' The board is 32 x 32 cells, cell i = y * 32 + x; the border is always wall.
' Directions: 0 up, 1 right, 2 down, 3 left (clockwise).
' Every cell that changes is queued in dq() for the screen (en.mark).
'
' The hot subs keep their scratch in globals (LOCAL is slow in mmcore with
' many variables). Each group of subs has its own scratch names, so a sub
' never clobbers the values of the sub that called it.
const LW = 32, LN = 1024
const TICK_MS = 50, MOVE_T = 4, SLIDE_T = 2, SEC_T = 20

' terrain
const T_FLOOR = 0, T_WALL = 1, T_HIDDEN = 2, T_HIDDEN_R = 3, T_FAKE = 4, T_TOG_C = 5, T_TOG_O = 6
const T_SOCKET = 7, T_EXIT = 8, T_DIRT = 9, T_GRAVEL = 10, T_HINT = 11
const T_DOOR_R = 12, T_DOOR_B = 13, T_DOOR_Y = 14, T_DOOR_G = 15
const T_WATER = 16, T_FIRE = 17, T_ICE = 18, T_ICE_NW = 19, T_ICE_NE = 20, T_ICE_SE = 21, T_ICE_SW = 22
const T_FORCE_U = 23, T_FORCE_R = 24, T_FORCE_D = 25, T_FORCE_L = 26
const T_BTN_G = 27, T_BTN_B = 28, T_BTN_BR = 29, T_BOMB = 30, T_TRAP = 31, T_THIEF = 32, T_TELE = 33
' level characters for terrain codes 0..33 (levels.inc)
const TCH$ = " #HhFTtSEd,?RBYG~%=()}{^>v<+-@*X$O"
' cosmetic level characters (the rules see floor, floor, floor, wall): via, trace across,
' trace down, bare board; their number 1..4 goes to ld() for the screen only
const DCH$ = ":_!&"
' objects
const O_CHIP = 1, O_KEY_R = 2, O_KEY_G = 5, O_BOOT_W = 6, O_BOOT_U = 9, O_BLOCK = 10
const OCH$ = ".crbygwfsuk"
' monsters
const M_BUG = 0, M_FIREBALL = 1, M_BALL = 2, M_TANK = 3, M_GLIDER = 4, M_TEETH = 5, M_BLOB = 6, M_WALKER = 7
dim string en.mname$(7) = ("BUG", "FIREBALL", "BALL", "TANK", "GLIDER", "TEETH", "BLOB", "WALKER")
' how an attempt ended
const D_NONE = 0, D_DROWN = 1, D_BURN = 2, D_BOMB = 3, D_MONSTER = 4, D_TIME = 5
const MON_MAX = 64

' ---- the levels (read once from levels.inc)
dim string lv.name$(LV_N - 1) length 16, lv.hint$(LV_N - 1) length 80, lv.dirs$(LV_N - 1) length 64
dim lv.time(LV_N - 1, 2), lv.seed(LV_N - 1)
dim string lv.row$(LV_N * 64 - 1) length 32

sub lv.loadAll()
  local k, y
  restore levelData
  for k = 0 to LV_N - 1
    read lv.name$(k), lv.hint$(k), lv.time(k, 0), lv.time(k, 1), lv.time(k, 2), lv.dirs$(k), lv.seed(k)
    for y = 0 to 63
      read lv.row$(k * 64 + y)
    next y
  next k
end sub

' ---- the board
dim lt(LN - 1), lo(LN - 1), lm(LN - 1)   ' terrain, object, monster number + 1 (0 none)
dim ld(LN - 1)                           ' cosmetic look of a floor / wall cell (0 plain, DCH$ 1..4)
dim mk(MON_MAX - 1), mi(MON_MAX - 1), md(MON_MAX - 1), mf(MON_MAX - 1)  ' kind, cell, direction, frame
dim bbl(63), bbn, tpl(63), tpn, tgl(LN - 1), tgn   ' brown buttons, teleports, toggle walls
dim dq(LN - 1), dm(LN - 1), dqn                     ' changed cells (queue + flags)
dim en.pi, en.pd, en.pf, en.dead, en.won, en.cl, en.tk, en.pdue, en.slide, en.tl, en.rs, en.mp, en.mn
dim en.keys(3), en.boots(3)
dim en.inp = -1      ' the direction asked for on this tick (-1 none)
dim en.lvl, en.chips0
dim en.off(3), en.cl1(3), en.cl2(3), en.bok(33), en.mok(33)
' scratch, one set per group of subs
dim integer en_c, en_d, en_j, en_k, en_a, en_b, en_s, en_p, en_q
dim integer pm_i, pm_j, pm_c, pm_k, pu_k, pu_c, tr_k, tr_b, bt_k, bt_q, mk_c
dim integer mo_m, mo_k, mo_i, mo_d, mo_n, mo_x, mo_nd, mo_j, mo_r, mo_slow, mo_dx, mo_dy, mo_hd, mo_vd
dim integer mo_o(3)

sub en.init()
  local i
  en.off(0) = -LW : en.off(1) = 1 : en.off(2) = LW : en.off(3) = -1
  ' ice corners: closed sides (NW: up + left, NE: up + right, SE: right + down, SW: down + left)
  en.cl1(0) = 0 : en.cl2(0) = 3
  en.cl1(1) = 0 : en.cl2(1) = 1
  en.cl1(2) = 1 : en.cl2(2) = 2
  en.cl1(3) = 2 : en.cl2(3) = 3
  ' where a block may be pushed / a monster may walk
  for i = 0 to 33
    en.bok(i) = 0
    en.mok(i) = 0
  next i
  en.bok(T_FLOOR) = 1 : en.bok(T_GRAVEL) = 1 : en.bok(T_HINT) = 1 : en.bok(T_TOG_O) = 1 : en.bok(T_BTN_G) = 1
  en.bok(T_BTN_B) = 1 : en.bok(T_BTN_BR) = 1 : en.bok(T_WATER) = 1 : en.bok(T_BOMB) = 1 : en.bok(T_TRAP) = 1
  en.mok(T_FLOOR) = 1 : en.mok(T_TOG_O) = 1 : en.mok(T_BTN_G) = 1 : en.mok(T_BTN_B) = 1
  en.mok(T_BTN_BR) = 1 : en.mok(T_TRAP) = 1
end sub

' set up level k (0-based) on difficulty dif
sub en.load(k, dif)
  local y, x, i, c, r$, s$
  en.lvl = k
  en.mn = 0 : en.cl = 0 : bbn = 0 : tpn = 0 : tgn = 0 : dqn = 0
  for y = 0 to 31
    r$ = lv.row$(k * 64 + y)
    s$ = lv.row$(k * 64 + 32 + y)
    for x = 0 to 31
      i = y * LW + x
      dm(i) = 0
      lm(i) = 0
      lo(i) = 0
      c = instr(TCH$, mid$(r$, x + 1, 1)) - 1
      ld(i) = 0
      if c < 0 then
        ld(i) = instr(DCH$, mid$(r$, x + 1, 1))
        c = T_FLOOR
        if ld(i) = 4 then c = T_WALL
      end if
      lt(i) = c
      if c = T_BTN_BR then
        bbl(bbn) = i
        bbn = bbn + 1
      elseif c = T_TELE then
        tpl(tpn) = i
        tpn = tpn + 1
      elseif c = T_TOG_C or c = T_TOG_O then
        tgl(tgn) = i
        tgn = tgn + 1
      end if
      c = asc(mid$(s$, x + 1, 1))
      if c = 80 then
        en.pi = i
      elseif c >= 49 and c <= 56 then
        mk(en.mn) = c - 49
        mi(en.mn) = i
        md(en.mn) = instr("urdl", mid$(lv.dirs$(k), en.mn + 1, 1)) - 1
        mf(en.mn) = 0
        en.mn = en.mn + 1
        lm(i) = en.mn
      elseif c <> 46 then
        lo(i) = instr(OCH$, chr$(c)) - 1
        if lo(i) = O_CHIP then en.cl = en.cl + 1
      end if
    next x
  next y
  en.chips0 = en.cl
  en.pd = 2 : en.pf = 0 : en.dead = D_NONE : en.won = 0
  for i = 0 to 3
    en.keys(i) = 0
    en.boots(i) = 0
  next i
  en.tk = 0 : en.pdue = 0 : en.slide = -1
  en.tl = lv.time(k, dif)
  en.mp = df.mp(dif)
  en.rs = lv.seed(k)
  en.inp = -1
end sub

' a cell changed: redraw it
sub en.mark(i)
  if dm(i) = 0 then
    dm(i) = 1
    dq(dqn) = i
    dqn = dqn + 1
  end if
end sub

function en.rnd()
  en.rs = (en.rs * 1103515245 + 12345) mod 2147483648
  en.rnd = en.rs \ 65536
end function

' ---------------------------------------------------------------- one tick
sub en.step()
  en.tk = en.tk + 1
  if en.dead or en.won then exit sub
  en.player()
  if en.dead or en.won then exit sub
  if (en.tk mod en.mp) = 0 then
    en.monsters()
    if en.dead then exit sub
  end if
  if (en.tk mod SEC_T) = 0 then
    en.tl = en.tl - 1
    if en.tl <= 0 then
      en.dead = D_TIME
      en.mark(en.pi)
    end if
  end if
end sub

' TRUE when the player can act on the coming tick
function en.ready()
  en.ready = (en.dead = 0 and en.won = 0 and en.tk + 1 >= en.pdue)
end function

sub en.player()
  if en.tk < en.pdue then exit sub
  if lt(en.pi) = T_TRAP then
    if not en.trapOpen() then exit sub
  end if
  if en.slide >= 0 then
    en_d = en.slide
    if en.pmove(en_d, 1) then exit sub
    if en.dead or en.won then exit sub
    en_c = lt(en.pi)
    if en_c >= T_ICE and en_c <= T_ICE_SW then en.slide = (en_d + 2) mod 4
    en.pdue = en.tk + SLIDE_T
    if en.inp >= 0 and en.inp <> en_d then
      en.pd = en.inp
      en.mark(en.pi)
      if not en.pmove(en.inp, 0) then en.pdue = en.tk + MOVE_T
    end if
    exit sub
  end if
  if en.inp < 0 then exit sub
  en.pd = en.inp
  en.mark(en.pi)
  if not en.pmove(en.inp, 0) then en.pdue = en.tk + MOVE_T
end sub

' step the player one cell in direction d (fo: forced by ice / conveyor)
function en.pmove(d, fo)
  en.pmove = 0
  pm_i = en.pi
  pm_j = pm_i + en.off(d)
  pm_c = lt(pm_i)
  if pm_c >= T_ICE_NW and pm_c <= T_ICE_SW then
    if d = en.cl1(pm_c - T_ICE_NW) or d = en.cl2(pm_c - T_ICE_NW) then exit function
  end if
  pm_c = lt(pm_j)
  if pm_c = T_WALL or pm_c = T_HIDDEN_R or pm_c = T_TOG_C then exit function
  if pm_c = T_HIDDEN then
    lt(pm_j) = T_HIDDEN_R
    en.mark(pm_j)
    exit function
  end if
  if pm_c = T_FAKE then
    lt(pm_j) = T_FLOOR
    en.mark(pm_j)
    exit function
  end if
  if pm_c = T_SOCKET then
    if en.cl > 0 then exit function
    lt(pm_j) = T_FLOOR
  elseif pm_c >= T_DOOR_R and pm_c <= T_DOOR_G then
    pm_k = pm_c - T_DOOR_R
    if en.keys(pm_k) = 0 then exit function
    if pm_k < 3 then en.keys(pm_k) = en.keys(pm_k) - 1
    lt(pm_j) = T_FLOOR
  elseif pm_c >= T_ICE_NW and pm_c <= T_ICE_SW then
    pm_k = (d + 2) mod 4
    if pm_k = en.cl1(pm_c - T_ICE_NW) or pm_k = en.cl2(pm_c - T_ICE_NW) then exit function
  end if
  if lo(pm_j) = O_BLOCK then
    if fo then exit function
    if not en.push(pm_j, d) then exit function
  end if
  en.mark(pm_i)
  en.mark(pm_j)
  en.pi = pm_j
  en.pf = en.pf xor 1
  en.enter(d)
  en.pmove = 1
end function

' push the block at cell j one cell in direction d
function en.push(j, d)
  en.push = 0
  pu_k = j + en.off(d)
  if lt(j) = T_TRAP then
    if not en.trapOpen() then exit function
  end if
  pu_c = lt(pu_k)
  if lo(pu_k) <> 0 or lm(pu_k) > 0 then exit function
  if en.bok(pu_c) = 0 then exit function
  lo(j) = 0
  en.mark(j)
  en.mark(pu_k)
  if pu_c = T_WATER or pu_c = T_BOMB then
    lt(pu_k) = T_FLOOR
  else
    lo(pu_k) = O_BLOCK
    en.button(pu_c)
  end if
  en.push = 1
end function

' the player arrived on en.pi, moving in direction d: pick up, then the floor
sub en.enter(d)
  en_j = en.pi
  if lm(en_j) > 0 then
    en.dead = D_MONSTER
    exit sub
  end if
  en_c = lo(en_j)
  if en_c = O_CHIP then
    en.cl = en.cl - 1
    lo(en_j) = 0
  elseif en_c >= O_KEY_R and en_c <= O_KEY_G then
    en.keys(en_c - O_KEY_R) = en.keys(en_c - O_KEY_R) + 1
    lo(en_j) = 0
  elseif en_c >= O_BOOT_W and en_c <= O_BOOT_U then
    en.boots(en_c - O_BOOT_W) = 1
    lo(en_j) = 0
  end if
  en.floor()
  if en.dead or en.won then exit sub
  en.slideSet(d)
end sub

' what the floor at en_j does to the player (short IF chains: #1333)
sub en.floor()
  en_c = lt(en_j)
  if en_c = T_DIRT then
    lt(en_j) = T_FLOOR
  elseif en_c = T_WATER then
    if en.boots(0) = 0 then en.dead = D_DROWN
  elseif en_c = T_FIRE then
    if en.boots(1) = 0 then en.dead = D_BURN
  elseif en_c = T_BOMB then
    en.dead = D_BOMB
  else
    en.floor2()
  end if
end sub

sub en.floor2()
  if en_c = T_EXIT then
    en.won = 1
  elseif en_c = T_THIEF then
    for en_k = 0 to 3
      en.boots(en_k) = 0
    next en_k
  elseif en_c = T_TELE then
    en.teleport()
  elseif en_c = T_BTN_G or en_c = T_BTN_B then
    en.button(en_c)
  end if
end sub

' ice and conveyors: set the slide direction and the next step time
sub en.slideSet(d)
  en_c = lt(en.pi)
  en.slide = -1
  if en_c >= T_ICE and en_c <= T_ICE_SW then
    if en.boots(2) = 0 then
      if en_c = T_ICE then
        en.slide = d
      else
        en_a = en.cl1(en_c - T_ICE_NW)
        en_b = en.cl2(en_c - T_ICE_NW)
        en_k = (d + 2) mod 4
        for en_s = 0 to 3
          if en_s <> en_a and en_s <> en_b and en_s <> en_k then en.slide = en_s
        next en_s
      end if
    end if
  elseif en_c >= T_FORCE_U and en_c <= T_FORCE_L then
    if en.boots(3) = 0 then en.slide = en_c - T_FORCE_U
  end if
  if en.slide >= 0 then
    en.pdue = en.tk + SLIDE_T
  else
    en.pdue = en.tk + MOVE_T
  end if
end sub

' to the next free teleport in reading order (wrapping round)
sub en.teleport()
  en_p = 0
  for en_k = tpn - 1 to 0 step -1
    if tpl(en_k) = en.pi then en_p = en_k
  next en_k
  for en_k = 1 to tpn - 1
    en_q = tpl((en_p + en_k) mod tpn)
    if lo(en_q) = 0 and lm(en_q) = 0 then
      en.mark(en.pi)
      en.pi = en_q
      en.mark(en_q)
      exit sub
    end if
  next en_k
end sub

' traps let go while anything stands on a brown button
function en.trapOpen()
  en.trapOpen = 0
  for tr_k = 0 to bbn - 1
    tr_b = bbl(tr_k)
    if en.pi = tr_b or lo(tr_b) = O_BLOCK or lm(tr_b) > 0 then
      en.trapOpen = 1
      exit function
    end if
  next tr_k
end function

' something arrived on terrain c: green buttons flip the toggle walls, blue turn the tanks
sub en.button(c)
  if c = T_BTN_G then
    for bt_k = 0 to tgn - 1
      bt_q = tgl(bt_k)
      if lt(bt_q) = T_TOG_C then
        lt(bt_q) = T_TOG_O
      elseif lt(bt_q) = T_TOG_O then
        lt(bt_q) = T_TOG_C
      end if
      en.mark(bt_q)
    next bt_k
  elseif c = T_BTN_B then
    for bt_k = 0 to en.mn - 1
      if mk(bt_k) = M_TANK then
        md(bt_k) = (md(bt_k) + 2) mod 4
        en.mark(mi(bt_k))
      end if
    next bt_k
  end if
end sub

' ---------------------------------------------------------------- monsters
function en.monOk(k, j)
  en.monOk = 0
  if lo(j) <> 0 or lm(j) > 0 then exit function
  mk_c = lt(j)
  if en.mok(mk_c) then
    en.monOk = 1
  elseif mk_c = T_FIRE and k = M_FIREBALL then
    en.monOk = 1
  elseif mk_c = T_WATER and k = M_GLIDER then
    en.monOk = 1
  end if
end function

sub en.monsters()
  mo_slow = ((en.tk mod (2 * en.mp)) = 0)
  for mo_m = 0 to en.mn - 1
    en.mon1()
    if en.dead then exit sub
  next mo_m
end sub

' one monster (number mo_m) takes its step
sub en.mon1()
  mo_k = mk(mo_m)
  if (mo_k = M_TEETH or mo_k = M_BLOB) and mo_slow = 0 then exit sub
  mo_i = mi(mo_m)
  if lt(mo_i) = T_TRAP then
    if not en.trapOpen() then exit sub
  end if
  mo_d = md(mo_m)
  en.order()
  for mo_x = 0 to mo_n - 1
    mo_nd = mo_o(mo_x)
    mo_j = mo_i + en.off(mo_nd)
    if en.monOk(mo_k, mo_j) then
      lm(mo_i) = 0
      mi(mo_m) = mo_j
      lm(mo_j) = mo_m + 1
      md(mo_m) = mo_nd
      mf(mo_m) = mf(mo_m) xor 1
      en.mark(mo_i)
      en.mark(mo_j)
      if mo_j = en.pi then
        en.dead = D_MONSTER
        exit sub
      end if
      en.button(lt(mo_j))
      exit sub
    end if
  next mo_x
  if mo_k = M_TEETH then en.mark(mo_i)
end sub

' the directions monster mo_m tries, in order: mo_o(0 .. mo_n - 1)
sub en.order()
  if mo_k = M_BUG then
    mo_o(0) = (mo_d + 3) mod 4 : mo_o(1) = mo_d : mo_o(2) = (mo_d + 1) mod 4 : mo_o(3) = (mo_d + 2) mod 4
    mo_n = 4
  elseif mo_k = M_FIREBALL then
    mo_o(0) = mo_d : mo_o(1) = (mo_d + 1) mod 4 : mo_o(2) = (mo_d + 3) mod 4 : mo_o(3) = (mo_d + 2) mod 4
    mo_n = 4
  elseif mo_k = M_GLIDER then
    mo_o(0) = mo_d : mo_o(1) = (mo_d + 3) mod 4 : mo_o(2) = (mo_d + 1) mod 4 : mo_o(3) = (mo_d + 2) mod 4
    mo_n = 4
  else
    en.order2()
  end if
end sub

sub en.order2()
  if mo_k = M_BALL then
    mo_o(0) = mo_d : mo_o(1) = (mo_d + 2) mod 4
    mo_n = 2
  elseif mo_k = M_TANK then
    mo_o(0) = mo_d
    mo_n = 1
  elseif mo_k = M_WALKER then
    if en.monOk(mo_k, mo_i + en.off(mo_d)) then
      mo_o(0) = mo_d
      mo_n = 1
    else
      mo_r = en.rnd() mod 3
      mo_o(0) = (mo_d + 1 + mo_r) mod 4
      mo_o(1) = (mo_d + 1 + (mo_r + 1) mod 3) mod 4
      mo_o(2) = (mo_d + 1 + (mo_r + 2) mod 3) mod 4
      mo_n = 3
    end if
  else
    en.order3()
  end if
end sub

sub en.order3()
  if mo_k = M_BLOB then
    mo_r = en.rnd() mod 4
    mo_o(0) = mo_r : mo_o(1) = (mo_r + 1) mod 4 : mo_o(2) = (mo_r + 2) mod 4 : mo_o(3) = (mo_r + 3) mod 4
    mo_n = 4
    exit sub
  end if
  ' teeth: towards the player, the longer way first
  mo_n = 0
  mo_dx = (en.pi and 31) - (mo_i and 31)
  mo_dy = (en.pi \ LW) - (mo_i \ LW)
  if mo_dx = 0 and mo_dy = 0 then exit sub
  if mo_dx > 0 then mo_hd = 1 else mo_hd = 3
  if mo_dy > 0 then mo_vd = 2 else mo_vd = 0
  if abs(mo_dx) >= abs(mo_dy) then
    mo_o(0) = mo_hd
    mo_n = 1
    if mo_dy <> 0 then
      mo_o(1) = mo_vd
      mo_n = 2
    end if
  else
    mo_o(0) = mo_vd
    mo_n = 1
    if mo_dx <> 0 then
      mo_o(1) = mo_hd
      mo_n = 2
    end if
  end if
  md(mo_m) = mo_o(0)
end sub
