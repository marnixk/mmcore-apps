' view.inc - the actors as SPRITEs on the back page over the static cantina.
'
' Every actor slot owns one SPRITE number: the cook, 16 customers, 20 plates,
' 4 coins and two effects (smash, puff). vw.set(id, frame, x, y) asks for a
' frame at a screen position (frame -1 = hidden) and does the least work:
'   same frame, new place  -> SPRITE NEXT (all moves go out in one SPRITE MOVE)
'   new frame              -> SPRITE HIDE, SPRITE READ from the sheet, SPRITE SHOW
'                             (READ drops the old background copy, so HIDE first)
' All sprites share one layer, so a new SHOW lands on top without redrawing
' the others. The old and new boxes are marked for scr.flush.
' Customers and the cook are cropped at the counter top, so the counter
' (drawn in the backdrop) seems to stand in front of them.
'
' Frame numbers: customers t * CF_N + f (sheetc), cook FR_COOK + f, items
' FR_ITEM + i (sheetk items row).
const SPR_COOK = 1, SPR_CU = 2, SPR_PL = 18, SPR_CO = 38, SPR_SMASH = 42, SPR_PUFF = 43
const SPR_BOWL = 44, SPR_CHIP = 47, SPR_PTR = 48, SPR_N = 49
const FR_COOK = 100, FR_ITEM = 200
const IT_PF = 0, IT_PE = 1, IT_CO = 2, IT_SM = 3, IT_SP = 4, IT_HT = 5, IT_PU = 6
' draw layer per sprite (SPRITE SHOW keeps the stack sorted by layer, so a frame change
' re-shows a customer under the plates instead of popping it on top)
dim vs.lay(SPR_N - 1)
dim vs.fr(SPR_N - 1), vs.x(SPR_N - 1), vs.y(SPR_N - 1), vs.ax(SPR_N - 1), vs.ay(SPR_N - 1), vs.w(SPR_N - 1), vs.h(SPR_N - 1)
dim it.x(6), it.w(6), it.h(6)
' 3+ customer frame changes in one frame: re-stack by hand (vw.batchHide)
const VW_BATCH = 3
dim vw.nmov, vw.lastMov, vw.cf(NC - 1)
dim integer vw_n, vw_u, vw_ux, vw_uy, vw_ux2, vw_uy2
dim vs.ox(SPR_N - 1), vs.oy(SPR_N - 1), vs.hid(SPR_N - 1)
' per layout: lane tops (px) of the figures, plate / coin rows, x scales
dim vw.ty(3), vw.pfy(3), vw.pey(3), vw.coy(3), vw.cm, vw.pm, vw.km
' scratch (globals: the hot path passes values in these, LOCAL is slow)
dim integer vw_i, vw_f, vw_x, vw_y, vw_w, vw_h, vw_sx, vw_sy, vw_pg, vw_k, vw_l, vw_ox, vw_oy, vw_id

