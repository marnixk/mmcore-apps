' highscore.inc - one top 10 (all skills, SKILL column) plus the round
' reached per skill, kept in C:/.salsaslide (on C: so it also works when the
' game runs as a package from a read-only drive)
' file format:
'   R,easy,normal,hard        furthest round reached per skill (0-based)
'   score,round,skill,name    one line per entry; round = last round played (1-based)
const HS_FILE$ = "C:/.salsaslide"
const HS_MAXNAME = 8, RD_MAX = 98
' hs.ok: FALSE once a save failed (no C: or read-only), then saving is skipped
' hs.boot: TRUE during the start-up save in the main program (see hsSaveFail there)
dim hs.ok = TRUE, hs.boot = FALSE

dim string hs.name$(9) length HS_MAXNAME
dim hs.score(9), hs.rd(9), hs.diff(9)
dim hs.reach(2)

sub hs.clear()
  local i
  for i = 0 to 9
    hs.name$(i) = ""
    hs.score(i) = 0
    hs.rd(i) = 0
    hs.diff(i) = 0
  next i
  for i = 0 to 2
    hs.reach(i) = 0
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
  local l$, sc, rd, df, rk, f$, i
  hs.clear()
  f$ = HS_FILE$
  if dir$(f$, FILE) = "" then exit sub
  open f$ for input as #1
  do while not eof(#1)
    line input #1, l$
    if left$(l$, 2) = "R," then
      for i = 0 to 2
        hs.reach(i) = max(0, min(RD_MAX, val(hs.field$(l$, i + 2))))
      next i
    elseif len(l$) > 6 then
      sc = val(hs.field$(l$, 1))
      rd = val(hs.field$(l$, 2))
      df = val(hs.field$(l$, 3))
      if df >= 0 and df <= 2 and sc > 0 and rd >= 1 and rd <= RD_MAX + 1 then
        rk = hs.rank(sc)
        if rk >= 0 then hs.insert(rk, left$(hs.field$(l$, 4), HS_MAXNAME), sc, rd, df)
      end if
    end if
  loop
  close #1
end sub

sub hs.save()
  local i
  if not hs.ok then exit sub
  ' a failed write jumps to hsSaveFail in the main program (no C:, read-only card)
  on error goto hsSaveFail
  open HS_FILE$ for output as #1
  print #1, "R," + str$(hs.reach(0)) + "," + str$(hs.reach(1)) + "," + str$(hs.reach(2))
  for i = 0 to 9
    if hs.score(i) > 0 then
      print #1, str$(hs.score(i)) + "," + str$(hs.rd(i)) + "," + str$(hs.diff(i)) + "," + hs.name$(i)
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

sub hs.insert(rk, n$, sc, rd, df)
  local i = 9
  do while i > rk
    hs.name$(i) = hs.name$(i - 1)
    hs.score(i) = hs.score(i - 1)
    hs.rd(i) = hs.rd(i - 1)
    hs.diff(i) = hs.diff(i - 1)
    i = i - 1
  loop
  hs.name$(rk) = n$
  hs.score(rk) = sc
  hs.rd(rk) = rd
  hs.diff(rk) = df
end sub
