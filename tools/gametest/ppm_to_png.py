#!/usr/bin/env python3
"""Convert harness PPM captures to PNG and remove the PPM."""
from __future__ import annotations

import glob
import os
import sys

from PIL import Image


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: ppm_to_png.py <directory>", file=sys.stderr)
        return 2
    d = sys.argv[1]
    n = 0
    for path in glob.glob(os.path.join(d, "*.ppm")):
        out = path[:-4] + ".png"
        Image.open(path).save(out)
        os.remove(path)
        n += 1
    print("converted", n, "ppm")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
