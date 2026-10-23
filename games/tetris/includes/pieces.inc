' pieces.inc - the seven tetrominoes, SRS rotation states + wall kicks, 7-bag
'
' Kinds 0..6 = I O T S Z J L (also the column in the tile sheets).
' Each rotation state is 4 cells (x, y) inside the piece's 4x4 / 3x3 box,
' y grows downwards. States 0, R, 2, L = 0..3 (clockwise order).

dim pc.cx(6, 3, 3), pc.cy(6, 3, 3)
' kicks: pc.kx(t, tr, i) / pc.ky(t, tr, i): t 0 = JLSTZ, 1 = I;
' tr = from * 2 + dir (dir 0 clockwise, 1 counter-clockwise); 5 tests.
' Stored in the SRS convention (y up) and negated when applied.
dim pc.kx(1, 7, 4), pc.ky(1, 7, 4)
dim pc.bag(6), pc.bagN = 0

sub pc.init()
  local k, r, i, s$
  local string st$(27) length 32
  st$(0) = "01112131"
  st$(1) = "20212223"
  st$(2) = "02122232"
  st$(3) = "10111213"
  st$(4) = "10201121"
  st$(5) = "10201121"
  st$(6) = "10201121"
  st$(7) = "10201121"
  st$(8) = "10011121"
  st$(9) = "10112112"
  st$(10) = "01112112"
  st$(11) = "10011112"
  st$(12) = "10200111"
  st$(13) = "10112122"
  st$(14) = "11210212"
  st$(15) = "00011112"
  st$(16) = "00101121"
  st$(17) = "20112112"
  st$(18) = "01111222"
  st$(19) = "10011102"
  st$(20) = "00011121"
  st$(21) = "10201112"
  st$(22) = "01112122"
  st$(23) = "10110212"
  st$(24) = "20011121"
  st$(25) = "10111222"
  st$(26) = "01112102"
  st$(27) = "00101112"
  local string kj$(7) length 32
  kj$(0) = "0,0,-1,0,-1,1,0,-2,-1,-2"
  kj$(1) = "0,0,1,0,1,1,0,-2,1,-2"
  kj$(2) = "0,0,1,0,1,-1,0,2,1,2"
  kj$(3) = "0,0,1,0,1,-1,0,2,1,2"
  kj$(4) = "0,0,1,0,1,1,0,-2,1,-2"
  kj$(5) = "0,0,-1,0,-1,1,0,-2,-1,-2"
  kj$(6) = "0,0,-1,0,-1,-1,0,2,-1,2"
  kj$(7) = "0,0,-1,0,-1,-1,0,2,-1,2"
  local string ki$(7) length 32
  ki$(0) = "0,0,-2,0,1,0,-2,-1,1,2"
  ki$(1) = "0,0,-1,0,2,0,-1,2,2,-1"
  ki$(2) = "0,0,-1,0,2,0,-1,2,2,-1"
  ki$(3) = "0,0,2,0,-1,0,2,1,-1,-2"
  ki$(4) = "0,0,2,0,-1,0,2,1,-1,-2"
  ki$(5) = "0,0,1,0,-2,0,1,-2,-2,1"
  ki$(6) = "0,0,1,0,-2,0,1,-2,-2,1"
  ki$(7) = "0,0,-2,0,1,0,-2,-1,1,2"
  for k = 0 to 6
    for r = 0 to 3
      s$ = st$(k * 4 + r)
      for i = 0 to 3
        pc.cx(k, r, i) = val(mid$(s$, i * 2 + 1, 1))
        pc.cy(k, r, i) = val(mid$(s$, i * 2 + 2, 1))
      next i
    next r
  next k
  for r = 0 to 7
    pc.kick(0, r, kj$(r))
    pc.kick(1, r, ki$(r))
  next r
  pc.bagN = 0
end sub

sub pc.kick(t, tr, s$)
  local i, p = 1, q, n = 0, v(9)
  do while n < 10
    q = instr(p, s$ + ",", ",")
    v(n) = val(mid$(s$, p, q - p))
    p = q + 1
    n = n + 1
  loop
  for i = 0 to 4
    pc.kx(t, tr, i) = v(i * 2)
    pc.ky(t, tr, i) = v(i * 2 + 1)
  next i
end sub

' standard 7-bag randomiser
function pc.next()
  local i, j, t
  if pc.bagN = 0 then
    for i = 0 to 6
      pc.bag(i) = i
    next i
    for i = 6 to 1 step -1
      j = int(rnd * (i + 1))
      t = pc.bag(i)
      pc.bag(i) = pc.bag(j)
      pc.bag(j) = t
    next i
    pc.bagN = 7
  end if
  pc.bagN = pc.bagN - 1
  pc.next = pc.bag(pc.bagN)
end function
