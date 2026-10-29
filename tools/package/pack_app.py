#!/usr/bin/env python3
"""Build an mmcore .app the way PACKAGE does, without running mmcore.

The writer matches mmb_zip_add / mmb_zip_finish in mmcore mmbasic/src/zip.c
(ZIP store, method 0, DOS time and date 0, version 20, no extra fields) and
the PACKAGE wizard's PACKAGE.INF in mmbasic/src/cmd_package.c.

Walk order matches pack_walk: hidden names (leading '.') are skipped, each
directory is listed folders-first then files, case-insensitive A-Z, and the
walk is depth-first. PACKAGE.INF is appended last.

Dev-only paths are left out of the archive: directories art/, tools/, and
tests/, plus README.md, README.txt, ASSETS.txt, screenshot.png, and
layout.txt. Those are repo notes, not files the games LOAD or OPEN.
"""

from __future__ import annotations

import argparse
import os
import re
import struct
import sys
import tempfile
import zlib
from functools import cmp_to_key

ZIP_MAX_BYTES = 16 * 1024 * 1024
ZIP_MAX_FILES = 65535
ZIP_NAME_MAX = 120
WALK_DEPTH_MAX = 8

AUTHOR = "mmcore"
DEV_DIRS = {"art", "tools", "tests"}
DEV_FILES = {
    "readme.md",
    "readme.txt",
    "assets.txt",
    "screenshot.png",
    "layout.txt",
}


def crc32(data: bytes) -> int:
    return zlib.crc32(data) & 0xFFFFFFFF


def fold_char(ch: str) -> int:
    o = ord(ch)
    if 97 <= o <= 122:
        return o - 32
    return o


def cmp_names(a: str, b: str) -> int:
    ia = ib = 0
    while ia < len(a) and ib < len(b):
        ca = fold_char(a[ia])
        cb = fold_char(b[ib])
        if ca != cb:
            return ca - cb
        ia += 1
        ib += 1
    ca = fold_char(a[ia]) if ia < len(a) else 0
    cb = fold_char(b[ib]) if ib < len(b) else 0
    return ca - cb


def cmp_entries(a: tuple[str, bool], b: tuple[str, bool]) -> int:
    if a[1] != b[1]:
        return -1 if a[1] else 1
    c = cmp_names(a[0], b[0])
    if c:
        return c
    if a[0] < b[0]:
        return -1
    if a[0] > b[0]:
        return 1
    return 0


def path_ok(name: str) -> bool:
    if not name:
        return False
    buf = name.replace("\\", "/")
    if buf[0] == "/" or ":" in buf:
        return False
    for tok in buf.split("/"):
        if not tok or tok == ".":
            continue
        if tok.lower() == "..":
            return False
    return True


def zip_store(entries: list[tuple[str, bytes]]) -> bytes:
    if len(entries) > ZIP_MAX_FILES:
        raise SystemExit(f"too many zip entries ({len(entries)})")
    local = bytearray()
    central = bytearray()
    total = 0
    for name, data in entries:
        nb = name.encode("utf-8")
        nlen = len(nb)
        if nlen == 0 or nlen > ZIP_NAME_MAX or not path_ok(name):
            raise SystemExit(f"zip path rejected: {name!r}")
        size = len(data)
        total += size
        if total > ZIP_MAX_BYTES:
            raise SystemExit("package exceeds 16MB")
        c = crc32(data)
        off = len(local)
        local += struct.pack(
            "<IHHHHHIIIHH",
            0x04034B50,
            20,
            0,
            0,
            0,
            0,
            c,
            size,
            size,
            nlen,
            0,
        )
        local += nb
        local += data
        central += struct.pack(
            "<IHHHHHHIIIHHHHHII",
            0x02014B50,
            20,
            20,
            0,
            0,
            0,
            0,
            c,
            size,
            size,
            nlen,
            0,
            0,
            0,
            0,
            0,
            off,
        )
        central += nb
    cd_off = len(local)
    cd_len = len(central)
    nent = len(entries)
    eocd = struct.pack(
        "<IHHHHIIH",
        0x06054B50,
        0,
        0,
        nent,
        nent,
        cd_len,
        cd_off,
        0,
    )
    blob = bytes(local) + bytes(central) + eocd
    if len(blob) > ZIP_MAX_BYTES:
        raise SystemExit("package exceeds 16MB")
    return blob


