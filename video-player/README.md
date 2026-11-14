# DSi Video Player

A standalone DSi video-player project inside dsi-craft.

## Playback engine

The ROM build currently packages **Rocket Video Player** as the real playback engine. This is a deliberate step beyond the original mock UI: it produces an actual player rather than pretending the placeholder buttons decode video. The upstream engine is pinned in the workflow to commit `50803b9027ca43ab278c1486442fcfc70c90b309` and retains its original MIT license and credits.

- Supported video container: `.rvid` (not MP4 and not FastVideoDS `.fv`).
- Video playback supports audio and touch controls.
- The upstream player provides its own file browser and playback interface.
- The custom interface in `source/main.c` (upper-screen output, lower touchscreen library, PLAY/PAUSE/STOP and START-to-list) is still a separate prototype and is **not yet wired to the decoder**.

## Convert videos

MP4 files must be converted to Rocket Video `.rvid` format with [Vid2RVID](https://github.com/RocketRobz/Vid2RVID/releases). Simply renaming an MP4 file does not convert it. The converter is a desktop tool; an iPhone-only conversion workflow has not yet been implemented.

Put converted `.rvid` files in the `/videos` folder on the SD card. Launch the ROM from TWiLight Menu++.

## Controls in the current playback-engine build

- A / touch Play-Pause: play or pause.
- L / touch stop area: stop.
- B / touch Return: exit the video and return to the file browser.
- The file browser lets you choose the next video.

The requested custom behavior (START returning to the file list, dedicated lower-screen controls and upper-screen-only video) remains the next integration task.

## Build

GitHub Actions builds the pinned upstream engine with the BlocksDS SDK Docker image and publishes an artifact named `DSi-Video-Player-ROM`. The ROM filename is `DSiVideoPlayer.nds`.
