/* What a DS game can do with other consoles, for the library's badges: one card each (GameTDB's count, a table
   built in), and DS Download Play (read from the ROM). The library reads its ROMs on a thread of its own
   (rcw_*), near the picked game first and then the rest, so a slow card never holds the screen or the buttons.
   The answers are kept in memory and in a small file, by path, size and time, so a game is read once. */
#pragma once

enum { ROM_MULTICARD = 1, ROM_DLPLAY = 2 };

typedef struct {
    int flags;    /* ROM_MULTICARD | ROM_DLPLAY */
    int players;  /* with ROM_MULTICARD: the most players with one card each */
} RomCaps;

/* the ROM read now, without the kept answers (previews and checks). 0 for a file it can't read (a .zip, a missing
   file): no badges */
int rom_caps_read(const char *rom, RomCaps *out);

/* ---- the library's reader thread ---- */
/* the library's games, n paths stride bytes apart (copied; again whenever the list changes or is sorted). Starts
   the thread the first time */
void rcw_set_list(const char *paths, int stride, int n);
/* the picked game: it and the games within radius on either side are read before the rest */
void rcw_focus(int idx, int radius);
/* 1 with the answer for list entry idx (no badges for a file it can't read), 0 while it hasn't been read */
int rcw_get(int idx, RomCaps *out);
/* nothing read while paused (a reading under way stops at its next step and is read again later) */
void rcw_pause(int on);
/* the thread stopped (a reading under way given up) and the answers written to the file */
void rcw_stop(void);