def u16(b: bytes, off: int) -> int:
    return struct.unpack_from("<H", b, off)[0]


def u32(b: bytes, off: int) -> int:
    return struct.unpack_from("<I", b, off)[0]


def parse_zip(blob: bytes) -> list[tuple[str, bytes, dict]]:
    """Return (name, data, header fields) in central-directory order.

    Rejects anything mmb_zip_finish would not have written.
    """
    if len(blob) < 22 or len(blob) > ZIP_MAX_BYTES:
        raise SystemExit("zip size out of range")
    eocd = len(blob) - 22
    if u32(blob, eocd) != 0x06054B50:
        raise SystemExit("EOCD signature missing at end of file")
    if u16(blob, eocd + 4) or u16(blob, eocd + 6) or u16(blob, eocd + 20):
        raise SystemExit("EOCD disk or comment is not zero")
    nent = u16(blob, eocd + 8)
    if nent != u16(blob, eocd + 10):
        raise SystemExit("EOCD entry counts differ")
    cd_len = u32(blob, eocd + 12)
    cd_off = u32(blob, eocd + 16)
    if cd_off + cd_len != eocd:
        raise SystemExit("central directory does not end at EOCD")
    out = []
    pos = cd_off
    total = 0
    for _ in range(nent):
        if pos + 46 > eocd or u32(blob, pos) != 0x02014B50:
            raise SystemExit("bad central directory header")
        made, need, flags, method, tm, dt = struct.unpack_from("<HHHHHH", blob, pos + 4)
        crc, csz, usz = struct.unpack_from("<III", blob, pos + 16)
        nlen, elen, clen, disk, internal = struct.unpack_from("<HHHHH", blob, pos + 28)
        external, local = struct.unpack_from("<II", blob, pos + 38)
        if (made, need, flags, method, tm, dt) != (20, 20, 0, 0, 0, 0):
            raise SystemExit("central header is not a PACKAGE store entry")
        if elen or clen or disk or internal or external:
            raise SystemExit("central header has extra, comment, or attributes")
        if csz != usz:
            raise SystemExit("compressed size does not match stored size")
        name = blob[pos + 46 : pos + 46 + nlen].decode("utf-8")
        pos += 46 + nlen
        if local + 30 > cd_off or u32(blob, local) != 0x04034B50:
            raise SystemExit(f"bad local header for {name}")
        lneed, lflags, lmethod, ltm, ldt = struct.unpack_from("<HHHHH", blob, local + 4)
        lcrc, lcsz, lusz = struct.unpack_from("<III", blob, local + 14)
        lnlen, lelen = struct.unpack_from("<HH", blob, local + 26)
        if (lneed, lflags, lmethod, ltm, ldt) != (20, 0, 0, 0, 0):
            raise SystemExit(f"local header is not a PACKAGE store entry: {name}")
        if lelen or lnlen != nlen or lcrc != crc or lcsz != usz or lusz != usz:
            raise SystemExit(f"local header does not match central directory: {name}")
        if blob[local + 30 : local + 30 + lnlen] != name.encode("utf-8"):
            raise SystemExit(f"local name does not match central name: {name}")
        data = blob[local + 30 + lnlen : local + 30 + lnlen + usz]
        if crc32(data) != crc:
            raise SystemExit(f"crc mismatch: {name}")
        total += usz
        if total > ZIP_MAX_BYTES:
            raise SystemExit("uncompressed total exceeds 16MB")
        out.append(
            (
                name,
                data,
                {"crc": crc, "size": usz, "local": local},
            )
        )
    if pos != eocd:
        raise SystemExit("central directory length mismatch")
    return out


def is_excluded(name: str, is_dir: bool) -> bool:
    low = name.lower()
    if is_dir and low in DEV_DIRS:
        return True
    if not is_dir and low in DEV_FILES:
        return True
    return False