sub vw.init()
  it.x(0) = IT_PLATEF_X : it.w(0) = IT_PLATEF_W : it.h(0) = IT_PLATEF_H
  it.x(1) = IT_PLATEE_X : it.w(1) = IT_PLATEE_W : it.h(1) = IT_PLATEE_H
  it.x(2) = IT_COIN_X : it.w(2) = IT_COIN_W : it.h(2) = IT_COIN_H
  it.x(3) = IT_SMASH_X : it.w(3) = IT_SMASH_W : it.h(3) = IT_SMASH_H
  it.x(4) = IT_SPARK_X : it.w(4) = IT_SPARK_W : it.h(4) = IT_SPARK_H
  it.x(5) = IT_HEART_X : it.w(5) = IT_HEART_W : it.h(5) = IT_HEART_H
  it.x(6) = IT_PUFF_X : it.w(6) = IT_PUFF_W : it.h(6) = IT_PUFF_H
  for vw_i = 0 to SPR_N - 1
    vs.fr(vw_i) = -1
    vs.lay(vw_i) = 1
    if vw_i >= SPR_CU and vw_i < SPR_PL then vs.lay(vw_i) = 2
    if vw_i = SPR_COOK then vs.lay(vw_i) = 3
    if vw_i >= SPR_PL and vw_i < SPR_CO then vs.lay(vw_i) = 4
    if vw_i >= SPR_CO and vw_i < SPR_SMASH then vs.lay(vw_i) = 5
    if vw_i = SPR_SMASH or vw_i = SPR_PUFF then vs.lay(vw_i) = 6
  next vw_i
  for vw_i = 0 to 3
    vw.ty(vw_i) = (lay.yc(vw_i) - FR_H) * scr.k
    vw.pfy(vw_i) = (lay.yc(vw_i) - IT_PLATEF_H + 1) * scr.k
    vw.pey(vw_i) = (lay.yc(vw_i) - IT_PLATEE_H + 1) * scr.k
    vw.coy(vw_i) = (lay.yc(vw_i) - 8) * scr.k
  next vw_i
  vw.cm = lay.cx1 - 36
  vw.pm = lay.cx1 - lay.cx0 - 18
  vw.km = lay.cx1 - lay.cx0 - 8
end sub

' all sprites off (their backgrounds restored) and forgotten
sub vw.clear()
  sprite close all
  for vw_i = 0 to SPR_N - 1
    vs.fr(vw_i) = -1
    vs.hid(vw_i) = 0
  next vw_i
end sub

' where frame f lives: vw_sx, vw_sy, vw_w, vw_h (px), vw_pg, and the offset
' of its opaque box inside the cell vw_ox, vw_oy (px)
sub vw.src(f)
  vw_k = scr.k
  if f < FR_COOK then
    vw_pg = PG_SHC
    vw_ox = cf.dx(f) * vw_k
    vw_oy = cf.dy(f) * vw_k
    vw_sx = (f mod lay.ccols) * CC_W * vw_k + vw_ox
    vw_sy = (f \ lay.ccols) * FR_H * vw_k + vw_oy
    vw_w = cf.w(f) * vw_k
    vw_h = cf.h(f) * vw_k
  elseif f < FR_ITEM then
    vw_pg = PG_SHK
    vw_ox = kf.dx(f - FR_COOK) * vw_k
    vw_oy = kf.dy(f - FR_COOK) * vw_k
    vw_sx = ((f - FR_COOK) mod lay.kcols) * KC_W * vw_k + vw_ox
    vw_sy = ((f - FR_COOK) \ lay.kcols) * FR_H * vw_k + vw_oy
    vw_w = kf.w(f - FR_COOK) * vw_k
    vw_h = kf.h(f - FR_COOK) * vw_k
  else
    vw_pg = PG_SHK
    vw_ox = 0
    vw_oy = 0
    vw_w = it.w(f - FR_ITEM) * vw_k
    vw_h = it.h(f - FR_ITEM) * vw_k
    vw_sx = it.x(f - FR_ITEM) * vw_k
    vw_sy = lay.iy * vw_k
  end if
end sub

' ---- the sprite stack
' Every actor has a fixed draw layer (vs.lay: customers 2, cook 3, plates 4,
' tips 5, smash / puff 6). SPRITE SHOW keeps the stack sorted by layer, so the
' HIDE / READ / SHOW of a frame change puts a customer back under the plates
' (the interpreter re-stacks natively, cheap) instead of popping it on top.
' Moves of an unchanged frame are queued with SPRITE NEXT and sent by vw.commit.

