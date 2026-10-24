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
  key.arrowReset()
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

' ---- Left/Right: every physical press moves the piece once at once; while the arrow stays held
' key.repeat() hands it out again after KEY_DAS ms and then every KEY_ARR ms (KEYDOWN driven,
' independent of the OS key repeat). A press is taken from whichever source sees it first: the
' ON KEY queue (key.arrowKeep) or a KEYDOWN rising edge (key.arrowPoll), so a press whose queued
' code was flushed (game start, leaving the pause dialog) still moves the piece.
' Per arrow i (0 = Left, 1 = Right): key.adn(i) = held at the last poll, key.aused(i) = this
' press has already moved, key.aT(i) = when it moved, key.aEdge(i) = it moved from KEYDOWN.
' Queued copies of an arrow that is held and has already moved are OS repeats, and the queued
' copy of a press that already moved from KEYDOWN (within KEY_SAME ms) is the same press; both
' are dropped. A press is over once it was seen held and then released (or KEY_SAME ms after an
' unseen quick tap). key.flush() resets all arrow state, so nothing carries over from the menu,
' a previous game or the pause dialog.
const KEY_DAS = 170, KEY_ARR = 50, KEY_OSREP = 120, KEY_SAME = 100
dim key.rpt = 0, key.rptStart = 0, key.rptNext = 0
dim key.adn(1), key.aused(1), key.aT(1), key.aEdge(1)

sub key.arrowReset()
  key.rpt = 0
  key.adn(0) = FALSE
  key.adn(1) = FALSE
  key.aused(0) = FALSE
  key.aused(1) = FALSE
  key.aEdge(0) = FALSE
  key.aEdge(1) = FALSE
end sub

' a new press of arrow k: it moves now and (re)starts the DAS timer
sub key.arrowStart(k, edge)
  key.aused(k - K_LEFT) = TRUE
  key.aT(k - K_LEFT) = timer
  key.aEdge(k - K_LEFT) = edge
  key.rpt = k
  key.rptStart = timer
  key.rptNext = timer + KEY_DAS
end sub

' queued arrow k: returns k when it should move, -1 for an OS repeat
function key.arrowKeep(k)
  local i, ign
  i = k - K_LEFT
  ign = key.aused(i)
  if ign then ign = key.adn(i)
  if ign then ign = key.held(k)
  if not ign then
    if key.aEdge(i) then ign = (timer - key.aT(i) < KEY_SAME)
  end if
  key.aEdge(i) = FALSE
  key.arrowKeep = -1
  if ign then exit function
  key.arrowStart(k, FALSE)
  key.arrowKeep = k
end function

' once per loop pass: K_LEFT / K_RIGHT for an arrow that KEYDOWN shows newly held and that has
' not moved yet, else 0. Released arrows are forgotten.
function key.arrowPoll()
  local i, k, h
  key.arrowPoll = 0
  for i = 0 to 1
    k = K_LEFT + i
    h = key.held(k)
    if h then
      if not key.adn(i) then
        if not key.aused(i) then
          key.arrowStart(k, TRUE)
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