def collect(folder: str, dest: str) -> list[tuple[str, bytes]]:
    dest_real = os.path.realpath(dest)
    seen_dirs: set[str] = set()
    entries: list[tuple[str, bytes]] = []

    def walk(absdir: str, rel: str, depth: int) -> None:
        if depth > WALK_DEPTH_MAX:
            raise SystemExit(f"folder is nested too deep for PACKAGE: {rel or folder}")
        real = os.path.realpath(absdir)
        if real in seen_dirs:
            raise SystemExit(f"directory cycle while packing: {absdir}")
        seen_dirs.add(real)
        try:
            names = sorted(os.listdir(absdir))
        except OSError as exc:
            raise SystemExit(f"cannot list {absdir}: {exc}") from exc
        listed = [n for n in names if n]
        nonhidden = [n for n in listed if not n.startswith(".")]
        if not nonhidden:
            if rel and not listed:
                entries.append((rel + "/", b""))
            return
        kept: list[tuple[str, bool, str]] = []
        for name in nonhidden:
            full = os.path.join(absdir, name)
            if os.path.isdir(full):
                is_dir = True
            elif os.path.isfile(full):
                is_dir = False
            else:
                raise SystemExit(f"unsupported file type: {full}")
            if is_excluded(name, is_dir):
                continue
            kept.append((name, is_dir, full))
        kept.sort(key=cmp_to_key(lambda a, b: cmp_entries((a[0], a[1]), (b[0], b[1]))))
        for name, is_dir, full in kept:
            child = name if not rel else rel + "/" + name
            if is_dir:
                walk(full, child, depth + 1)
                continue
            if os.path.realpath(full) == dest_real:
                continue
            with open(full, "rb") as fh:
                data = fh.read()
            entries.append((child, data))

    if not os.path.isdir(folder):
        raise SystemExit(f"not a directory: {folder}")
    walk(folder, "", 0)
    return entries


def has_main(entries: list[tuple[str, bytes]]) -> bool:
    for name, _data in entries:
        if "/" not in name and name.lower() == "main.bas":
            return True
    return False


def humanize_title(text: str) -> str:
    letters = [c for c in text if c.isalpha()]
    if letters and all(c.isupper() for c in letters):
        return " ".join(w[:1].upper() + w[1:].lower() for w in text.split())
    return text


def titles_from_readme(text: str) -> dict[str, str]:
    found: dict[str, str] = {}
    for line in text.splitlines():
        if not line.startswith("|"):
            continue
        cells = [c.strip() for c in line.strip().strip("|").split("|")]
        if len(cells) < 2:
            continue
        match = re.search(r"games/([^/)\s]+)/", cells[-1])
        if not match:
            continue
        raw = cells[-2]
        if " - " in raw:
            raw = raw.split(" - ", 1)[0].strip()
        raw = " ".join(raw.split())
        if not raw or raw.lower() in {"game"} or set(raw) <= {"-"}:
            continue
        found[match.group(1)] = raw
    return found


def title_from_main(path: str) -> str | None:
    try:
        fh = open(path, "r", encoding="utf-8", errors="replace")
    except OSError:
        return None
    with fh:
        for line in fh:
            stripped = line.strip()
            if stripped in {"", "'"}:
                continue
            if not stripped.startswith("'"):
                break
            body = stripped[1:].strip()
            if not body:
                continue
            if " - " in body:
                body = body.split(" - ", 1)[0].strip()
            return humanize_title(body)
    return None


def clean_field(text: str, limit: int = 79) -> str:
    text = text.replace("\r", " ").replace("\n", " ")
    text = " ".join(text.split())
    return text[:limit]


def game_title(root: str, game: str) -> str:
    readme = os.path.join(root, "README.md")
    if os.path.isfile(readme):
        with open(readme, "r", encoding="utf-8") as fh:
            titled = titles_from_readme(fh.read())
        if titled.get(game):
            return clean_field(titled[game])
    from_main = title_from_main(os.path.join(root, "games", game, "main.bas"))
    if from_main:
        return clean_field(from_main)
    return game


def package_inf(game: str, title: str) -> bytes:
    title = clean_field(title) or game
    lines = [
        f"name={game}.APP",
        "main=MAIN.BAS",
        f"title={title}",
        f"author={AUTHOR}",
    ]
    return ("".join(line + "\n" for line in lines)).encode("utf-8")


