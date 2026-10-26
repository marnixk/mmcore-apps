' engine.inc - the rules (mirror of tools/m3sim.py: same codes, same order of
' operations, same LCG, so a seed and a list of swaps give the same boards).
'
' Board 9 x 9, cell i = y * 9 + x. Per cell:
'  ac  1 = part of the level's shape      gc  gem colour 0..5, PRISM_C, -1 none
'  gk  kind: 0 plain, K_SH / K_SV striped (clears its row / column),
'      K_WR charged (3 x 3 blast), K_PR prism
'  fr  frost layers under the gem          ch  1 = chained gem (fixed)
'  bd  boulder hits left (0 = none), bd0 its hits at the start (cracked frame)
'
' A move: en.swap(a, b) (a = the picked gem, b = the neighbour), then
' en.clear() / en.fall() until en.clear() finds nothing, then en.settle().
' en.clear leaves what happened for the animations: ev.* (cells hit: colour,
' -2 chain broken, -3 boulder hit), fx.* (specials fired), mk.* (new specials);
' en.fall leaves fl.y (the row each gem fell from) and fl.mv.
' Scratch lives in globals (en_*): LOCAL is slow in mmcore with many globals.
const N9 = 9, NN = 81
const K_SH = 1, K_SV = 2, K_WR = 3, K_PR = 4, PRISM_C = 6
const G_COL = 1, G_FROST = 2, G_BOULDER = 3, G_CHAIN = 4
const PTS_MOVE = 250
dim ac(80), gc(80), gk(80), fr(80), ch(80), bd(80), bd0(80)
dim en.lvl, en.nc, en.moves, en.score, en.spc, en.rs, en.p, en.a, en.b, en.combo, en.shuf, en.made
dim en.ng, en.gt(2), en.gcol(2), en.gl(2), en.g0(2), en.ha = -1, en.hb = -1
dim en.q(80), en.qm(80), bm(80), en.qn
dim ru.i(63), ru.l(63), ru.d(63), ru.g(63), en.nr
dim hr(80), vr(80), grp(80)
dim mk.pos(31), mk.kind(31), mk.c(31), en.mn
dim ev.i(199), ev.c(199), en.hn
dim fx.i(99), fx.k(99), en.fn
dim fl.y(80), fl.mv(80), en.fmax
dim sl.i(8), en.cnt(5)
dim integer en_i, en_j, en_k, en_c, en_x, en_y, en_n, en_t, en_r, en_w, en_v, en_g, en_q, en_kp, en_ko
dim integer en_mx, en_hh, en_vv, en_r0, en_kind, en_pos, en_qh, en_pp, en_op, en_oc, en_ok, en_s, en_bx, en_by
dim integer en_ls, en_lc, en_lx, en_ly, en_ln

function en.rnd()
  en.rs = (en.rs * 1103515245 + 12345) and &H7FFFFFFF
  en.rnd = en.rs \ 65536
end function

' a level: shape and blockers from levels.inc, gems from the seed
sub en.load(li, mv, seed)
  local s$, c$
  en.lvl = li
  en.nc = lv.nc(li)
  en.rs = seed and &H7FFFFFFF
  en.moves = mv
  en.score = 0 : en.spc = 0 : en.shuf = 0 : en.made = 0 : en.combo = 0 : en.p = 0
  en.a = -1 : en.b = -1
  s$ = lv.cells$(li)
  for en_i = 0 to 80
    c$ = mid$(s$, en_i + 1, 1)
    ac(en_i) = (c$ <> ".")
    fr(en_i) = 0 : bd(en_i) = 0 : ch(en_i) = 0 : gc(en_i) = -1 : gk(en_i) = 0
    if c$ = "1" then fr(en_i) = 1
    if c$ = "2" then fr(en_i) = 2
    if c$ = "b" then bd(en_i) = 1
    if c$ = "B" then bd(en_i) = 2
    if c$ = "c" then ch(en_i) = 1
    bd0(en_i) = bd(en_i)
  next en_i
  en.ng = lv.ng(li)
  for en_k = 0 to en.ng - 1
    en.gt(en_k) = lv.gt(li, en_k)
    en.gcol(en_k) = lv.gc(li, en_k)
    en.gl(en_k) = lv.gn(li, en_k)
    en.g0(en_k) = en.gl(en_k)
  next en_k
  for en_i = 0 to 80
    if ac(en_i) and bd(en_i) = 0 then
      en_x = en_i mod 9
      en_y = en_i \ 9
      do
        en_c = en.rnd() mod en.nc
        en_t = 0
        if en_x >= 2 then
          if gc(en_i - 1) = en_c and gc(en_i - 2) = en_c then en_t = 1
        end if
        if en_y >= 2 then
          if gc(en_i - 9) = en_c and gc(en_i - 18) = en_c then en_t = 1
        end if
      loop while en_t
      gc(en_i) = en_c
    end if
  next en_i
  if not en.findMove() then en.shuffle()
