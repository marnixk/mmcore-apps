# Jetpack (DOS) — licence and redistribution (phase 1 findings)

## Official source

| Item | Detail |
|------|--------|
| Product | **Jetpack v1.5** freeware (`jetpak15.zip`) |
| Author | Adam Pedersen / **Adept Software** |
| Official pages | [adeptsoftware.com/jetpack/](https://www.adeptsoftware.com/jetpack/), [adeptsoftware.com/classics/](https://www.adeptsoftware.com/classics/) |
| README in archive (1998-12-17) | *"Jetpack and Squarez Deluxe. They are now released as freeware, since they are old and there's no way to order them any more."* |

### Download used for extraction (2026-10-27)

The Adept site direct ZIP returned HTTP 404 during this run. The same **jetpak15.zip** was fetched from the mirror documented on [classicdosgames.com](https://classicdosgames.com/files/games/adampedersen/jetpak15.zip).

| File | MD5 | SHA-256 |
|------|-----|---------|
| `jetpak15.zip` | `8ae51e671e97edd7423eebe94fdaa433` | `c643ab8f478a913f840a000f69faa9eca15c3a2c10d3cb38a51f5935e865115e` |

Per-file hashes of extracted contents match the [TASVideos JPC-RR image listing](https://tasvideos.org/GameResources/DOS/Jetpack) (e.g. `JETLEV.DAT` MD5 `2fb5d8421b228975b622162be33a6b4d`, `JETPACK.EXE` MD5 `351d5f7893180e2925901c10b29dc1b6`).

## Manual / copyright (restrictive)

`MANUAL.TXT` in the freeware package states (abridged):

> This program is a product of Adam Pedersen. It is protected by United States copyright laws … You may **NOT** modify or create derivative works based on this program … You may **NOT** sell, lease, rent or perform any modifications …

That is the classic shareware manual wording; it does **not** explicitly grant redistribution of extracted art in a third-party repo.

## Discord permission (permissive for inspired ports)

Adam Pedersen [reported on the official Jetpack Discord](https://gist.github.com/downerj/0bd0664cd407da0eb4d438531176fd37) (quoted in [Level File Layout](https://gist.githubusercontent.com/downerj/0bd0664cd407da0eb4d438531176fd37/raw/Level%20File%20Layout.md)):

> I hereby grant anyone the right to make a Jetpack-inspired game, including use of all assets (graphics, audio, and levels), as long as it provides clear attribution to the original, and acknowledges that all intellectual property rights remain with Adept Software.

## Recommendation for this repository

| Asset | Commit to public repo? |
|-------|-------------------------|
| `jetpak15.zip`, `JETPACK.EXE`, `JETSOUND.DAT`, etc. | **No** — freeware download, not an open-content grant in the manual |
| Python extraction scripts + format docs | **Yes** |
| Level JSON + PNG previews derived from `JETLEV.DAT` | **Yes**, with attribution (Discord terms) and README credit |
| Decoded PNG sprite sheet | **Yes** for port development, with attribution; treat as derived from copyrighted data |
| Mockups (composite UI) | **Yes** |

**Default workflow:** scripts read `JETLEV.DAT` / `JETPACK*.DAT` from a local path (`tools/jetpack/_download/…`, gitignored). CI or other clones run `scripts/extract_all.py` after obtaining `jetpak15.zip` from Adept or a mirror.

**Attribution in game:** title screen line *"JETPACK, a port for mmcore"* plus credit to Adept Software / Adam Pedersen and link to [adeptsoftware.com/jetpack/](https://www.adeptsoftware.com/jetpack/).
