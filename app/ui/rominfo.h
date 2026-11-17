/* What a DS game can do with other consoles, for the library's badges: one card each (GameTDB's count, a table
   built in), and DS Download Play (read from the ROM). The ROM's answer is kept in a small file, by path, size and
   time, so a game is read once. */
#pragma once

enum { ROM_MULTICARD = 1, ROM_DLPLAY = 2 };

typedef struct {
    int flags;    /* ROM_MULTICARD | ROM_DLPLAY */
    int players;  /* with ROM_MULTICARD: the most players with one card each */
} RomCaps;

/* 0 for a file it can't read (a .zip, a missing file): no badges */
int rom_caps(const char *rom, RomCaps *out);
/* the same without the cache: the ROM read now (for previews and checks) */
int rom_caps_read(const char *rom, RomCaps *out);
