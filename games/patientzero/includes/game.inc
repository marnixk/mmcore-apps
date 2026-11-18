' game.inc - Jerry, the play loop, dirty-rect drawing.
' Positions are base pixels in 256ths. One full page copy only after a scene change.

dim gm.score, gm.cured, gm.lcured, gm.level, gm.round
dim gm.won, gm.leave, gm.over, gm.frame, gm.opened
dim pl.x, pl.y, pl.vx, pl.vy, pl.face, pl.frm
dim pl.lives, pl.flems, pl.flem, pl.over, pl.chill
dim pl.charging, pl.sneeze, pl.reach, pl.shield
dim pl.lx, pl.ly, pl.dropT, pl.showH, pl.inP, pl.refilled, pl.spawnT
dim pl.ox, pl.oy, pl.ow, pl.oh
dim pl.mox, pl.moy, pl.mow, pl.moh
dim pl.dox, pl.doy, pl.dow, pl.doh
dim pl.hox, pl.hoy, pl.how, pl.hoh
dim pl_sid, pl_nx, pl_ny, pl_k, pl_n
dim stx, sty, stw, sth
dim gx0, gy0, gx1, gy1
dim gm_t0, gm_dt, gm_acc, gm_mx

sub gm.stamp(id, bx, by)
  if sth > 0 then spr.back stx, sty, stw, sth
  if id < 0 then
    if sth > 0 then scr.mark stx, sty, stw, sth
    sth = 0
    exit sub
  end if
  spr.at id, bx, by
  if drw < 1 then
    if sth > 0 then scr.mark stx, sty, stw, sth
    sth = 0
    exit sub
  end if
  if sth < 1 then
    scr.mark drx, dry, drw, drh
    stx = drx
    sty = dry
    stw = drw
    sth = drh
    exit sub
  end if
  if stx < drx then
    gx0 = stx
  else
    gx0 = drx
  end if
  if sty < dry then
    gy0 = sty
  else
    gy0 = dry
  end if
  if stx + stw > drx + drw then
    gx1 = stx + stw
  else
    gx1 = drx + drw
  end if
  if sty + sth > dry + drh then
    gy1 = sty + sth
  else
    gy1 = dry + drh
  end if
  scr.mark gx0, gy0, gx1 - gx0, gy1 - gy0
  stx = drx
  sty = dry
  stw = drw
  sth = drh
end sub

sub gm.markUnion(nx, ny, nw, nh)
  if sth < 1 then
    scr.mark nx, ny, nw, nh
  else
    if stx < nx then
      gx0 = stx
    else
      gx0 = nx
    end if
    if sty < ny then
      gy0 = sty
    else
      gy0 = ny
    end if
    if stx + stw > nx + nw then
      gx1 = stx + stw
    else
      gx1 = nx + nw
    end if
    if sty + sth > ny + nh then
      gy1 = sty + sth
    else
      gy1 = ny + nh
    end if
    scr.mark gx0, gy0, gx1 - gx0, gy1 - gy0
  end if
  stx = nx
  sty = ny
  stw = nw
  sth = nh
end sub

sub pl.boot()
  pl.lives = 5
  if TH_LIVES > 0 then pl.lives = TH_LIVES
  pl.flems = 5
  gm.score = 0
  gm.cured = 0
  gm.round = 0
  gm.level = 0
  gm.leave = 0
end sub

sub pl.reset()
  pl.x = lv.px * 256
  pl.y = lv.py * 256
  pl.vx = 0
  pl.vy = 0
  pl.face = 0
  pl.frm = 0
  pl.flem = 0
  pl.over = 0
  pl.chill = 0
  pl.charging = 0
  pl.sneeze = 0
  pl.reach = 0
  pl.shield = 0
  pl.lx = 0
  pl.ly = 1
  pl.flems = 5
  pl.dropT = 0
  pl.showH = 0
  pl.inP = 0
  pl.refilled = 0
  pl.spawnT = lv.spawnEvery
  pl.ow = 0
  pl.mow = 0
  pl.dow = 0
  pl.how = 0
  gm.lcured = 0
  gm.frame = 0
  gm.won = 0
  gm.over = 0
  gm.opened = 0
  key.reset