' frame vw_f of sprite vw_id with its cell at screen vw_x, vw_y (vw_f = -1:
' hide). Callers skip it when nothing changed.
sub vw.put()
  if vs.hid(vw_id) then
    ' taken off by vw.batchHide: show it again (on top of what is shown so far)
    vs.hid(vw_id) = 0
    if vw_f = vs.fr(vw_id) then
      if vw_x <> vs.x(vw_id) or vw_y <> vs.y(vw_id) then vw.markMove()
      sprite show vw_id, vs.ax(vw_id), vs.ay(vw_id), vs.lay(vw_id)
      exit sub
    end if
  end if
  if vw_f = vs.fr(vw_id) then
    if vw_f < 0 then exit sub
    vw.markMove()
    sprite next vw_id, vs.ax(vw_id), vs.ay(vw_id)
    vw.nmov = vw.nmov + 1
    vw.lastMov = vw_id
    exit sub
  end if
  vw_u = 0
  if vs.fr(vw_id) >= 0 then
    sprite hide vw_id
    vw_u = 1
    vw_ux = vs.ax(vw_id)
    vw_uy = vs.ay(vw_id)
    vw_ux2 = vw_ux + vs.w(vw_id)
    vw_uy2 = vw_uy + vs.h(vw_id)
  end if
  vs.fr(vw_id) = vw_f
  if vw_f < 0 then
    if vw_u then scr.mark(vw_ux, vw_uy, vw_ux2 - vw_ux, vw_uy2 - vw_uy)
    exit sub
  end if
  vw.src(vw_f)
  sprite read vw_id, vw_sx, vw_sy, vw_w, vw_h, vw_pg
  vs.x(vw_id) = vw_x
  vs.y(vw_id) = vw_y
  vs.ox(vw_id) = vw_ox
  vs.oy(vw_id) = vw_oy
  vs.ax(vw_id) = vw_x + vw_ox
  vs.ay(vw_id) = vw_y + vw_oy
  vs.w(vw_id) = vw_w
  vs.h(vw_id) = vw_h
  sprite show vw_id, vs.ax(vw_id), vs.ay(vw_id), vs.lay(vw_id)
  ' one dirty rect for the old and the new frame (same cell, or a short move)
  if vw_u then
    vw_ux = min(vw_ux, vs.ax(vw_id))
    vw_uy = min(vw_uy, vs.ay(vw_id))
    vw_ux2 = max(vw_ux2, vs.ax(vw_id) + vw_w)
    vw_uy2 = max(vw_uy2, vs.ay(vw_id) + vw_h)
    scr.mark(vw_ux, vw_uy, vw_ux2 - vw_ux, vw_uy2 - vw_uy)
  else
    scr.mark(vs.ax(vw_id), vs.ay(vw_id), vw_w, vw_h)
  end if
end sub

' sprite vw_id moves to vw_x, vw_y (same frame): one dirty rect covering the old
' and the new place (they overlap: a few px per tick)
sub vw.markMove()
  vw_ox = vs.ax(vw_id)
  vw_oy = vs.ay(vw_id)
  vs.ax(vw_id) = vw_x + vs.ox(vw_id)
  vs.ay(vw_id) = vw_y + vs.oy(vw_id)
  vs.x(vw_id) = vw_x
  vs.y(vw_id) = vw_y
  vw_sx = min(vw_ox, vs.ax(vw_id))
  vw_sy = min(vw_oy, vs.ay(vw_id))
  scr.mark(vw_sx, vw_sy, max(vw_ox, vs.ax(vw_id)) - vw_sx + vs.w(vw_id), max(vw_oy, vs.ay(vw_id)) - vw_sy + vs.h(vw_id))
end sub

' Many frame changes in one frame: each SHOW under the plates would re-stack the
' whole pile. Instead take every sprite off top-down (each hide is the top one),
' then vw.update shows them again bottom-up (each show is again the top one).
sub vw.batchHide()
  for vw_i = SPR_PUFF to SPR_PL step -1
    if vs.fr(vw_i) >= 0 then
      sprite hide vw_i
      vs.hid(vw_i) = 1
    end if
  next vw_i
  if vs.fr(SPR_COOK) >= 0 then
    sprite hide SPR_COOK
    vs.hid(SPR_COOK) = 1
  end if
  for vw_i = SPR_CU + NC - 1 to SPR_CU step -1
    if vs.fr(vw_i) >= 0 then
      sprite hide vw_i
      vs.hid(vw_i) = 1
    end if
  next vw_i
  vw.nmov = 0