end sub

function en.swappable(i)
  en.swappable = 0
  if ac(i) then
    if bd(i) = 0 and gc(i) >= 0 and ch(i) = 0 then en.swappable = 1
  end if
end function

sub en.goal(t, c)
  for en_g = 0 to en.ng - 1
    if en.gt(en_g) = t then
      if t <> G_COL or en.gcol(en_g) = c then
        if en.gl(en_g) > 0 then en.gl(en_g) = en.gl(en_g) - 1
      end if
    end if
  next en_g
end sub

function en.won()
  en.won = 1
  for en_g = 0 to en.ng - 1
    if en.gl(en_g) > 0 then en.won = 0
  next en_g
end function

' TRUE if cell i is part of a line of 3 or more
function en.lineAt(i)
  en.lineAt = 0
  en_lc = gc(i)
  if en_lc < 0 or en_lc > 5 then
    exit function
  end if
  en_lx = i mod 9
  en_ly = i \ 9
  en_ln = 1
  en_ls = en_lx - 1
  do while en_ls >= 0
    if gc(en_ly * 9 + en_ls) <> en_lc then exit do
    en_ln = en_ln + 1
    en_ls = en_ls - 1
  loop
  en_ls = en_lx + 1
  do while en_ls < 9
    if gc(en_ly * 9 + en_ls) <> en_lc then exit do
    en_ln = en_ln + 1
    en_ls = en_ls + 1
  loop
  if en_ln >= 3 then
    en.lineAt = 1
    exit function
  end if
  en_ln = 1
  en_ls = en_ly - 1
  do while en_ls >= 0
    if gc(en_ls * 9 + en_lx) <> en_lc then exit do
    en_ln = en_ln + 1
    en_ls = en_ls - 1
  loop
  en_ls = en_ly + 1
  do while en_ls < 9
    if gc(en_ls * 9 + en_lx) <> en_lc then exit do
    en_ln = en_ln + 1
    en_ls = en_ls + 1
  loop
  if en_ln >= 3 then en.lineAt = 1
end function

sub en.xchg(i, j)
  en_v = gc(i) : gc(i) = gc(j) : gc(j) = en_v
  en_v = gk(i) : gk(i) = gk(j) : gk(j) = en_v
end sub

function en.moveOk(i, j)
  en.moveOk = 0
  if en.swappable(i) = 0 or en.swappable(j) = 0 then
    exit function
  end if
  if gk(i) = K_PR or gk(j) = K_PR then
    en.moveOk = 1
    exit function
  end if
  if gk(i) > 0 and gk(j) > 0 then
    en.moveOk = 1
    exit function
  end if
  en.xchg(i, j)
  if en.lineAt(i) then
    en.moveOk = 1
  elseif en.lineAt(j) then
    en.moveOk = 1
  end if
  en.xchg(i, j)
end function

' the first legal swap in reading order -> en.ha, en.hb (TRUE if any)
function en.findMove()
  local i
  en.findMove = 0
  en.ha = -1
  en.hb = -1
  for i = 0 to 80
    if (i mod 9) < 8 then
      if en.moveOk(i, i + 1) then
        en.ha = i
        en.hb = i + 1
        en.findMove = 1
        exit function
      end if
    end if
    if i < 72 then
      if en.moveOk(i, i + 9) then
        en.ha = i
        en.hb = i + 9
        en.findMove = 1
        exit function
      end if
    end if
  next i
end function