end sub

sub gm.hurt(drag)
  if pl.shield > 0 then
    exit sub
  end if
  pl.lives = pl.lives - 1
  pl.shield = 100
  pl.showH = 90
  if drag = 1 then
    pl_k = pl.x \ 256
    pl_n = pl.y \ 256
    pl_nx = pl.x
    pl_ny = pl.y
    gx0 = pl_k
    gx1 = 0
    if (639 - pl_k) < gx0 then
      gx0 = 639 - pl_k
      gx1 = 1
    end if
    if pl_n < gx0 then
      gx0 = pl_n
      gx1 = 2
    end if
    if (359 - pl_n) < gx0 then gx1 = 3
    if gx1 = 0 then pl_nx = pl.x - 40 * 256
    if gx1 = 1 then pl_nx = pl.x + 40 * 256
    if gx1 = 2 then pl_ny = pl.y - 40 * 256
    if gx1 = 3 then pl_ny = pl.y + 40 * 256
    if lv.blocked(pl_nx \ 256, pl_ny \ 256) = 0 then
      pl.x = pl_nx
      pl.y = pl_ny
    end if
  end if
  if pl.lives < 1 then gm.over = 1
end sub

sub pl.tick()
  pl.vx = 0
  pl.vy = 0
  if key.left = 1 then pl.vx = 0 - SP_JERRY
  if key.right = 1 then pl.vx = SP_JERRY
  if key.up = 1 then pl.vy = 0 - SP_JERRY
  if key.dn = 1 then pl.vy = SP_JERRY
  pl_nx = pl.x + pl.vx
  if lv.blocked(pl_nx \ 256, pl.y \ 256) = 0 then pl.x = pl_nx
  pl_ny = pl.y + pl.vy
  if lv.blocked(pl.x \ 256, pl_ny \ 256) = 0 then pl.y = pl_ny
  if pl.vx <> 0 or pl.vy <> 0 then
    if pl.vx < 0 then
      pl_k = 0 - pl.vx
    else
      pl_k = pl.vx
    end if
    if pl.vy < 0 then
      pl_n = 0 - pl.vy
    else
      pl_n = pl.vy
    end if
    if pl_k > pl_n then
      if pl.vx > 0 then
        pl.face = 2
      else
        pl.face = 3
      end if
    else
      if pl.vy > 0 then
        pl.face = 0
      else
        pl.face = 1
      end if
    end if
    pl.lx = 0
    pl.ly = 0
    if pl.vx > 0 then pl.lx = 1
    if pl.vx < 0 then pl.lx = -1
    if pl.vy > 0 then pl.ly = 1
    if pl.vy < 0 then pl.ly = -1
    if gm.frame = (gm.frame \ 6) * 6 then pl.frm = (pl.frm + 1) mod 4
  end if
  if key.z = 1 and pl.flems > 0 and pl.sneeze = 0 then
    pl.charging = 1
    if pl.over > 10 then
      pl.flem = pl.flem - 8
      if pl.flem < 0 then pl.flem = 0
    else
      if pl.flem < 100 then
        pl.flem = pl.flem + 1
      else
        pl.over = pl.over + 1
      end if
    end if
  else
    pl.charging = 0
    if pl.flem > 0 then
      pl.flem = pl.flem - 5
      if pl.flem < 0 then pl.flem = 0
    end if
    if pl.over > 0 then pl.over = pl.over - 1
    if pl.flem = 0 then pl.over = 0
  end if
  if key.sneeze = 1 and pl.flems > 0 and pl.sneeze = 0 then
    pl.flems = pl.flems - 1
    pl.reach = (300 * pl.flem) \ 100
    if pl.reach < 10 then pl.reach = 10
    pl.reach = (pl.reach * 9) \ 16
    pl.sneeze = 36
    pl.dropT = 90
    pt.sneeze
  end if
  key.sneeze = FALSE
  if pl.sneeze > 0 then pl.sneeze = pl.sneeze - 1
  if pd.under() = 1 then
    pl.inP = 1
    pl.chill = pl.chill + 6
    if pl.chill >= 1000 then
      if pl.flems < 5 then
        pl.flems = pl.flems + 1
        pl.chill = 0
        pl.refilled = 1
      else
        pl.chill = 1000
      end if
    end if
  else
    pl.inP = 0
    pl.chill = pl.chill - 3
    if pl.chill < 0 then pl.chill = 0
  end if
  if pl.dropT > 0 then pl.dropT = pl.dropT - 1
  if pl.showH > 0 then pl.showH = pl.showH - 1
  if pl.shield > 0 then pl.shield = pl.shield - 1
  if TH_SNEEZE > 0 and gm.frame >= TH_SNEEZE - 1 then
    if pl.sneeze = 0 or (gm.frame mod 18) = 0 then
      pl.flem = 80
      pl.face = 2
      pl.dropT = 90
      pt.sneeze
    end if
    if gm.frame <= TH_SNEEZE + 2 then
      pl.reach = 70
    else
      pl.reach = 0
    end if
    if pl.sneeze < 14 then pl.sneeze = 22
  end if
