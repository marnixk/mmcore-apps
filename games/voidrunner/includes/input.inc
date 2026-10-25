' input.inc - keyboard input through ON KEY (classic CMM2 key codes)
'
' key.isr is the ON KEY interrupt handler: it only moves every waiting INKEY$
' code into a small ring buffer and returns (no drawing, no logic). The loops
' drain the buffer with key.get(), so a press that arrives while the screen is
' being redrawn is queued instead of lost. The handler writes key.w only and the
' main code moves key.r only. One slot stays free (full/empty). The queue serves
' the menus, name entry and P / Esc / Q; the thruster uses KEYDOWN (below).

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
  local t
  t = timer
  do while inkey$ <> "" and timer - t < 500
  loop
  key.r = key.w
  key.thrustReset()
end sub

' ---- the thruster is read from KEYDOWN, never from the INKEY$ queue.
' key.scan() runs on every wake of the game loop and returns TRUE when a thrust
' key (Space, Up, W) went down since the previous scan: a rising edge. Holding a
' key gives ONE thrust; OS auto-repeat never reaches KEYDOWN. KEYDOWN(0) is the
' number of held keys and KEYDOWN(1..n) their codes (CMM2 semantics).
' key.held(i) = thrust key i (0 Space, 1 Up, 2 W) was down at the last scan.
dim key.held(2), key.now(2)

' forget held keys: one still held after this does NOT count as a new press
' until it is released (a held Space from the menu never fires a thrust)
sub key.thrustReset()
  local i, n, k
  for i = 0 to 2
    key.held(i) = FALSE
  next i
  n = keydown(0)
  for i = 1 to n
    k = key.thrustIx(keydown(i))
    if k >= 0 then key.held(k) = TRUE
  next i
end sub

' 0 Space, 1 Up, 2 W, -1 not a thrust key
function key.thrustIx(k)
  key.thrustIx = -1
  if k = K_SPACE then key.thrustIx = 0
  if k = K_UP then key.thrustIx = 1
  if k = asc("w") or k = asc("W") then key.thrustIx = 2
end function

function key.scan()
  local i, n, k
  key.scan = FALSE
  key.now(0) = FALSE
  key.now(1) = FALSE
  key.now(2) = FALSE
  n = keydown(0)
  for i = 1 to n
    k = key.thrustIx(keydown(i))
    if k >= 0 then key.now(k) = TRUE
  next i
  for i = 0 to 2
    if key.now(i) and not key.held(i) then key.scan = TRUE
    key.held(i) = key.now(i)
  next i
end function

function key.isEnter(k)
  key.isEnter = (k = K_ENTER or k = K_LF)
end function

' P / Esc pause the game, Q quits from the pause dialog
function key.isPause(k)
  key.isPause = (k = K_ESC or k = asc("p") or k = asc("P"))
end function

function key.isQuit(k)
  key.isQuit = (k = asc("q") or k = asc("Q"))
end function
