' input.inc - keyboard (classic CMM2 key codes)
'
' Menus, dialogs and name entry read INKEY$ directly in a simple loop:
'   do : k$ = inkey$ : if k$ <> "" then (handle it) else pause 5 : loop
' No ON KEY handler, no queue, no flushing: every press is handled once, in order.
'
' In play two interrupt handlers take over instead (below and in game.inc): an
' ON KEY handler sees every key typed, a timer tick moves the player on a beat.

' throw away typed-ahead keys (used once when a panel opens, so arrows still
' buffered from play do not land in it)
sub key.drain()
  do while inkey$ <> ""
  loop
end sub

function key.isEnter(k$)
  key.isEnter = (k$ = chr$(K_ENTER) or k$ = chr$(K_LF))
end function

' P / Esc pause the game, Q quits from the pause dialog
function key.isPause(k$)
  key.isPause = (k$ = chr$(K_ESC) or k$ = "p" or k$ = "P")
end function

function key.isQuit(k$)
  key.isQuit = (k$ = "q" or k$ = "Q")
end function

' ---- arrows during play
' While a level runs (and only then) two handlers do the input:
'  key.onKey (ON KEY): runs for every key typed. An arrow press is queued, at
'    most KQ_MAX deep, so two quick taps give two turns / steps, and it becomes
'    key.held. P / Esc ask the main loop for the pause (key.pause).
'    Holding a key makes the OS type it again and again; such a repeat of an
'    arrow that is still held is dropped (the game does its own repeat).
'  key.poll (from the timer tick, game.inc): ON KEY never reports a key going
'    up, so the held arrows are checked against KEYDOWN to notice releases.
' KEYDOWN(0) = number of keys held, KEYDOWN(1..n) = their codes (CMM2).
' Directions: 0 up, 1 down, 2 left, 3 right (= code - K_UP).
const KQ_MAX = 2
dim key.ah(3)              ' arrow held, as far as the game knows
dim key.q(KQ_MAX - 1), key.qn
dim key.held = -1, key.t0  ' the last arrow pressed and still held, and when
dim key.pause              ' P / Esc typed
' scratch for key.poll / key.isDown, which run from the tick 200 times a
' second: LOCAL costs O(variables) per call in mmcore
dim integer key_i, key_j

sub key.onKey()
  local k$, d
  do
    k$ = inkey$
    if k$ = "" then exit do
    d = asc(k$) - K_UP
    if d >= 0 and d <= 3 then
      if not (key.ah(d) and key.isDown(d)) then key.press(d)
    elseif key.isPause(k$) then
      key.pause = TRUE
    end if
  loop
end sub

sub key.press(d)
  if key.qn < KQ_MAX then
    key.q(key.qn) = d
    key.qn = key.qn + 1
  end if
  key.ah(d) = TRUE
  key.held = d
  key.t0 = timer
end sub

function key.isDown(d)
  key.isDown = FALSE
  for key_i = 1 to keydown(0)
    if keydown(key_i) = K_UP + d then key.isDown = TRUE
  next key_i
end function

' releases: drop arrows that are no longer down; if the held arrow went up,
' fall back to another arrow that is still held
sub key.poll()
  for key_j = 0 to 3
    if key.ah(key_j) then
      if not key.isDown(key_j) then key.ah(key_j) = FALSE
    end if
  next key_j
  if key.held >= 0 then
    if not key.ah(key.held) then
      key.held = -1
      for key_j = 0 to 3
        if key.ah(key_j) then key.held = key_j
      next key_j
    end if
  end if
end sub

' next queued tap (-1 none)
function key.pop()
  local i
  key.pop = -1
  if key.qn = 0 then exit function
  key.pop = key.q(0)
  for i = 1 to key.qn - 1
    key.q(i - 1) = key.q(i)
  next i
  key.qn = key.qn - 1
end function

' before the handlers start (level start, after a pause, a death): nothing
' typed so far counts, and arrows still held only count once they are
' released and pressed again
sub key.reset()
  local j
  key.drain()
  for j = 0 to 3
    key.ah(j) = key.isDown(j)
  next j
  key.qn = 0
  key.held = -1
  key.pause = FALSE
end sub
