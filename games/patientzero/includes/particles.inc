' particles.inc - fixed pool. Sneeze spray and rain are decoration.

type TPart
  x as integer
  y as integer
  vx as integer
  vy as integer
  life as integer
  kind as integer
  alive as integer
  grav as integer
  ox as integer
  oy as integer
  ow as integer
  oh as integer
end type

dim part(PT_LAST) as TPart
dim pt_i, pt_j, pt_k, pt_n, pt_dx, pt_dy

sub pt.clear()
  for pt_i = 0 to PT_LAST
    part(pt_i).alive = 0
    part(pt_i).ow = 0
  next pt_i
end sub

function pt.slot()
  pt.slot = -1
  for pt_j = 0 to PT_LAST
    if part(pt_j).alive = 0 then
      pt.slot = pt_j
      exit function
    end if
  next pt_j
end function

sub pt.spawn(x, y, vx, vy, life, kind, grav)
  pt_n = pt.slot()
  if pt_n < 0 then
    exit sub
  end if
  part(pt_n).x = x
  part(pt_n).y = y
  part(pt_n).vx = vx
  part(pt_n).vy = vy
  part(pt_n).life = life
  part(pt_n).kind = kind
  part(pt_n).grav = grav
  part(pt_n).alive = 1
  part(pt_n).ow = 0
end sub

' Both sides of Jerry. Puffs are placed already spread out so the first frame reads as a cloud.
sub pt.sneeze()
  for pt_k = 0 to 15
    if pt_k < 8 then
      pt_dx = 0 - (14 + rndn(52))
    else
      pt_dx = 22 + rndn(52)
    end if
    pt_dy = 2 + rndn(20)
    pt_j = F_PUFF16 + (pt_k mod 4)
    if (pt_k mod 5) = 0 then pt_j = F_PUFF24 + (pt_k mod 2)
    if pt_k = 0 or pt_k = 8 then pt_j = F_PUFF32
    pt.spawn pl.x + (pt_dx * 256), pl.y + (pt_dy * 256), pt_dx * 6, 0 - rndn(30), 40, pt_j, 4
  next pt_k
end sub

sub pt.spark(x, y)
  pt.spawn(x * 256, y * 256, rndn(80) - 40, 0 - rndn(60), 12, F_SPARK_G, 0)
  pt.spawn(x * 256, y * 256, rndn(80) - 40, 0 - rndn(40), 12, F_SPARK, 0)
end sub

sub pt.rain()
  pt_n = 0
  for pt_k = 0 to PT_LAST
    if part(pt_k).alive = 1 then
      if part(pt_k).kind = F_RAIN_0 or part(pt_k).kind = F_RAIN_1 then pt_n = pt_n + 1
    end if
  next pt_k
  do while pt_n < 14
    pt.spawn rndn(BASE_W) * 256, rndn(40) * 256, 180, 900, 48 + rndn(24), F_RAIN_0 + rndn(2), 0
    pt_n = pt_n + 1
    if pt.slot() < 0 then
      exit sub
    end if
  loop
end sub

sub pt.tick()
  if lv.raining = 1 then pt.rain()
  for pt_i = 0 to PT_LAST
    if part(pt_i).alive = 1 then
      part(pt_i).x = part(pt_i).x + part(pt_i).vx
      part(pt_i).y = part(pt_i).y + part(pt_i).vy
      part(pt_i).vy = part(pt_i).vy + part(pt_i).grav
      part(pt_i).life = part(pt_i).life - 1
      if part(pt_i).life <= 0 then part(pt_i).alive = 0
      if part(pt_i).y > BASE_H * 256 then part(pt_i).alive = 0
      if part(pt_i).x < -20 * 256 or part(pt_i).x > (BASE_W + 20) * 256 then part(pt_i).alive = 0
    end if
  next pt_i
end sub