end sub

' the same with arguments (dialogs, the life-lost scene; not the hot path)
sub vw.set(id, f, x, y)
  vw_id = id
  vw_f = f
  vw_x = x
  vw_y = y
  if vw_f = vs.fr(vw_id) and vw_x = vs.x(vw_id) and vw_y = vs.y(vw_id) then exit sub
  vw.put()
end sub

' send the queued SPRITE NEXT moves: one SPRITE MOVE for many, or a single
' sprite's own move
sub vw.commit()
  if vw.nmov = 1 then
    sprite move vw.lastMov, vs.ax(vw.lastMov), vs.ay(vw.lastMov)
  elseif vw.nmov > 1 then
    sprite move
  end if
  vw.nmov = 0
end sub

' ---- engine units -> screen px
function vw.cuX(x)
  vw.cuX = (2 + CC_X0 + x * vw.cm \ 960) * scr.k
end function
function vw.plX(x)
  vw.plX = (lay.cx0 + x * vw.pm \ 1024) * scr.k
end function
function vw.ckX(x)
  vw.ckX = (lay.cx1 - 2 - (HOME - x) * vw.km \ 1024) * scr.k
end function
function vw.topY(l)
  vw.topY = vw.ty(l)
end function

' bring every sprite up to date with the engine (main loop, tick waiting).
' Inline arithmetic and an early "unchanged" test per slot: this runs every frame.
sub vw.update()
  vw_k = scr.k
  ' customer frames this tick: how many change?
  vw_n = 0
  for vw_l = 0 to NC - 1
    vw_id = SPR_CU + vw_l
    if cu.on(vw_l) then
      vw_f = cu.t(vw_l) * CF_N
      if cu.s(vw_l) = CS_SLIDE then
        vw_f = vw_f + CF_CATCH
      elseif cu.s(vw_l) = CS_EAT then
        vw_f = vw_f + CF_EAT + (cu.an(vw_l) \ 12) mod 2
      elseif cu.s(vw_l) = CS_LEAVE then
        vw_f = vw_f + CF_EAT + 1
      else
        vw_f = vw_f + CF_WALK + (cu.an(vw_l) \ 6) mod 4
      end if
      vw.cf(vw_l) = vw_f
      if vw_f <> vs.fr(vw_id) then vw_n = vw_n + 1
    end if
  next vw_l
  if vw_n >= VW_BATCH then vw.batchHide()
  ' customers
  for vw_l = 0 to NC - 1
    vw_id = SPR_CU + vw_l
    if cu.on(vw_l) then
      vw_f = vw.cf(vw_l)
      vw_x = (2 + CC_X0 + cu.x(vw_l) * vw.cm \ 960) * vw_k
      vw_y = vw.ty(cu.l(vw_l))
      if vw_f <> vs.fr(vw_id) or vw_x <> vs.x(vw_id) or vw_y <> vs.y(vw_id) or vs.hid(vw_id) then vw.put()
    elseif vs.fr(vw_id) >= 0 then
      vw_f = -1
      vw.put()
    end if
  next vw_l
  ' the cook
  if en.show > 0 then
    vw_f = FR_COOK + KF_SPIN0 + (en.tk \ 6) mod 2
  elseif ck.jn > 0 then
    if ck.jd = 1 then
      vw_f = FR_COOK + KF_UP0 + (ck.jn <= 2)
    else
      vw_f = FR_COOK + KF_DOWN0 + (ck.jn <= 2)
    end if
  elseif ck.sv > 0 then
    vw_f = FR_COOK + KF_SERVE0 + (ck.sv <= SERVE_AT)
  elseif ck.cn > 0 then
    vw_f = FR_COOK + KF_CATCH
  elseif ck.wn > 0 then
    vw_f = FR_COOK + KF_WALK0 + (ck.x \ 16) mod 4
  else
    vw_f = FR_COOK + KF_IDLE
  end if
  vw_x = (lay.cx1 - 2 - (HOME - ck.x) * vw.km \ 1024) * vw_k
  vw_y = vw.ty(ck.l)
  vw_id = SPR_COOK
  if vw_f <> vs.fr(vw_id) or vw_x <> vs.x(vw_id) or vw_y <> vs.y(vw_id) or vs.hid(vw_id) then vw.put()
  ' plates
  for vw_l = 0 to NP - 1
    vw_id = SPR_PL + vw_l
    if pl.on(vw_l) then
      vw_x = (lay.cx0 + pl.x(vw_l) * vw.pm \ 1024) * vw_k
      if pl.f(vw_l) then
        vw_f = FR_ITEM + IT_PF
        vw_y = vw.pfy(pl.l(vw_l))
      else
        vw_f = FR_ITEM + IT_PE
        vw_y = vw.pey(pl.l(vw_l))
      end if
      if vw_f <> vs.fr(vw_id) or vw_x <> vs.x(vw_id) or vw_y <> vs.y(vw_id) or vs.hid(vw_id) then vw.put()
    elseif vs.fr(vw_id) >= 0 then
      vw_f = -1
      vw.put()
    end if
  next vw_l
  ' tips (one about to go blinks)
  for vw_l = 0 to NO - 1
    vw_id = SPR_CO + vw_l
    vw_f = -1
    if co.on(vw_l) then
      if co.n(vw_l) > 100 or (co.n(vw_l) \ 8) mod 2 = 0 then vw_f = FR_ITEM + IT_CO
    end if
    if vw_f >= 0 then
      vw_x = (lay.cx0 + co.x(vw_l) * vw.pm \ 1024 + 5) * vw_k
      vw_y = vw.coy(co.l(vw_l))
      if vw_f <> vs.fr(vw_id) or vw_x <> vs.x(vw_id) or vw_y <> vs.y(vw_id) or vs.hid(vw_id) then vw.put()
    elseif vs.fr(vw_id) >= 0 then
      vw.put()
    end if
  next vw_l
  vw.commit()
