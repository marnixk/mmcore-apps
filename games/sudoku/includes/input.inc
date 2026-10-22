' input.inc - keyboard input through ON KEY (classic CMM2 key codes)
'
' key.isr is the ON KEY interrupt handler: it only moves every waiting
' INKEY$ code into a small ring buffer and returns (no drawing, no logic).
' The game loops drain the buffer with key.get(), so a press that arrives
' while the screen is being redrawn is queued instead of lost.
'
' mmcore notes: only the general form ON KEY subname is supported (the
' per-key ON KEY ascii, sub form is a syntax error), and the handler is
' switched off with a bare ON KEY (ON KEY 0 is a syntax error).
' The handler reads key.w only, the main code key.r only, so neither side
' can corrupt the other. One slot stays free to tell full from empty.

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

' start / stop the interrupt-driven input
sub key.start()
  key.r = key.w
  on key key.isr
  key.on = TRUE
end sub

sub key.stop()
  on key
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
  ' fallback while the handler is off
  if key.on then exit function
  k$ = inkey$
  if k$ <> "" then key.get = asc(k$)
end function

' TRUE when another key is already waiting in the queue
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
end sub

' ---- held arrows: auto-repeat driven by KEYDOWN, independent of OS repeat
' KEYDOWN(0) = number of keys held, KEYDOWN(1..n) = their CMM2 codes
' (arrows 128-131, the same values INKEY$ gives).
' The first press of an arrow arrives through the ON KEY queue and moves at
' once; while that arrow stays held, key.repeat() hands it out again after
' KEY_RDELAY ms and then every KEY_RRATE ms. Queued copies of the same arrow
' that arrive while it is held past the first moment are OS key-repeats and
' are dropped, so nothing moves twice.
const KEY_RDELAY = 250, KEY_RRATE = 70, KEY_OSREP = 150
dim key.rpt = 0, key.rptStart, key.rptNext

' TRUE when the key with CMM2 code k is held right now
function key.held(k)
  local i, n
  key.held = FALSE
  n = keydown(0)
  if n < 1 then exit function
  for i = 1 to n
    if keydown(i) = k then key.held = TRUE
  next i
end function

' a queued arrow: returns k when it is a fresh press, -1 when it is an OS
' repeat of the arrow we are already auto-repeating
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
  key.rptNext = timer + KEY_RDELAY
end function

' arrow to move again now because it is still held, 0 if none
function key.repeat()
  key.repeat = 0
  if key.rpt = 0 then exit function
  if not key.held(key.rpt) then
    key.rpt = 0
    exit function
  end if
  if timer < key.rptNext then exit function
  key.repeat = key.rpt
  key.rptNext = key.rptNext + KEY_RRATE
  if key.rptNext < timer then key.rptNext = timer + KEY_RRATE
end function

function key.isEnter(k)
  key.isEnter = (k = K_ENTER or k = K_LF)
end function
