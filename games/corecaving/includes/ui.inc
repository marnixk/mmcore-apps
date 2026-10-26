' ui.inc - small drawing helpers. All coordinates are UI ART units; af()
' scales to screen pixels. Colours C_* come from the art palette in layouts.inc.

' filled rectangle in art units
' scratch for the sub below (globals: LOCAL clears mmcore's fast-path cache)
dim af_s
sub af(x, y, w, h, c)
  af_s = scr.sc
  if w > 0 and h > 0 then box x * af_s, y * af_s, w * af_s, h * af_s, 1, c, c
end sub

' an image from the UI block (art position sx, sy from the UI origin, size
' w x h) at art x, y; black = see-through
' scratch for the sub below (globals: LOCAL clears mmcore's fast-path cache)
dim us_s
sub ui.spr(sx, sy, w, h, x, y)
  us_s = scr.sc
  blit sx * us_s, (lay.uy + sy) * us_s, x * us_s, y * us_s, w * us_s, h * us_s, PG_SHEET, 4
end sub

' highlighted row bar (menu and score table): warm torch-lit stone
sub ui.bar(x, y, w)
  af x, y, w, 17, C_T0
  af x, y, w, 1, C_T2
  af x, y + 16, w, 1, C_K
end sub

' a dialog tablet PNG (gfx/<name>_<res>.png, w x h tablet + 2 px margin)
' centred on the back page; alpha-0 pixels are skipped, so the dimmed scene
' shows round it. Returns the tablet's left edge, its top in ui.py
dim ui.py
function ui.dialog(n$, w, h)
  local x
  x = (lay.aw - w) \ 2
  ui.py = (lay.ah - h) \ 2
  page write PG_BACK
  load png "gfx/" + n$ + "_" + lay.res$ + ".png", (x - 2) * scr.sc, (ui.py - 2) * scr.sc
  ui.dialog = x
end function