end sub

' ---- the life-lost scene: frame k of the animation (the board is frozen)
sub vw.lostFrame(k)
  vw.set(SPR_COOK, FR_COOK + KF_DIZZY0 + (k \ 8) mod 2, vw.ckX(ck.x), vw.ty(ck.l))
  select case en.lost
    case L_REACH
      vw_i = en.li
      vw.set(SPR_CU + vw_i, cu.t(vw_i) * CF_N + CF_ANGRY, vw.cuX(cu.x(vw_i)), vw.ty(cu.l(vw_i)) - ((k \ 6) mod 2) * scr.k)
    case L_FULL
      vw.set(SPR_SMASH, FR_ITEM + IT_SM, (lay.cx0 - 22) * scr.k, (lay.yc(en.ll) + 3 - ((k \ 8) mod 2)) * scr.k)
      vw.set(SPR_PUFF, FR_ITEM + IT_PU, (lay.cx0 - 18 + (k \ 4) mod 3) * scr.k, (lay.yc(en.ll) - 4 - (k \ 6)) * scr.k)
    case L_EMPTY
      vw.set(SPR_SMASH, FR_ITEM + IT_SM, (lay.artw - 26) * scr.k, (lay.yc(en.ll) + 3 - ((k \ 8) mod 2)) * scr.k)
      vw.set(SPR_PUFF, FR_ITEM + IT_PU, (lay.artw - 20 + (k \ 4) mod 3) * scr.k, (lay.yc(en.ll) - 4 - (k \ 6)) * scr.k)
  end select
  vw.commit()
end sub
