' highscore.inc - one top 10 (all skills, SKILL column), kept in C:/.tronix
' (on C: so it also works when the game runs as a package from read-only B:)
' file format: one line per entry  score,level,difficulty,name
const HS_FILE$ = "C:/.tronix"
' hs.ok: FALSE once a save failed (no C: or read-only), then saving is skipped
' hs.boot: TRUE during the start-up save in the main program (see hsSaveFail there)
dim hs.ok = TRUE, hs.boot = FALSE
const HS_MAXNAME = 8

dim string hs.name$(9)
dim hs.score(9), hs.level(9), hs.diff(9)

sub hs.clear()
  local i
  for i = 0 to 9
    hs.name$(i) = ""
    hs.score(i) = 0
    hs.level(i) = 0
    hs.diff(i) = 0
  next i
end sub

' field n (1-based) of a comma separated line; the 4th (name) takes the rest
function hs.field$(s$, n)
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
  if n = 4 or q = 0 then
    hs.field$ = mid$(s$, p)
  else
    hs.field$ = mid$(s$, p, q - p)
  end if
end function

sub hs.load()
  local l$, sc, lvl, df, rk, f$
  hs.clear()
  f$ = HS_FILE$
  if not hs.found(f$) then exit sub
  open f$ for input as #1
  do while not eof(#1)
    line input #1, l$
    if len(l$) > 6 then
      sc = val(hs.field$(l$, 1))
      lvl = val(hs.field$(l$, 2))
      df = val(hs.field$(l$, 3))
      if df >= 0 and df <= 2 and sc > 0 then
        rk = hs.rank(sc)
        if rk >= 0 then hs.insert(rk, left$(hs.field$(l$, 4), HS_MAXNAME), sc, lvl, df)
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
  local i
  if not hs.ok then exit sub
  ' a failed write jumps to hsSaveFail in the main program (no C:, read-only card)
  on error goto hsSaveFail
  open HS_FILE$ for output as #1
  for i = 0 to 9
    if hs.score(i) > 0 then
      print #1, str$(hs.score(i)) + "," + str$(hs.level(i)) + "," + str$(hs.diff(i)) + "," + hs.name$(i)
    end if
  next i
  close #1
  on error
end sub

' position a score would take (0..9), or -1 if it doesn't make the top 10
function hs.rank(sc)
  local i
  hs.rank = -1
  if sc <= 0 then exit function
  for i = 0 to 9
    if sc > hs.score(i) then
      hs.rank = i
      exit function
    end if
  next i
end function

sub hs.insert(rk, n$, sc, lvl, df)
  local i = 9
  do while i > rk
    hs.name$(i) = hs.name$(i - 1)
    hs.score(i) = hs.score(i - 1)
    hs.level(i) = hs.level(i - 1)
    hs.diff(i) = hs.diff(i - 1)
    i = i - 1
  loop
  hs.name$(rk) = n$
  hs.score(rk) = sc
  hs.level(rk) = lvl
  hs.diff(rk) = df
end sub
