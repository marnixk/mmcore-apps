#!/usr/bin/env python3
"""Export moving graphics from a conventional-memory dump.

Reproducible capture (not committed):

    python3 scripts/dump_vga_level.py --level 99 --display :240 --out /tmp/lv99.bin
    python3 scripts/dump_vga_level.py --level 7 --display :241 --sequence --out /tmp/seq7.bin

``--sequence`` asks the TSR (``vga_dump.asm``) to write ``F00.BIN``..``F19.BIN``
every 3 IRQ0 ticks, then ``MEM.BIN``. This script reads that memory image and
the single-frame ``JPVG`` dumps. It does not read copyrighted binaries out of
``games/``.
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from jetpack.levels import load_jetlev  # noqa: E402
from jetpack.moving import (  # noqa: E402
    DOOR_ORIGINS,
    PITCH,
    SEPARATOR,
    SPARK_INDICES,
    SPRITE_ROWS,
    STRIDE,
    cell,
    door_frame,
    find_sheet_base,
    paint,
    playfield_any,
    sheet_view,
)
from jetpack.vga_index import VGA_TILE  # noqa: E402
from jetpack.vga_index import dac_to_rgb, load_dac6, load_sheet, render_indices  # noqa: E402

# Sheet row, columns, enemy type id (None for the player), and which columns
# were the bitmap actually on a dumped framebuffer. Columns not listed under
# observed are stored in the sheet and did not appear in the captures.
GROUPS = [
    {
        "name": "player_facing_a",
        "row": 114,
        "cols": list(range(18)),
        "enemy_type_id": None,
        "observed_cols": [0, 1, 2, 3, 10, 16, 17],
        "note": (
            "Facing right, frame = column. Walk cols 0–3 are (x >> 6) mod 4, taken from x "
            "before the step is added (EXE file 0x10175). Levels 5 and 37 stored all four "
            "while x changed, and the bitmap of column 0 and column 1 was exact on level 1. "
            "There is no fixed IRQ period: the frame follows position. Cols 4–9 are the jet "
            "flame (EXE file 0xFEAA): (counter >> 1) mod 3, plus 7 when thrusting right, "
            "plus 4 when the thrust axis is zero, two counter ticks per step. Cols 10–12 "
            "are the phaser pose, not the jet (EXE file 0x1146B); level 1 drew column 10 "
            "exactly while Delete was held. Cols 16 and 17 are the airborne poses level 6 "
            "stored while the jet key was held. Cols 13–15 were not sampled, and the "
            "flame cycle in cols 4–9 did not run during that hold."
        ),
    },
    {
        "name": "player_facing_b",
        "row": 127,
        "cols": list(range(18)),
        "enemy_type_id": None,
        "observed_cols": [0, 1, 2, 3, 16, 17],
        "note": (
            "Facing left, frame = 24 + column. Walk frames 24–27 are the same (x >> 6) mod 4 "
            "plus 24 (EXE file 0x10175 when the facing word is negative). Levels 5 and 1 "
            "stored all four, and level 1 drew frames 24 and 26 with every opaque pixel "
            "matching the sheet. The left jet is frames 31–33 or 28–30 (EXE file 0xFED7 / "
            "0xFEFF) and the left phaser is frames 34–39 (EXE file 0x113DA). Those were not "
            "sampled. Frames 40 and 41 (cols 16 and 17) are the left airborne poses and "
            "level 6 stored both while the jet key was held."
        ),
    },
    {
        "name": "player_more",
        "row": 140,
        "cols": [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 17, 18, 19, 20, 21, 22, 23],
        "enemy_type_id": None,
        "observed_cols": [0, 1, 2, 3],
        "note": (
            "Climb is frames 48–51 (cols 0–3): (y / 200) mod 4 + 48 (EXE file 0x101AA). "
            "Level 1 drew all four, each an exact sheet match, while y changed on a ladder. "
            "The frame follows y, so it has no fixed IRQ period. Cols 4–13 are the up/down "
            "phaser poses (frames 52–60, EXE file 0x114B2 and 0x11513) and were not sampled. "
            "Cols 17–22 are frames 65–70, a counter flash written just before one blit "
            "(EXE file 0x142FA) and not seen. Col 23 is frame 71, the breakup hold for "
            "counter/100 = 0 (EXE file 0x1241A); the deaths captured were later groups and "
            "held 83, 89 and 95 instead."
        ),
    },
    {
        "name": "player_breakup",
        "row": 153,
        "cols": list(range(24)),
        "enemy_type_id": None,
        "observed_cols": [7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23],
        "note": (
            "Death breakup, EXE file 0x123C2. While (counter mod 100) < 36 and counter < 1000, "
            "frame = (counter/100)*6 + ((counter mod 100)/6) mod 6 + 72. Each of those six "
            "steps is 6 counter ticks. After that the frame holds: 71 when counter/100 is 0, "
            "otherwise (counter/100)*6 + 77 (83, 89 or 95). Level 1 (died on a ladder) stored "
            "84–89 and held 89; frames 86 and 89 matched the sheet exactly, and 84 was still "
            "the climb sprite, so the byte leads the blit by about one update. Level 8 "
            "stored 79–83 and held 83. Levels 6 and 37 stored 91–95 and held 95. Each of "
            "the six frames occupied one sample, and samples are 3 IRQ0 ticks apart, so "
            "the on-screen hold per frame is one sample under this DOSBox run. Group "
            "72–77 did not come up. The same routine covers every death; nothing in it "
            "branches on enemy versus spark."
        ),
    },
    {
        "name": "steel_ball",
        "row": 166,
        "cols": [0, 1, 2, 3],
        "enemy_type_id": 2,
        "observed_cols": [0, 1, 2, 3],
        "note": (
            "Roll, EXE file 0x15472. While the anim byte is below 0x40 the frame is "
            "2 - ((byte >> 1) mod 3), so 2, 1, 0 for two counter ticks each. At or above "
            "0x40 the frame is the remainder itself, 0, 1, 2. Frame 3 is written when the "
            "bounce flag is set (EXE file 0x154D0). Level 1 stored all four. Bounce holds "
            "were 1–14 samples and roll holds 1–4 samples; a sample is 3 IRQ0 ticks."
        ),
    },
    {
        "name": "trackbot",
        "row": 166,
        "cols": [4, 5, 6, 7, 8, 9, 10, 11],
        "enemy_type_id": 1,
        "observed_cols": [4, 5, 6, 7, 8, 9, 10, 11],
        "note": (
            "Walk, EXE file 0x14E83: (x >> 6) mod 3 + 4, plus 3 when horizontal velocity "
            "is negative, so frames 4–6 and 7–9. Ladder, EXE file 0x14E58: (y >> 6) bit 0 "
            "+ 10, frames 10–11. Level 37 stored all eight. While the bot was moving a "
            "frame usually lasted 1–2 samples (3–6 IRQ0 ticks); it also sat on one frame "
            "for the rest of a capture. The frame follows position, not a timer."
        ),
    },
    {
        "name": "spring",
        "row": 166,
        "cols": [12, 13, 14, 15],
        "enemy_type_id": 3,
        "observed_cols": [12, 13, 14, 15],
        "note": (
            "Bob, EXE file 0x15924: (y / 200) bit 0 + 13, frames 13 and 14. Turn poses are "
            "immediates: frame 12 when the anim byte is 0 against a solid tile (the byte "
            "is then set to 10) and frame 15 when it is 1 (set to 20), EXE file 0x15905 "
            "and 0x15967. A level-16 capture stored all four. Bob holds were 1–8 samples "
            "and each turn pose was a single sample (3 IRQ0 ticks). How fast the anim byte "
            "counts is not separated from that. Level 55 left both springs on 13 for the "
            "whole 13 s, so the bob does not always run."
        ),
    },
    {
        "name": "spikes",
        "row": 166,
        "cols": [16, 17, 18],
        "enemy_type_id": 5,
        "observed_cols": [16],
        "note": (
            "Init frame is 16 (EXE file 0xDDBF and the same store at 0xE538). One later "
            "store, EXE file 0x16450, writes (x >> 5) mod 3 + 16, which can be 16, 17 or 18, "
            "but the same block multiplies both velocity words by 20, so it is a launch "
            "rather than a per-frame cycle. Level 3 held frame 16 on all four spikes for "
            "every sample. Frames 17 and 18 were not stored."
        ),
    },
    {
        "name": "homer",
        "row": 166,
        "cols": [19, 20],
        "enemy_type_id": 7,
        "observed_cols": [19],
        "note": (
            "EXE file 0x16D96 increments a byte, wraps it at 0x40, and stores (byte >> 5) + 19. "
            "That is frame 19 for 32 counter ticks, then frame 20 for 32, then repeat. "
            "Level 55 kept the byte at 19 for all 80 samples (about 13 s) while the "
            "playfield still changed by 5487 pixels, so the switch to frame 20 was not "
            "observed and the call rate of this routine is not known. Both columns use "
            "indices 247 and 254, so a homer on frame 19 still blinks with the palette cycle."
        ),
    },
    {
        "name": "missile_up",
        "row": 166,
        "cols": [21, 22, 23],
        "enemy_type_id": 4,
        "observed_cols": [21, 23],
        "note": (
            "EXE file 0x15E32: frame = (counter / 10) * 3 + ((counter >> 1) mod 3) + 21. "
            "The /10 term is the facing (0 up, 1 down, 2 left, 3 right) and the shift is "
            "the flame, two counter ticks per flame step. This row is facing 0, frames "
            "21–23. Level 5 stored 21 and 23. Frame 22 was not in a sample. Holds of a "
            "single flame step were 1–4 samples (3–12 IRQ0 ticks)."
        ),
    },
    {
        "name": "missile_other",
        "row": 179,
        "cols": [0, 1, 2, 3, 4, 5, 6, 7, 8],
        "enemy_type_id": 4,
        "observed_cols": [0, 1, 2, 4, 5, 6, 7, 8],
        "note": (
            "Facings 1–3 of the same formula: frames 24–26, 27–29 and 30–32. Level 5 "
            "stored 24, 25, 26, 28, 29, 30, 31 and 32. Frame 27 (column 3) was not in a "
            "sample. Frame 32 matched the sheet at one cell with every opaque pixel. "
            "Flame holds were 1–4 samples, the same as the upward facing."
        ),
    },
    {
        "name": "flitzer",
        "row": 179,
        "cols": list(range(9, 18)),
        "enemy_type_id": 6,
        "observed_cols": [9, 11, 12, 13, 14, 15, 16, 17],
        "note": (
            "EXE file 0x16B22 stores (counter / 20) + 33. Frame 33 is the value it has at "
            "rest. Level 37 stored 33 and 35–41. Frame 34 was not in a sample. The byte "
            "jumps (33 then 38 then 37) instead of counting up, and a frame then sat for "
            "58 samples, so this is a direction id rather than a 9-step cycle. When it "
            "did change, the new frame usually lasted 1–3 samples (3–9 IRQ0 ticks)."
        ),
    },
    {
        "name": "batbot",
        "row": 179,
        "cols": [18, 19, 20, 21, 22],
        "enemy_type_id": 8,
        "observed_cols": [18],
        "note": (
            "EXE file 0x17374 writes frame 42 while vertical velocity is positive. Otherwise "
            "frame = (counter >> 4) / 3 + 42, and another block adds 0x10 to that counter "
            "while a timer is below 15, then clears the high nibble. Quotients 0–4 are "
            "frames 42–46; three increments of the high nibble share one frame. Col 23 "
            "duplicates an earlier cell and is not its own frame number. Level 8 held "
            "frame 42 for every sample. Frames 43–46 were not stored."
        ),
    },
]

# Map tiles whose bitmap did not change, exported as a one-frame strip.
STATIC_TILES = [
    ("tube_4a", 0x4A),
    ("tube_4b", 0x4B),
    ("tube_4c", 0x4C),
    ("tube_4d", 0x4D),
    ("tube_4e", 0x4E),
    ("tube_4f", 0x4F),
    ("teleport_yellow", 6),
    ("teleport_pink", 7),
    ("teleport_green", 8),
    ("conveyor_left", 0x72),
    ("conveyor_right", 0x75),
    ("gem", 4),
    ("fuel_double", 0x0F),
    ("fuel_single", 0x10),
]


STATE_LABEL = {
    "player_facing_a": "player right — walk 0-3; jet 4-9; phaser 10-15",
    "player_facing_b": "player left — walk 24-27; jet 28-33; phaser 34-39",
    "player_more": "climb 48-51; phaser up/down 52-60; flash 65-70",
    "player_breakup": "death — six frames x 6 counter ticks, then a hold",
    "steel_ball": "steel ball — roll 0-2 (2 counts), bounce 3",
    "trackbot": "trackbot — walk 4-9 by (x>>6) mod 3; climb 10-11",
    "spring": "spring — bob 13/14 by y/200; turn poses 12 and 15",
    "spikes": "spikes — init 16; 17 and 18 only from the launch store",
    "homer": "homer — 19 then 20, 32 counter ticks each",
    "missile_up": "missile up 21-23 — flame (counter>>1) mod 3",
    "missile_other": "missile down/left/right 24-32, same flame",
    "flitzer": "flitzer — frame = counter/20 + 33 (33 and 35-41 seen)",
    "batbot": "batbot — 42 while rising, else (counter>>4)/3 + 42",
}


# Absolute frame ids. sheet column is frame % 24 on that group's row.
# timing is the EXE rule. observed is what the scripted captures stored.
ANIM_STATES = {
    "player_facing_a": [
        {"name": "walk_right", "frames": [0, 1, 2, 3], "timing": "(x >> 6) mod 4, from x before the step is added. One step per physics update, not a fixed IRQ period.", "exe_file": "0x10175", "observed": True},
        {"name": "jet_right", "frames": [7, 8, 9], "timing": "(counter >> 1) mod 3 + 7 while thrusting right. Two counter ticks per frame.", "exe_file": "0xFEAA", "observed": False},
        {"name": "jet_idle_right", "frames": [4, 5, 6], "timing": "(counter >> 1) mod 3 + 4 when the thrust axis is zero and the suit is still facing right.", "exe_file": "0xFEFF", "observed": False},
        {"name": "phaser_right", "frames": [10, 11, 12], "timing": "(counter >> 1) mod 3 + 10. Two counter ticks per frame. No separate beam bitmap; the routine then reads the map cell ahead.", "exe_file": "0x1146B", "observed": "frame 10 only"},
        {"name": "phaser_right_alt", "frames": [13, 14, 15], "timing": "Same cycle plus 3 when the alt byte is set.", "exe_file": "0x1146B",         "observed": False},
        {"name": "airborne_right", "frames": [16, 17], "timing": "Single poses: 16 when horizontal speed is past 0x14, else 17. Level 6 stored both while the jet key was held.", "exe_file": "0x1043D", "observed": True},
    ],
    "player_facing_b": [
        {"name": "walk_left", "frames": [24, 25, 26, 27], "timing": "(x >> 6) mod 4 + 24.", "exe_file": "0x10175", "observed": True},
        {"name": "jet_left", "frames": [31, 32, 33], "timing": "(counter >> 1) mod 3 + 31.", "exe_file": "0xFED7", "observed": False},
        {"name": "jet_idle_left", "frames": [28, 29, 30], "timing": "(counter >> 1) mod 3 + 28.", "exe_file": "0xFEFF", "observed": False},
        {"name": "phaser_left", "frames": [34, 35, 36], "timing": "(counter >> 1) mod 3 + 34, or +37 when the alt byte is set.", "exe_file": "0x113DA", "observed": False},
        {"name": "airborne_left", "frames": [40, 41], "timing": "40 when horizontal speed is past 0x14, else 41. Level 6 stored both.", "exe_file": "0x1044A", "observed": True},
    ],
    "player_more": [
        {"name": "climb", "frames": [48, 49, 50, 51], "timing": "(y / 200) mod 4 + 48. Follows height, no fixed IRQ period.", "exe_file": "0x101AA", "observed": True},
        {"name": "phaser_up", "frames": [52, 53, 54, 55, 56, 57], "timing": "(counter >> 1) mod 3 + 52, plus 3 when the alt byte is set.", "exe_file": "0x114D9", "observed": False},
        {"name": "phaser_down", "frames": [58, 59, 60], "timing": "(counter mod 3) + 58. One counter tick per frame.", "exe_file": "0x11522", "observed": False},
        {"name": "airborne_state4", "frames": [61], "timing": "Single pose when the state byte is 4.", "exe_file": "0x10481", "observed": False},
        {"name": "flash", "frames": [65, 66, 67, 68, 69, 70], "timing": "Mostly 65/66 (counter bit 1) while counter mod 25 is 0..17; briefly 67/68 then 69/70.", "exe_file": "0x142FA", "observed": False},
        {"name": "breakup_hold_group0", "frames": [71], "timing": "Hold when the death counter mod 100 has passed 35 and counter/100 is 0.", "exe_file": "0x1241A", "observed": False},
    ],
    "player_breakup": [
        {"name": "breakup", "frames": list(range(72, 96)), "timing": "Six frames of 6 counter ticks, group = counter/100 (0..3), then hold 71, 83, 89 or 95. Captured holds were 83, 89 and 95. Each captured step lasted one 3-IRQ sample.", "exe_file": "0x123C2", "observed": "groups 1, 2 and 3"},
    ],
    "steel_ball": [
        {"name": "roll", "frames": [0, 1, 2], "timing": "Two counter ticks per frame. Order 2,1,0 while the anim byte is below 64, else 0,1,2.", "exe_file": "0x15472", "observed": True},
        {"name": "bounce", "frames": [3], "timing": "Written for the bounce, not a timed cycle. Holds of 1–14 samples on level 1.", "exe_file": "0x154D0", "observed": True},
    ],
    "trackbot": [
        {"name": "walk_right", "frames": [4, 5, 6], "timing": "(x >> 6) mod 3 + 4.", "exe_file": "0x14E83", "observed": True},
        {"name": "walk_left", "frames": [7, 8, 9], "timing": "Same plus 3 when velocity is negative.", "exe_file": "0x14E83", "observed": True},
        {"name": "climb", "frames": [10, 11], "timing": "(y >> 6) bit 0 + 10.", "exe_file": "0x14E58", "observed": True},
    ],
    "spring": [
        {"name": "turn_ground", "frames": [12], "timing": "Set when the anim byte is 0 on a solid tile; the byte is then loaded with 10.", "exe_file": "0x15905", "observed": True},
        {"name": "bob", "frames": [13, 14], "timing": "(y / 200) bit 0 + 13.", "exe_file": "0x15924", "observed": True},
        {"name": "turn_ceiling", "frames": [15], "timing": "Set when the anim byte is 1 on a solid tile; the byte is then loaded with 20.", "exe_file": "0x15967", "observed": True},
    ],
    "spikes": [
        {"name": "idle", "frames": [16], "timing": "Init value. No per-frame cycle was found.", "exe_file": "0xDDBF", "observed": True},
        {"name": "launch_pose", "frames": [16, 17, 18], "timing": "(x >> 5) mod 3 + 16, in the block that also multiplies velocity by 20. Not shown to repeat.", "exe_file": "0x16450", "observed": False},
    ],
    "homer": [
        {"name": "blink_pose", "frames": [19, 20], "timing": "(counter >> 5) + 19, counter wraps at 64, so 32 counter ticks each. The 19 to 20 switch was not seen in 13 s.", "exe_file": "0x16D96", "observed": "frame 19 only"},
    ],
    "missile_up": [
        {"name": "up", "frames": [21, 22, 23], "timing": "Facing 0 of (counter/10)*3 + ((counter>>1) mod 3) + 21. Two counter ticks per flame frame.", "exe_file": "0x15E32", "observed": "21 and 23"},
    ],
    "missile_other": [
        {"name": "down", "frames": [24, 25, 26], "timing": "Facing 1, same flame.", "exe_file": "0x15E32", "observed": True},
        {"name": "left", "frames": [27, 28, 29], "timing": "Facing 2, same flame.", "exe_file": "0x15E32", "observed": "28 and 29"},
        {"name": "right", "frames": [30, 31, 32], "timing": "Facing 3, same flame.", "exe_file": "0x15E32", "observed": True},
    ],
    "flitzer": [
        {"name": "direction", "frames": list(range(33, 42)), "timing": "(counter / 20) + 33. Capture stored 33 and 35–41, jumping rather than counting, usually for 1–3 samples.", "exe_file": "0x16B22", "observed": "all except 34"},
    ],
    "batbot": [
        {"name": "rise", "frames": [42], "timing": "While vertical velocity is positive. Level 8 stayed on 42 for every sample.", "exe_file": "0x17382", "observed": True},
        {"name": "fly", "frames": [42, 43, 44, 45, 46], "timing": "(counter >> 4) / 3 + 42. Three high-nibble steps share a frame. Frames 43–46 were not stored.", "exe_file": "0x17390", "observed": False},
    ],
}


def rgba_of(sprite: np.ndarray, rgb: np.ndarray, transparent: bool) -> Image.Image:
    img = np.zeros(sprite.shape + (4,), np.uint8)
    img[..., :3] = rgb[sprite]
    img[..., 3] = 255
    if transparent:
        img[sprite == 0, 3] = 0
    return Image.fromarray(img, "RGBA")


def strip(frames: list[np.ndarray], rgb: np.ndarray, transparent: bool) -> Image.Image:
    if not frames:
        raise ValueError("empty strip")
    h = max(f.shape[0] for f in frames)
    w = sum(f.shape[1] for f in frames)
    acc = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    x = 0
    for frame in frames:
        acc.paste(rgba_of(frame, rgb, transparent), (x, 0))
        x += frame.shape[1]
    return acc


def palette_steps(sequence: Path) -> dict:
    frames = sorted(sequence.glob("F??.BIN"))
    if len(frames) < 4:
        raise RuntimeError(f"need four JPFR frames in {sequence}")
    steps = []
    ticks = []
    for path in frames[:4]:
        raw = path.read_bytes()
        ticks.append(int.from_bytes(raw[4:6], "little"))
        dac = np.frombuffer(raw[8 : 8 + 768], np.uint8).reshape(256, 3)
        steps.append(dac[247:255].tolist())
    deltas = [ticks[i + 1] - ticks[i] for i in range(3)]
    if deltas != [3, 3, 3]:
        raise RuntimeError(f"palette ticks are {ticks}, expected a step every 3 IRQ0 ticks")
    # The next three frames must be the same four colors, rotated.
    for i, path in enumerate(frames[4:8]):
        dac = np.frombuffer(path.read_bytes()[8 : 8 + 768], np.uint8).reshape(256, 3)
        if dac[247:255].tolist() != steps[i]:
            raise RuntimeError(f"{path.name} did not repeat palette step {i}")
    return {
        "indices": list(range(247, 255)),
        "groups": [[247, 248, 249, 250], [251, 252, 253, 254]],
        "direction": "rotate right inside each group of four",
        "ticks_per_step": 3,
        "period_steps": 4,
        "period_ticks": 12,
        "irq0_hz": 18.2065,
        "source_frames": [p.name for p in frames[:4]],
        "source_ticks": ticks,
        "steps_dac6": steps,
    }


def tube_changes(sequence: Path, level: int) -> dict:
    """Index changes of tube cells across a JPFR sequence. Zero means the bitmap is static."""
    from jetpack.levels import load_jetlev as _load

    frames = sorted(sequence.glob("F??.BIN"))
    levels = _load(ROOT / "_download" / "extracted" / "jetpak15" / "JETLEV.DAT")
    ids = np.array(levels[level].tiles).reshape(16, 26)
    base = playfield_any(frames[0])
    out = {}
    for tid in list(range(0x4A, 0x50)) + [6, 7, 8, 0x72, 0x75]:
        pos = list(zip(*np.where(ids == tid)))
        if not pos:
            out[f"0x{tid:02X}"] = {"present": False}
            continue
        r, c = pos[0]
        changed = 0
        for path in frames[1:]:
            fb = playfield_any(path)
            changed += int(
                (fb[r * 12 : (r + 1) * 12, c * 12 : (c + 1) * 12] != base[r * 12 : (r + 1) * 12, c * 12 : (c + 1) * 12]).sum()
            )
        out[f"0x{tid:02X}"] = {"present": True, "at": [int(r), int(c)], "changed_pixels": changed, "frames": len(frames)}
    return out


def _anchors(sprite: np.ndarray, fb: np.ndarray) -> list[tuple[int, int]]:
    """Candidate top-lefts from colors that occur more than once in the sprite.

    A one-pixel color is often the spark that is not on this frame, so it is
    not used as the anchor.
    """
    mask = sprite != 0
    vals, counts = np.unique(sprite[mask], return_counts=True)
    hist = np.bincount(fb.ravel(), minlength=256)
    ranked = []
    for value, count in zip(vals.tolist(), counts.tolist()):
        if count < 2:
            continue
        ranked.append((hist[value] / count, -count, int(value)))
    ranked.sort()
    found: list[tuple[int, int]] = []
    seen: set[tuple[int, int]] = set()
    h, w = sprite.shape
    for _score, _count, color in ranked[:4]:
        ys, xs = np.where(sprite == color)
        ay, ax = int(ys[0]), int(xs[0])
        cy, cx = np.where(fb == color)
        for y, x in zip((cy - ay).tolist(), (cx - ax).tolist()):
            if y < 0 or x < 0 or y + h > fb.shape[0] or x + w > fb.shape[1]:
                continue
            if (y, x) in seen:
                continue
            seen.add((y, x))
            found.append((y, x))
            if len(found) > 500:
                return found
    return found


def composite_level(buf: np.ndarray, tiles: list[int], door: tuple[int, int], fb: np.ndarray) -> tuple[np.ndarray, dict]:
    comp = render_indices(tiles)
    placed = []
    dx, dy = door
    if dx and dy:
        origin = ((dy - 1) * VGA_TILE, (dx - 1) * VGA_TILE)
        closed = door_frame(buf, 0)
        before = int((comp == fb).sum())
        paint(comp, closed, origin[0], origin[1], transparent=False)
        after = int((comp == fb).sum())
        err = None
        if 0 <= origin[0] and 0 <= origin[1] and origin[0] + 24 <= comp.shape[0] and origin[1] + 24 <= comp.shape[1]:
            err = int((fb[origin[0] : origin[0] + 24, origin[1] : origin[1] + 24] != closed).sum())
        placed.append({"name": "door", "frame": 0, "at": list(origin), "error": err, "gained": after - before})

    candidates = []
    for group in GROUPS:
        for col in group["cols"]:
            sprite = cell(buf, group["row"], col)
            mask = sprite != 0
            opaque = int(mask.sum())
            if opaque < 6:
                continue
            for y, x in _anchors(sprite, fb):
                win_fb = fb[y : y + 12, x : x + 12]
                win_c = comp[y : y + 12, x : x + 12]
                agree = mask & (win_fb == sprite)
                gain = int((agree & (win_c != win_fb)).sum())
                loss = int((mask & (win_fb != sprite) & (win_c == win_fb)).sum())
                uncovered = mask & (win_fb != sprite)
                clipped = int((uncovered & (win_fb == 0)).sum())
                real_miss = int(uncovered.sum()) - clipped
                # A spark on the sprite is a few non-zero misses. A black corner
                # can cover half of a sprite with index 0; that is clipped, not a
                # different frame. Another frame of the same enemy fails this.
                if loss == 0 and gain >= max(6, opaque // 2) and real_miss <= 6:
                    candidates.append((gain, group["name"], col, sprite, y, x, real_miss, clipped))
    candidates.sort(reverse=True)
    taken = np.zeros(fb.shape, np.uint8)
    for gain, name, col, sprite, y, x, real_miss, clipped in candidates:
        mask = sprite != 0
        if int((taken[y : y + 12, x : x + 12][mask]).sum()) > 4:
            continue
        paint(comp, sprite, y, x, transparent=True)
        taken[y : y + 12, x : x + 12][mask] = 1
        placed.append(
            {
                "name": name,
                "col": col,
                "at": [y, x],
                "gained": gain,
                "spark_or_phase_pixels": real_miss,
                "clipped_by_zero": clipped,
            }
        )
    diff = fb != comp
    residual = int(diff.sum())
    spark = int(np.isin(fb[diff], list(SPARK_INDICES)).sum()) if residual else 0
    black = int(((fb == 0) & diff).sum()) if residual else 0
    return comp, {
        "placed": placed,
        "pixels": int(fb.size),
        "mismatch": residual,
        "exact_index_pct": round(100.0 * (1.0 - residual / fb.size), 4),
        "residual_spark_indices": spark,
        "residual_framebuffer_zero": black,
        "residual_other": residual - spark,
    }


def side_by_side(fb: np.ndarray, comp: np.ndarray, rgb: np.ndarray) -> Image.Image:
    a = Image.fromarray(rgb[fb], "RGB").resize((fb.shape[1] * 2, fb.shape[0] * 2), Image.NEAREST)
    b = Image.fromarray(rgb[comp], "RGB").resize((comp.shape[1] * 2, comp.shape[0] * 2), Image.NEAREST)
    gap = 8
    out = Image.new("RGB", (a.width + gap + b.width, a.height), (0, 0, 0))
    out.paste(a, (0, 0))
    out.paste(b, (a.width + gap, 0))
    return out


def contact_sheet(images: list[tuple[str, Image.Image]]) -> Image.Image:
    pad = 8
    label_h = 14
    scale = 3
    width = 0
    height = pad
    scaled = []
    for name, im in images:
        big = im.resize((im.width * scale, im.height * scale), Image.NEAREST)
        scaled.append((name, big))
        width = max(width, big.width + pad * 2)
        height += label_h + big.height + pad
    sheet = Image.new("RGB", (width, height), (24, 24, 28))
    draw = ImageDraw.Draw(sheet)
    y = pad
    for name, big in scaled:
        draw.text((pad, y), name, fill=(230, 230, 230))
        y += label_h
        bg = Image.new("RGB", big.size, (48, 48, 52))
        bg.paste(big, mask=big.split()[-1])
        sheet.paste(bg, (pad, y))
        y += big.height + pad
    return sheet


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--mem", type=Path, required=True)
    ap.add_argument("--dumps", required=True, help="level=path pairs, comma separated")
    ap.add_argument("--sequence", type=Path, required=True, help="directory of F00.BIN.. JPFR frames")
    ap.add_argument("--sequence-level", type=int, default=7)
    ap.add_argument("--anim-dir", type=Path, default=ROOT / "gfx" / "anim")
    ap.add_argument("--sprites-json", type=Path, default=ROOT / "gfx" / "sprites.json")
    ap.add_argument("--stats", type=Path, default=ROOT / "reference" / "sprite_match.json")
    ap.add_argument("--artifacts", type=Path, default=None)
    args = ap.parse_args()

    mem = np.frombuffer(args.mem.read_bytes(), np.uint8)
    base = find_sheet_base(mem)
    buf = sheet_view(mem, base)
    rgb = dac_to_rgb(load_dac6())
    terrain = load_sheet()
    args.anim_dir.mkdir(parents=True, exist_ok=True)

    catalog = []
    index_blobs = []
    atlas_at = 0
    contact_rows = []
    mirror_pairs = []
    for col in range(18):
        a = cell(buf, 114, col)
        b = cell(buf, 127, col)
        if np.array_equal(b, a[:, ::-1]):
            mirror_pairs.append(col)

    for group in GROUPS:
        frames = []
        meta_frames = []
        for col in group["cols"]:
            sprite = cell(buf, group["row"], col)
            if int((sprite != 0).sum()) < 6 and group["name"].startswith("player_breakup") is False:
                if int((sprite != 0).sum()) == 0:
                    continue
            if int((sprite != 0).sum()) == 0:
                continue
            frames.append(sprite)
            raw = sprite.tobytes()
            index_blobs.append(raw)
            meta_frames.append(
                {
                    "sheet_row": group["row"],
                    "sheet_col": col,
                    "w": 12,
                    "h": 12,
                    "atlas_offset": atlas_at,
                    "opaque": int((sprite != 0).sum()),
                    "observed": col in group["observed_cols"],
                }
            )
            atlas_at += len(raw)
        if not frames:
            continue
        image = strip(frames, rgb, transparent=True)
        image.save(args.anim_dir / f"{group['name']}.png")
        contact_rows.append((STATE_LABEL.get(group["name"], group["name"]), image))
        catalog.append(
            {
                "name": group["name"],
                "file": f"gfx/anim/{group['name']}.png",
                "enemy_type_id": group["enemy_type_id"],
                "size": [12, 12],
                "transparency_index": 0,
                "frames": meta_frames,
                "states": ANIM_STATES.get(group["name"], []),
                "note": group["note"],
            }
        )

    doors = [door_frame(buf, i) for i in range(len(DOOR_ORIGINS))]
    door_img = strip(doors, rgb, transparent=False)
    door_img.save(args.anim_dir / "door.png")
    contact_rows.append(("door — frames 0-7, one step per 3 IRQ0 ticks, then hold open", door_img))
    door_frames = []
    for i, (sprite, (y, x)) in enumerate(zip(doors, DOOR_ORIGINS)):
        raw = sprite.tobytes()
        index_blobs.append(raw)
        door_frames.append(
            {
                "index": i,
                "sheet_y": y,
                "sheet_x": x,
                "atlas_offset": atlas_at,
                "observed": True,
            }
        )
        atlas_at += len(raw)
    catalog.append(
        {
            "name": "door",
            "file": "gfx/anim/door.png",
            "size": [24, 24],
            "transparency_index": None,
            "blit": "opaque",
            "frames": door_frames,
            "note": (
                "Eight unique 24×24 opaque frames. Level 16, with one gem placed on the "
                "player's right and the enemies removed, drew frame 0 closed, then frames "
                "1 through 7 in order. Each of 1–6 was on screen for one sample and frame 7 "
                "then held. Samples are 3 IRQ0 ticks apart and no opening frame was skipped "
                "or repeated, so the door advances one frame per 3 IRQ0 ticks and stays on "
                "the open frame. The 38-frame delay after the last gem (TAS) falls inside "
                "the sample before frame 1 and was not measured on its own."
            ),
        }
    )

    for name, tid in STATIC_TILES:
        sprite = terrain[tid]
        image = strip([sprite], rgb, transparent=False)
        image.save(args.anim_dir / f"{name}.png")
        contact_rows.append((name, image))
        raw = sprite.tobytes()
        index_blobs.append(raw)
        catalog.append(
            {
                "name": name,
                "file": f"gfx/anim/{name}.png",
                "tile_id": tid,
                "size": [12, 12],
                "transparency_index": None,
                "frames": [{"sheet_cell": tid, "atlas_offset": atlas_at, "observed": True}],
                "note": "Single bitmap in the terrain sheet. No second phase was found in the buffer.",
            }
        )
        atlas_at += len(raw)

    # Palette swatch: 4 steps × 8 indices.
    cycle = palette_steps(args.sequence)
    swatch = np.zeros((4 * 14, 8 * 14), np.uint8)
    dac = np.array(cycle["steps_dac6"], np.uint8)
    expanded = dac_to_rgb(dac.reshape(-1, 3)).reshape(4, 8, 3)
    sw = np.zeros((4 * 14, 8 * 14, 3), np.uint8)
    for s in range(4):
        for i in range(8):
            sw[s * 14 + 1 : s * 14 + 13, i * 14 + 1 : i * 14 + 13] = expanded[s, i]
    Image.fromarray(sw, "RGB").save(args.anim_dir / "palette_cycle.png")
    contact_rows.append(("palette 247-254, four steps", Image.fromarray(sw, "RGB").convert("RGBA")))

    atlas = b"".join(index_blobs)
    (args.anim_dir / "frames.u8").write_bytes(atlas)

    dumps = {}
    for part in args.dumps.split(","):
        level_s, path_s = part.split("=", 1)
        dumps[int(level_s)] = Path(path_s)
    levels = load_jetlev(ROOT / "_download" / "extracted" / "jetpak15" / "JETLEV.DAT")
    stats = {}
    if args.artifacts:
        args.artifacts.mkdir(parents=True, exist_ok=True)
    for level, path in sorted(dumps.items()):
        fb = playfield_any(path)
        level_obj = levels[level]
        comp, info = composite_level(buf, level_obj.tiles, (level_obj.door_x, level_obj.door_y), fb)
        # The running framebuffer paints the four corners index 0, about one
        # tile and sometimes the cell beside it, on top of whatever the map says.
        corner = np.zeros(fb.shape, dtype=bool)
        for row, col in ((0, 0), (0, 1), (0, 24), (0, 25), (15, 0), (15, 1), (15, 24), (15, 25)):
            block = fb[row * 12 : (row + 1) * 12, col * 12 : (col + 1) * 12]
            if float((block == 0).mean()) > 0.4:
                corner[row * 12 : (row + 1) * 12, col * 12 : (col + 1) * 12] = True
        outside = fb != comp
        outside[corner] = False
        kept = int((~corner).sum())
        info["corner_zero_pixels"] = int(corner.sum())
        info["exact_index_pct_outside_corners"] = round(100.0 * (1.0 - int(outside.sum()) / kept), 4)
        info["enemies"] = [
            {"type_id": e.type_id, "name": e.type_name, "x": e.x, "y": e.y} for e in level_obj.enemies
        ]
        stats[str(level)] = info
        print(
            f"level {level:3d}  exact {info['exact_index_pct']:8.4f}%  "
            f"outside corners {info['exact_index_pct_outside_corners']:8.4f}%  "
            f"mismatch {info['mismatch']:4d}  placed {len(info['placed'])}"
        )
        if args.artifacts:
            side_by_side(fb, comp, rgb).save(args.artifacts / f"side_level_{level:02d}.png")

    document = {
        "sheet": {
            "stride": STRIDE,
            "pitch": PITCH,
            "separator_index": SEPARATOR,
            "cell": VGA_TILE,
            "sprite_rows": list(SPRITE_ROWS),
            "columns": "content at col*13+1, width 12, columns 0..23",
            "base_linear": base,
            "base_note": (
                "Linear address in this dump. It moves with the EXE load segment. "
                "find_sheet_base() relocates it by matching terrain tile 0 and separator row 12."
            ),
            "transparency_index": 0,
            "door_blit": "opaque 24x24",
            "rows_not_exported": "y>=192 is the HIGH SCORE / menu / title bitmap and is not game art",
            "index_atlas": "gfx/anim/frames.u8 concatenates every frame in catalog order. atlas_offset is the byte start; 12x12 frames are 144 bytes and door frames are 576.",
        },
        "palette_cycle": cycle,
        "timed_tile_changes": tube_changes(args.sequence, args.sequence_level),
        "player_mirror_columns": mirror_pairs,
        "not_found": [
            "No separate phaser or digging bitmap. Delete cycles the player through directional poses (right 10–15, left 34–39, up 52–57, down 58–60) and reads the map cell in front. Level 1 drew pose 10 exactly; the other poses and the tile-erase timing were not measured. TAS quotes 5, 26 and 56 frames for crate, light brick and dark brick; that clock was not re-measured.",
            "Jet flame frames 4–9 and 28–33 are what EXE file 0xFEAA stores. Holding the jet key on level 6 produced the airborne poses 16, 17, 40 and 41 instead, so the flame cycle was not the bitmap at a sample.",
            "Frame 61 and the pre-blit flash 65–70 are in the EXE and were not on screen.",
            "Death group 72–77. The deaths captured held 83, 89 and 95.",
            "Missile frames 22 and 27. The other ten missile frames were stored on level 5.",
            "Flitzer frame 34. Frames 33 and 35–41 were stored on level 37.",
            "Homer frame 20. Level 55 stayed on 19 for about 13 s. The EXE does define 20.",
            "Spikes frames 17 and 18. Level 3 held 16. Batbot frames 43–46. Level 8 held 42.",
            "Tubes, teleports, conveyors and toggles still have one bitmap each. Fuel and the gem are map tiles. Spark pixels are the palette cycle, not a second bitmap, and the breakup routine does not branch on spark versus enemy.",
        ],
        "sprites": catalog,
    }
    args.sprites_json.write_text(json.dumps(document, indent=2) + "\n")
    args.stats.parent.mkdir(parents=True, exist_ok=True)
    # Stats without the long placed list duplicated: keep placed, it's the evidence.
    args.stats.write_text(json.dumps({"sheet_base": base, "levels": stats}, indent=2) + "\n")
    if args.artifacts:
        contact_sheet(contact_rows).save(args.artifacts / "sprite_contact.png")
        print(f"contact {args.artifacts / 'sprite_contact.png'}")
    print(f"wrote {args.sprites_json} base {hex(base)} mirror cols {mirror_pairs}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
