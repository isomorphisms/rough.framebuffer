#!/usr/bin/env python3
"""Native pixel tests. These do not claim Field Mouse interpreter acceptance."""
from pathlib import Path
from subprocess import run
from tempfile import TemporaryDirectory
import sys

root = Path(__file__).resolve().parents[1]
exe = str(Path(sys.argv[1]).resolve())
with TemporaryDirectory() as directory:
    output = Path(directory) / "sample.ppm"
    result = run([exe, str(root / "fixtures/straight.ops"), str(output), "20", "10"], capture_output=True, check=True)
    assert result.returncode == 0
    blob = output.read_bytes()
    header = b"P6\n20 10\n255\n"
    assert blob.startswith(header)
    pixels = blob[len(header):]
    assert len(pixels) == 20 * 10 * 3
    index = lambda x, y: (y * 20 + x) * 3
    assert pixels[index(2, 2):index(2, 2) + 3] == bytes((240, 20, 50))
    assert pixels[index(10, 2):index(10, 2) + 3] == bytes((240, 20, 50))
    assert pixels[index(10, 4):index(10, 4) + 3] == bytes((240, 20, 50))
    assert pixels[index(10, 8):index(10, 8) + 3] == b"\xff\xff\xff"
    assert run([exe, str(root / "fixtures/malformed.ops"), str(output), "20", "10"], capture_output=True).returncode != 0
    # Out-of-bounds line should be clipped rather than traversing thousands of pixels.
    far = Path(directory) / "far.ops"
    far.write_text("M -100000000 -100000000\nL 100000000 100000000\n")
    run([exe, str(far), str(output), "20", "10"], check=True, timeout=5)
    assert output.read_bytes()[len(header):][0:3] == b"\x00\x00\x00"
    # Long or trailing-extra fields cannot enter the native parser.
    far.write_text("M 0 0 junk\n")
    assert run([exe, str(far), str(output), "20", "10"], capture_output=True).returncode != 0
    far.write_text("M " + "0 " * 500 + "\n")
    assert run([exe, str(far), str(output), "20", "10"], capture_output=True).returncode != 0
print("native pixel fixture: PASS")
