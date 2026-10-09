#!/usr/bin/env python3
"""End-to-end native Field Mouse execution, deterministic Rough.js operations, pixels."""
from pathlib import Path
from subprocess import run
from tempfile import TemporaryDirectory
import math
import os
import sys

root = Path(__file__).resolve().parents[1]
exe = str(Path(sys.argv[1]).resolve())
fm = os.environ.get("FIELD_MOUSE")
if not fm:
    raise SystemExit("FIELD_MOUSE not set: refusing Node/transpiler fallback")


def reference_first_stroke():
    """Reference math/PRNG transcription from upstream renderer.ts and math.ts."""
    seed = 42

    def rand():
        nonlocal seed
        seed = (seed * 48271) % (2 ** 31)
        return seed / (2 ** 31)

    def off(a):
        return (rand() * 2 - 1) * a

    x1, y1, x2, y2 = 14., 14., 114., 14.
    dx, dy = x2 - x1, y2 - y1
    offset, gain = 2., 1.
    diverge = 0.2 + rand() * 0.2
    midx = off((y2 - y1) / 100)
    midy = off((x1 - x2) / 100)
    move = [x1 + off(offset), y1 + off(offset)]
    cubic = [
        midx + x1 + dx * diverge + off(offset),
        midy + y1 + dy * diverge + off(offset),
        midx + x1 + 2 * dx * diverge + off(offset),
        midy + y1 + 2 * dy * diverge + off(offset),
        x2 + off(offset), y2 + off(offset),
    ]
    return move, cubic


def parse_ops(path):
    output = []
    for line in path.read_text().splitlines():
        parts = line.split()
        output.append((parts[0], [float(v) for v in parts[1:]]))
    return output

with TemporaryDirectory() as directory:
    out_a = Path(directory) / "a.ops"
    out_b = Path(directory) / "b.ops"
    pic = Path(directory) / "image.ppm"
    for output in (out_a, out_b):
        run([fm, str(root / "src/rough.fm"), str(root / "fixtures/scene.json"), str(output)], check=True, timeout=30)
    assert out_a.read_bytes() == out_b.read_bytes(), "seeded rendering is nondeterministic"
    ops = parse_ops(out_a)
    assert {name: sum(o[0] == name for o in ops) for name in "PMC"} == {"P": 4, "M": 22, "C": 22}
    start, bezier = reference_first_stroke()
    assert ops[1][0] == "M" and ops[2][0] == "C"
    for got, expected in zip(ops[1][1], start):
        assert math.isclose(got, expected, rel_tol=1e-9, abs_tol=1e-9), (got, expected)
    for got, expected in zip(ops[2][1], bezier):
        assert math.isclose(got, expected, rel_tol=1e-9, abs_tol=1e-9), (got, expected)
    run([exe, str(out_a), str(pic), "240", "200"], check=True)
    ppm = pic.read_bytes()
    assert ppm.startswith(b"P6\n240 200\n255\n")
    colored = sum(ppm[i:i+3] != b"\xff\xff\xff" for i in range(len(b"P6\n240 200\n255\n"), len(ppm), 3))
    assert colored > 100, colored
print("real Field Mouse -> framebuffer acceptance: PASS")
