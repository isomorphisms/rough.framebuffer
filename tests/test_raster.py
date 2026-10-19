#!/usr/bin/env python3
"""Native stroke, triangle, depth, clipping, and parser tests."""
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

    # Triangle winding is irrelevant, while the top-left rule makes two faces
    # tile a rectangle without a diagonal crack or double-owned sample.
    operations.write_text(
        "P 20 80 220\n"
        "T 2 2 0.4 10 2 0.4 10 10 0.4\n"
        "T 2 2 0.4 10 10 0.4 2 10 0.4\n"
    )
    run([exe, str(operations), str(output), "14", "14"], check=True)
    pixels = pixels_from(output, 14, 14)
    for y in range(2, 10):
        for x in range(2, 10):
            assert pixel(pixels, 14, x, y) == bytes((20, 80, 220)), (x, y)
    assert pixel(pixels, 14, 10, 5) == b"\xff\xff\xff"
    assert pixel(pixels, 14, 5, 10) == b"\xff\xff\xff"

    # A farther triangle submitted later cannot overwrite a nearer one.
    operations.write_text(
        "P 30 60 220\n"
        "T 2 2 0.2 18 2 0.2 2 18 0.2\n"
        "P 220 40 30\n"
        "T 2 2 0.8 18 2 0.8 2 18 0.8\n"
    )
    run([exe, str(operations), str(output), "20", "20"], check=True)
    pixels = pixels_from(output, 20, 20)
    assert pixel(pixels, 20, 4, 4) == bytes((30, 60, 220))

    # Screen-space barycentric depth interpolation can cross another face.
    operations.write_text(
        "P 20 190 70\n"
        "T 2 2 0.5 18 2 0.5 2 18 0.5\n"
        "P 220 40 40\n"
        "T 2 2 0.1 18 2 0.9 2 18 0.9\n"
    )
    run([exe, str(operations), str(output), "20", "20"], check=True)
    pixels = pixels_from(output, 20, 20)
    assert pixel(pixels, 20, 3, 3) == bytes((220, 40, 40))
    assert pixel(pixels, 20, 12, 3) == bytes((20, 190, 70))

    # Long, extra, nonfinite, or out-of-range fields cannot enter the renderer.
    malformed = [
        "M 0 0 junk\n",
        "M " + "0 " * 500 + "\n",
        "W 0\n",
        "W 1000001\n",
        "A -0.1\n",
        "A 1.1\n",
        "M 0 0\nL 1 1 2 3\n",
        "M 0 0\nC 1 1 2 2 3 3 4 5\n",
        "T 0 0 0 1 0 0 0 1\n",
        "T 0 0 0 1 0 0 0 1 0 2\n",
        "T 0 0 NaN 1 0 0 0 1 0\n",
        "T 0 0 1e39 1 0 0 0 1 0\n",
    ]
    for text in malformed:
        operations.write_text(text)
        assert run(
            [exe, str(operations), str(output), "20", "10"], capture_output=True
        ).returncode != 0, text

print("native raster fixtures: PASS")