def build_app(folder: str, dest: str, game: str, title: str) -> tuple[bytes, bool]:
    entries = collect(folder, dest)
    if not has_main(entries):
        raise SystemExit(f"{game}: no main.bas (PACKAGE requires MAIN.BAS)")
    entries.append(("PACKAGE.INF", package_inf(game, title)))
    blob = zip_store(entries)
    parse_zip(blob)
    os.makedirs(os.path.dirname(os.path.abspath(dest)), exist_ok=True)
    changed = True
    if os.path.isfile(dest):
        with open(dest, "rb") as fh:
            changed = fh.read() != blob
    if changed:
        tmp = dest + ".tmp"
        try:
            with open(tmp, "wb") as fh:
                fh.write(blob)
            os.replace(tmp, dest)
        except Exception:
            if os.path.exists(tmp):
                os.remove(tmp)
            raise
    return blob, changed


def check_app(path: str, game: str | None = None, title: str | None = None) -> None:
    with open(path, "rb") as fh:
        blob = fh.read()
    entries = parse_zip(blob)
    if not entries or entries[-1][0] != "PACKAGE.INF":
        raise SystemExit(f"{path}: PACKAGE.INF is not the last entry")
    names = [name for name, _data, _meta in entries]
    if names.count("PACKAGE.INF") != 1:
        raise SystemExit(f"{path}: expected one PACKAGE.INF")
    if not any("/" not in name and name.lower() == "main.bas" for name in names):
        raise SystemExit(f"{path}: no root main.bas")
    inf = entries[-1][1].decode("utf-8")
    fields: dict[str, str] = {}
    for line in inf.split("\n"):
        if not line:
            continue
        if "=" not in line:
            raise SystemExit(f"{path}: bad PACKAGE.INF line {line!r}")
        key, val = line.split("=", 1)
        fields[key] = val
    for key in ("name", "main", "title", "author"):
        if key not in fields or not fields[key]:
            raise SystemExit(f"{path}: PACKAGE.INF missing {key}")
    if fields["main"] != "MAIN.BAS":
        raise SystemExit(f"{path}: main field is {fields['main']!r}")
    if fields["author"] != AUTHOR:
        raise SystemExit(f"{path}: author field is {fields['author']!r}")
    if game is not None and fields["name"] != f"{game}.APP":
        raise SystemExit(f"{path}: name field is {fields['name']!r}")
    if title is not None and fields["title"] != clean_field(title):
        raise SystemExit(
            f"{path}: title field is {fields['title']!r}, expected {title!r}"
        )
    prev = -1
    for name, _data, meta in entries:
        if meta["local"] < prev:
            raise SystemExit(f"{path}: local headers are not in write order")
        prev = meta["local"]
        if name != "PACKAGE.INF" and not path_ok(name):
            raise SystemExit(f"{path}: bad stored path {name!r}")


def cmd_pack(args: argparse.Namespace) -> None:
    root = os.path.abspath(args.root)
    game = args.game
    folder = os.path.join(root, "games", game)
    title = args.title or game_title(root, game)
    dest = args.out or os.path.join(root, "dist", game + ".app")
    _blob, changed = build_app(folder, dest, game, title)
    check_app(dest, game, title)
    state = "wrote" if changed else "unchanged"
    print(f"{state} {os.path.relpath(dest, root)}  title={title}")


def cmd_check(args: argparse.Namespace) -> None:
    check_app(args.app, args.game, args.title)
    print(f"ok {args.app}")


