# DSi Video Player

Standalone Nintendo DS/DSi homebrew subproject. It is separate from the DSi Craft game.

## Intended design

- Video image on the **upper** screen.
- Touch controls and file selection on the **lower** screen.
- START returns to/rescans the video library.
- PLAY, PAUSE, STOP and RESCAN touch targets.
- Files are read from `sd:/videos`.

## Current status

This first commit is a **buildable interface and SD file-browser prototype**, not yet a functioning video player. The playback decoder is deliberately not represented as working: an actual FastVideoDS decoder/output backend still needs to be integrated and tested on hardware.

The DSi does not decode arbitrary MP4 files natively. The intended pipeline is MP4 on iPhone/another device -> compatible FastVideoDS stream -> DSi player. Renaming an MP4 to `.fv` will not convert it.

The prototype currently lists `.fv` files only. It does not play MP4 or FV video yet.

## Build

Requires devkitPro/devkitARM, libnds and libfat:

```sh
cd video-player
make
```

Output: `DSiVideoPlayer.nds`

## SD card layout

Create a `videos` directory at the root of the SD card and put compatible `.fv` files there.
