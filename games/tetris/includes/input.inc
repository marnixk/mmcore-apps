' input.inc - keyboard (classic CMM2 key codes)
'
' Menus, dialogs and name entry read INKEY$ directly in a simple loop:
'   do : k$ = inkey$ : if k$ <> "" then (handle it) else pause 5 : loop
' No ON KEY handler, no queue, no flushing: every press is handled once, in order.
' The game loop reads INKEY$ the same way on every pass; KEYDOWN (below) adds
' the held-key timing for the arrows, soft drop and the one-shot keys.

' throw away typed-ahead keys (used once: when the game-over banner has been
' up for a moment, so keys still buffered from play do not skip it)
sub key.drain()
  do while inkey$ <> ""
  loop
end sub

' forget held-key state, so nothing carries over from the menu, a previous
' game or the pause dialog
sub key.reset()
  local i
  key.arrowReset()
  for i = 0 to KEY_NR - 1
    key.nr(i) = 0
  next i
end sub

function key.isEnter(k$)
  key.isEnter = (k$ = chr$(K_ENTER) or k$ = chr$(K_LF))
end function

' Esc or P pauses play and resumes from the pause dialog
function key.isPause(k$)
  key.isPause = (k$ = chr$(K_ESC) or k$ = "p" or k$ = "P")
end function

function key.isQuit(k$)
  key.isQuit = (k$ = "q" or k$ = "Q")
end function

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
' independent of the OS key repeat). A press is taken from whichever source sees it first: its
' INKEY$ code (key.arrowKeep) or a KEYDOWN rising edge (key.arrowPoll), so a press whose INKEY$
' code was read elsewhere (e.g. by the pause dialog) still moves the piece.
' Per arrow i (0 = Left, 1 = Right): key.adn(i) = held at the last poll, key.aused(i) = this
' press has already moved, key.aT(i) = when it moved, key.aEdge(i) = it moved from KEYDOWN.
' INKEY$ copies of an arrow that is held and has already moved are OS repeats, and the INKEY$
' copy of a press that already moved from KEYDOWN (within KEY_SAME ms) is the same press; both
' are dropped. A press is over once it was seen held and then released (or KEY_SAME ms after an
' unseen quick tap). key.reset() clears all arrow state.
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

' INKEY$ arrow k: returns k when it should move, -1 for an OS repeat
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

' ---- one-shot keys (hard drop, hold, rotate): one action per physical press.
' Space hard-drops, Up and X rotate clockwise, Z rotates counter-clockwise,
' Q holds (in the pause dialog Q quits instead). A key we acted on stays in
' key.nr() until KEYDOWN shows it released; while it is still held its INKEY$
' copies are only OS repeats and are ignored.
' Each key is tracked on its own, so holding Up never rotates again.
const KEY_NR = 5
dim key.nr(KEY_NR - 1)

' k = INKEY$ code, kd = its KEYDOWN code. returns TRUE when it should act
function key.once(k, kd)
  local i, slot = -1
  key.once = FALSE
  for i = 0 to KEY_NR - 1
    if key.nr(i) = kd then
      if key.held(kd) then exit function
      key.nr(i) = 0
    end if
    if key.nr(i) = 0 and slot < 0 then slot = i
  next i
  if slot < 0 then slot = 0
  key.nr(slot) = kd
  key.once = TRUE
end function

' once per pass: forget one-shot keys that have been released
sub key.nrPoll()
  local i
  for i = 0 to KEY_NR - 1
    if key.nr(i) <> 0 then
      if not key.held(key.nr(i)) then key.nr(i) = 0
    end if
  next i
end sub

' after the pause dialog: forget held-key state, but mark the one-shot keys
' that are still held as used, so their OS repeats do nothing until they are
' released (a Q held through the dialog must not hold a piece on resume)
sub key.resume()
  key.reset()
  key.markHeld(K_UP, 0)
  key.markHeld(K_SPACE, 1)
  key.markHeld(asc("x"), 2)
  key.markHeld(asc("z"), 3)
  key.markHeld(asc("q"), 4)
end sub

sub key.markHeld(kd, slot)
  if key.held(kd) then key.nr(slot) = kd
end sub
