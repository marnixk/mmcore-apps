' actors.inc - the things that move: the player (0), the orbs (1 .. ORB_MAX)
' and the sentinels (A_EN ..), plus one effect slot (A_FX, the hit spark).
'
' Logic is in whole cells (o.x, o.y); on screen an actor glides from its
' previous cell (o.lx, o.ly) to the new one over the step time o.T.
' Drawing, once per frame: every actor whose picture or place changed is
' erased (its old 16 x 16 box copied back from the clean field page), then all
' actors are drawn over the field (black = see-through) and only the old + new
' boxes are marked for the screen. Field changes under an actor set act.dirty,
' which redraws all actors that frame.
' hot-path helpers keep their scratch in globals: LOCAL costs O(variables)
' per call in mmcore (each LOCAL scans the whole variable table)
dim integer act_blit_sx, act_blit_sy, act_blit_x, act_blit_y, act_blit_w, act_blit_h, act_blit_s
' act.render scratch, and per-frame tables: ar_ks = each actor's animation
' frame this frame, ar_ord = drawing order (player last); ao / al = orb and
' sentinel frames by phase (so the frame is a table lookup, no MOD per actor)
dim integer ar_i, ar_j, ar_k, ar_m, ar_now, ar_f, ar_x, ar_y, ar_sp, ar_any, ar_x0, ar_y0, ar_x1, ar_y1
dim integer ar_T, ar_s, ar_w, ar_h, ar_q, ar_c, ar_in, ar_e, ar_lx, ar_ly, ar_ox, ar_oy, ar_px, ar_py, ar_bs, ar_sx
dim integer ar_mx, ar_my, ar_dx, ar_dy

const A_N = 13, A_EN = ORB_MAX + 1, A_FX = 12
dim o.on(A_N - 1), o.x(A_N - 1), o.y(A_N - 1), o.lx(A_N - 1), o.ly(A_N - 1)
dim o.dx(A_N - 1), o.dy(A_N - 1), o.t0(A_N - 1), o.T(A_N - 1)
dim o.ax(A_N - 1), o.ay(A_N - 1), o.sp(A_N - 1), o.vis(A_N - 1)
' o.fs: a fixed sprite for this actor (-1 = animate normally): derez, spark
dim o.fs(A_N - 1)
dim act.nOrb, act.nEn, act.dirty
dim integer ar_ks(A_N - 1), ar_ord(A_N - 1), ao(A_N + 3), al(A_N + 1)

sub act.clear()
  local i
  for i = 0 to A_N + 3
    ao(i) = S_ORB_0 + i mod 4
  next i
  for i = 0 to A_N + 1
    al(i) = S_LAND_0 + i mod 2
  next i
  for i = 0 to A_N - 2
    ar_ord(i) = i + 1
  next i
  ar_ord(A_N - 1) = 0
  for i = 0 to A_N - 1
    o.on(i) = FALSE
    o.vis(i) = FALSE
    o.fs(i) = -1
    o.T(i) = 1
  next i
  act.nOrb = 0
  act.nEn = 0
end sub

' put actor i on cell (x, y), not moving
sub act.place(i, x, y)
  o.on(i) = TRUE
  o.x(i) = x
  o.y(i) = y
  o.lx(i) = x
  o.ly(i) = y
  o.t0(i) = 0
  o.T(i) = 1
end sub

' a step to cell (x, y) that takes T ms on screen
sub act.move(i, x, y, T, now)
  o.lx(i) = o.x(i)
  o.ly(i) = o.y(i)
  o.x(i) = x
  o.y(i) = y
  o.t0(i) = now
  o.T(i) = T
end sub

' everything is redrawn from scratch next frame (after a full scene change)
sub act.invalidate()
  local i
  for i = 0 to A_N - 1
    o.vis(i) = FALSE
  next i
end sub

