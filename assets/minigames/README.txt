# Mini-game assets

Each game has a named folder. Put a PCM WAV named `music.wav` in its
`level-01`, `level-02`, etc. folder. Pulse Runner has ten stages; Crazy Fish
can continue beyond the supplied twelve folders (create another level folder).
Other games currently use level-01. Tracks loop, decode in the background,
and follow OS audio and master volume settings. Missing tracks stay silent.
Maximum source size: 32 MB; converted audio is capped at 64 MB.

On the handheld these folders live under
`/userdata/system/snapos/assets/minigames/`. On your Linux PC they live under
`~/snapos-ui/assets/minigames/`. You can keep artwork alongside the music;
the current game scenery is drawn by the game and does not load arbitrary images.
