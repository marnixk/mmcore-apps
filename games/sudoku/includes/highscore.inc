' highscore.inc - top 10 per level, kept in C:/.sudoku
' (on C: so it also works when the game runs as a package from read-only B:)
' file format: one line per entry  level,score,seconds,name
const HS_FILE$ = "C:/.sudoku"
const HS_OLD$ = "sudoku.hi"   ' old place next to the program, read once if C: has no file yet
' hs.ok: FALSE once a save failed (no C: or read-only), then saving is skipped
' hs.boot: TRUE during the start-up save in the main program (see hsSaveFail there)
dim hs.ok = TRUE, hs.boot = FALSE
const HS_MAXNAME = 10

dim string hs.name$(2, 9)
dim hs.score(2, 9), hs.secs(2, 9)

sub hs.clear()
  local l, i
  for l = 0 to 2
    for i = 0 to 9
      hs.name$(l, i) = ""
      hs.score(l, i) = 0
      hs.secs(l, i) = 0
    next i
  next l
end sub

' split helper: field n (1-based) of a comma separated line
function hs.field$(s$, n)
  local i, p = 1, q
  for i = 1 to n - 1
    q = instr(p, s$, ",")
    if q = 0 then
      hs.field$ = ""
      exit function
    end if
    p = q + 1
  next i
  q = instr(p, s$, ",")
  if n = 4 or q = 0 then
    hs.field$ = mid$(s$, p)
  else
    hs.field$ = mid$(s$, p, q - p)
  end if
end function

sub hs.load()
  local l$, lv, sc, sx, rk, n$, f$
  hs.clear()
  f$ = HS_FILE$
  if not hs.found(f$) then f$ = HS_OLD$
  if not hs.found(f$) then exit sub
  open f$ for input as #1
  do while not eof(#1)
    line input #1, l$
    if len(l$) > 4 then
      lv = val(hs.field$(l$, 1))
      sc = val(hs.field$(l$, 2))
      sx = val(hs.field$(l$, 3))
      if lv >= 0 and lv <= 2 and sc > 0 then
        rk = hs.rank(lv, sc)
        n$ = hs.field$(l$, 4)
        if rk >= 0 then hs.insert(lv, rk, n$, sc, sx)
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
  local l, i
  if not hs.ok then exit sub
  ' a failed write jumps to hsSaveFail in the main program (no C:, read-only card)
  on error goto hsSaveFail
  open HS_FILE$ for output as #1
  for l = 0 to 2
    for i = 0 to 9
      if hs.score(l, i) > 0 then
        print #1, str$(l) + "," + str$(hs.score(l, i)) + "," + str$(hs.secs(l, i)) + "," + hs.name$(l, i)
      end if
    next i
  next l
  close #1
  on error
end sub

' table position a score would take (0..9), or -1 if it doesn't make the top 10
function hs.rank(lv, sc)
  local i
  hs.rank = -1
  for i = 0 to 9
    if sc > hs.score(lv, i) then
      hs.rank = i
      exit function
    end if
  next i
end function

sub hs.insert(lv, rk, n$, sc, sx)
  local i
  for i = 9 to rk + 1 step -1
    hs.name$(lv, i) = hs.name$(lv, i - 1)
    hs.score(lv, i) = hs.score(lv, i - 1)
    hs.secs(lv, i) = hs.secs(lv, i - 1)
  next i
  hs.name$(lv, rk) = n$
  hs.score(lv, rk) = sc
  hs.secs(lv, rk) = sx
end sub
