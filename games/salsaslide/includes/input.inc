' input.inc - keyboard (classic CMM2 key codes)
'
' Menus, dialogs, name entry and the Salsa Shuffle pick read INKEY$ in a
' plain loop:  do : k$ = inkey$ : if k$ <> "" then (handle it) else pause 5 : loop
'
' In play two interrupt handlers take over (here and in game.inc):
'  key.onKey (ON KEY): runs for every key typed. An arrow press is queued (at
'    most KQ_MAX deep, so two quick taps give two moves) and becomes key.held;
'    Space adds a plate to serve (at most KQ_MAX waiting; one plate per
'    press); P / Esc ask the main loop for the pause, R for a restart. The
'    OS repeats a held key; a repeat of a key that is still held is dropped
'    (the game does its own walking repeat).
'  key.poll (from the timer tick): ON KEY never reports a key going up, so
'    the held keys are checked against KEYDOWN to notice releases.
' KEYDOWN(0) = number of keys held, KEYDOWN(1..n) = their codes.
' Directions: 0 up, 1 down, 2 left, 3 right (= code - K_UP).
const KQ_MAX = 2
dim key.ah(3)              ' arrow held, as far as the game knows
dim key.q(KQ_MAX - 1), key.qn
dim key.held = -1, key.t0  ' the last arrow pressed and still held, and when
dim key.sv, key.sh         ' plates asked for, Space held
dim key.pause, key.restart
dim integer key_i, key_j, key_c

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

sub key.onKey()
  local k$, d
  do
    k$ = inkey$
    if k$ = "" then exit do
    d = asc(k$) - K_UP
    if d >= 0 and d <= 3 then
      if not (key.ah(d) and key.isDown(d)) then key.press(d)
    elseif k$ = " " then
      if not (key.sh and key.codeDown(32)) then
        if key.sv < KQ_MAX then key.sv = key.sv + 1
        key.sh = TRUE
      end if
    elseif key.isPause(k$) then
      key.pause = TRUE
    elseif key.isRestart(k$) then
      key.restart = TRUE
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

function key.codeDown(c)
  key.codeDown = FALSE
  for key_i = 1 to keydown(0)
    if keydown(key_i) = c then key.codeDown = TRUE
  next key_i
end function

function key.isDown(d)
  key.isDown = key.codeDown(K_UP + d)
end function

' releases: drop keys that are no longer down; if the held arrow went up,
' fall back to another arrow that is still held
sub key.poll()
  for key_j = 0 to 3
    if key.ah(key_j) then
      if not key.isDown(key_j) then key.ah(key_j) = FALSE
    end if
  next key_j
  if key.sh then
    if not key.codeDown(32) then key.sh = FALSE
  end if
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
  key.pop = -1
  if key.qn = 0 then exit function
  key.pop = key.q(0)
  for key_j = 1 to key.qn - 1
    key.q(key_j - 1) = key.q(key_j)
  next key_j
  key.qn = key.qn - 1
end function

' before the handlers start: nothing typed so far counts, and keys still
' held only count once they are released and pressed again
sub key.reset()
  local j
  key.drain()
  for j = 0 to 3
    key.ah(j) = key.isDown(j)
  next j
  key.sh = key.codeDown(32)
  key.qn = 0
  key.sv = 0
  key.held = -1
  key.pause = FALSE
  key.restart = FALSE
end sub
