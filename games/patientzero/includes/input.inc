' input.inc
' Menus and name entry read INKEY$. Play samples KEYDOWN once per tick,
' so a move ends on the frame the key is released.

const K_UP = 128
const K_DOWN = 129
const K_LEFT = 130
const K_RIGHT = 131
const K_ENTER = 10
const K_LF = 13
const K_ESC = 27
const K_BKSP = 8
const K_DEL = 127
const K_Z = 122
const K_V = 118
const K_P = 112
const K_Q = 113

dim key.left, key.right, key.up, key.dn
dim key.z, key.sneeze, key.pause
dim key.wasV, key.wasP
dim key_i, key_n, key_k
dim string key_s$

sub key.drain()
  do while inkey$ <> ""
  loop
end sub

function key.isDown(code)
  key.isDown = FALSE
  key_n = keydown(0)
  for key_i = 1 to key_n
    if keydown(key_i) = code then key.isDown = TRUE
  next key_i
end function

sub key.reset()
  key.left = FALSE
  key.right = FALSE
  key.up = FALSE
  key.dn = FALSE
  key.z = FALSE
  key.sneeze = FALSE
  key.pause = FALSE
  key.drain()
  key.wasV = FALSE
  key.wasP = FALSE
  if key.isDown(K_V) then key.wasV = TRUE
  if key.isDown(K_ESC) or key.isDown(K_P) or key.isDown(80) then key.wasP = TRUE
end sub

' Held arrows and Z come from KEYDOWN. V and pause also latch from INKEY$,
' because a harness tap is a press and a release in the same step.
sub key.logic()
  key.left = key.isDown(K_LEFT)
  key.right = key.isDown(K_RIGHT)
  key.up = key.isDown(K_UP)
  key.dn = key.isDown(K_DOWN)
  key.z = key.isDown(K_Z)
  key_k = 0
  if key.isDown(K_V) then key_k = 1
  if key_k = 1 and key.wasV = 0 then key.sneeze = TRUE
  key.wasV = key_k
  key_k = 0
  if key.isDown(K_ESC) or key.isDown(K_P) or key.isDown(80) then key_k = 1
  if key_k = 1 and key.wasP = 0 then key.pause = TRUE
  key.wasP = key_k
end sub

sub key.onKey()
  do
    key_s$ = inkey$
    if key_s$ = "" then
      exit do
    end if
    key_k = asc(key_s$)
    if key_k = K_ESC or key_k = K_P or key_k = 80 then key.pause = TRUE
    if key_k = K_V or key_k = 86 then key.sneeze = TRUE
  loop
end sub

function key.enter(k$)
  key.enter = FALSE
  if k$ = chr$(K_ENTER) or k$ = chr$(K_LF) then key.enter = TRUE
end function
