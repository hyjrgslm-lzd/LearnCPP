"""Generate deterministic C16 media fixtures under a build directory."""
from __future__ import annotations

import argparse
import math
from pathlib import Path
import struct
import sys
import wave

sys.dont_write_bytecode = True

COURSE = Path(__file__).resolve().parents[1]
SAMPLE_RATE = 48_000
FPS = 10
SECONDS = 1
WIDTH = 64
HEIGHT = 48


def pcm16(samples: int = SAMPLE_RATE * SECONDS) -> bytes:
    data = bytearray()
    for n in range(samples):
        pulse = 20_000 if any(abs(n - center) < 120 for center in (4_800, 24_000, 43_200)) else 0
        tone = int(2_000 * math.sin(2 * math.pi * 440 * n / SAMPLE_RATE))
        value = max(-32768, min(32767, pulse + tone))
        data += struct.pack("<h", value)
    return bytes(data)


def write_wav(path: Path, samples: bytes) -> None:
    with wave.open(str(path), "wb") as out:
        out.setnchannels(1)
        out.setsampwidth(2)
        out.setframerate(SAMPLE_RATE)
        out.writeframes(samples)


def chunk(tag: bytes, payload: bytes) -> bytes:
    return tag + struct.pack("<I", len(payload)) + payload + (b"\0" if len(payload) % 2 else b"")


def list_chunk(kind: bytes, payload: bytes) -> bytes:
    return chunk(b"LIST", kind + payload)


def riff(kind: bytes, payload: bytes) -> bytes:
    return b"RIFF" + struct.pack("<I", 4 + len(payload)) + kind + payload


def video_frame(index: int) -> bytes:
    rows = []
    for y in reversed(range(HEIGHT)):
        row = bytearray()
        for x in range(WIDTH):
            r = (index * 23 + x * 3) % 256
            g = (index * 41 + y * 5) % 256
            b = 255 if (x // 16 + y // 12 + index) % 2 == 0 else 32
            row += bytes((b, g, r))
        rows.append(bytes(row))
    return b"".join(rows)


def stream_header(kind: bytes, handler: bytes, scale: int, rate: int, length: int,
                  suggested: int, sample_size: int, width: int = 0, height: int = 0) -> bytes:
    return struct.pack("<4s4sIHHIIIIIIIIhhhh", kind, handler, 0, 0, 0, 0, scale, rate, 0, length,
                       suggested, 0xFFFFFFFF, sample_size, 0, 0, width, height)


def write_avi(path: Path, samples: bytes) -> None:
    frames = FPS * SECONDS
    frame_bytes = WIDTH * HEIGHT * 3
    block_align = 2
    byte_rate = SAMPLE_RATE * block_align
    audio_per_frame = len(samples) // frames

    avih = struct.pack("<IIIIIIIIIIIIII", 1_000_000 // FPS, byte_rate + frame_bytes * FPS, 0, 0x10,
                       frames, 0, 2, frame_bytes, WIDTH, HEIGHT, 0, 0, 0, 0)
    video_strl = list_chunk(b"strl",
        chunk(b"strh", stream_header(b"vids", b"DIB ", 1, FPS, frames, frame_bytes, 0, WIDTH, HEIGHT)) +
        chunk(b"strf", struct.pack("<IiiHHIIiiII", 40, WIDTH, HEIGHT, 1, 24, 0, frame_bytes, 2_835, 2_835, 0, 0)))
    audio_strl = list_chunk(b"strl",
        chunk(b"strh", stream_header(b"auds", b"\0\0\0\0", block_align, byte_rate,
                                      len(samples) // block_align, audio_per_frame, block_align)) +
        chunk(b"strf", struct.pack("<HHIIHHH", 1, 1, SAMPLE_RATE, byte_rate, block_align, 16, 0)))
    hdrl = list_chunk(b"hdrl", chunk(b"avih", avih) + video_strl + audio_strl)

    movi_payload = bytearray()
    index = bytearray()
    for i in range(frames):
        for tag, payload, flags in ((b"00db", video_frame(i), 0x10),
                                    (b"01wb", samples[i * audio_per_frame:(i + 1) * audio_per_frame], 0)):
            offset = len(movi_payload) + 4
            movi_payload += chunk(tag, payload)
            index += struct.pack("<4sIII", tag, flags, offset, len(payload))
    path.write_bytes(riff(b"AVI ", hdrl + list_chunk(b"movi", bytes(movi_payload)) + chunk(b"idx1", bytes(index))))


def generate(output: Path) -> dict[str, Path]:
    output.mkdir(parents=True, exist_ok=True)
    samples = pcm16()
    files = {
        "wav": output / "c16_pulse_mono_48k_s16.wav",
        "avi": output / "c16_rgb24_pcm_1s.avi",
        "empty": output / "c16_empty.media",
        "truncated": output / "c16_truncated.avi",
    }
    write_wav(files["wav"], samples)
    write_avi(files["avi"], samples)
    files["empty"].write_bytes(b"")
    files["truncated"].write_bytes(files["avi"].read_bytes()[:128])
    return files


def verify(files: dict[str, Path]) -> None:
    with wave.open(str(files["wav"]), "rb") as inp:
        assert inp.getnchannels() == 1
        assert inp.getsampwidth() == 2
        assert inp.getframerate() == SAMPLE_RATE
        assert inp.getnframes() == SAMPLE_RATE * SECONDS
    avi = files["avi"].read_bytes()
    assert avi[:4] == b"RIFF" and avi[8:12] == b"AVI "
    assert b"00db" in avi and b"01wb" in avi and b"idx1" in avi
    assert files["empty"].stat().st_size == 0
    assert files["truncated"].stat().st_size == 128


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=COURSE / "build/fixtures")
    parser.add_argument("--self-check", action="store_true")
    args = parser.parse_args()
    output = args.output.resolve()
    build = (COURSE / "build").resolve()
    if output != build and build not in output.parents:
        parser.error("--output must be inside C16_Desktop_Multimedia/build")
    files = generate(output)
    verify(files)
    for name, path in files.items():
        print(f"{name}: {path} ({path.stat().st_size} bytes)")
    if args.self_check:
        print("make_media_fixtures self-check PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
