' highscore.inc - one top 10 (all skills, SKILL column), the level reached,
' the stars and the best score per level and skill, kept in C:/.crystalcore
' (on C: so it also works when the game runs as a package from the
' read-only B: drive). File format:
'   R,easy,normal,hard        furthest level reached per skill (0-based)
'   S<skill>,<LV_N digits>    stars per level (0 = not cleared yet)
'   B<skill>,b1,b2,...        best level score per level
'   score,level,skill,name    one line per top-10 entry; level = furthest
'                             level reached (0-based, LV_N = all cleared)
const HS_FILE$ = "C:/.crystalcore"
' hs.ok: FALSE once a save failed (no C: or read-only), then saving is skipped
' hs.boot: TRUE during the start-up save in the main program (see hsSaveFail there)
dim hs.ok = TRUE, hs.boot = FALSE
const HS_MAXNAME = 8

dim string hs.name$(9) length HS_MAXNAME
dim hs.score(9), hs.lvl(9), hs.diff(9)
dim hs.reach(2), hs.st(2, LV_N - 1), hs.best(2, LV_N - 1)

sub hs.clear()
  local i, d
  for i = 0 to 9
    hs.name$(i) = ""
    hs.score(i) = 0
    hs.lvl(i) = 0
    hs.diff(i) = 0
  next i
  for d = 0 to 2
    hs.reach(d) = 0
    for i = 0 to LV_N - 1
      hs.st(d, i) = 0
      hs.best(d, i) = 0
    next i
  next d
end sub

' field n (1-based) of a comma separated line; the 4th (name) takes the rest
' when rest = 1
function hs.field$(s$, n, rest)
  local i = 1, p = 1, q
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
  if rest or q = 0 then
    hs.field$ = mid$(s$, p)
  else
    hs.field$ = mid$(s$, p, q - p)
  end if
end function

sub hs.load()
  local l$, sc, cn, df, rk, f$, i, d, v$
  hs.clear()
  f$ = HS_FILE$
  if not hs.found(f$) then
    exit sub
  end if
  open f$ for input as #1
  do while not eof(#1)
    line input #1, l$
    if left$(l$, 2) = "R," then
      for i = 0 to 2
        hs.reach(i) = max(0, min(LV_N - 1, val(hs.field$(l$, i + 2, 0))))
      next i
    elseif left$(l$, 1) = "S" then
      d = val(mid$(l$, 2, 1))
      v$ = hs.field$(l$, 2, 1)
      if d >= 0 and d <= 2 then
        for i = 0 to min(LV_N, len(v$)) - 1
          hs.st(d, i) = max(0, min(3, val(mid$(v$, i + 1, 1))))
        next i
      end if
    elseif left$(l$, 1) = "B" then
      d = val(mid$(l$, 2, 1))
      if d >= 0 and d <= 2 then
        for i = 0 to LV_N - 1
          hs.best(d, i) = max(0, val(hs.field$(l$, i + 2, 0)))
        next i
      end if
    elseif len(l$) > 6 then
      sc = val(hs.field$(l$, 1, 0))
      cn = val(hs.field$(l$, 2, 0))
      df = val(hs.field$(l$, 3, 0))
      if df >= 0 and df <= 2 and sc > 0 and cn >= 0 and cn <= LV_N then
        rk = hs.rank(sc)
        if rk >= 0 then hs.insert(rk, left$(hs.field$(l$, 4, 1), HS_MAXNAME), sc, cn, df)
      end if
    end if
  loop
  close #1
end sub

' TRUE when file f$ exists (a missing file just means empty tables)
function hs.found(f$)
  hs.found = (dir$(f$, FILE) <> "")
end function

sub hs.save()
  local i, d, s$
  if not hs.ok then
    exit sub
  end if
  ' a failed write jumps to hsSaveFail in the main program (no C:, read-only card)
  on error goto hsSaveFail
  open HS_FILE$ for output as #1
  print #1, "R," + str$(hs.reach(0)) + "," + str$(hs.reach(1)) + "," + str$(hs.reach(2))
  for d = 0 to 2
    s$ = ""
    for i = 0 to LV_N - 1
      s$ = s$ + str$(hs.st(d, i))
    next i
    print #1, "S" + str$(d) + "," + s$
    s$ = "B" + str$(d)
    for i = 0 to LV_N - 1
      s$ = s$ + "," + str$(hs.best(d, i))
    next i
    print #1, s$
  next d
  for i = 0 to 9
    if hs.score(i) > 0 then
      print #1, str$(hs.score(i)) + "," + str$(hs.lvl(i)) + "," + str$(hs.diff(i)) + "," + hs.name$(i)
    end if
  next i
  close #1
  on error
end sub

' position a score would take (0..9), or -1 if it doesn't make the top 10
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

sub hs.insert(rk, n$, sc, cn, df)
  local i = 9
  do while i > rk
    hs.name$(i) = hs.name$(i - 1)
    hs.score(i) = hs.score(i - 1)
    hs.lvl(i) = hs.lvl(i - 1)
    hs.diff(i) = hs.diff(i - 1)
    i = i - 1
  loop
  hs.name$(rk) = n$
  hs.score(rk) = sc
  hs.lvl(rk) = cn
  hs.diff(rk) = df
end sub

' the level column: the number reached, ALL = every level cleared
function hs.lvlName$(k)
  if k >= LV_N then
    hs.lvlName$ = "ALL"
  else
    hs.lvlName$ = n2$(k + 1)
  end if
end function

' stars of level li on skill d as text (for the title's level row)
function hs.stars$(d, li)
  hs.stars$ = left$("***", hs.st(d, li)) + left$("---", 3 - hs.st(d, li))
end function
