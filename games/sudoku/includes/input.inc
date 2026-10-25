' input.inc - keyboard (classic CMM2 key codes)
'
' Menus, dialogs and name entry read INKEY$ directly in a simple loop:
'   do : k$ = inkey$ : if k$ <> "" then (handle it) else pause : loop
' No ON KEY handler, no queue, no flushing: every press is handled once, in
' order, and a held arrow auto-repeats through INKEY$ like any key.
' The game loop reads INKEY$ the same way on every pass; on the board a held
' arrow is repeated from KEYDOWN (below) so the cursor speed does not depend
' on the OS key repeat.

' throw away typed-ahead keys (used once: when the win dialog has been up for
' a moment, so a digit still bouncing from the last move does not skip it)
sub key.drain()
  do while inkey$ <> ""
  loop
end sub

' forget the held arrow (game start, leaving the pause dialog)
sub key.reset()
  key.rpt = 0
end sub

function key.isEnter(k$)
  key.isEnter = (k$ = chr$(K_ENTER) or k$ = chr$(K_LF))
end function

function key.isQuit(k$)
  key.isQuit = (k$ = "q" or k$ = "Q")
end function

' ---- held arrows: auto-repeat driven by KEYDOWN, independent of OS repeat
' KEYDOWN(0) = number of keys held, KEYDOWN(1..n) = their CMM2 codes
' (arrows 128-131, the same values INKEY$ gives).
' The first press of an arrow arrives through INKEY$ and moves at once; while
' that arrow stays held, key.repeat() hands it out again after KEY_RDELAY ms
' and then every KEY_RRATE ms. INKEY$ copies of the same arrow
' that arrive while it is held past the first moment are OS key-repeats and
' are dropped, so nothing moves twice.
const KEY_RDELAY = 250, KEY_RRATE = 70, KEY_OSREP = 150
dim key.rpt = 0, key.rptStart, key.rptNext

' TRUE when the key with CMM2 code k is held right now
function key.held(k)
  local i
  key.held = FALSE
  for i = 1 to keydown(0)
    if keydown(i) = k then key.held = TRUE
  next i
end function

' an INKEY$ arrow: returns k when it is a fresh press, -1 when it is an OS
' repeat of the arrow we are already auto-repeating
function key.arrowKeep(k)
  if k = key.rpt and timer - key.rptStart >= KEY_OSREP and key.held(k) then
    key.arrowKeep = -1
    exit function
  end if
  key.arrowKeep = k
  key.rpt = k
  key.rptStart = timer
  key.rptNext = timer + KEY_RDELAY
end function

' arrow to move again now because it is still held, 0 if none
function key.repeat()
  key.repeat = 0
  if key.rpt = 0 then exit function
  if not key.held(key.rpt) then
    key.rpt = 0
    exit function
  end if
  if timer < key.rptNext then exit function
  key.repeat = key.rpt
  key.rptNext = key.rptNext + KEY_RRATE
  if key.rptNext < timer then key.rptNext = timer + KEY_RRATE
end function