end sub

sub gm.rebuild()
  page copy PG_FIELD to PG_BACK
  page write PG_BACK
  pl.ow = 0
  pl.mow = 0
  pl.dow = 0
  pl.how = 0
  for en_i = 0 to EN_LAST
    en.ow(en_i) = 0
  next en_i
  for pt_i = 0 to PT_LAST
    part(pt_i).ow = 0
  next pt_i
  for pd_i = 0 to PD_LAST
    pool(pd_i).ow = 0
  next pd_i
  tr.ow(0) = 0
  tr.ow(1) = 0
  tr.ww(0) = 0
  tr.ww(1) = 0
  for en_j = 0 to DOOR_LAST
    door.ow(en_j) = 0
  next en_j
  scr.markAll
end sub

sub pd.draw()
  for pd_i = 0 to pd.n - 1
    stx = pool(pd_i).ox
    sty = pool(pd_i).oy
    stw = pool(pd_i).ow
    sth = pool(pd_i).oh
    if pool(pd_i).alive = 1 then
      gm.stamp pd.fid(pd_i), pool(pd_i).x, pool(pd_i).y
    else
      gm.stamp -1, 0, 0
    end if
    pool(pd_i).ox = stx
    pool(pd_i).oy = sty
    pool(pd_i).ow = stw
    pool(pd_i).oh = sth
  next pd_i
end sub

sub pt.draw()
  for pt_i = 0 to PT_LAST
    stx = part(pt_i).ox
    sty = part(pt_i).oy
    stw = part(pt_i).ow
    sth = part(pt_i).oh
    if part(pt_i).alive = 1 then
      gm.stamp part(pt_i).kind, part(pt_i).x \ 256, part(pt_i).y \ 256
    else
      gm.stamp -1, 0, 0
    end if
    part(pt_i).ox = stx
    part(pt_i).oy = sty
    part(pt_i).ow = stw
    part(pt_i).oh = sth
  next pt_i
end sub

