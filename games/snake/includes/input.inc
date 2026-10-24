' input.inc - keyboard input through ON KEY (classic CMM2 key codes)
'
' key.isr is the ON KEY interrupt handler: it only moves every waiting INKEY$
' code into a small ring buffer and returns (no drawing, no logic). The loops
' drain the buffer with key.get(), so a press that arrives while the screen is
' being redrawn is queued instead of lost. The handler writes key.w only and the
' main code moves key.r only. One slot stays free (full/empty).

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
  key.arrowReset()
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

' ---- arrows: every physical press counts once. A press is taken from whichever
' source sees it first: the ON KEY queue (key.arrowKeep) or a KEYDOWN rising
' edge (key.arrowPoll), so a press whose queued code was flushed (level start,
' leaving the pause dialog) or arrives late still steers.
' Per arrow i (0 Up, 1 Down, 2 Left, 3 Right): key.adn(i) = held at the last
' poll, key.aused(i) = this press has been used, key.aT(i) = when, key.aEdge(i)
' = it was used from KEYDOWN. Queued copies of an arrow that is held and already
' used are OS repeats; the queued copy of a press already used from KEYDOWN
' (within KEY_SAME ms) is the same press. Both are dropped.
const KEY_SAME = 100
dim key.adn(3), key.aused(3), key.aT(3), key.aEdge(3)

sub key.arrowReset()
  local i
  for i = 0 to 3
    key.adn(i) = FALSE
    key.aused(i) = FALSE
    key.aEdge(i) = FALSE
  next i
end sub

sub key.arrowUse(k, edge)
  key.aused(k - K_UP) = TRUE
  key.aT(k - K_UP) = timer
  key.aEdge(k - K_UP) = edge
end sub

' queued arrow k: returns k when it counts, 0 for a repeat / duplicate
function key.arrowKeep(k)
  local i, ign
  i = k - K_UP
  ign = key.aused(i)
  if ign then ign = key.adn(i)
  if ign then ign = key.held(k)
  if not ign then
    if key.aEdge(i) then ign = (timer - key.aT(i) < KEY_SAME)
  end if
  key.aEdge(i) = FALSE
  key.arrowKeep = 0
  if ign then exit function
  key.arrowUse(k, FALSE)
  key.arrowKeep = k
end function

' once per loop pass: an arrow that KEYDOWN shows newly held and that has not
' been used yet, else 0. Released arrows are forgotten.
function key.arrowPoll()
  local i, k, h
  key.arrowPoll = 0
  for i = 0 to 3
    k = K_UP + i
    h = key.held(k)
    if h then
      if not key.adn(i) then
        if not key.aused(i) then
          key.arrowUse(k, TRUE)
          key.arrowPoll = k
        end if
      end if
    elseif key.adn(i) then
      key.aused(i) = FALSE
    elseif timer - key.aT(i) >= KEY_SAME then
      key.aused(i) = FALSE
    end if
    key.adn(i) = h
  next i
end function

function key.isEnter(k)
  key.isEnter = (k = K_ENTER or k = K_LF)
end function

function key.isArrow(k)
  key.isArrow = (k >= K_UP and k <= K_RIGHT)
end function

' P / Esc pause the game, Q quits from the pause dialog
function key.isPause(k)
  key.isPause = (k = K_ESC or k = asc("p") or k = asc("P"))
end function
