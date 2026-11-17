# Community packages

Prebuilt `.app` archives submitted by the community live here. The hub builder
(`tools/hub/build_index.py`) copies them into `dist/<id>.app` beside official
game packages and refreshes `dist/index.json` and `dist/info/`.

## How to submit

1. Build your project with mmcore's `PACKAGE` wizard (ZIP store, root `MAIN.BAS`).
2. Add a `PACKAGE.INF` with at least `title`, `author`, and `category` (see
   [mmcore PACKAGE metadata](https://github.com/marnixk/mmcore/issues/1523)).
3. Optionally add `INFO/ICON.PNG` (32×32), `INFO/THUMB.PNG` (160×90),
   `INFO/HERO.PNG` (640×360, indexed as the optional `hero` field),
   `INFO/SHOT1.PNG`…`INFO/SHOT4.PNG`, and `INFO/ABOUT.TXT` inside the archive.
4. Name the file `<id>.app` where `<id>` matches `^[a-z0-9_-]{1,32}$` (usually
   the lowercase package `name` without `.APP`).
5. Open a pull request adding `community/<id>.app`. CI runs the hub index builder
   to validate metadata and assets before merge.

Duplicate ids across `dist/` and `community/` are rejected.