sub en.drawOne(i)
  stx = en.ox(i)
  sty = en.oy(i)
  stw = en.ow(i)
  sth = en.oh(i)
  if zed(i).alive = 0 then
    gm.stamp -1, 0, 0
  else
    en_id = i
    en.picture
    if zed(i).st = ST_ANGEL then
      if sth > 0 then spr.back stx, sty, stw, sth
      gx0 = 0
      gy0 = 0
      gx1 = 0
      gy1 = 0
      spr.at F_BEAM, (zed(i).x \ 256) - 8, (zed(i).y \ 256) + 14
      if drw > 0 then
        gx0 = drx
        gy0 = dry
        gx1 = drx + drw
        gy1 = dry + drh
      end if
      spr.at en_sid, zed(i).x \ 256, (zed(i).y \ 256) - 2
      if drw > 0 then
        if gx1 <= gx0 then
          gx0 = drx
          gy0 = dry
          gx1 = drx + drw
          gy1 = dry + drh
        else
          if drx < gx0 then gx0 = drx
          if dry < gy0 then gy0 = dry
          if drx + drw > gx1 then gx1 = drx + drw
          if dry + drh > gy1 then gy1 = dry + drh
        end if
      end if
      if gx1 > gx0 then
        gm.markUnion gx0, gy0, gx1 - gx0, gy1 - gy0
      else
        gm.stamp -1, 0, 0
      end if
    else
      gm.stamp en_sid, zed(i).x \ 256, zed(i).y \ 256
    end if
  end if
  en.ox(i) = stx
  en.oy(i) = sty
  en.ow(i) = stw
  en.oh(i) = sth
end sub

sub en.draw()
  for en_i = 0 to EN_LAST
    en.drawOne en_i
  next en_i
end sub

sub tr.draw()
  if tr.on = 0 then
    exit sub
  end if
  for tr_i = 0 to 1
    stx = tr.ox(tr_i)
    sty = tr.oy(tr_i)
    stw = tr.ow(tr_i)
    sth = tr.oh(tr_i)
    if tr.st(tr_i) = 2 then
      gm.stamp F_TRAIN, tr.x(tr_i) \ 256, tr.y(tr_i)
    else
      gm.stamp -1, 0, 0
    end if
    tr.ox(tr_i) = stx
    tr.oy(tr_i) = sty
    tr.ow(tr_i) = stw
    tr.oh(tr_i) = sth
    if tr.ww(tr_i) > 0 then
      spr.back tr.wx(tr_i), tr.wy(tr_i), tr.ww(tr_i), tr.wh(tr_i)
      scr.mark tr.wx(tr_i), tr.wy(tr_i), tr.ww(tr_i), tr.wh(tr_i)
      tr.ww(tr_i) = 0
    end if
    if tr.st(tr_i) = 1 and ((tr.t(tr_i) \ 8) mod 2) = 0 then
      gx0 = 20 * scr.sc
      if tr.vx(tr_i) < 0 then gx0 = (BASE_W - 60) * scr.sc
      gy0 = (tr.y(tr_i) - 18) * scr.sc
      box gx0, gy0, 40 * scr.sc, 6 * scr.sc, 1, c.warn, c.warn
      tr.wx(tr_i) = gx0
      tr.wy(tr_i) = gy0
      tr.ww(tr_i) = 40 * scr.sc
      tr.wh(tr_i) = 6 * scr.sc
      scr.mark gx0, gy0, tr.ww(tr_i), tr.wh(tr_i)
    end if
  next tr_i
end sub

sub door.draw()
  if lv.themeNow <> 2 then
    exit sub
  end if
  for en_j = 0 to door.n - 1
    if door.ow(en_j) > 0 then
      spr.back door.ox(en_j), door.oy(en_j), door.ow(en_j), door.oh(en_j)
      scr.mark door.ox(en_j), door.oy(en_j), door.ow(en_j), door.oh(en_j)
      door.ow(en_j) = 0
    end if
    if door.cool(en_j) > 0 and ((gm.frame \ 4) mod 2) = 0 then
      gx0 = door.tx(en_j) * TILE * scr.sc
      gy0 = door.ty(en_j) * TILE * scr.sc
      box gx0, gy0, TILE * scr.sc, 4 * scr.sc, 1, c.warn, c.warn
      door.ox(en_j) = gx0
      door.oy(en_j) = gy0
      door.ow(en_j) = TILE * scr.sc
      door.oh(en_j) = 4 * scr.sc
      scr.mark gx0, gy0, door.ow(en_j), door.oh(en_j)
    end if
  next en_j
end sub

