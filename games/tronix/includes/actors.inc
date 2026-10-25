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

const A_N = 13, A_EN = ORB_MAX + 1, A_FX = 12
dim o.on(A_N - 1), o.x(A_N - 1), o.y(A_N - 1), o.lx(A_N - 1), o.ly(A_N - 1)
dim o.dx(A_N - 1), o.dy(A_N - 1), o.t0(A_N - 1), o.T(A_N - 1)
dim o.ax(A_N - 1), o.ay(A_N - 1), o.sp(A_N - 1), o.vis(A_N - 1)
' o.fs: a fixed sprite for this actor (-1 = animate normally): derez, spark
dim o.fs(A_N - 1)
dim act.nOrb, act.nEn, act.dirty

sub act.clear()
  local i
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

sub act.render()
  local i, now, f, ax, ay, sp, any, x0, y0, T
  now = timer
  any = act.dirty
  for i = 0 to A_N - 1
    if o.on(i) then
      f = now - o.t0(i)
      T = o.T(i)
      if f >= T then
        ax = lay.ox + o.x(i) * 8 - 4
        ay = lay.oy + o.y(i) * 8 - 4
      else
        ax = lay.ox + o.lx(i) * 8 + ((o.x(i) - o.lx(i)) * 8 * f) \ T - 4
        ay = lay.oy + o.ly(i) * 8 + ((o.y(i) - o.ly(i)) * 8 * f) \ T - 4
      end if
      sp = act.sprite(i, now)
      if not o.vis(i) then
        o.ax(i) = ax
        o.ay(i) = ay
        o.sp(i) = sp
        o.vis(i) = TRUE
        scr.markA(ax, ay, 16, 16)
        any = TRUE
      elseif ax <> o.ax(i) or ay <> o.ay(i) or sp <> o.sp(i) then
        scr.copyA(o.ax(i), o.ay(i), 16, 16, PG_FIELD)
        x0 = min(ax, o.ax(i))
        y0 = min(ay, o.ay(i))
        scr.markA(x0, y0, max(ax, o.ax(i)) - x0 + 16, max(ay, o.ay(i)) - y0 + 16)
        o.ax(i) = ax
        o.ay(i) = ay
        o.sp(i) = sp
        any = TRUE
      end if
    elseif o.vis(i) then
      scr.copyA(o.ax(i), o.ay(i), 16, 16, PG_FIELD)
      scr.markA(o.ax(i), o.ay(i), 16, 16)
      o.vis(i) = FALSE
      any = TRUE
    end if
  next i
  if any then
    for i = 1 to A_N - 1
      if o.vis(i) then act.blit(o.sp(i), o.ax(i), o.ay(i))
    next i
    if o.vis(0) then act.blit(o.sp(0), o.ax(0), o.ay(0))
  end if
  act.dirty = FALSE
end sub
