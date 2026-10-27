#!/usr/bin/env python3
"""Fail if an mmcore serial log looks like a BASIC/interpreter error."""
from __future__ import annotations

import re
import sys

# Strip ANSI colour sequences from mmcore console output.
ANSI = re.compile(r"\x1b\[[0-9;?]*[A-Za-z]")

PATTERNS = [
    re.compile(r"^\?"),  # ?SYNTAX ERROR, ?LABEL NOT FOUND, ...
    re.compile(r"\bSYNTAX ERROR\b", re.I),
    re.compile(r"\bUNDEFINED\b", re.I),
    re.compile(r"\bOUT OF MEMORY\b", re.I),
    re.compile(r"\bDIVIDE BY ZERO\b", re.I),
    re.compile(r"\bTYPE MISMATCH\b", re.I),
    re.compile(r"\bLABEL NOT FOUND\b", re.I),
    re.compile(r"\bSUBSCRIPT OUT OF RANGE\b", re.I),
    re.compile(r"\bBREAK\b"),
]


def check(text: str) -> list[str]:
    hits: list[str] = []
    for raw in text.splitlines():
        line = ANSI.sub("", raw).strip()
        if not line:
            continue
        for pat in PATTERNS:
            if pat.search(line):
                hits.append(line)
                break
    return hits


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: check_serial.py serial.log", file=sys.stderr)
        return 2
    with open(sys.argv[1], encoding="utf-8", errors="replace") as fh:
        body = fh.read()
    bad = check(body)
    if bad:
        print("serial errors:")
        for line in bad:
            print(" ", line)
        return 1
    print("serial ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
