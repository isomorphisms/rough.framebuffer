#!/usr/bin/env python3
"""Native coverage, taper, alpha, clipping, and parser tests."""
from pathlib import Path
from subprocess import run
from tempfile import TemporaryDirectory
import sys

root = Path(__file__).resolve().parents[1]
exe = str(Path(sys.argv[1]).resolve())


def pixels_from(path, width, height):
    blob = path.read_bytes()
    header = f"P6\n{width} {height}\n255\n".encode()
    assert blob.startswith(header)
    pixels = blob[len(header):]
    assert len(pixels) == width * height * 3
    return pixels


def pixel(pixels, width, x, y):
    start = (y * width + x) * 3
    return pixels[start:start + 3]


with TemporaryDirectory() as directory:
    temporary = Path(directory)
    output = temporary / "sample.ppm"

    result = run(
        [exe, str(root / "fixtures/straight.ops"), str(output), "20", "10"],
        capture_output=True,
        check=True,
    )
    assert result.returncode == 0
    pixels = pixels_from(output, 20, 10)
    assert pixel(pixels, 20, 2, 2) == bytes((240, 20, 50))
    assert pixel(pixels, 20, 10, 2) == bytes((240, 20, 50))
    assert pixel(pixels, 20, 10, 4) == bytes((240, 20, 50))
    assert pixel(pixels, 20, 10, 8) == b"\xff\xff\xff"

    assert run(
        [exe, str(root / "fixtures/malformed.ops"), str(output), "20", "10"],
        capture_output=True,
    ).returncode != 0

    # Out-of-bounds geometry is clipped before any pixel-sized traversal.
    operations = temporary / "case.ops"
    operations.write_text("M -100000000 -100000000\nL 100000000 100000000\n")
    run([exe, str(operations), str(output), "20", "10"], check=True, timeout=5)
    pixels = pixels_from(output, 20, 10)
    assert pixel(pixels, 20, 0, 0) == b"\x00\x00\x00"

    # A diagonal has fractional edge coverage rather than a Bresenham staircase.
    operations.write_text("P 0 0 0\nW 1\nM 2 2\nL 15 8\n")
    run([exe, str(operations), str(output), "20", "12"], check=True)
    pixels = pixels_from(output, 20, 12)
    grey = pixel(pixels, 20, 3, 3)
    assert 0 < grey[0] < 255 and grey[0] == grey[1] == grey[2], grey

    # Optional end width tapers a segment and becomes the next current width.
    operations.write_text(
        "P 0 0 0\nW 1\nM 2 2\nL 17 2 5\nA 0.5\nM 2 8\nL 17 8\n"
    )
    run([exe, str(operations), str(output), "20", "12"], check=True)
    pixels = pixels_from(output, 20, 12)
    assert pixel(pixels, 20, 2, 4) == b"\xff\xff\xff"
    assert pixel(pixels, 20, 17, 4) == b"\x00\x00\x00"
    assert pixel(pixels, 20, 10, 8) == bytes((128, 128, 128))

    # Adaptive subdivision follows the cubic rather than its endpoint chord.
    operations.write_text("P 0 0 0\nW 1\nM 2 9\nC 2 1 17 1 17 9\n")
    run([exe, str(operations), str(output), "20", "12"], check=True)
    pixels = pixels_from(output, 20, 12)
    assert pixel(pixels, 20, 10, 3) != b"\xff\xff\xff"
    assert pixel(pixels, 20, 10, 8) == b"\xff\xff\xff"

    # Zero perpendicular flatness does not hide a reversing collinear cubic.
    operations.write_text("P 0 0 0\nW 1\nM 10 5\nC 18 5 2 5 10 5\n")
    run([exe, str(operations), str(output), "20", "12"], check=True)
    pixels = pixels_from(output, 20, 12)
    assert pixel(pixels, 20, 12, 5) != b"\xff\xff\xff"
    assert pixel(pixels, 20, 8, 5) != b"\xff\xff\xff"

    # Long, extra, or out-of-range fields cannot enter the native parser.
    malformed = [
        "M 0 0 junk\n",
        "M " + "0 " * 500 + "\n",
        "W 0\n",
        "W 1000001\n",
        "A -0.1\n",
        "A 1.1\n",
        "M 0 0\nL 1 1 2 3\n",
        "M 0 0\nC 1 1 2 2 3 3 4 5\n",
    ]
    for text in malformed:
        operations.write_text(text)
        assert run(
            [exe, str(operations), str(output), "20", "10"], capture_output=True
        ).returncode != 0, text

print("native coverage fixture: PASS")
