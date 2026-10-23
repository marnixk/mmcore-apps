' generator.inc - deal a fresh puzzle: pick a bank puzzle for the level and apply
' random validity-preserving transforms (digit relabel, band/stack order, row and
' column order inside each band/stack, optional transpose). Uniqueness is kept.

dim gen.p(9), gen.rmap(8), gen.cmap(8), gen.tmp(2)

' shuffle gen.tmp(0..2)
sub gen.shuffle3()
  local i, j, t
  for i = 2 to 1 step -1
    j = int(rnd * (i + 1))
    t = gen.tmp(i)
    gen.tmp(i) = gen.tmp(j)
    gen.tmp(j) = t
  next i
end sub

' build a row (or column) map: random band order + random order inside each band
sub gen.makeMap(which)
  local band(2), b, k
  gen.tmp(0) = 0
  gen.tmp(1) = 1
  gen.tmp(2) = 2
  gen.shuffle3()
  for b = 0 to 2
    band(b) = gen.tmp(b)
  next b
  for b = 0 to 2
    gen.tmp(0) = 0
    gen.tmp(1) = 1
    gen.tmp(2) = 2
    gen.shuffle3()
    for k = 0 to 2
      if which = 0 then
        gen.rmap(b * 3 + k) = band(b) * 3 + gen.tmp(k)
      else
        gen.cmap(b * 3 + k) = band(b) * 3 + gen.tmp(k)
      end if
    next k
  next b
end sub

' fill bd.g() / bd.giv() with a new puzzle for level lv
sub gen.deal(lv)
  local s$, i, j, r, c, t, tr, ch$
  s$ = bank$(lv * BANK_PER_LEVEL + int(rnd * BANK_PER_LEVEL))
  ' digit relabel
  for i = 1 to 9
    gen.p(i) = i
  next i
  for i = 9 to 2 step -1
    j = 1 + int(rnd * i)
    t = gen.p(i)
    gen.p(i) = gen.p(j)
    gen.p(j) = t
  next i
  gen.makeMap(0)
  gen.makeMap(1)
  tr = (rnd < 0.5)
  for i = 0 to 8
    for j = 0 to 8
      r = gen.rmap(i)
      c = gen.cmap(j)
      if tr then t = r : r = c : c = t
      ch$ = mid$(s$, r * 9 + c + 1, 1)
      if ch$ = "." then
        bd.g(i * 9 + j) = 0
        bd.giv(i * 9 + j) = FALSE
      else
        bd.g(i * 9 + j) = gen.p(val(ch$))
        bd.giv(i * 9 + j) = TRUE
      end if
    next j
  next i
end sub
