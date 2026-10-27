LEVEL_SIZE = 506
LEVEL_HEADER_SKIP = 2
MAP_W, MAP_H = 26, 16
TILE_PX = 24

ENEMY_NAMES = {
    0: "none",
    1: "trackbot",
    2: "steel_ball",
    3: "spring",
    4: "missile",
    5: "spikes",
    6: "flitzer",
    7: "homer",
    8: "batbot",
}

ITEM_TILE_IDS = {
    0x04: "gem",
    0x0F: "fuel_double",
    0x10: "fuel_single",
    0x11: "charger",
    0x12: "drain",
    0x13: "stunner",
    0x14: "coin",
    0x15: "gold_dust",
    0x16: "gold_vase",
    0x17: "extra_life",
}

# JETLEV editor spawn markers (grey "n" tiles); 0x3C (60) is the playfield backdrop.
SPAWN_MARKER_TILE_IDS = frozenset(range(0x32, 0x3C)) | frozenset(range(0x3D, 0x45))

TILE_NAMES = {
    0x00: "blank",
    0x01: "ladder",
    0x02: "escalator_up",
    0x03: "escalator_down",
    0x04: "gem",
    0x05: "gem_collected",
    0x06: "teleport_yellow",
    0x07: "teleport_pink",
    0x08: "teleport_green",
    0x09: "toggle_blue_off",
    0x0A: "toggle_blue_on",
    0x0B: "toggle_red_off",
    0x0C: "toggle_red_on",
    0x0D: "toggle_cyan_off",
    0x0E: "toggle_cyan_on",
    0x0F: "fuel_double",
    0x10: "fuel_single",
    0x11: "charger",
    0x12: "drain",
    0x13: "stunner",
    0x46: "crate",
    0x47: "steel_barrier",
    0x48: "diamond_block",
    0x3C: "backdrop",
    0x72: "conveyor_left",
    0x75: "conveyor_right",
}
