' music.inc - sound is OFF: the game is completely silent.
'
' Marnix plays with JUKE running in the background. mmcore has one global audio
' player, so any PLAY command from the game would stop or change JUKE's track.
' With both switches at 0 the game issues no PLAY or SOUND command at all.
' sfx/ is kept (empty) for structure, like the other games.
const MUSIC_ON = 0
const SFX_ON = 0

sub music.start()
  if MUSIC_ON = 0 then exit sub
end sub

sub music.stop()
  if MUSIC_ON = 0 then exit sub
  play stop
end sub