' every line of 3+ -> ru.* (rows first, then columns), en.nr
sub en.findRuns()
  en.nr = 0
  for en_y = 0 to 8
    en_x = 0
    do while en_x < 9
      en_i = en_y * 9 + en_x
      en_c = gc(en_i)
      en_n = 1
      if en_c >= 0 and en_c <= 5 then
        do while en_x + en_n < 9
          if gc(en_i + en_n) <> en_c then exit do
          en_n = en_n + 1
        loop
        if en_n >= 3 then
          ru.i(en.nr) = en_i : ru.l(en.nr) = en_n : ru.d(en.nr) = 0
          en.nr = en.nr + 1
        end if
      end if
      en_x = en_x + en_n
    loop
  next en_y
  for en_x = 0 to 8
    en_y = 0
    do while en_y < 9
      en_i = en_y * 9 + en_x
      en_c = gc(en_i)
      en_n = 1
      if en_c >= 0 and en_c <= 5 then
        do while en_y + en_n < 9
          if gc(en_i + 9 * en_n) <> en_c then exit do
          en_n = en_n + 1
        loop
        if en_n >= 3 then
          ru.i(en.nr) = en_i : ru.l(en.nr) = en_n : ru.d(en.nr) = 1
          en.nr = en.nr + 1
        end if
      end if
      en_y = en_y + en_n
    loop
  next en_x
end sub

' no move left: mix the free gems (Fisher-Yates on the LCG) until the board has
' no line and at least one move (at most 100 tries)
sub en.shuffle()
  local n, t, k, j, v
  en.shuf = en.shuf + 1
  n = 0
  for en_i = 0 to 80
    if ac(en_i) and bd(en_i) = 0 and ch(en_i) = 0 and gc(en_i) >= 0 then
      en.q(n) = en_i
      bm(n) = gc(en_i)
      en.qm(n) = gk(en_i)
      n = n + 1
    end if
  next en_i
  for t = 1 to 100
    for k = n - 1 to 1 step -1
      j = en.rnd() mod (k + 1)
      v = bm(k) : bm(k) = bm(j) : bm(j) = v
      v = en.qm(k) : en.qm(k) = en.qm(j) : en.qm(j) = v
    next k
    for k = 0 to n - 1
      gc(en.q(k)) = bm(k)
      gk(en.q(k)) = en.qm(k)
    next k
    en.findRuns()
    if en.nr = 0 then
      if en.findMove() then exit for
    end if
  next t
  for k = 0 to 80
    bm(k) = 0
    en.qm(k) = 0
  next k
end sub

' a swap: FALSE (board unchanged) if it makes nothing
function en.swap(a, b)
  en.swap = 0
  if en.moveOk(a, b) = 0 then
    exit function
  end if
  en.xchg(a, b)
  en_kp = gk(b)
  en_ko = gk(a)
  en.combo = 0
  if en_kp = K_PR or en_ko = K_PR then en.combo = 1
  if en_kp > 0 and en_ko > 0 then en.combo = 1
  en.a = a
  en.b = b
  en.p = 0
  en.moves = en.moves - 1
  en.swap = 1
end function

sub en.enq(j)
  if ac(j) then
    if en.qm(j) = 0 then
      en.qm(j) = 1
      en.q(en.qn) = j
      en.qn = en.qn + 1
    end if
  end if
end sub

sub en.row(y)
  if y >= 0 and y < 9 then
    for en_r = y * 9 to y * 9 + 8
      en.enq(en_r)
    next en_r
  end if
end sub

sub en.col(x)
  if x >= 0 and x < 9 then
    for en_r = x to 72 + x step 9
      en.enq(en_r)
    next en_r
  end if
end sub

sub en.addFx(i, k)
  fx.i(en.fn) = i
  fx.k(en.fn) = k
  en.fn = en.fn + 1
end sub

