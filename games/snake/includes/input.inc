' input.inc - keyboard input through ON KEY (classic CMM2 key codes)
'
' key.isr is the ON KEY interrupt handler: it only moves every waiting INKEY$
' code into a small ring buffer and returns (no drawing, no logic). The loops
' drain the buffer with key.get(), so a press that arrives while the screen is
' being redrawn is queued instead of lost. The handler writes key.w only and the
' main code moves key.r only. One slot stays free (full/empty). The queue serves
' the menus, name entry and P / Esc / Q; in-game steering uses KEYDOWN (below).

const KEYQ = 32
dim key.q(KEYQ - 1)
dim key.r = 0, key.w = 0
dim key.on = FALSE

sub key.isr()
  local k$, nw
  k$ = inkey$
  do while k$ <> ""
    nw = (key.w + 1) mod KEYQ
    if nw <> key.r then
      key.q(key.w) = asc(k$)
      key.w = nw
    end if
    k$ = inkey$
  loop
end sub

sub key.start()
  key.r = key.w
  on key key.isr
  key.on = TRUE
end sub

sub key.stop()
  on key 0
  key.on = FALSE
end sub

' next key code from the queue, 0 if none
function key.get()
  local k$
  if key.r <> key.w then
    key.get = key.q(key.r)
    key.r = (key.r + 1) mod KEYQ
    exit function
  end if
  key.get = 0
  if key.on then exit function
  k$ = inkey$
  if k$ <> "" then key.get = asc(k$)
end function

function key.waiting()
  key.waiting = (key.r <> key.w)
end function

' throw away anything typed so far
sub key.flush()
  local t = timer
  do while inkey$ <> "" and timer - t < 500
  loop
  key.r = key.w
  key.arrowReset()
end sub

' ---- arrows during play are read from KEYDOWN, not from the INKEY$ queue.
' key.scan() runs once per loop pass and lists every arrow that went down since
' the previous pass (a rising edge) in key.ev(0 .. key.evn - 1), in KEYDOWN
' order, so the newest press comes last. A held key counts once; OS auto-repeat
' never reaches KEYDOWN. key.adn(i) = arrow i (0 Up, 1 Down, 2 Left, 3 Right)
' was held at the last scan. The loops drop arrow codes from the ON KEY queue.
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

function key.isEnter(k)
  key.isEnter = (k = K_ENTER or k = K_LF)
end function

function key.isArrow(k)
  key.isArrow = (k >= K_UP and k <= K_RIGHT)
end function

' P / Esc pause the game, Q quits from the pause dialog
function key.isPause(k)
  key.isPause = (k = K_ESC or k = asc("p") or k = asc("P"))
end function
