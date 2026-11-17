#define _GNU_SOURCE
#include "rominfo.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>

#include "multicart_table.h"

/* the kept answers: ROMCAPS_FILE, else BANANA_PARTY_ROMCAPS (empty: none kept, as in the previews) */
#define ROMCAPS_FILE "/data/bananads-lan/rom-caps.txt"
/* a ROM's FAT: at most this many entries are looked at (some 30 ms of reads on the handheld's card at worst) */
#define MAX_CANDIDATES 4096

static uint32_t le32(const unsigned char *p) { return p[0] | p[1] << 8 | p[2] << 16 | (uint32_t)p[3] << 24; }

static int read_at(FILE *f, long off, void *buf, size_t n)
{
    return fseek(f, off, SEEK_SET) == 0 && fread(buf, 1, n, f) == n;
}

/* the most players with one card each, from the table (0: not a multi-card game) */
static int table_players(const char id[4])
{
    int lo = 0, hi = MULTICART_N - 1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2, c = memcmp(id, multicart_ids[mid], 3);
        if (!c) return multicart_players[mid];
        if (c < 0) hi = mid - 1;
        else lo = mid + 1;
    }
    return 0;
}

/* the RSA signature DS Download Play needs after the used part of a ROM (the header's 0x80): "ac", 1, 0 */
static int signed_at(FILE *f, long base, uint32_t used, long limit)
{
    unsigned char s[4];
    if (used < 0x200 || (long)used + 4 > limit) return 0;
    return read_at(f, base + (long)used, s, 4) && s[0] == 'a' && s[1] == 'c' && s[2] == 1 && s[3] == 0;
}

typedef struct { uint32_t start, end; } Span;
static int by_start(const void *a, const void *b)
{
    uint32_t x = ((const Span *)a)->start, y = ((const Span *)b)->start;
    return x < y ? -1 : x > y;
}

/* DS Download Play: the game sends itself (signed like a child program, as Mario Kart DS), or it carries child
   programs in its files: a file of 16 KB to 4 MB with a DS header (the logo's first bytes at 0xC0) and the
   signature at its own used size. A child without the signature can't be downloaded by a DS, so it doesn't count. */
static int has_dlplay(FILE *f, const unsigned char *h, long size)
{
    static const unsigned char logo[8] = { 0x24, 0xff, 0xae, 0x51, 0x69, 0x9a, 0xa2, 0x21 };
    uint32_t fat = le32(h + 0x48), fat_n = le32(h + 0x4c) / 8;
    unsigned char *t;
    Span *c;
    int n = 0, found = 0;
    if (signed_at(f, 0, le32(h + 0x80), size)) return 1;
    if (!fat_n || fat < 0x200 || (long)fat + (long)fat_n * 8 > size) return 0;
    if (fat_n > 65536) fat_n = 65536;
    t = malloc((size_t)fat_n * 8);
    c = malloc(sizeof *c * MAX_CANDIDATES);
    if (!t || !c || !read_at(f, (long)fat, t, (size_t)fat_n * 8)) { free(t); free(c); return 0; }
    for (uint32_t i = 0; i < fat_n && n < MAX_CANDIDATES; i++) {
        uint32_t s = le32(t + i * 8), e = le32(t + i * 8 + 4);
        if (e > s && e - s >= 0x4000 && e - s <= 0x400000 && (long)e <= size) c[n++] = (Span){ s, e };
    }
    qsort(c, (size_t)n, sizeof *c, by_start);  /* in order through the file */
    for (int i = 0; i < n && !found; i++) {
        unsigned char k[0xc8];
        if (!read_at(f, (long)c[i].start, k, sizeof k) || memcmp(k + 0xc0, logo, sizeof logo)) continue;
        found = signed_at(f, (long)c[i].start, le32(k + 0x80), (long)(c[i].end - c[i].start) + (long)c[i].start);
    }
    free(t);
    free(c);
    return found;
}

static int ends_with(const char *s, const char *e)
{
    size_t a = strlen(s), b = strlen(e);
    return a >= b && strcasecmp(s + a - b, e) == 0;
}

