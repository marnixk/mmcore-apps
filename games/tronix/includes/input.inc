' input.inc - keyboard (classic CMM2 key codes)
'
' Menus, dialogs and name entry read INKEY$ directly in a simple loop:
'   do : k$ = inkey$ : if k$ <> "" then (handle it) else pause 5 : loop
' No ON KEY handler, no queue, no flushing: every press is handled once, in order.
'
' In play the arrows are read from KEYDOWN instead (below): a press counts on
' its rising edge, OS auto-repeat never reaches KEYDOWN, and the game does its
' own smooth repeat for a held arrow on claimed land.

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

' ---- arrows during play: KEYDOWN(0) = number of keys held, KEYDOWN(1..n) =
' their codes (CMM2 semantics). key.scan() runs once per loop pass:
'  - every arrow that went down since the previous pass (a rising edge) is
'    queued, at most KQ_MAX deep, so two quick taps give two turns / steps
'  - key.held = the most recently pressed arrow that is still held (-1 none)
' Directions: 0 up, 1 down, 2 left, 3 right (= code - K_UP).
const KQ_MAX = 2
dim key.adn(3), key.ah(3), key.q(KQ_MAX - 1), key.qn, key.held = -1
dim key.t0   ' time the held arrow went down

sub key.scan()
  local i, j, n, k
  for j = 0 to 3
    key.ah(j) = FALSE
  next j
  n = keydown(0)
  for i = 1 to n
    k = keydown(i)
    if k >= K_UP and k <= K_RIGHT then key.ah(k - K_UP) = TRUE
  next i
  for j = 0 to 3
    if key.ah(j) and not key.adn(j) then
      if key.qn < KQ_MAX then
        key.q(key.qn) = j
        key.qn = key.qn + 1
      end if
      key.held = j
      key.t0 = timer
    end if
    key.adn(j) = key.ah(j)
  next j
  if key.held >= 0 then
    if not key.ah(key.held) then
      key.held = -1
      for j = 0 to 3
        if key.ah(j) then key.held = j
      next j
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

' after a pause, a death or a panel: arrows still held only count once they
' are released and pressed again, and nothing stays queued
sub key.reset()
  local i, n, k
  for i = 0 to 3
    key.adn(i) = FALSE
  next i
  n = keydown(0)
  for i = 1 to n
    k = keydown(i)
    if k >= K_UP and k <= K_RIGHT then key.adn(k - K_UP) = TRUE
  next i
  key.qn = 0
  key.held = -1
end sub