function act.sprite(i, now)
  if o.fs(i) >= 0 then
    act.sprite = o.fs(i)
  elseif i = 0 then
    act.sprite = S_PLAYER_0 + (now \ 300) mod 2
  elseif i < A_EN then
    act.sprite = S_ORB_0 + (now \ 70 + i) mod 4
  else
    act.sprite = S_LAND_0 + (now \ 260 + i) mod 2
  end if
end function

' a 16 x 16 sheet sprite at art x, y on the back page, clipped to the screen
sub act.blit(sp, ax, ay)
  act_blit_sx = sp * 16
  act_blit_sy = 0
  act_blit_x = ax
  act_blit_y = ay
  act_blit_w = 16
  act_blit_h = 16
  if act_blit_x < 0 then
    act_blit_sx = act_blit_sx - act_blit_x
    act_blit_w = act_blit_w + act_blit_x
    act_blit_x = 0
  end if
  if act_blit_y < 0 then
    act_blit_sy = act_blit_sy - act_blit_y
    act_blit_h = act_blit_h + act_blit_y
    act_blit_y = 0
  end if
  if act_blit_x + act_blit_w > lay.aw then act_blit_w = lay.aw - act_blit_x
  if act_blit_y + act_blit_h > lay.ah then act_blit_h = lay.ah - act_blit_y
  act_blit_s = scr.sc
  blit act_blit_sx * act_blit_s, act_blit_sy * act_blit_s, act_blit_x * act_blit_s, act_blit_y * act_blit_s, act_blit_w * act_blit_s, act_blit_h * act_blit_s, PG_SHEET, 4
end sub

' (one frame for all actors, about 60 times a second. Written for mmcore's
' fast path: a statement "x = arithmetic" or "IF a <op> b THEN x = ..." on
' its own line, with array subscripts that are plain variables, runs several
' times faster than anything else (AND / OR, NOT, MIN, MAX, \, MOD, block
' IFs, calls, BLIT arguments). A box inside the screen - nearly always - is
' copied and marked directly; one at the edge goes through the clipping
' helpers scr.copyA / scr.markA / act.blit.)
sub act.render()
  ar_now = timer : ar_any = act.dirty : ar_s = scr.sc : ar_bs = 16 * ar_s
  ar_mx = lay.aw - 16 : ar_my = lay.ah - 16
  ar_ks(0) = S_PLAYER_0 + (ar_now \ 300) mod 2
  ar_m = (ar_now \ 70) mod 4
  for ar_i = 1 to A_EN - 1
    ar_k = ar_m + ar_i : ar_ks(ar_i) = ao(ar_k)
  next ar_i
  ar_m = (ar_now \ 260) mod 2
  for ar_i = A_EN to A_N - 1
    ar_k = ar_m + ar_i : ar_ks(ar_i) = al(ar_k)
  next ar_i
  for ar_i = 0 to A_N - 1
    if o.on(ar_i) = 0 then goto arOff
    ' where it is: a glide from (lx, ly) to (x, y) over T ms
    ar_f = ar_now - o.t0(ar_i) : ar_T = o.T(ar_i)
    if ar_f > ar_T then ar_f = ar_T
    ar_lx = o.lx(ar_i) : ar_ly = o.ly(ar_i) : ar_dx = o.x(ar_i) - ar_lx : ar_dy = o.y(ar_i) - ar_ly
    ar_q = ar_dx * 8 * ar_f / ar_T
    ar_x = lay.ox + ar_lx * 8 + ar_q - 4
    ar_q = ar_dy * 8 * ar_f / ar_T
    ar_y = lay.oy + ar_ly * 8 + ar_q - 4
    ar_sp = o.fs(ar_i)
    if ar_sp < 0 then ar_sp = ar_ks(ar_i)
    if o.vis(ar_i) = 0 then goto arNew
    ar_ox = o.ax(ar_i) : ar_oy = o.ay(ar_i)
    ar_c = 0
    if ar_x <> ar_ox then ar_c = 1
    if ar_y <> ar_oy then ar_c = 1
    if ar_sp <> o.sp(ar_i) then ar_c = 1
    if ar_c = 0 then continue for
    ' moved: erase the old box, mark old + new box
    ar_x0 = ar_x
    if ar_ox < ar_x0 then ar_x0 = ar_ox
    ar_x1 = ar_x
    if ar_ox > ar_x1 then ar_x1 = ar_ox
    ar_y0 = ar_y
    if ar_oy < ar_y0 then ar_y0 = ar_oy
    ar_y1 = ar_y
    if ar_oy > ar_y1 then ar_y1 = ar_oy
    ar_w = ar_x1 - ar_x0 + 16 : ar_h = ar_y1 - ar_y0 + 16
    ar_in = 1
    if ar_x0 < 0 then ar_in = 0
    if ar_y0 < 0 then ar_in = 0
    ar_e = ar_x0 + ar_w
    if ar_e > lay.aw then ar_in = 0
    ar_e = ar_y0 + ar_h
    if ar_e > lay.ah then ar_in = 0
    if dr.n >= DR_MAX then ar_in = 0
    if scr.full <> 0 then ar_in = 0
    if ar_in = 0 then goto arEdge
    ar_px = ar_ox * ar_s : ar_py = ar_oy * ar_s
    blit ar_px, ar_py, ar_px, ar_py, ar_bs, ar_bs, PG_FIELD
    ar_k = dr.n : dr.x(ar_k) = ar_x0 * ar_s : dr.y(ar_k) = ar_y0 * ar_s : dr.w(ar_k) = ar_w * ar_s : dr.h(ar_k) = ar_h * ar_s
    dr.n = ar_k + 1 : scr.dirty = TRUE
    o.ax(ar_i) = ar_x : o.ay(ar_i) = ar_y : o.sp(ar_i) = ar_sp : ar_any = TRUE
    continue for