' a special that was hit goes off
sub en.fire(i, k)
  local x, y, dx, dy, c, t
  en.spc = en.spc + 1
  x = i mod 9
  y = i \ 9
  en.addFx(i, k)
  if k = K_SH then
    en.score = en.score + 60
    en.row(y)
  elseif k = K_SV then
    en.score = en.score + 60
    en.col(x)
  elseif k = K_WR then
    en.score = en.score + 100
    for dy = -1 to 1
      for dx = -1 to 1
        if x + dx >= 0 and x + dx < 9 and y + dy >= 0 and y + dy < 9 then en.enq((y + dy) * 9 + x + dx)
      next dx
    next dy
  else
    en.score = en.score + 150
    for c = 0 to 5
      en.cnt(c) = 0
    next c
    for t = 0 to 80
      c = gc(t)
      if c >= 0 and c <= 5 then en.cnt(c) = en.cnt(c) + 1
    next t
    c = 0
    for t = 1 to 5
      if en.cnt(t) > en.cnt(c) then c = t
    next t
    if en.cnt(c) > 0 then
      for t = 0 to 80
        if gc(t) = c then en.enq(t)
      next t
    end if
  end if
end sub

' the first phase of a swap of two specials (or a prism and anything)
sub en.comboStart()
  local a, b, dx, dy
  en.combo = 0
  a = en.a
  b = en.b
  en_kp = gk(b)
  en_ko = gk(a)
  en_bx = b mod 9
  en_by = b \ 9
  if en_kp = K_PR and en_ko = K_PR then
    gk(a) = 0 : gk(b) = 0
    en.spc = en.spc + 2
    en.score = en.score + 1500
    en.addFx(b, 9)
    for en_j = 0 to 80
      en.enq(en_j)
    next en_j
    exit sub
  end if
  if en_kp = K_PR or en_ko = K_PR then
    if en_kp = K_PR then
      en_pp = b : en_op = a
    else
      en_pp = a : en_op = b
    end if
    en_oc = gc(en_op)
    en_ok = gk(en_op)
    gk(en_pp) = 0
    en.spc = en.spc + 1
    en.addFx(en_pp, K_PR)
    if en_ok = 0 then
      en.score = en.score + 150
    elseif en_ok = K_WR then
      en.score = en.score + 900
      for en_j = 0 to 80
        if gc(en_j) = en_oc and gk(en_j) = 0 and ch(en_j) = 0 then gk(en_j) = K_WR
      next en_j
    else
      en.score = en.score + 800
      for en_j = 0 to 80
        if gc(en_j) = en_oc and gk(en_j) = 0 and ch(en_j) = 0 then
          if ((en_j mod 9) + (en_j \ 9)) mod 2 = 0 then
            gk(en_j) = K_SH
          else
            gk(en_j) = K_SV
          end if
        end if
      next en_j
    end if
    en.enq(en_pp)
    for en_j = 0 to 80
      if gc(en_j) = en_oc then en.enq(en_j)
    next en_j
    exit sub
  end if
  en_s = 0
  if en_kp = K_SH or en_kp = K_SV then en_s = en_s + 1
  if en_ko = K_SH or en_ko = K_SV then en_s = en_s + 1
  gk(a) = 0 : gk(b) = 0
  en.spc = en.spc + 2
  if en_s = 2 then
    en.score = en.score + 300
    en.addFx(b, 5)
    en.row(en_by)
    en.col(en_bx)
  elseif en_s = 1 then
    en.score = en.score + 500
    en.addFx(b, 6)
    for dy = -1 to 1
      en.row(en_by + dy)
    next dy
    for dx = -1 to 1
      en.col(en_bx + dx)
    next dx
  else
    en.score = en.score + 600
    en.addFx(b, 7)
    for dy = -2 to 2
      for dx = -2 to 2
        if en_bx + dx >= 0 and en_bx + dx < 9 and en_by + dy >= 0 and en_by + dy < 9 then en.enq((en_by + dy) * 9 + en_bx + dx)
      next dx
    next dy
  end if
end sub

