#!/usr/bin/env python3
"""Prepare low-resolution MPEG-1 test files for the DSi player prototype.

Requires FFmpeg on the computer. This does NOT run on the Nintendo DSi and
does not make the current ROM capable of playback; it creates test media for
the future decoder integration.
"""
from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
from pathlib import Path


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path, help="input MP4 (or another FFmpeg-supported file)")
    parser.add_argument("-o", "--output", type=Path, help="output MPEG-1 program stream (.mpg)")
    args = parser.parse_args()

    ffmpeg = shutil.which("ffmpeg")
    if not ffmpeg:
        print("Errore: FFmpeg non trovato. Installalo e riprova.", file=sys.stderr)
        return 2
    source = args.input.expanduser()
    if not source.is_file():
        print(f"Errore: file non trovato: {source}", file=sys.stderr)
        return 2
    output = args.output or source.with_suffix(".mpg")
    output = output.expanduser()
    output.parent.mkdir(parents=True, exist_ok=True)

    command = [
        ffmpeg, "-hide_banner", "-y", "-i", str(source),
        "-map", "0:v:0", "-map", "0:a:0?",
        "-vf", "scale=256:144:force_original_aspect_ratio=decrease,pad=256:144:(ow-iw)/2:(oh-ih)/2,setsar=1,fps=15",
        "-c:v", "mpeg1video", "-b:v", "350k", "-maxrate", "500k",
        "-bufsize", "500k", "-g", "15", "-pix_fmt", "yuv420p",
        "-c:a", "mp2", "-b:a", "64k", "-ar", "22050", "-ac", "1",
        "-f", "mpeg", str(output),
    ]
    print("Conversione su PC (non sulla console):")
    print(" ".join(command))
    result = subprocess.run(command, check=False)
    if result.returncode == 0:
        print(f"Creato: {output}")
        print("Nota: la ROM attuale non riproduce ancora questo file.")
    return result.returncode


if __name__ == "__main__":
    raise SystemExit(main())
