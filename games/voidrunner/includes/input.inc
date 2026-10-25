' input.inc - keyboard (classic CMM2 key codes)
'
' Menus, dialogs and name entry read INKEY$ directly in a simple loop:
'   do : k$ = inkey$ : if k$ <> "" then (handle it) else pause 5 : loop
' No ON KEY handler, no queue, no flushing: every press is handled once, in order.
'
' In play the thruster is read from KEYDOWN instead (below), so holding a key
' gives exactly one thrust and OS auto-repeat never fires extra ones.

' throw away typed-ahead keys (used once: when the game-over panel opens, so a
' Space still bouncing on the thrust key does not skip it)
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

' ---- thruster: Space, Up or W, on the KEYDOWN rising edge
' KEYDOWN(0) = number of keys held, KEYDOWN(1..n) = their codes (CMM2 semantics).
' key.was = a thrust key was held at the previous scan.
dim key.was = FALSE

function key.thrustHeld()
  local i, k
  key.thrustHeld = FALSE
  for i = 1 to keydown(0)
    k = keydown(i)
    if k = K_SPACE or k = K_UP or k = asc("w") or k = asc("W") then key.thrustHeld = TRUE
  next i
end function

' TRUE once per press: a thrust key went down since the last scan
function key.scan()
  local now
  now = key.thrustHeld()
  key.scan = (now and not key.was)
  key.was = now
end function

' a key still held now (e.g. from the menu or the pause dialog) only counts
' once it has been released and pressed again
sub key.thrustReset()
  key.was = key.thrustHeld()
end sub