' the lines on the board -> groups (lines that share a cell), the specials they
' make (mk.*), the hit queue and the boulders next to them (bm)
sub en.matchStart()
  for en_i = 0 to 80
    hr(en_i) = -1
    vr(en_i) = -1
    grp(en_i) = -1
  next en_i
  for en_r = 0 to en.nr - 1
    ru.g(en_r) = en_r
    en_i = ru.i(en_r)
    if ru.d(en_r) = 0 then
      for en_k = 0 to ru.l(en_r) - 1
        hr(en_i + en_k) = en_r
      next en_k
    else
      for en_k = 0 to ru.l(en_r) - 1
        vr(en_i + 9 * en_k) = en_r
      next en_k
    end if
  next en_r
  for en_i = 0 to 80
    if hr(en_i) >= 0 and vr(en_i) >= 0 then
      en_j = ru.g(hr(en_i))
      en_k = ru.g(vr(en_i))
      if en_j <> en_k then
        en_v = min(en_j, en_k)
        en_w = max(en_j, en_k)
        for en_r = 0 to en.nr - 1
          if ru.g(en_r) = en_w then ru.g(en_r) = en_v
        next en_r
      end if
    end if
  next en_i
  for en_i = 0 to 80
    if hr(en_i) >= 0 then
      grp(en_i) = ru.g(hr(en_i))
    elseif vr(en_i) >= 0 then
      grp(en_i) = ru.g(vr(en_i))
    end if
  next en_i
  for en_g = 0 to en.nr - 1
    if ru.g(en_g) = en_g then en.groupSpecial(en_g)
  next en_g
  for en_i = 0 to 80
    if grp(en_i) >= 0 then
      en.enq(en_i)
      en_x = en_i mod 9
      en_y = en_i \ 9
      if en_y > 0 then
        if bd(en_i - 9) then bm(en_i - 9) = 1
      end if
      if en_y < 8 then
        if bd(en_i + 9) then bm(en_i + 9) = 1
      end if
      if en_x > 0 then
        if bd(en_i - 1) then bm(en_i - 1) = 1
      end if
      if en_x < 8 then
        if bd(en_i + 1) then bm(en_i + 1) = 1
      end if
    end if
  next en_i
end sub

' group g: 5+ in a line -> prism, an L / T -> charged gem, 4 -> striped
' (row match -> clears the row). It appears where the swapped gem landed (or
' the cell it left), else mid-line / at the corner; never on a chained gem.
sub en.groupSpecial(g)
  en_mx = 0 : en_hh = 0 : en_vv = 0 : en_r0 = -1
  for en_r = 0 to en.nr - 1
    if ru.g(en_r) = g then
      if ru.d(en_r) = 0 then
        en_hh = 1
      else
        en_vv = 1
      end if
      if ru.l(en_r) > en_mx then
        en_mx = ru.l(en_r)
        en_r0 = en_r
      end if
    end if
  next en_r
  en_kind = 0
  if en_mx >= 5 then
    en_kind = K_PR
  elseif en_hh and en_vv then
    en_kind = K_WR
  elseif en_mx = 4 then
    en_kind = K_SH
    if ru.d(en_r0) = 1 then en_kind = K_SV
  end if
  if en_kind = 0 then
    exit sub
  end if
  en_pos = -1
  if en.p = 1 and grp(en.b) = g then
    en_pos = en.b
  elseif en.p = 1 and grp(en.a) = g then
    en_pos = en.a
  elseif en_kind = K_WR then
    for en_i = 0 to 80
      if hr(en_i) >= 0 and vr(en_i) >= 0 then
        if ru.g(hr(en_i)) = g then
          en_pos = en_i
          exit for
        end if
      end if
    next en_i
  else
    en_pos = ru.i(en_r0) + ((ru.l(en_r0) - 1) \ 2) * (1 + 8 * ru.d(en_r0))
  end if
  if ch(en_pos) then
    en_pos = -1
    for en_i = 0 to 80
      if grp(en_i) = g and ch(en_i) = 0 then
        en_pos = en_i
        exit for
      end if
    next en_i
  end if
  if en_pos < 0 then
    exit sub
  end if
  mk.pos(en.mn) = en_pos
  mk.kind(en.mn) = en_kind
  if en_kind = K_PR then
    mk.c(en.mn) = PRISM_C
    en.score = en.score + 150
  else
    mk.c(en.mn) = gc(en_pos)
    if en_kind = K_WR then
      en.score = en.score + 100
    else
      en.score = en.score + 60
    end if
  end if
  en.mn = en.mn + 1
end sub

sub en.addEv(i, c)
  ev.i(en.hn) = i
  ev.c(en.hn) = c
  en.hn = en.hn + 1
end sub

