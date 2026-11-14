# Video pipeline (development note)

## Current status

The ROM is currently a storage/browser prototype: it mounts the SD card and
lists MP4 files in `/video`. It does **not** decode or play video. In
particular, it does not convert MP4 on the DSi.

## Why conversion is initially hosted on a computer

The DSi has a very small CPU/memory budget for modern MP4 codecs such as
H.264/AAC. A reliable general-purpose MP4-to-playable conversion on the
console is not a realistic first milestone. The practical route is to convert
on a PC and copy a constrained low-resolution stream to the SD card.

## Test-media preset

`tools/convert_video.py` uses FFmpeg to create a 256x144, 15 fps MPEG-1
program stream with mono MP2 audio. This is a **test format candidate**, not a
claim that the DSi ROM can decode it yet. The next engineering milestone is
to integrate and benchmark an actual decoder on hardware/emulator before
choosing the final container and bitrate.

Example:

```sh
python3 tools/convert_video.py movie.mp4
# writes movie.mpg beside the source
```

Copy the resulting file to the SD card only for decoder testing. The current
ROM will not play it.

## Next milestones

1. Build and verify the ROM with devkitPro in GitHub Actions.
2. Add a decoder proof-of-concept for a tiny MPEG-1 sample.
3. Measure frame rate and audio sync on a real DSi.
4. Only then wire playback into the file browser and design the end-to-end
   workflow. Do not promise arbitrary MP4 support.