/* the ROM itself: its game code and DS Download Play */
static int scan(const char *rom, char id[4], int *dlplay)
{
    unsigned char h[0x200];
    struct stat st;
    FILE *f;
    if (!(ends_with(rom, ".nds") || ends_with(rom, ".dsi"))) return 0;
    if (stat(rom, &st) != 0 || st.st_size < 0x200) return 0;
    if (!(f = fopen(rom, "rb"))) return 0;
    if (!read_at(f, 0, h, sizeof h)) { fclose(f); return 0; }
    memcpy(id, h + 0x0c, 4);
    *dlplay = has_dlplay(f, h, (long)st.st_size);
    fclose(f);
    return 1;
}

static void fill(RomCaps *out, const char id[4], int dlplay)
{
    int p = table_players(id);
    out->flags = (p >= 2 ? ROM_MULTICARD : 0) | (dlplay ? ROM_DLPLAY : 0);
    out->players = p >= 2 ? p : 0;
}

int rom_caps_read(const char *rom, RomCaps *out)
{
    char id[4];
    int dl = 0;
    memset(out, 0, sizeof *out);
    if (!scan(rom, id, &dl)) return 0;
    fill(out, id, dl);
    return 1;
}

/* ---- the kept answers: "size mtime dlplay code path" a line; a newer line for a path wins ---- */
typedef struct { char *path; long long size, mtime; int dl; char id[4]; } Kept;
static Kept *kept;
static int nkept, capkept, loaded;

static const char *cache_file(void)
{
    const char *e = getenv("BANANA_PARTY_ROMCAPS");
    return e ? e : ROMCAPS_FILE;
}

static Kept *find(const char *path)
{
    for (int i = nkept - 1; i >= 0; i--)
        if (!strcmp(kept[i].path, path)) return &kept[i];
    return NULL;
}

static Kept *add(const char *path)
{
    Kept *k = find(path);
    if (k) return k;
    if (nkept == capkept) {
        int nc = capkept ? capkept * 2 : 128;
        Kept *nk = realloc(kept, sizeof *nk * (size_t)nc);
        if (!nk) return NULL;
        kept = nk;
        capkept = nc;
    }
    k = &kept[nkept];
    memset(k, 0, sizeof *k);
    if (!(k->path = strdup(path))) return NULL;
    nkept++;
    return k;
}

static void load(void)
{
    const char *fn = cache_file();
    char line[1024];
    FILE *f;
    loaded = 1;
    if (!fn[0] || !(f = fopen(fn, "r"))) return;
    while (fgets(line, sizeof line, f)) {
        long long size, mtime;
        int dl, off = 0;
        char id[5];
        size_t n = strlen(line);
        if (n && line[n - 1] == '\n') line[--n] = 0;
        if (sscanf(line, "%lld %lld %d %4s %n", &size, &mtime, &dl, id, &off) != 4 || !off || strlen(id) != 4) continue;
        Kept *k = add(line + off);
        if (!k) break;
        k->size = size;
        k->mtime = mtime;
        k->dl = dl ? 1 : 0;
        memcpy(k->id, id, 4);
    }
    fclose(f);
}

int rom_caps(const char *rom, RomCaps *out)
{
    struct stat st;
    Kept *k;
    char id[4];
    int dl = 0;
    memset(out, 0, sizeof *out);
    if (!loaded) load();
    if (stat(rom, &st) != 0) return 0;
    k = find(rom);
    if (k && k->size == (long long)st.st_size && k->mtime == (long long)st.st_mtime) {
        fill(out, k->id, k->dl);
        return 1;
    }
    if (!scan(rom, id, &dl)) return 0;
    fill(out, id, dl);
    if ((k = add(rom))) {
        const char *fn = cache_file();
        FILE *f;
        k->size = (long long)st.st_size;
        k->mtime = (long long)st.st_mtime;
        k->dl = dl;
        memcpy(k->id, id, 4);
        /* the game code: four printable letters, or it isn't kept (a line must read back) */
        for (int i = 0; i < 4; i++) if (id[i] <= ' ' || id[i] > '~') return 1;
        if (fn[0] && !strchr(rom, '\n') && (f = fopen(fn, "a"))) {
            fprintf(f, "%lld %lld %d %.4s %s\n", k->size, k->mtime, dl, id, rom);
            fclose(f);
        }
    }
    return 1;
}
