#!/usr/bin/env python3
"""Build hub.mmcore.dev static site: index.json, apps/, info/, _headers."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import shutil
import struct
import subprocess
import sys
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

INFO_ICON = "INFO/ICON.PNG"
INFO_THUMB = "INFO/THUMB.PNG"
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
        out = ""
    if not out:
        return ""
    if out.endswith("Z"):
        return out
    try:
        ts = datetime.fromisoformat(out.replace("Z", "+00:00"))
        if ts.tzinfo is None:
            ts = ts.replace(tzinfo=timezone.utc)
        return ts.astimezone(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
    except ValueError:
        return out


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
        for key in [INFO_ICON, INFO_THUMB, INFO_ABOUT, *INFO_SHOTS]:
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
    result: dict[str, object] = {"icon": "", "thumb": "", "shots": [], "about": ""}

    if INFO_ICON in info:
        check_info_png(path, INFO_ICON, info[INFO_ICON], 32, 32)
        (dest / "icon.png").write_bytes(info[INFO_ICON])
        result["icon"] = f"info/{app_id}/icon.png"

    if INFO_THUMB in info:
        check_info_png(path, INFO_THUMB, info[INFO_THUMB], 160, 90)
        (dest / "thumb.png").write_bytes(info[INFO_THUMB])
        result["thumb"] = f"info/{app_id}/thumb.png"

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
    name = fields.get("name", "")
    title = fields.get("title", "") or app_id
    category = fields.get("category", "") or "OTHER"
    if category not in CATEGORY_SET:
        category = "OTHER"
    updated = git_updated(repo, source_git)
    rec: dict[str, object] = {
        "id": app_id,
        "file": f"apps/{app_id}.app",
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
        "shots": info_meta.get("shots", []),
        "about": info_meta.get("about", ""),
    }
    return rec


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


def write_headers(out: Path) -> None:
    text = (
        "/index.json\n"
        "  Cache-Control: max-age=300\n"
        "/apps/*\n"
        "  Cache-Control: max-age=86400\n"
        "/info/*\n"
        "  Cache-Control: max-age=86400\n"
    )
    (out / "_headers").write_text(text, encoding="utf-8")


def collect_apps(repo: Path, dist: Path, community: Path) -> list[tuple[Path, Path]]:
    found: list[tuple[Path, Path]] = []
    for folder, git_path_fn in (
        (dist, lambda p: repo / "games" / p.stem),
        (community, lambda p: p),
    ):
        if not folder.is_dir():
            continue
        for path in sorted(folder.glob("*.app")):
            found.append((path, git_path_fn(path)))
    return found


def build(repo: Path, out: Path, generated: str) -> None:
    dist = repo / "dist"
    community = repo / "community"
    apps_dir = out / "apps"
    info_dir = out / "info"
    if out.exists():
        shutil.rmtree(out)
    apps_dir.mkdir(parents=True)
    info_dir.mkdir(parents=True)

    packages = collect_apps(repo, dist, community)
    seen_ids: dict[str, Path] = {}
    records: list[dict[str, object]] = []

    for path, git_src in packages:
        fields, info = read_zip_inf(path)
        validate_fields(path, fields)
        basename = path.stem.lower()
        app_id = app_id_from_inf(fields.get("name", ""), basename)
        validate_app_id(app_id, str(path))
        if app_id in seen_ids:
            raise SystemExit(
                f"duplicate id {app_id!r}: {seen_ids[app_id]} and {path}"
            )
        seen_ids[app_id] = path

        dest_app = apps_dir / f"{app_id}.app"
        shutil.copy2(path, dest_app)
        info_meta = extract_info(path, app_id, info, info_dir)
        records.append(app_record(path, repo, git_src, fields, info_meta, app_id))

    sort_apps(records)
    index = {
        "schema": 1,
        "generated": generated,
        "categories": CATEGORIES,
        "apps": records,
    }
    (out / "index.json").write_bytes(dump_index(index))
    write_headers(out)


def main(argv: list[str]) -> None:
    parser = argparse.ArgumentParser(description="Build mmcore hub static site")
    parser.add_argument(
        "--root",
        default=".",
        help="Repository root (default: cwd)",
    )
    parser.add_argument(
        "--out",
        default="site",
        help="Output directory (default: site/)",
    )
    parser.add_argument(
        "--generated",
        help="ISO-8601 UTC timestamp for index generated field",
    )
    args = parser.parse_args(argv)
    repo = Path(args.root).resolve()
    out = Path(args.out)
    if not out.is_absolute():
        out = (repo / out).resolve()
    if args.generated:
        generated = args.generated
    else:
        generated = datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
    build(repo, out, generated)
    print(f"wrote {out / 'index.json'} ({len(list((out / 'apps').glob('*.app')))} apps)")


if __name__ == "__main__":
    main(sys.argv[1:])