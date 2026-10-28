' scores.inc - top 10 in C:/.jetpack, one line each: score,level,name
const HS_FILE$ = "C:/.jetpack"
const HS_MAXNAME = 8

dim hs.ok = TRUE, hs.boot = FALSE
dim string hs.name$(9), hs.entered$
dim hs.score(9), hs.level(9)
dim string hs_line$, hs_f$

sub hs.clear()
  local i
  for i = 0 to 9
    hs.name$(i) = ""
    hs.score(i) = 0
    hs.level(i) = 0
  next i
end sub

function hs.field$(s$, n)
  local i, p, q
  p = 1
  i = 1
  do while i < n
    q = instr(p, s$, ",")
    if q = 0 then
      hs.field$ = ""
      exit function
    end if
    p = q + 1
    i = i + 1
  loop
  q = instr(p, s$, ",")
  if n >= 3 or q = 0 then
    hs.field$ = mid$(s$, p)
  else
    hs.field$ = mid$(s$, p, q - p)
  end if
end function

function hs.found(f$)
  hs.found = (dir$(f$, FILE) <> "")
end function

function hs.rank(sc)
  local i
  hs.rank = -1
  if sc <= 0 then
    exit function
  end if
  for i = 0 to 9
    if sc > hs.score(i) then
      hs.rank = i
      exit function
    end if
  next i
end function

sub hs.insert(rk, name$, sc, lvl)
  local i
  for i = 9 to rk + 1 step -1
    hs.name$(i) = hs.name$(i - 1)
    hs.score(i) = hs.score(i - 1)
    hs.level(i) = hs.level(i - 1)
  next i
  hs.name$(rk) = name$
  hs.score(rk) = sc
  hs.level(rk) = lvl
end sub

sub hs.load()
  local sc, lvl, rk
  hs.clear()
  hs_f$ = HS_FILE$
  if not hs.found(hs_f$) then
    exit sub
  end if
  open hs_f$ for input as #1
  do while not eof(#1)
    line input #1, hs_line$
    if len(hs_line$) > 4 then
      sc = val(hs.field$(hs_line$, 1))
      lvl = val(hs.field$(hs_line$, 2))
      rk = hs.rank(sc)
      if rk >= 0 then hs.insert(rk, left$(hs.field$(hs_line$, 3), HS_MAXNAME), sc, lvl)
    end if
  loop
  close #1
end sub

sub hs.save()
  local i
  if not hs.ok then
    exit sub
  end if
  on error goto hsSaveFail
  open HS_FILE$ for output as #1
  for i = 0 to 9
    if hs.score(i) > 0 then
      print #1, num$(hs.score(i)) + "," + num$(hs.level(i)) + "," + hs.name$(i)
    end if
  next i
  close #1
end sub

sub hs.draw(hilite)
  local i, style, y
  page copy PG_SCORES to PG_BACK
  for i = 0 to 9
    y = lay.s0 + i * lay.srow
    style = 0
    if i = hilite then style = 1
    if hs.score(i) > 0 then
      ui.text(lay.snum, y, num$(i + 1), style)
      ui.text(lay.sname, y, hs.name$(i), style)
      ui.text(lay.slv, y, num$(hs.level(i)), style)
      ui.textR(lay.ssc, y, num$(hs.score(i)), 2)
    end if
  next i
  scr.present()
end sub

sub hs.show(hilite)
  local k$
  hs.draw(hilite)
  key.drain()
  do
    k$ = inkey$
    if k$ = "" then pause 5
  loop until k$ <> ""
end sub

' blocks until Enter. Writes hs.entered$
sub hs.ask()
  local k$, n, k
  hs.entered$ = ""
  key.drain()
  do
    page copy PG_SCORES to PG_BACK
    ui.textC(lay.sx + lay.sw \ 2, lay.sy + 8 * lay.fsc, "HIGH SCORE", 4)
    ui.textC(lay.sx + lay.sw \ 2, lay.sy + 28 * lay.fsc, hs.entered$ + "_", 1)
    ui.textC(lay.sx + lay.sw \ 2, lay.sy + lay.sh - 16 * lay.fsc, "ENTER", 3)
    scr.present()
    do
      k$ = inkey$
      if k$ = "" then pause 5
    loop until k$ <> ""
    k = asc(k$)
    n = len(hs.entered$)
    if k >= 97 and k <= 122 then k = k - 32
    if k = K_ENTER or k = K_LF then
      exit do
    end if
    if (k = K_BKSP or k = K_DEL) and n > 0 then
      hs.entered$ = left$(hs.entered$, n - 1)
    end if
    if n < HS_MAXNAME then
      if (k >= 65 and k <= 90) or (k >= 48 and k <= 57) or k = 32 then
        hs.entered$ = hs.entered$ + chr$(k)
      end if
    end if
  loop
  if hs.entered$ = "" then hs.entered$ = "PLAYER"
end sub
