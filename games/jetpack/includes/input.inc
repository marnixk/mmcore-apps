' input.inc
' Menus, dialogs and name entry read INKEY$ in a loop.
' In play, movement and jet are the KEYDOWN held set, sampled once per
' logic tick. Releasing a key clears the action on the next tick.
' The phaser is the rising edge of Enter or Z (one shot per press).
' ON KEY only drains INKEY$, so OS auto-repeat cannot drive movement.
' mmcore KEYDOWN does not report Alt, Ctrl, Insert or Delete as held
' keys. Jet is Space and 0. Phaser is Enter and Z.

dim key.pause
dim key.left, key.right, key.up, key.dn, key.jet, key.phase
dim key.prevJ, key.prevD
dim key.wasP, key.wasQ
dim integer key_i, key_down

sub key.drain()
  do while inkey$ <> ""
  loop
end sub

sub key.sampleEdges()
  key.wasP = 0
  if key.isDown(K_ENTER) or key.isDown(K_Z) then key.wasP = 1
  key.wasQ = 0
  if key.isDown(K_ESC) or key.isDown(112) or key.isDown(80) then key.wasQ = 1
end sub

sub key.reset()
  key.pause = FALSE
  key.left = FALSE
  key.right = FALSE
  key.up = FALSE
  key.dn = FALSE
  key.jet = FALSE
  key.phase = FALSE
  key.prevJ = FALSE
  key.prevD = FALSE
  key.drain()
  key.sampleEdges()
end sub

function key.isDown(code)
  key.isDown = FALSE
  for key_i = 1 to keydown(0)
    if keydown(key_i) = code then key.isDown = TRUE
  next key_i
end function

' once per logic frame. Held state is KEYDOWN only.
sub key.logic()
  key.drain()
  key.left = key.isDown(K_LEFT)
  key.right = key.isDown(K_RIGHT)
  key.up = key.isDown(K_UP)
  key.dn = key.isDown(K_DOWN)
  key.jet = FALSE
  if key.isDown(K_SPACE) or key.isDown(K_0) then key.jet = TRUE
  key_down = 0
  if key.isDown(K_ENTER) or key.isDown(K_Z) then key_down = 1
  key.phase = FALSE
  if key_down = 1 and key.wasP = 0 then key.phase = TRUE
  key.wasP = key_down
  key_down = 0
  if key.isDown(K_ESC) or key.isDown(112) or key.isDown(80) then key_down = 1
  if key_down = 1 and key.wasQ = 0 then key.pause = TRUE
  key.wasQ = key_down
end sub

' Play's ON KEY handler. The buffer is discarded. It never latches a move.
sub key.onKey()
  key.drain()
end sub