sub pl.sprite()
  if pl.sneeze > 0 then
    if pl.face = 3 then
      pl_sid = S_JERRY_SNEEZE_L
    elseif pl.face = 2 then
      pl_sid = S_JERRY_SNEEZE_R
    else
      pl_sid = S_JERRY_SNEEZE + (gm.frame mod 2)
    end if
    exit sub
  end if
  if pl.showH > 80 then
    pl_sid = S_JERRY_HURT
    exit sub
  end if
  if pl.charging = 1 then
    pl_sid = S_JERRY_CHARGE
    if pl.flem >= 60 then pl_sid = S_JERRY_CHARGE + 1
    exit sub
  end if
  if pl.vx = 0 and pl.vy = 0 then
    pl_sid = S_JERRY_IDLE + ((gm.frame \ 16) mod 2)
    exit sub
  end if
  pl_sid = S_JERRY + pl.face * 4 + (pl.frm mod 4)
end sub

sub pl.drawBody()
  stx = pl.ox
  sty = pl.oy
  stw = pl.ow
  sth = pl.oh
  if pl.shield > 0 and (gm.frame mod 2) = 0 then
    gm.stamp -1, 0, 0
  else
    pl.sprite
    gm.stamp pl_sid, pl.x \ 256, pl.y \ 256
  end if
  pl.ox = stx
  pl.oy = sty
  pl.ow = stw
  pl.oh = sth
end sub

sub pl.drawMeter()
  stx = pl.mox
  sty = pl.moy
  stw = pl.mow
  sth = pl.moh
  if stw > 0 then spr.back stx, sty, stw, sth
  if pl.charging = 0 then
    if stw > 0 then scr.mark stx, sty, stw, sth
    pl.mow = 0
    exit sub
  end if
  gx0 = (pl.x \ 256) - 1
  gy0 = (pl.y \ 256) - 10
  if gy0 < 0 then gy0 = 0
  if pl.over > 10 then
    pl_sid = F_METER_OVER
  else
    pl_sid = F_METER + (pl.flem \ 10)
  end if
  spr.at pl_sid, gx0, gy0
  if drw < 1 then
    if stw > 0 then scr.mark stx, sty, stw, sth
    pl.mow = 0
    exit sub
  end if
  gm.markUnion drx, dry, drw, drh
  pl.mox = stx
  pl.moy = sty
  pl.mow = stw
  pl.moh = sth
end sub

sub pl.drawDrops()
  stx = pl.dox
  sty = pl.doy
  stw = pl.dow
  sth = pl.doh
  if stw > 0 then spr.back stx, sty, stw, sth
  if pl.dropT <= 0 and pl.inP = 0 then
    if stw > 0 then scr.mark stx, sty, stw, sth
    pl.dow = 0
    exit sub
  end if
  gx0 = (pl.x \ 256) - 10
  gy0 = (pl.y \ 256) - 22
  if gy0 < 0 then gy0 = 0
  if gx0 < 0 then gx0 = 0
  for pl_k = 0 to 4
    pl_n = F_DROP
    if pl_k < pl.flems then pl_n = F_DROP + 4
    if pl_k = pl.flems and pl.inP = 1 then pl_n = F_DROP + (pl.chill \ 250)
    spr.at pl_n, gx0 + pl_k * 8, gy0
  next pl_k
  gm.markUnion gx0 * scr.sc, gy0 * scr.sc, 40 * scr.sc, 10 * scr.sc
  pl.dox = stx
  pl.doy = sty
  pl.dow = stw
  pl.doh = sth
end sub

sub pl.drawHearts()
  stx = pl.hox
  sty = pl.hoy
  stw = pl.how
  sth = pl.hoh
  if stw > 0 then spr.back stx, sty, stw, sth
  if pl.showH <= 0 then
    if stw > 0 then scr.mark stx, sty, stw, sth
    pl.how = 0
    exit sub
  end if
  gx0 = (pl.x \ 256) - 12
  gy0 = (pl.y \ 256) - 34
  if gy0 < 0 then gy0 = 0
  if gx0 < 0 then gx0 = 0
  for pl_k = 0 to 4
    if pl_k < pl.lives then
      pl_n = F_HEART
    else
      pl_n = F_HEART_EMPTY
    end if
    spr.at pl_n, gx0 + pl_k * 10, gy0
  next pl_k
  gm.markUnion gx0 * scr.sc, gy0 * scr.sc, 50 * scr.sc, 10 * scr.sc
  pl.hox = stx
  pl.hoy = sty
  pl.how = stw
  pl.hoh = sth
