#!/usr/bin/env python3

from __future__ import annotations

import json
import shutil
import struct
import subprocess
import sys
import tempfile
import unittest
import warnings
import zlib
from pathlib import Path

HUB = Path(__file__).resolve().parent
REPO = HUB.parents[1]
sys.path[:0] = [str(HUB), str(HUB.parent / "package")]

import build_index  # noqa: E402
import pack_app  # noqa: E402

FIXED_TIME = "2026-01-15T12:00:00Z"


def png_chunk(tag: bytes, data: bytes) -> bytes:
    crc = zlib.crc32(tag + data) & 0xFFFFFFFF
    return struct.pack(">I", len(data)) + tag + data + struct.pack(">I", crc)


def minimal_png(w: int, h: int) -> bytes:
    sig = b"\x89PNG\r\n\x1a\n"
    ihdr = struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0)
    return sig + png_chunk(b"IHDR", ihdr) + png_chunk(b"IEND", b"")


def init_git(repo: Path) -> None:
    subprocess.check_call(["git", "init", "-q"], cwd=repo)
    subprocess.check_call(["git", "config", "user.email", "t@test"], cwd=repo)
    subprocess.check_call(["git", "config", "user.name", "test"], cwd=repo)


def commit_all(repo: Path, msg: str = "init") -> None:
    subprocess.check_call(["git", "add", "-A"], cwd=repo)
    subprocess.check_call(["git", "commit", "-q", "-m", msg], cwd=repo)


def pack_game(
    repo: Path,
    name: str,
    title: str,
    extra_inf: str = "",
    info_files: dict[str, bytes] | None = None,
) -> Path:
    game = repo / "games" / name
    game.mkdir(parents=True)
    (game / "main.bas").write_text("' test\n", encoding="utf-8")
    if extra_inf:
        (game / "PACKAGE.INF").write_text(extra_inf, encoding="utf-8")
    if info_files:
        info = game / "INFO"
        info.mkdir()
        for fname, data in info_files.items():
            (info / fname).write_bytes(data)
    dest = repo / "dist" / f"{name}.app"
    pack_app.build_app(str(game), str(dest), name, title)
    return dest


class HubIndexTests(unittest.TestCase):
    def setUp(self) -> None:
        self.tmp = tempfile.TemporaryDirectory()
        self.repo = Path(self.tmp.name)
        (self.repo / "dist").mkdir()
        (self.repo / "community").mkdir()
        init_git(self.repo)

    def tearDown(self) -> None:
        self.tmp.cleanup()

    def build_site(self, out_name: str = "site") -> Path:
        out = self.repo / out_name
        build_index.build(self.repo, out, FIXED_TIME)
        return out

    def test_happy_path(self) -> None:
        pack_game(
            self.repo,
            "alpha",
            "Alpha Game",
            "category=GAMES\nversion=1.0\nsummary=Short line\nminver=0.1.0\n",
            {
                "ICON.PNG": minimal_png(32, 32),
                "THUMB.PNG": minimal_png(160, 90),
                "SHOT1.PNG": minimal_png(10, 10),
                "ABOUT.TXT": "Hello\r\nworld\u2019s\n".encode("utf-8"),
            },
        )
        commit_all(self.repo)
        site = self.build_site()
        data = json.loads((site / "index.json").read_text(encoding="utf-8"))
        self.assertEqual(data["schema"], 1)
        self.assertEqual(len(data["apps"]), 1)
        app = data["apps"][0]
        self.assertEqual(app["id"], "alpha")
        self.assertEqual(app["category"], "GAMES")
        self.assertEqual(app["icon"], "info/alpha/icon.png")
        self.assertEqual(app["shots"], ["info/alpha/shot1.png"])
        self.assertEqual(app["about"], "info/alpha/about.txt")
        self.assertTrue((site / "apps" / "alpha.app").is_file())
        about = (site / "info" / "alpha" / "about.txt").read_text(encoding="ascii")
        self.assertIn("Hello", about)
        self.assertIn("world's", about)
        self.assertTrue(app["sha256"])
        self.assertEqual(app["size"], (site / "apps" / "alpha.app").stat().st_size)

    def test_missing_info(self) -> None:
        pack_game(self.repo, "plain", "Plain")
        commit_all(self.repo)
        site = self.build_site()
        app = json.loads((site / "index.json").read_text())["apps"][0]
        self.assertEqual(app["icon"], "")
        self.assertEqual(app["shots"], [])
        self.assertEqual(app["category"], "OTHER")

    def test_bad_category(self) -> None:
        pack_game(self.repo, "bad", "Bad", "category=ARCADE\n")
        commit_all(self.repo)
        with self.assertRaises(SystemExit):
            self.build_site()

    def test_duplicate_id(self) -> None:
        pack_game(self.repo, "dupe", "One")
        shutil_copy = self.repo / "community" / "dupe.app"
        shutil_copy.write_bytes((self.repo / "dist" / "dupe.app").read_bytes())
        commit_all(self.repo)
        with self.assertRaises(SystemExit):
            self.build_site()

    def test_determinism(self) -> None:
        pack_game(self.repo, "zeta", "Zeta", "category=UTILITIES\n")
        pack_game(self.repo, "beta", "Beta", "category=GAMES\n")
        commit_all(self.repo)
        a = self.build_site("site-a")
        b = self.build_site("site-b")
        self.assertEqual(
            (a / "index.json").read_bytes(),
            (b / "index.json").read_bytes(),
        )

    def test_png_warn_wrong_size(self) -> None:
        pack_game(
            self.repo,
            "warn",
            "Warn",
            info_files={"ICON.PNG": minimal_png(16, 16)},
        )
        commit_all(self.repo)
        with warnings.catch_warnings(record=True) as caught:
            warnings.simplefilter("always")
            self.build_site()
        self.assertTrue(any("ICON.PNG" in str(w.message) for w in caught))


class LiveRepoSmoke(unittest.TestCase):
    def test_current_repo_builds(self) -> None:
        if not (REPO / "dist").is_dir():
            self.skipTest("no dist/")
        out = REPO / "site-test-out"
        if out.exists():
            shutil.rmtree(out)
        build_index.build(REPO, out, FIXED_TIME)
        data = json.loads((out / "index.json").read_text(encoding="utf-8"))
        self.assertGreaterEqual(len(data["apps"]), 10)
        shutil.rmtree(out)


if __name__ == "__main__":
    unittest.main()