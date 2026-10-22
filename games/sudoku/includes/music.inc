' music.inc - sound, switched OFF for now.
'
' Music and sound effects are disabled so the game can run alongside JUKE.
' mmcore has one global audio player: PLAY MODFILE, PLAY WAV/EFFECT, PLAY STOP
' and PLAY VOLUME from the game would stop or change JUKE's track. With both
' flags at 0 the game issues no PLAY command at all.
'
' To bring the forest music back later set MUSIC_ON = 1 (sfx/grove.mod is kept
' on disk; mmcore plays a module once, so music.poll() restarts it when
' PLAYING() drops to 0). SFX_ON = 1 re-enables the win jingle (sfx/win.wav).
const MUSIC_ON = 0
const SFX_ON = 0
const MUSIC_FILE$ = "sfx/grove.mod"
const MUSIC_VOL = 70
dim mus.on = FALSE

sub music.start()
  if MUSIC_ON = 0 then exit sub
  play modfile MUSIC_FILE$
  play volume MUSIC_VOL, MUSIC_VOL
  mus.on = TRUE
end sub

sub music.poll()
  if MUSIC_ON = 0 then exit sub
  if mus.on then
    if playing() = 0 then music.start()
  end if
end sub

sub music.stop()
  if MUSIC_ON = 0 then exit sub
  mus.on = FALSE
  play stop
end sub

' one-shot jingle (stops the music: single audio voice)
sub music.jingle(f$)
  if SFX_ON = 0 then exit sub
  if MUSIC_ON then
    mus.on = FALSE
    play stop
  end if
  play wav f$
end sub
