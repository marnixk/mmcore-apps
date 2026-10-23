' input.inc - keyboard input through ON KEY (classic CMM2 key codes)
'
' key.isr is the general ON KEY interrupt handler: it only moves every waiting
' INKEY$ code into a small ring buffer and returns (no drawing, no logic). The
' loops drain the buffer with key.get(), so a press that arrives while the
' screen is being redrawn is queued instead of lost. The handler writes key.w
' only and the main code moves key.r only. One slot stays free (full/empty).

const KEYQ = 32
dim key.q(KEYQ - 1)
dim key.r = 0, key.w = 0
dim key.on = FALSE

sub key.isr()
  local k$, nw
  k$ = inkey$
  do while k$ <> ""
    nw = (key.w + 1) mod KEYQ
    if nw <> key.r then
      key.q(key.w) = asc(k$)
      key.w = nw
    end if
    k$ = inkey$
  loop
end sub

sub key.start()
  key.r = key.w
  on key key.isr
  key.on = TRUE
end sub

sub key.stop()
  on key 0
  key.on = FALSE
end sub

' next key code from the queue, 0 if none
function key.get()
  local k$
  if key.r <> key.w then
    key.get = key.q(key.r)
    key.r = (key.r + 1) mod KEYQ
    exit function
  end if
  key.get = 0
  if key.on then exit function
  k$ = inkey$
  if k$ <> "" then key.get = asc(k$)
end function

function key.waiting()
  key.waiting = (key.r <> key.w)
end function

' throw away anything typed so far
sub key.flush()
  local t = timer
  do while inkey$ <> "" and timer - t < 500
  loop
  key.r = key.w
  key.rpt = 0
  key.nr = 0
end sub

' TRUE when the key with CMM2 code k is held right now
' (KEYDOWN(0) = number of keys held, KEYDOWN(1..n) = their codes)
function key.held(k)
  local i, n
  key.held = FALSE
  n = keydown(0)
  if n < 1 then exit function
  for i = 1 to n
    if keydown(i) = k then key.held = TRUE
  next i
end function

' ---- held Left/Right: DAS / ARR auto-repeat driven by KEYDOWN, independent of
' the OS key repeat. The first press arrives through the queue and moves once
' at once; while that arrow stays held key.repeat() hands it out again after
' KEY_DAS ms and then every KEY_ARR ms. Queued copies of the same arrow that
' arrive while it is held are OS repeats and are dropped, so a single tap
' moves exactly one cell.
const KEY_DAS = 170, KEY_ARR = 50, KEY_OSREP = 120
dim key.rpt = 0, key.rptStart, key.rptNext

function key.arrowKeep(k)
  local ign
  ign = (k = key.rpt)
  if ign then ign = (timer - key.rptStart >= KEY_OSREP)
  if ign then ign = key.held(k)
  key.arrowKeep = -1
  if ign then exit function
  key.arrowKeep = k
  key.rpt = k
  key.rptStart = timer
  key.rptNext = timer + KEY_DAS
end function

function key.repeat()
  key.repeat = 0
  if key.rpt = 0 then exit function
  if not key.held(key.rpt) then
    key.rpt = 0
    exit function
  end if
  if timer < key.rptNext then exit function
  key.repeat = key.rpt
  key.rptNext = key.rptNext + KEY_ARR
  if key.rptNext < timer then key.rptNext = timer + KEY_ARR
end function

' ---- one-shot keys (rotate, hard drop, hold): a key that is still held from
' the press we already acted on only repeats through the OS, so its queued
' copies are ignored until it has been released (key.nrPoll notices that).
dim key.nr = 0

' k = queued code, kd = its KEYDOWN code. returns TRUE when it should act
function key.once(k, kd)
  key.once = TRUE
  if kd = key.nr then
    if key.held(kd) then
      key.once = FALSE
      exit function
    end if
  end if
  key.nr = kd
end function

sub key.nrPoll()
  if key.nr <> 0 then
    if not key.held(key.nr) then key.nr = 0
  end if
end sub

function key.isEnter(k)
  key.isEnter = (k = K_ENTER or k = K_LF)
end function
