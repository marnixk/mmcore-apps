' input.inc - keyboard (classic CMM2 key codes)
'
' Menus, dialogs and name entry read INKEY$ directly in a simple loop:
'   do : k$ = inkey$ : if k$ <> "" then (handle it) else pause 5 : loop
' No ON KEY handler, no queue, no flushing: every press is handled once, in order.
'
' In play two interrupt handlers take over instead (below and in game.inc): an
' ON KEY handler sees every key typed, a timer tick acts on it.

' throw away typed-ahead keys (used once when a panel opens)
sub key.drain()
  do while inkey$ <> ""
  loop
end sub

function key.isEnter(k$)
  key.isEnter = (k$ = chr$(K_ENTER) or k$ = chr$(K_LF))
end function

function key.isPause(k$)
  key.isPause = (k$ = chr$(K_ESC) or k$ = "p" or k$ = "P")
end function

function key.isQuit(k$)
  key.isQuit = (k$ = "q" or k$ = "Q")
end function

function key.isRestart(k$)
  key.isRestart = (k$ = "r" or k$ = "R")
end function

' ---- keys during play
' While a level runs (and only then) two handlers do the input:
'  key.onKey (ON KEY): runs for every key typed. Arrows, Space / Enter (pick)
'    and Esc go into a small queue (KQ_MAX deep, so quick taps all count); an
'    arrow also becomes key.held. P asks the main loop for the pause.
'    Holding a key makes the OS type it again and again; such a repeat of an
'    arrow that is still held is dropped (the game does its own repeat).
'  key.poll (from the timer tick, game.inc): ON KEY never reports a key going
'    up, so the held arrows are checked against KEYDOWN to notice releases.
' KEYDOWN(0) = number of keys held, KEYDOWN(1..n) = their codes (CMM2).
' Queue codes: 0 up, 1 down, 2 left, 3 right (= code - K_UP), KC_PICK, KC_ESC.
const KQ_MAX = 3, KC_PICK = 4, KC_ESC = 5
dim key.ah(3)              ' arrow held, as far as the game knows
dim key.q(KQ_MAX - 1), key.qn
dim key.held = -1, key.t0  ' the last arrow pressed and still held, and when
dim key.pause              ' P typed (or Esc with nothing picked, set by the tick)
dim key.cmd                ' R typed (1: restart the level) or Q (2: quit to the title)
' scratch for key.poll / key.isDown, which run from the tick 200 times a
' second: LOCAL costs O(variables) per call in mmcore
dim integer key_i, key_j, key_d
dim string key_k$ length 4

sub key.onKey()
  do
    key_k$ = inkey$
    if key_k$ = "" then exit do
    key_d = asc(key_k$) - K_UP
    if key_d >= 0 and key_d <= 3 then
      if not (key.ah(key_d) and key.isDown(key_d)) then key.press(key_d)
    elseif key_k$ = " " or key.isEnter(key_k$) then
      key.push(KC_PICK)
    elseif key_k$ = chr$(K_ESC) then
      key.push(KC_ESC)
    elseif key_k$ = "p" or key_k$ = "P" then
      key.pause = TRUE
    elseif key_k$ = "r" or key_k$ = "R" then
      key.cmd = 1
    elseif key_k$ = "q" or key_k$ = "Q" then
      key.cmd = 2
    end if
  loop
end sub

sub key.push(c)
  if key.qn < KQ_MAX then
    key.q(key.qn) = c
    key.qn = key.qn + 1
  end if
end sub

sub key.press(d)
  key.push(d)
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

' next queued key (-1 none)
function key.pop()
  key.pop = -1
  if key.qn = 0 then
    exit function
  end if
  key.pop = key.q(0)
  for key_j = 1 to key.qn - 1
    key.q(key_j - 1) = key.q(key_j)
  next key_j
  key.qn = key.qn - 1
end function

' before the handlers start (level start, after a pause): nothing typed so far
' counts, and arrows still held only count once released and pressed again
sub key.reset()
  local j
  key.drain()
  for j = 0 to 3
    key.ah(j) = key.isDown(j)
  next j
  key.qn = 0
  key.held = -1
  key.pause = FALSE
  key.cmd = 0
end sub
