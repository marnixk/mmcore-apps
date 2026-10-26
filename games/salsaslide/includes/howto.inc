' howto.inc - HOW TO PLAY, from the title menu: two full-screen pages
' (gfx/howto1_<res>.png, gfx/howto2_<res>.png; the text and the little
' scenes are baked in by art/howto_v3.py, so every size fills its screen).
' Left / Right turn the page, Enter / Space go on (on the last page: back),
' Esc / Q go back to the menu. A plain INKEY$ loop, one action per press.
const HT_PAGES = 2
dim ht.pg
dim string ht_k$

' page n onto the back page, then one PAGE COPY to the screen (no flicker)
sub howto.show(n)
  scr.loadScn("howto" + str$(n))
  page copy PG_SCN to PG_BACK
  scr.present()
end sub

sub howto.run()
  ht.pg = 1
  howto.show(ht.pg)
  key.drain()
  do
    ht_k$ = inkey$
    if ht_k$ = "" then
      pause 5
    else
      select case asc(ht_k$)
        case K_LEFT
          if ht.pg > 1 then
            ht.pg = ht.pg - 1
            howto.show(ht.pg)
          end if
        case K_RIGHT
          if ht.pg < HT_PAGES then
            ht.pg = ht.pg + 1
            howto.show(ht.pg)
          end if
        case K_ENTER, K_LF, 32
          if ht.pg >= HT_PAGES then exit do
          ht.pg = ht.pg + 1
          howto.show(ht.pg)
        case K_ESC, asc("q"), asc("Q")
          exit do
      end select
    end if
  loop
end sub
