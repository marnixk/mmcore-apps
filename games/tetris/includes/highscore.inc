' highscore.inc - top 10 per difficulty, kept in tetris.hi next to the program
' file format: one line per entry  difficulty,score,level,name
const HS_FILE$ = "tetris.hi"
const HS_MAXNAME = 8

dim string hs.name$(2, 9)
dim hs.score(2, 9), hs.level(2, 9)

sub hs.clear()
  local l, i
  for l = 0 to 2
    for i = 0 to 9
      hs.name$(l, i) = ""
      hs.score(l, i) = 0
      hs.level(l, i) = 0
    next i
  next l
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
  local l$, lv, sc, lvl, rk, n$
  hs.clear()
  if not hs.exists() then exit sub
  open HS_FILE$ for input as #1
  do while not eof(#1)
    line input #1, l$
    if len(l$) > 4 then
      lv = val(hs.field$(l$, 1))
      sc = val(hs.field$(l$, 2))
      lvl = val(hs.field$(l$, 3))
      n$ = left$(hs.field$(l$, 4), HS_MAXNAME)
      if lv >= 0 and lv <= 2 and sc > 0 then
        rk = hs.rank(lv, sc)
        if rk >= 0 then hs.insert(lv, rk, n$, sc, lvl)
      end if
    end if
  loop
  close #1
end sub

' DIR$ needs a wildcard, so scan *.hi for the file
function hs.exists()
  local f$
  hs.exists = FALSE
  f$ = dir$("*.hi", "FILE")
  do while f$ <> ""
    if ucase$(f$) = ucase$(HS_FILE$) then hs.exists = TRUE
    f$ = dir$("")
  loop
end function

sub hs.save()
  local l, i
  open HS_FILE$ for output as #1
  for l = 0 to 2
    for i = 0 to 9
      if hs.score(l, i) > 0 then
        print #1, str$(l) + "," + str$(hs.score(l, i)) + "," + str$(hs.level(l, i)) + "," + hs.name$(l, i)
      end if
    next i
  next l
  close #1
end sub

' position a score would take (0..9), or -1 if it doesn't make the top 10
function hs.rank(lv, sc)
  local i
  hs.rank = -1
  if sc <= 0 then exit function
  for i = 0 to 9
    if sc > hs.score(lv, i) then
      hs.rank = i
      exit function
    end if
  next i
end function

sub hs.insert(lv, rk, n$, sc, lvl)
  local i = 9
  do while i > rk
    hs.name$(lv, i) = hs.name$(lv, i - 1)
    hs.score(lv, i) = hs.score(lv, i - 1)
    hs.level(lv, i) = hs.level(lv, i - 1)
    i = i - 1
  loop
  hs.name$(lv, rk) = n$
  hs.score(lv, rk) = sc
  hs.level(lv, rk) = lvl
end sub
