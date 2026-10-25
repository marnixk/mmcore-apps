' input.inc - keyboard (classic CMM2 key codes)
'
' Menus, dialogs and name entry read INKEY$ directly in a simple loop:
'   do : k$ = inkey$ : if k$ <> "" then (handle it) else pause 5 : loop
' No ON KEY handler, no queue, no flushing: every press is handled once, in order.
'
' In play the arrows are read from KEYDOWN instead (below), so a held arrow
' turns once and OS auto-repeat never queues extra turns.

' throw away typed-ahead keys (used once: when the game-over panel opens, so
' arrows still buffered from play do not land in the name entry)
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

' ---- arrows during play are read from KEYDOWN, not from the INKEY$ queue.
' key.scan() runs once per loop pass and lists every arrow that went down since
' the previous pass (a rising edge) in key.ev(0 .. key.evn - 1), in KEYDOWN
' order, so the newest press comes last. A held key counts once; OS auto-repeat
' never reaches KEYDOWN. key.adn(i) = arrow i (0 Up, 1 Down, 2 Left, 3 Right)
' was held at the last scan. Arrow codes that also reach INKEY$ are ignored.
dim key.adn(3), key.ah(3), key.ev(3), key.evn

' forget held arrows: one still held after this counts as a fresh press
sub key.arrowReset()
  local i
  for i = 0 to 3
    key.adn(i) = FALSE
  next i
  key.evn = 0
end sub

sub key.scan()
  local i, j, n, k
  key.evn = 0
  for j = 0 to 3
    key.ah(j) = FALSE
  next j
  n = keydown(0)
  for i = 1 to n
    k = keydown(i)
    if k >= K_UP and k <= K_RIGHT then
      j = k - K_UP
      if not key.ah(j) then
        key.ah(j) = TRUE
        if not key.adn(j) then
          if key.evn < 4 then
            key.ev(key.evn) = k
            key.evn = key.evn + 1
          end if
        end if
      end if
    end if
  next i
  for j = 0 to 3
    key.adn(j) = key.ah(j)
  next j
end sub
