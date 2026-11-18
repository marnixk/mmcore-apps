' highscore.inc - top 10 in C:/.patientzero (packages run from read-only B:).
' One line: score,cured,level,name

const HS_FILE$ = "C:/.patientzero"
const HS_MAXNAME = 8

dim hs.ok = TRUE, hs.boot = FALSE
dim string hs.name$(9), hs.entered$
dim hs.score(9), hs.cured(9), hs.level(9)
dim string hs_line$, hs_f$
dim hs_i, hs_sc, hs_cu, hs_lv, hs_rk

sub hs.clear()
  for hs_i = 0 to 9
    hs.name$(hs_i) = ""
    hs.score(hs_i) = 0
    hs.cured(hs_i) = 0
    hs.level(hs_i) = 0
  next hs_i
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
  if n >= 4 or q = 0 then
    hs.field$ = mid$(s$, p)
  else
    hs.field$ = mid$(s$, p, q - p)
  end if
end function

function hs.rank(sc)
  hs.rank = -1
  if sc <= 0 then
    exit function
  end if
  for hs_i = 0 to 9
    if sc > hs.score(hs_i) then
      hs.rank = hs_i
      exit function
    end if
  next hs_i
end function

sub hs.insert(rk, name$, sc, cu, lv)
  for hs_i = 9 to rk + 1 step -1
    hs.name$(hs_i) = hs.name$(hs_i - 1)
    hs.score(hs_i) = hs.score(hs_i - 1)
    hs.cured(hs_i) = hs.cured(hs_i - 1)
    hs.level(hs_i) = hs.level(hs_i - 1)
  next hs_i
  hs.name$(rk) = name$
  hs.score(rk) = sc
  hs.cured(rk) = cu
  hs.level(rk) = lv
end sub

sub hs.load()
  hs.clear()
  hs_f$ = HS_FILE$
  if dir$(hs_f$, FILE) = "" then
    exit sub
  end if
  open hs_f$ for input as #1
  do while not eof(#1)
    line input #1, hs_line$
    if len(hs_line$) > 4 then
      hs_sc = val(hs.field$(hs_line$, 1))
      hs_cu = val(hs.field$(hs_line$, 2))
      hs_lv = val(hs.field$(hs_line$, 3))
      hs_rk = hs.rank(hs_sc)
      if hs_rk >= 0 then hs.insert hs_rk, left$(hs.field$(hs_line$, 4), HS_MAXNAME), hs_sc, hs_cu, hs_lv
    end if
  loop
  close #1
end sub

sub hs.save()
  if hs.ok = FALSE then
    exit sub
  end if
  on error goto hsSaveFail
  open HS_FILE$ for output as #1
  for hs_i = 0 to 9
    if hs.score(hs_i) > 0 then
      print #1, num$(hs.score(hs_i)) + "," + num$(hs.cured(hs_i)) + "," + num$(hs.level(hs_i)) + "," + hs.name$(hs_i)
    end if
  next hs_i
  close #1
end sub

' Blocks until Enter. Writes hs.entered$.
sub hs.ask()
  local k$, n, k
  hs.entered$ = ""
  key.drain()
  do
    dlg.overBody hs.entered$ + "_", hs.rank(gm.score)
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
  if hs.entered$ = "" then hs.entered$ = "JERRY"
end sub
