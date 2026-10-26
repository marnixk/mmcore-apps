' view.inc - the scrolling board view (left part of the screen, lay.vw x
' lay.vh px at 0,0) drawn on the back page PG_BACK, never on page 0.
'
' The camera keeps the player's cell in the middle (clamped at the board's
' edges); it moves a whole cell at a time. Per frame (vw.update):
'  - camera did not move: redraw just the cells the engine queued (dq) and
'    copy each of them to the screen (or the whole view when there are many)
'  - camera moved: copy the part of the view that stays from page 0 to its
'    new place on PG_BACK (one BLIT, no overlap: different pages), draw the
'    cells of the newly exposed strips and the queued cells, then copy the
'    view to the screen in one BLIT
' Cells at the edge of the view are clipped to it. Animated floors (coolant,
' fire, conveyors, teleports, the exit) step one frame every ANIM_MS; only
' the visible ones are redrawn, at most ANIM_CHUNK per frame (a screen full
' of coolant is spread over a few frames, so the tick never waits long).
const ANIM_MS = 400, FLUSH_MAX = 40, ANIM_CHUNK = 24
dim vw.ox, vw.oy        ' board px at the view's top left corner
dim vw.af, vw.anext     ' animation frame and when the next one is due
dim vw.ts, vw.markOn = 1
dim vw.tt0(33), vw.tn(33), vw.ot(10)   ' terrain: first tile and frame count; object tiles
dim vw.dk(4)                            ' cosmetic looks (ld): via, trace across, trace down, bare board
dim an(LN - 1), ann, anr(LW)            ' animated cells (reading order), first one per row
dim vw.ak, vw.aend                      ' animation beat in progress: next index into an(), end
dim integer vw_x, vw_y, vw_cx, vw_cy, vw_cw, vw_ch, vw_k, vw_c, vw_i, vw_n, vw_m, vw_mm
dim integer vw_tx, vw_ty, vw_dx, vw_dy, vw_r, vw_q, vw_c0, vw_c1, vw_r0, vw_r1

sub vw.init()
  local i
  vw.ts = lay.ts
  for i = 0 to 33
    vw.tn(i) = 1
  next i
  vw.tt0(T_FLOOR) = TL_FLOOR : vw.tt0(T_WALL) = TL_WALL : vw.tt0(T_HIDDEN) = TL_FLOOR
  vw.tt0(T_HIDDEN_R) = TL_WALL_HIDDEN : vw.tt0(T_FAKE) = TL_WALL_FAKE
  vw.tt0(T_TOG_C) = TL_TOGGLE_CLOSED : vw.tt0(T_TOG_O) = TL_TOGGLE_OPEN : vw.tt0(T_SOCKET) = TL_SOCKET
  vw.tt0(T_EXIT) = TL_EXIT_0 : vw.tn(T_EXIT) = 3
  vw.tt0(T_DIRT) = TL_DIRT : vw.tt0(T_GRAVEL) = TL_GRAVEL : vw.tt0(T_HINT) = TL_HINT
  for i = 0 to 3
    vw.tt0(T_DOOR_R + i) = TL_DOOR_RED + i
    vw.tt0(T_FORCE_U + i) = TL_FORCE_U_0 + 2 * i
    vw.tn(T_FORCE_U + i) = 2
    vw.tt0(T_ICE_NW + i) = TL_ICE_NW + i
    vw.ot(O_KEY_R + i) = TL_KEY_RED + i
    vw.ot(O_BOOT_W + i) = TL_BOOT_FLIPPERS + i
  next i
  vw.tt0(T_WATER) = TL_WATER_0 : vw.tn(T_WATER) = 2
  vw.tt0(T_FIRE) = TL_FIRE_0 : vw.tn(T_FIRE) = 2
  vw.tt0(T_ICE) = TL_ICE
  vw.tt0(T_BTN_G) = TL_BTN_GREEN : vw.tt0(T_BTN_B) = TL_BTN_BLUE : vw.tt0(T_BTN_BR) = TL_BTN_BROWN
  vw.tt0(T_BOMB) = TL_BOMB : vw.tt0(T_TRAP) = TL_TRAP : vw.tt0(T_THIEF) = TL_THIEF
  vw.tt0(T_TELE) = TL_TELEPORT_0 : vw.tn(T_TELE) = 2
  vw.ot(O_CHIP) = TL_CHIP : vw.ot(O_BLOCK) = TL_BLOCK
  vw.dk(1) = TL_FLOOR_VIA : vw.dk(2) = TL_FLOOR_TRACE_H : vw.dk(3) = TL_FLOOR_TRACE_V : vw.dk(4) = TL_BOARD
end sub

' a level (re)starts: animated cells, camera on the player, whole view on PG_BACK
sub vw.start()
  local i
  ann = 0
  for i = 0 to LN - 1
    if (i and 31) = 0 then anr(i \ LW) = ann
    if vw.tn(lt(i)) > 1 then
      an(ann) = i
      ann = ann + 1
    end if
  next i
  anr(LW) = ann
  vw.ak = 0
  vw.aend = 0
  for i = 0 to dqn - 1
    dm(dq(i)) = 0
  next i
  dqn = 0
  vw.target()
  vw.ox = vw_tx
  vw.oy = vw_ty
  vw.anext = timer + ANIM_MS
  vw.all()
end sub

' camera target for the player's cell
sub vw.target()
  vw_tx = (en.pi and 31) * vw.ts + vw.ts \ 2 - lay.vw \ 2
  if vw_tx > LW * vw.ts - lay.vw then vw_tx = LW * vw.ts - lay.vw
  if vw_tx < 0 then vw_tx = 0
  vw_ty = (en.pi \ LW) * vw.ts + vw.ts \ 2 - lay.vh \ 2
  if vw_ty > LW * vw.ts - lay.vh then vw_ty = LW * vw.ts - lay.vh
  if vw_ty < 0 then vw_ty = 0
end sub

' visible cell range: columns vw_c0..vw_c1, rows vw_r0..vw_r1
sub vw.range()
  vw_c0 = vw.ox \ vw.ts
  vw_c1 = (vw.ox + lay.vw - 1) \ vw.ts
  vw_r0 = vw.oy \ vw.ts
  vw_r1 = (vw.oy + lay.vh - 1) \ vw.ts
end sub

' every visible cell on PG_BACK (not copied to the screen)
sub vw.all()
  page write PG_BACK
  vw.markOn = 0
  vw.range()
  for vw_r = vw_r0 to vw_r1
    for vw_q = vw_c0 to vw_c1
      vw.cell(vw_r * LW + vw_q)
    next vw_q
  next vw_r
  vw.markOn = 1
end sub

' draw cell i on PG_BACK, clipped to the view; vw.markOn: also note it for scr.flush
sub vw.cell(i)
  vw_x = (i and 31) * vw.ts - vw.ox
  if vw_x <= -vw.ts or vw_x >= lay.vw then exit sub
  vw_y = (i \ LW) * vw.ts - vw.oy
  if vw_y <= -vw.ts or vw_y >= lay.vh then exit sub
  vw_cx = 0
  vw_cw = vw.ts
  if vw_x < 0 then
    vw_cx = -vw_x
    vw_cw = vw.ts + vw_x
  elseif vw_x + vw.ts > lay.vw then
    vw_cw = lay.vw - vw_x
  end if
  vw_cy = 0
  vw_ch = vw.ts
  if vw_y < 0 then
    vw_cy = -vw_y
    vw_ch = vw.ts + vw_y
  elseif vw_y + vw.ts > lay.vh then
    vw_ch = lay.vh - vw_y
  end if
  vw_c = lt(i)
  vw_k = vw.tt0(vw_c)
  if vw.tn(vw_c) > 1 then vw_k = vw_k + vw.af mod vw.tn(vw_c)
  if ld(i) then vw_k = vw.dk(ld(i))
  blit (vw_k and 15) * vw.ts + vw_cx, (vw_k \ 16) * vw.ts + vw_cy, vw_x + vw_cx, vw_y + vw_cy, vw_cw, vw_ch, PG_SHEET
  if lo(i) then
    vw_k = vw.ot(lo(i))
    vw.spr()
  end if
  if i = en.pi then
    vw_k = TL_PL_U_0 + en.pd * 2 + en.pf
    if en.dead = D_DROWN then vw_k = TL_PL_DROWN
    if en.dead = D_BURN or en.dead = D_BOMB then vw_k = TL_PL_BURN
    vw.spr()
  end if
  if lm(i) then
    vw_mm = lm(i) - 1
    vw_k = TL_BUG_U_0 + mk(vw_mm) * 8 + md(vw_mm) * 2 + mf(vw_mm)
    vw.spr()
  end if
  if vw.markOn then scr.mark(vw_x + vw_cx, vw_y + vw_cy, vw_cw, vw_ch)
end sub

' a see-through tile vw_k over the cell being drawn
sub vw.spr()
  blit (vw_k and 15) * vw.ts + vw_cx, (vw_k \ 16) * vw.ts + vw_cy, vw_x + vw_cx, vw_y + vw_cy, vw_cw, vw_ch, PG_SHEET, 4
end sub

' the queued cells (and nothing else)
sub vw.dirty()
  page write PG_BACK
  vw.markOn = (dqn <= FLUSH_MAX)
  for vw_n = 0 to dqn - 1
    vw_m = dq(vw_n)
    dm(vw_m) = 0
    vw.cell(vw_m)
  next vw_n
  if vw.markOn = 0 then scr.mark(0, 0, lay.vw, lay.vh)
  dqn = 0
  vw.markOn = 1
end sub

' all visible cells of board px columns xa .. xb - 1
sub vw.cols(xa, xb)
  for vw_q = xa \ vw.ts to (xb - 1) \ vw.ts
    for vw_r = vw_r0 to vw_r1
      vw.cell(vw_r * LW + vw_q)
    next vw_r
  next vw_q
end sub

' all visible cells of board px rows ya .. yb - 1
sub vw.rows(ya, yb)
  for vw_r = ya \ vw.ts to (yb - 1) \ vw.ts
    for vw_q = vw_c0 to vw_c1
      vw.cell(vw_r * LW + vw_q)
    next vw_q
  next vw_r
end sub

' bring the view up to date on PG_BACK and note what to copy to the screen
sub vw.update()
  vw.target()
  vw_dx = vw_tx - vw.ox
  vw_dy = vw_ty - vw.oy
  if vw_dx = 0 and vw_dy = 0 then
    vw.dirty()
    exit sub
  end if
  page write PG_BACK
  if abs(vw_dx) >= lay.vw or abs(vw_dy) >= lay.vh then
    vw.ox = vw_tx
    vw.oy = vw_ty
    vw.all()
  else
    blit max(0, vw_dx), max(0, vw_dy), max(0, -vw_dx), max(0, -vw_dy), lay.vw - abs(vw_dx), lay.vh - abs(vw_dy), 0
    vw.ox = vw_tx
    vw.oy = vw_ty
    vw.range()
    vw.markOn = 0
    if vw_dx > 0 then vw.cols(vw.ox + lay.vw - vw_dx, vw.ox + lay.vw)
    if vw_dx < 0 then vw.cols(vw.ox, vw.ox - vw_dx)
    if vw_dy > 0 then vw.rows(vw.oy + lay.vh - vw_dy, vw.oy + lay.vh)
    if vw_dy < 0 then vw.rows(vw.oy, vw.oy - vw_dy)
  end if
  vw.markOn = 0
  for vw_n = 0 to dqn - 1
    vw_m = dq(vw_n)
    dm(vw_m) = 0
    vw.cell(vw_m)
  next vw_n
  dqn = 0
  vw.markOn = 1
  scr.mark(0, 0, lay.vw, lay.vh)
end sub

' next animation frame: from now on queue the visible animated cells, a
' chunk per frame (vw.animStep)
sub vw.anim()
  vw.af = vw.af + 1
  vw.anext = vw.anext + ANIM_MS
  if vw.anext <= timer then vw.anext = timer + ANIM_MS
  vw.range()
  vw.ak = anr(vw_r0)
  vw.aend = anr(vw_r1 + 1)
end sub

sub vw.animStep()
  vw_n = 0
  do while vw.ak < vw.aend and vw_n < ANIM_CHUNK
    vw_i = an(vw.ak)
    vw.ak = vw.ak + 1
    vw_q = vw_i and 31
    if vw_q >= vw_c0 and vw_q <= vw_c1 then
      en.mark(vw_i)
      vw_n = vw_n + 1
    end if
  loop
end sub
