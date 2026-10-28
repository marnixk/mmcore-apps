' input.inc
' Menus and name entry read INKEY$ in a loop.
' Play uses ON KEY plus SETTICK. The tick never draws.
' mmcore KEYDOWN does not report Alt, Ctrl, Insert or Delete as held keys,
' so jet is Space and 0, and the phaser is Enter and Z. A tap still latches
' a short burst, because the harness key command is a tap rather than a hold.

dim key.pause
dim key.left, key.right, key.up, key.dn, key.jet, key.phase
dim key.prevJ, key.prevD
dim key.bL, key.bR, key.bU, key.bD, key.bJ, key.bP
dim integer key_i
dim string ko_k$

sub key.drain()
  do while inkey$ <> ""
  loop
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
  key.bL = 0
  key.bR = 0
  key.bU = 0
  key.bD = 0
  key.bJ = 0
  key.bP = 0
end sub

function key.isDown(code)
  key.isDown = FALSE
  for key_i = 1 to keydown(0)
    if keydown(key_i) = code then key.isDown = TRUE
  next key_i
end function

' once per logic frame
sub key.logic()
  key.left = FALSE
  key.right = FALSE
  key.up = FALSE
  key.dn = FALSE
  key.jet = FALSE
  key.phase = FALSE
  if key.isDown(K_LEFT) or key.bL > 0 then key.left = TRUE
  if key.isDown(K_RIGHT) or key.bR > 0 then key.right = TRUE
  if key.isDown(K_UP) or key.bU > 0 then key.up = TRUE
  if key.isDown(K_DOWN) or key.bD > 0 then key.dn = TRUE
  if key.isDown(K_SPACE) or key.isDown(K_0) or key.bJ > 0 then key.jet = TRUE
  if key.isDown(K_ENTER) or key.isDown(K_Z) or key.isDown(90) or key.bP > 0 then key.phase = TRUE
  if key.bL > 0 then key.bL = key.bL - 1
  if key.bR > 0 then key.bR = key.bR - 1
  if key.bU > 0 then key.bU = key.bU - 1
  if key.bD > 0 then key.bD = key.bD - 1
  if key.bJ > 0 then key.bJ = key.bJ - 1
  if key.bP > 0 then key.bP = key.bP - 1
end sub

sub key.latch(k)
  if k = K_LEFT then key.bL = 14
  if k = K_RIGHT then key.bR = 14
  if k = K_UP then key.bU = 14
  if k = K_DOWN then key.bD = 14
  if k = K_SPACE or k = K_0 or k = K_INS then key.bJ = 20
  if k = K_ENTER or k = K_LF or k = K_Z or k = 90 or k = K_DEL then key.bP = 32
  if k = K_ESC or k = 112 or k = 80 then key.pause = TRUE
end sub

sub key.onKey()
  do
    ko_k$ = inkey$
    if ko_k$ = "" then
      exit do
    end if
    key.latch(asc(ko_k$))
  loop
end sub