arEdge:
    scr.copyA(ar_ox, ar_oy, 16, 16, PG_FIELD)
    scr.markA(ar_x0, ar_y0, ar_w, ar_h)
    o.ax(ar_i) = ar_x : o.ay(ar_i) = ar_y : o.sp(ar_i) = ar_sp : ar_any = TRUE
    continue for
arNew:
    o.ax(ar_i) = ar_x : o.ay(ar_i) = ar_y : o.sp(ar_i) = ar_sp : o.vis(ar_i) = TRUE : ar_any = TRUE
    scr.markA(ar_x, ar_y, 16, 16)
    continue for
arOff:
    if o.vis(ar_i) = 0 then continue for
    scr.copyA(o.ax(ar_i), o.ay(ar_i), 16, 16, PG_FIELD)
    scr.markA(o.ax(ar_i), o.ay(ar_i), 16, 16)
    o.vis(ar_i) = FALSE : ar_any = TRUE
  next ar_i
  act.dirty = FALSE
  if ar_any = 0 then exit sub
  ' draw every visible actor over the field, the player last
  for ar_j = 0 to A_N - 1
    ar_i = ar_ord(ar_j)
    if o.vis(ar_i) = 0 then continue for
    ar_x = o.ax(ar_i) : ar_y = o.ay(ar_i)
    ar_in = 1
    if ar_x < 0 then ar_in = 0
    if ar_y < 0 then ar_in = 0
    if ar_x > ar_mx then ar_in = 0
    if ar_y > ar_my then ar_in = 0
    if ar_in = 0 then goto arClip
    ar_sx = o.sp(ar_i) * ar_bs : ar_px = ar_x * ar_s : ar_py = ar_y * ar_s
    blit ar_sx, 0, ar_px, ar_py, ar_bs, ar_bs, PG_SHEET, 4
    continue for
arClip:
    act.blit(o.sp(ar_i), ar_x, ar_y)
  next ar_j
end sub