end sub

sub gm.draw()
  page write PG_BACK
  pd.draw
  pt.draw
  en.draw
  tr.draw
  door.draw
  pl.drawBody
  pl.drawMeter
  pl.drawDrops
  pl.drawHearts
end sub

sub gm.spawnTick()
  if gm.lcured >= lv.cure then
    exit sub
  end if
  pl.spawnT = pl.spawnT - 1
  en_spd = lv.spawnEvery
  if lv.raining = 1 then en_spd = (en_spd * 2) \ 3
  if en_spd < 30 then en_spd = 30
  if pl.spawnT <= 0 and en_n < lv.maxA then
    pl.spawnT = en_spd
    en.request
  end if
end sub

sub gm.play()
  if TH_OVER = 1 then
    gm.score = 22729
    gm.cured = 17
    gm.lcured = 17
    dlg.gameOver
    gm.over = 1
    exit sub
  end if
  dlg.banner
  gm.rebuild
  if TH_PAUSE = 1 then
    gm.draw
    scr.flush
    dlg.pause
    gm.leave = 1
    exit sub
  end if
  if TH_WIN = 1 then
    gm.lcured = lv.cure
    gm.cured = gm.cured + lv.cure
    gm.score = gm.score + 1337 * lv.cure
    gm.draw
    scr.flush
    dlg.levelWin
    gm.won = 1
    exit sub
  end if
  on key key.onKey
  do
    key.logic
    if key.pause = 1 then
      key.pause = FALSE
      dlg.pause
      key.reset
      if gm.leave = 1 then
        exit do
      end if
      gm.rebuild
    end if
    pl.tick
    door.tick
    en.tick
    tr.tick
    pt.tick
    pd.tick
    gm.spawnTick
    gm.draw
    scr.flush
    gm.frame = gm.frame + 1
    if gm.won = 1 or gm.over = 1 then
      exit do
    end if
    if TH_FAST = 0 then pause IDLE_MS
  loop
  on key 0
  if gm.leave = 1 then
    exit sub
  end if
  if gm.over = 1 then
    dlg.gameOver
    exit sub
  end if
  if gm.won = 1 then dlg.levelWin
end sub

sub gm.run()
  pl.boot
  if TH_START > 0 then gm.level = TH_START
  if gm.level > LV_N - 1 then gm.level = 0
  do
    lv.apply gm.level
    lv.build
    pl.reset
    en.clear
    pt.clear
    en.initial
    gm.play
    if gm.leave = 1 then
      exit sub
    end if
    if gm.over = 1 then
      exit sub
    end if
    if gm.won = 1 then
      gm.level = gm.level + 1
      if gm.level >= LV_N then
        gm.level = 0
        gm.round = gm.round + 1
      end if
    end if
  loop
end sub

sub gm.bench()
  gm.round = 0
  gm.level = 0
  gm.score = 0
  gm.cured = 0
  lv.apply 0
  lv.build
  pl.reset
  en.clear
  pt.clear
  for en_i = 0 to 23
    en.fill en_i, KZ_ZOMBIE, 30 + (en_i mod 8) * 70, 100 + (en_i \ 8) * 40
  next en_i
  gm_acc = 0
  gm_mx = 0
  for gm.frame = 1 to 60
    gm_t0 = timer
    en.tick
    pt.tick
    gm.draw
    scr.flush
    gm_dt = timer - gm_t0
    gm_acc = gm_acc + gm_dt
    if gm_dt > gm_mx then gm_mx = gm_dt
  next gm.frame
  print "BENCH frames=60 avg=" + num$(gm_acc \ 60) + " max=" + num$(gm_mx) + " enemies=24"
end sub