def self_test() -> None:
    sample = "\n".join(
        [
            "| Title screen | Game | Folder |",
            "| --- | --- | --- |",
            "| img | Sudoku Grove - calm | [games/sudoku/](games/sudoku/) |",
            "| img | TRONIX - grid | [games/tronix/](games/tronix/) |",
        ]
    )
    titles = titles_from_readme(sample)
    if titles["sudoku"] != "Sudoku Grove" or titles["tronix"] != "TRONIX":
        raise SystemExit(f"readme titles parsed wrong: {titles}")
    if humanize_title("JETPACK") != "Jetpack":
        raise SystemExit("humanize JETPACK")
    if humanize_title("SUDOKU GROVE") != "Sudoku Grove":
        raise SystemExit("humanize SUDOKU GROVE")
    if humanize_title("Already Mixed") != "Already Mixed":
        raise SystemExit("humanize mixed")
    here = os.path.dirname(os.path.abspath(__file__))
    root = os.path.abspath(os.path.join(here, "..", ".."))
    if os.path.isfile(os.path.join(root, "games", "sudoku", "main.bas")):
        live = {
            "sudoku": "Sudoku Grove",
            "tetris": "Tetris",
            "snake": "Snake",
            "voidrunner": "Void Runner",
            "tronix": "TRONIX",
            "corecaving": "Core Caving",
            "circuitquest": "Circuit Quest",
            "crystalcore": "Crystal Core",
            "salsaslide": "Salsa Slide",
            "jetpack": "Jetpack",
        }
        for game, title in live.items():
            got = game_title(root, game)
            if got != title:
                raise SystemExit(f"title for {game} is {got!r}, expected {title!r}")
    with tempfile.TemporaryDirectory() as tmp:
        game = os.path.join(tmp, "demo")
        os.makedirs(os.path.join(game, "sub"))
        os.makedirs(os.path.join(game, "empty"))
        os.makedirs(os.path.join(game, "art"))
        os.makedirs(os.path.join(game, "sfx"))
        with open(os.path.join(game, "main.bas"), "wb") as fh:
            fh.write(b"' DEMO - test\n")
        with open(os.path.join(game, "B.txt"), "wb") as fh:
            fh.write(b"b")
        with open(os.path.join(game, "a.txt"), "wb") as fh:
            fh.write(b"hi")
        with open(os.path.join(game, "README.md"), "wb") as fh:
            fh.write(b"nope")
        with open(os.path.join(game, ".hidden"), "wb") as fh:
            fh.write(b"nope")
        with open(os.path.join(game, "sub", "z.txt"), "wb") as fh:
            fh.write(b"z")
        with open(os.path.join(game, "art", "skip.png"), "wb") as fh:
            fh.write(b"png")
        with open(os.path.join(game, "sfx", "README.txt"), "wb") as fh:
            fh.write(b"notes")
        with open(os.path.join(game, "screenshot.png"), "wb") as fh:
            fh.write(b"shot")
        dest = os.path.join(tmp, "demo.app")
        build_app(game, dest, "demo", "Demo")
        check_app(dest, "demo", "Demo")
        parsed = parse_zip(open(dest, "rb").read())
        names = [name for name, _data, _meta in parsed]
        expect = ["empty/", "sub/z.txt", "a.txt", "B.txt", "main.bas", "PACKAGE.INF"]
        if names != expect:
            raise SystemExit(f"walk order {names} != {expect}")
        inf = parsed[-1][1].decode("utf-8")
        if inf != "name=demo.APP\nmain=MAIN.BAS\ntitle=Demo\nauthor=mmcore\n":
            raise SystemExit(f"PACKAGE.INF bytes {inf!r}")
        if parsed[2][1] != b"hi" or parsed[2][2]["crc"] != crc32(b"hi"):
            raise SystemExit("payload crc")
    print("self-test ok")


def main(argv: list[str]) -> None:
    parser = argparse.ArgumentParser(description="Build mmcore PACKAGE .app files")
    sub = parser.add_subparsers(dest="cmd", required=True)

    pack = sub.add_parser("pack")
    pack.add_argument("--root", required=True)
    pack.add_argument("--game", required=True)
    pack.add_argument("--out")
    pack.add_argument("--title")
    pack.set_defaults(func=cmd_pack)

    check = sub.add_parser("check")
    check.add_argument("--app", required=True)
    check.add_argument("--game")
    check.add_argument("--title")
    check.set_defaults(func=cmd_check)

    test = sub.add_parser("self-test")
    test.set_defaults(func=lambda _args: self_test())

    args = parser.parse_args(argv)
    args.func(args)


if __name__ == "__main__":
    main(sys.argv[1:])