' one clear phase; FALSE when nothing matched (the move has settled)
function en.clear()
  en.p = en.p + 1
  en.clear = 0
  en.qn = 0 : en.fn = 0 : en.hn = 0 : en.mn = 0
  for en_i = 0 to 80
    en.qm(en_i) = 0
    bm(en_i) = 0
  next en_i
  if en.combo then
    en.comboStart()
  else
    en.findRuns()
    if en.nr = 0 then
      exit function
    end if
    en.matchStart()
  end if
  en.clear = 1
  en_qh = 0
  do while en_qh < en.qn
    en_q = en.q(en_qh)
    en_qh = en_qh + 1
    en.hit(en_q)
  loop
  for en_i = 0 to 80
    if bm(en_i) then
      bd(en_i) = bd(en_i) - 1
      en.score = en.score + 30
      en.addEv(en_i, -3)
      if bd(en_i) = 0 then en.goal(G_BOULDER, -1)
    end if
  next en_i
  for en_k = 0 to en.mn - 1
    gc(mk.pos(en_k)) = mk.c(en_k)
    gk(mk.pos(en_k)) = mk.kind(en_k)
    en.made = en.made + 1
  next en_k
end function

' one cell of the hit queue
sub en.hit(i)
  local c, k
  if bd(i) then
    bm(i) = 1
    exit sub
  end if
  c = gc(i)
  if c < 0 then
    exit sub
  end if
  if ch(i) then
    ch(i) = 0
    en.goal(G_CHAIN, -1)
    en.score = en.score + 20
    en.addEv(i, -2)
    exit sub
  end if
  k = gk(i)
  if c <= 5 then en.goal(G_COL, c)
  if fr(i) then
    fr(i) = fr(i) - 1
    en.goal(G_FROST, -1)
    en.score = en.score + 20
  end if
  en.score = en.score + 10 * en.p
  gc(i) = -1
  gk(i) = 0
  en.addEv(i, c)
  if k then en.fire(i, k)
end sub

' gravity: in each column the gems drop past fixed cells (holes, boulders,
' chained gems) to the lowest free cells, new gems come in from the top.
' fl.y(i) = the row the gem now in cell i came from (above the board for new
' ones), fl.mv(i) = 1 if it moved; en.fmax = the longest drop in rows
sub en.fall()
  local x, y, n, r, w, i, j, top
  en.fmax = 0
  for i = 0 to 80
    fl.mv(i) = 0
    fl.y(i) = i \ 9
  next i
  for x = 0 to 8
    n = 0
    for y = 0 to 8
      i = y * 9 + x
      if ac(i) and bd(i) = 0 and ch(i) = 0 then
        sl.i(n) = i
        n = n + 1
      end if
    next y
    w = n - 1
    for r = n - 1 to 0 step -1
      i = sl.i(r)
      if gc(i) >= 0 then
        if r <> w then
          j = sl.i(w)
          gc(j) = gc(i) : gk(j) = gk(i) : gc(i) = -1 : gk(i) = 0
          fl.y(j) = fl.y(i)
          fl.mv(j) = 1
          fl.y(i) = i \ 9
          en.fmax = max(en.fmax, j \ 9 - fl.y(j))
        end if
        w = w - 1
      end if
    next r
    if w >= 0 then
      top = sl.i(0) \ 9
      for r = w to 0 step -1
        j = sl.i(r)
        gc(j) = en.rnd() mod en.nc
        gk(j) = 0
        fl.y(j) = top - (w + 1) + r
        fl.mv(j) = 1
        en.fmax = max(en.fmax, j \ 9 - fl.y(j))
      next r
    end if
  next x
end sub

' after the last clear: 1 won, 2 lost (no moves left), 0 play on (shuffled
' first if no swap is left; the hint swap is in en.ha, en.hb)
function en.settle()
  en.settle = 0
  if en.won() then
    en.settle = 1
    exit function
  end if
  if en.moves <= 0 then
    en.settle = 2
    exit function
  end if
  if not en.findMove() then en.shuffle()
end function

function en.levelScore()
  en.levelScore = en.score + PTS_MOVE * en.moves
end function

' goal progress 0..1000 (all goals together)
function en.progress()
  local t, d
  t = 0
  d = 0
  for en_g = 0 to en.ng - 1
    t = t + en.g0(en_g)
    d = d + en.g0(en_g) - max(0, en.gl(en_g))
  next en_g
  en.progress = 0
  if t > 0 then en.progress = (1000 * d) \ t
end function
