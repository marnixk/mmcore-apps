#!/usr/bin/env python3
"""Build hub.mmcore.dev catalogue under dist/ (index.json, info/, _headers).

Each app entry always includes ``hero``. It is optional content: the value is
``info/<id>/hero.png`` when the package contains ``INFO/HERO.PNG`` (640x360),
and an empty string when that file is absent. Schema stays 1.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import shutil
import struct
import subprocess
import sys
import tempfile
import warnings
import zipfile
from datetime import datetime, timezone
from pathlib import Path

ZIP_MAX_BYTES = 16 * 1024 * 1024
ID_RE = re.compile(r"^[a-z0-9_-]{1,32}$")
VER_RE = re.compile(r"^[0-9]+(\.[0-9]+)*$")

CATEGORIES = [
    "GAMES",
    "DEMOS",
    "CREATIVE",
    "AUDIO",
    "PRODUCTIVITY",
    "DEVELOPMENT",
    "UTILITIES",
    "INTERNET",
    "LEARNING",
    "OTHER",
]
CATEGORY_SET = set(CATEGORIES)

GENERATED_PATHS = (
    "games",
    "community",
    "tools/hub",
    "tools/package",
    ":(glob)dist/*.app",
)

INFO_ICON = "INFO/ICON.PNG"
INFO_THUMB = "INFO/THUMB.PNG"
INFO_HERO = "INFO/HERO.PNG"
INFO_ABOUT = "INFO/ABOUT.TXT"
INFO_SHOTS = [f"INFO/SHOT{n}.PNG" for n in range(1, 5)]

APP_KEYS = (
    "id",
    "file",
    "title",
    "author",
    "category",
    "version",
    "summary",
    "minver",
    "size",
    "sha256",
    "updated",
    "icon",
    "thumb",
    "hero",
    "shots",
    "about",
)

SMART_ASCII = str.maketrans(
    {
        "\u2018": "'",
        "\u2019": "'",
        "\u201c": '"',
        "\u201d": '"',
        "\u2013": "-",
        "\u2014": "-",
        "\u2026": "...",
    }
)


def parse_inf(text: str) -> dict[str, str]:
    fields: dict[str, str] = {}
    for line in text.splitlines():
        if not line:
            continue
        if "=" not in line:
            raise ValueError(f"bad PACKAGE.INF line {line!r}")
        key, val = line.split("=", 1)
        fields[key] = val
    return fields


def app_id_from_inf(name_field: str, basename: str) -> str:
    raw = name_field.rsplit(".", 1)[0] if name_field else basename
    if not raw:
        raw = basename
    return raw.lower()


def validate_app_id(app_id: str, label: str) -> None:
    if not ID_RE.match(app_id):
        raise SystemExit(
            f"{label}: id {app_id!r} must match ^[a-z0-9_-]{{1,32}}$"
        )


def iso_to_z(value: str) -> str:
    if not value:
        return ""
    if value.endswith("Z"):
        return value
    try:
        ts = datetime.fromisoformat(value.replace("Z", "+00:00"))
        if ts.tzinfo is None:
            ts = ts.replace(tzinfo=timezone.utc)
        return ts.astimezone(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
    except ValueError:
        return value


def git_updated(repo: Path, source_path: Path) -> str:
    try:
        rel = source_path.relative_to(repo)
    except ValueError:
        return ""
    try:
        out = subprocess.check_output(
            ["git", "log", "-1", "--format=%cI", "--", str(rel)],
            cwd=repo,
            stderr=subprocess.DEVNULL,
            text=True,
        ).strip()
    except (subprocess.CalledProcessError, OSError):
        return ""
    return iso_to_z(out)


def git_generated(repo: Path) -> str:
    paths = [p for p in GENERATED_PATHS if (repo / p).exists()]
    if not paths:
        return "1970-01-01T00:00:00Z"
    try:
        out = subprocess.check_output(
            ["git", "log", "-1", "--format=%cI", "--", *paths],
            cwd=repo,
            stderr=subprocess.DEVNULL,
            text=True,
        ).strip()
    except (subprocess.CalledProcessError, OSError):
        return "1970-01-01T00:00:00Z"
    return iso_to_z(out) or "1970-01-01T00:00:00Z"


def png_size(data: bytes) -> tuple[int, int] | None:
    if len(data) < 24 or data[:8] != b"\x89PNG\r\n\x1a\n":
        return None
    if data[12:16] != b"IHDR":
        return None
    if struct.unpack(">I", data[8:12])[0] != 13:
        return None
    w, h = struct.unpack(">II", data[16:24])
    return w, h


def is_valid_png(data: bytes) -> bool:
    return png_size(data) is not None


def normalize_about(data: bytes) -> str:
    text = data.decode("utf-8", errors="replace")
    text = text.replace("\r\n", "\n").replace("\r", "\n")
    text = text.translate(SMART_ASCII)
    return text.encode("ascii", errors="replace").decode("ascii")


def read_zip_inf(path: Path) -> tuple[dict[str, str], dict[str, bytes]]:
    if path.stat().st_size > ZIP_MAX_BYTES:
        raise SystemExit(f"{path}: archive exceeds 16 MiB")
    with zipfile.ZipFile(path, "r") as zf:
        names = zf.namelist()
        if not any(n.upper() == "MAIN.BAS" and "/" not in n for n in names):
            raise SystemExit(f"{path}: missing root MAIN.BAS")
        if "PACKAGE.INF" not in names:
            raise SystemExit(f"{path}: missing PACKAGE.INF")
        if names[-1] != "PACKAGE.INF":
            raise SystemExit(f"{path}: PACKAGE.INF is not the last zip entry")
        inf_text = zf.read("PACKAGE.INF").decode("utf-8")
        info: dict[str, bytes] = {}
        for key in [INFO_ICON, INFO_THUMB, INFO_HERO, INFO_ABOUT, *INFO_SHOTS]:
            if key in names:
                info[key] = zf.read(key)
        return parse_inf(inf_text), info


def validate_fields(path: Path, fields: dict[str, str]) -> None:
    label = str(path)
    cat = fields.get("category", "")
    if cat and cat not in CATEGORY_SET:
        raise SystemExit(f"{label}: unknown category {cat!r}")
    for key in ("version", "minver"):
        val = fields.get(key, "")
        if val and not VER_RE.match(val):
            raise SystemExit(f"{label}: {key} {val!r} is not a dotted version")
    summary = fields.get("summary", "")
    if summary:
        if "\n" in summary or "\r" in summary:
            raise SystemExit(f"{label}: summary must be one line")
        if len(summary) > 80:
            raise SystemExit(f"{label}: summary exceeds 80 characters")


def check_info_png(path: Path, name: str, data: bytes, expect_w: int, expect_h: int) -> None:
    if not is_valid_png(data):
        raise SystemExit(f"{path}: {name} is not a valid PNG")
    size = png_size(data)
    if size and size != (expect_w, expect_h):
        warnings.warn(
            f"{path}: {name} is {size[0]}x{size[1]}, expected {expect_w}x{expect_h}"
        )


def extract_info(
    path: Path, app_id: str, info: dict[str, bytes], out_info: Path
) -> dict[str, object]:
    dest = out_info / app_id
    dest.mkdir(parents=True, exist_ok=True)
    result: dict[str, object] = {
        "icon": "",
        "thumb": "",
        "hero": "",
        "shots": [],
        "about": "",
    }

    if INFO_ICON in info:
        check_info_png(path, INFO_ICON, info[INFO_ICON], 32, 32)
        (dest / "icon.png").write_bytes(info[INFO_ICON])
        result["icon"] = f"info/{app_id}/icon.png"

    if INFO_THUMB in info:
        check_info_png(path, INFO_THUMB, info[INFO_THUMB], 160, 90)
        (dest / "thumb.png").write_bytes(info[INFO_THUMB])
        result["thumb"] = f"info/{app_id}/thumb.png"

    if INFO_HERO in info:
        check_info_png(path, INFO_HERO, info[INFO_HERO], 640, 360)
        (dest / "hero.png").write_bytes(info[INFO_HERO])
        result["hero"] = f"info/{app_id}/hero.png"

    shots: list[str] = []
    n = 1
    for key in INFO_SHOTS:
        if key not in info:
            continue
        if not is_valid_png(info[key]):
            raise SystemExit(f"{path}: {key} is not a valid PNG")
        name = f"shot{n}.png"
        (dest / name).write_bytes(info[key])
        shots.append(f"info/{app_id}/{name}")
        n += 1
    result["shots"] = shots

    if INFO_ABOUT in info:
        (dest / "about.txt").write_text(
            normalize_about(info[INFO_ABOUT]), encoding="ascii", newline="\n"
        )
        result["about"] = f"info/{app_id}/about.txt"

    return result


def app_record(
    path: Path,
    repo: Path,
    source_git: Path,
    fields: dict[str, str],
    info_meta: dict[str, object],
    app_id: str,
) -> dict[str, object]:
    data = path.read_bytes()
    title = fields.get("title", "") or app_id
    category = fields.get("category", "") or "OTHER"
    if category not in CATEGORY_SET:
        category = "OTHER"
    updated = git_updated(repo, source_git)
    return {
        "id": app_id,
        "file": f"{app_id}.app",
        "title": title,
        "author": fields.get("author", ""),
        "category": category,
        "version": fields.get("version", ""),
        "summary": fields.get("summary", ""),
        "minver": fields.get("minver", ""),
        "size": len(data),
        "sha256": hashlib.sha256(data).hexdigest(),
        "updated": updated,
        "icon": info_meta.get("icon", ""),
        "thumb": info_meta.get("thumb", ""),
        "hero": info_meta.get("hero", ""),
        "shots": info_meta.get("shots", []),
        "about": info_meta.get("about", ""),
    }


def sort_apps(apps: list[dict[str, object]]) -> None:
    def cat_key(c: str) -> int:
        try:
            return CATEGORIES.index(c)
        except ValueError:
            return len(CATEGORIES)

    apps.sort(key=lambda a: (cat_key(str(a["category"])), str(a["title"]).lower()))


def dump_index(obj: dict[str, object]) -> bytes:
    apps_out = [{k: app[k] for k in APP_KEYS} for app in obj["apps"]]
    out = {
        "schema": 1,
        "generated": obj["generated"],
        "categories": obj["categories"],
        "apps": apps_out,
    }
    return (json.dumps(out, indent=2, ensure_ascii=True) + "\n").encode("utf-8")


def write_headers(dist: Path) -> None:
    text = (
        "/index.json\n"
        "  Cache-Control: max-age=300\n"
        "/*\n"
        "  Cache-Control: max-age=86400\n"
    )
    (dist / "_headers").write_text(text, encoding="utf-8")


def game_source(repo: Path, app_id: str) -> Path | None:
    for stem in (app_id, app_id.lower()):
        path = repo / "games" / stem
        if (path / "main.bas").is_file():
            return path
    return None


def stage_community(repo: Path, dist: Path) -> dict[str, Path]:
    """Copy community/*.app into dist/<id>.app. Return id -> community source path."""
    community = repo / "community"
    sources: dict[str, Path] = {}
    if not community.is_dir():
        return sources
    for path in sorted(community.glob("*.app")):
        fields, _info = read_zip_inf(path)
        app_id = app_id_from_inf(fields.get("name", ""), path.stem)
        validate_app_id(app_id, str(path))
        if app_id in sources:
            raise SystemExit(
                f"duplicate id {app_id!r}: {sources[app_id]} and {path}"
            )
        dest = dist / f"{app_id}.app"
        if game_source(repo, app_id) is not None and dest.is_file():
            raise SystemExit(
                f"duplicate id {app_id!r}: dist/{app_id}.app (game) and {path}"
            )
        dist.mkdir(parents=True, exist_ok=True)
        shutil.copy2(path, dest)
        sources[app_id] = path
    return sources


def list_dist_packages(
    repo: Path, dist: Path, community_sources: dict[str, Path]
) -> list[tuple[Path, Path]]:
    packages: list[tuple[Path, Path]] = []
    for path in sorted(dist.glob("*.app")):
        fields, _info = read_zip_inf(path)
        app_id = app_id_from_inf(fields.get("name", ""), path.stem)
        if app_id in community_sources:
            git_src = community_sources[app_id]
        else:
            src = game_source(repo, app_id) or game_source(repo, path.stem)
            if src is not None:
                git_src = src
            else:
                git_src = path
        packages.append((path, git_src))
    return packages


def prune_stale_info(info_dir: Path, keep: set[str]) -> None:
    if not info_dir.is_dir():
        return
    for child in list(info_dir.iterdir()):
        if child.is_dir() and child.name not in keep:
            shutil.rmtree(child)


def build(
    repo: Path,
    dist: Path,
    generated: str | None = None,
) -> int:
    if generated is None:
        generated = git_generated(repo)
    community_sources = stage_community(repo, dist)
    info_dir = dist / "info"
    info_dir.mkdir(parents=True, exist_ok=True)

    packages = list_dist_packages(repo, dist, community_sources)
    if not packages:
        raise SystemExit("no packages in dist/")

    seen_ids: dict[str, Path] = {}
    records: list[dict[str, object]] = []
    active_ids: set[str] = set()

    for path, git_src in packages:
        fields, info = read_zip_inf(path)
        validate_fields(path, fields)
        app_id = app_id_from_inf(fields.get("name", ""), path.stem)
        validate_app_id(app_id, str(path))
        if app_id in seen_ids:
            raise SystemExit(
                f"duplicate id {app_id!r}: {seen_ids[app_id]} and {path}"
            )
        seen_ids[app_id] = path
        active_ids.add(app_id)
        info_meta = extract_info(path, app_id, info, info_dir)
        records.append(
            app_record(path, repo, git_src, fields, info_meta, app_id)
        )

    prune_stale_info(info_dir, active_ids)
    sort_apps(records)
    index = {
        "schema": 1,
        "generated": generated,
        "categories": CATEGORIES,
        "apps": records,
    }
    (dist / "index.json").write_bytes(dump_index(index))
    write_headers(dist)
    return len(records)


def dry_run(repo: Path) -> int:
    generated = git_generated(repo)
    with tempfile.TemporaryDirectory() as tmp:
        fake_repo = Path(tmp)
        (fake_repo / "games").mkdir()
        dist = fake_repo / "dist"
        dist.mkdir()
        src_dist = repo / "dist"
        if src_dist.is_dir():
            for app in src_dist.glob("*.app"):
                shutil.copy2(app, dist / app.name)
        if (repo / "community").is_dir():
            shutil.copytree(repo / "community", fake_repo / "community")
        return build(fake_repo, dist, generated)


def main(argv: list[str]) -> None:
    parser = argparse.ArgumentParser(
        description="Build mmcore hub catalogue under dist/"
    )
    parser.add_argument("--root", default=".", help="Repository root")
    parser.add_argument(
        "--generated",
        help="ISO-8601 UTC timestamp for index generated (default: git newest)",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Validate and build into a temp tree without writing repo dist/",
    )
    args = parser.parse_args(argv)
    repo = Path(args.root).resolve()
    dist = repo / "dist"
    if args.dry_run:
        count = dry_run(repo)
        print(f"dry-run ok ({count} apps)")
        return
    generated = args.generated or git_generated(repo)
    count = build(repo, dist, generated)
    print(f"wrote {dist / 'index.json'} ({count} apps)")


if __name__ == "__main__":
    main(sys.argv[1:])