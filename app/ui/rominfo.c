#define _GNU_SOURCE
#include "rominfo.h"

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>

#include "multicart_table.h"

/* the kept answers: ROMCAPS_FILE, else BANANA_PARTY_ROMCAPS (empty: none kept, as in the previews) */
#define ROMCAPS_FILE "/data/bananads-lan/rom-caps.txt"
/* a ROM's files looked at for a child program when its file names can't be read (each one a read on the card) */
#define MAX_CANDIDATES 4096
/* the file name table is read whole up to this size */
#define MAX_FNT (2 << 20)

/* set while the thread is asked to stop or pause: a reading gives up at its next read */
static volatile int g_abort;

static uint32_t le32(const unsigned char *p) { return p[0] | p[1] << 8 | p[2] << 16 | (uint32_t)p[3] << 24; }
static uint32_t le16(const unsigned char *p) { return p[0] | p[1] << 8; }

static int read_at(int fd, long off, void *buf, size_t n)
{
    size_t got = 0;
    while (got < n) {
        ssize_t r = pread(fd, (char *)buf + got, n - got, (off_t)off + (off_t)got);
        if (r < 0 && errno == EINTR) continue;
        if (r <= 0) return 0;
        got += (size_t)r;
    }
    return 1;
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
static int signed_at(int fd, long base, uint32_t used, long limit)
{
    unsigned char s[4];
    if (used < 0x200 || base + (long)used + 4 > limit) return 0;
    return read_at(fd, base + (long)used, s, 4) && s[0] == 'a' && s[1] == 'c' && s[2] == 1 && s[3] == 0;
}

typedef struct { uint32_t start, end; } Span;
static int by_start(const void *a, const void *b)
{
    uint32_t x = ((const Span *)a)->start, y = ((const Span *)b)->start;
    return x < y ? -1 : x > y;
}

static int child_name(const unsigned char *n, int len)
{
    return len >= 4 && n[len - 4] == '.' &&
           (!strncasecmp((const char *)n + len - 3, "srl", 3) || !strncasecmp((const char *)n + len - 3, "nds", 3));
}

/* the ids of the files named *.srl or *.nds (a DS program; every child program in a test library of 43 games is a
   .srl), from the file name table. -1: the table can't be read */
static int named_programs(int fd, const unsigned char *h, long size, uint32_t fat_n, uint32_t *ids, int max)
{
    uint32_t fnt = le32(h + 0x40), len = le32(h + 0x44), ndirs;
    unsigned char *t;
    int n = 0;
    if (len < 8 || len > MAX_FNT || fnt < 0x200 || (long)fnt + (long)len > size) return -1;
    if (!(t = malloc(len))) return -1;
    if (!read_at(fd, fnt, t, len)) { free(t); return -1; }
    ndirs = le16(t + 6);
    if (!ndirs || ndirs > 4096 || (uint64_t)ndirs * 8 > len) { free(t); return -1; }
    for (uint32_t d = 0; d < ndirs && n < max; d++) {
        uint32_t off = le32(t + d * 8), id = le16(t + d * 8 + 4);
        while (off < len && n < max) {
            unsigned b = t[off++], l = b & 0x7f;
            if (!b) break;
            if (off + l > len) break;
            if (b & 0x80) { off += l + 2; continue; }  /* a folder: its name and its id */
            if (child_name(t + off, (int)l) && id < fat_n) ids[n++] = id;
            off += l;
            id++;
        }
    }
    free(t);
    return n;
}

/* DS Download Play: the game sends itself (signed like a child program, as Mario Kart DS), or it carries child
   programs in its files: a .srl/.nds file of 16 KB to 4 MB with a DS header (the logo's first bytes at 0xC0) and
   the signature at its own used size. A child without the signature can't be downloaded by a DS, so it doesn't
   count. Only the files named as programs are read: on a slow card each read is some 12 ms, and a big game has
   thousands of files (a game whose names can't be read has its files of that size looked at instead) */
static int has_dlplay(int fd, const unsigned char *h, long size)
{
    static const unsigned char logo[8] = { 0x24, 0xff, 0xae, 0x51, 0x69, 0x9a, 0xa2, 0x21 };
    uint32_t fat = le32(h + 0x48), fat_n = le32(h + 0x4c) / 8;
    unsigned char *t;
    uint32_t *ids;
    Span *c;
    int n = 0, found = 0, named;
    if (signed_at(fd, 0, le32(h + 0x80), size)) return 1;
    if (!fat_n || fat < 0x200 || (long)fat + (long)fat_n * 8 > size) return 0;
    if (fat_n > 65536) fat_n = 65536;
    t = malloc((size_t)fat_n * 8);
    c = malloc(sizeof *c * MAX_CANDIDATES);
    ids = malloc(sizeof *ids * MAX_CANDIDATES);
    if (!t || !c || !ids || !read_at(fd, (long)fat, t, (size_t)fat_n * 8)) goto out;
    named = named_programs(fd, h, size, fat_n, ids, MAX_CANDIDATES);
    for (uint32_t k = 0; k < (named >= 0 ? (uint32_t)named : fat_n) && n < MAX_CANDIDATES; k++) {
        uint32_t i = named >= 0 ? ids[k] : k;
        uint32_t s = le32(t + i * 8), e = le32(t + i * 8 + 4);
        if (e > s && e - s >= 0x4000 && e - s <= 0x400000 && (long)e <= size) c[n++] = (Span){ s, e };
    }
    qsort(c, (size_t)n, sizeof *c, by_start);  /* in order through the file */
    for (int i = 0; i < n && !found && !g_abort; i++) {
        unsigned char k[0xc8];
        if (!read_at(fd, (long)c[i].start, k, sizeof k) || memcmp(k + 0xc0, logo, sizeof logo)) continue;
        found = signed_at(fd, (long)c[i].start, le32(k + 0x80), (long)c[i].end);
    }
out:
    free(t);
    free(c);
    free(ids);
    return found;
}

static int ends_with(const char *s, const char *e)
{
    size_t a = strlen(s), b = strlen(e);
    return a >= b && strcasecmp(s + a - b, e) == 0;
}

/* the ROM itself: its game code and DS Download Play. -1: given up (the thread asked to stop) */
static int scan(const char *rom, long size, char id[4], int *dlplay)
{
    unsigned char h[0x200];
    int fd;
    if (!(ends_with(rom, ".nds") || ends_with(rom, ".dsi"))) return 0;
    if (size < 0x200) return 0;
    if ((fd = open(rom, O_RDONLY | O_CLOEXEC)) < 0) return 0;
#ifdef POSIX_FADV_RANDOM
    posix_fadvise(fd, 0, 0, POSIX_FADV_RANDOM);  /* small reads here and there: no read-ahead */
#endif
    if (!read_at(fd, 0, h, sizeof h)) { close(fd); return 0; }
    memcpy(id, h + 0x0c, 4);
    *dlplay = has_dlplay(fd, h, size);
    close(fd);
    return g_abort ? -1 : 1;
}

static void fill(RomCaps *out, const char id[4], int dlplay)
{
    int p = table_players(id);
    out->flags = (p >= 2 ? ROM_MULTICARD : 0) | (dlplay ? ROM_DLPLAY : 0);
    out->players = p >= 2 ? p : 0;
}

int rom_caps_read(const char *rom, RomCaps *out)
{
    struct stat st;
    char id[4];
    int dl = 0;
    memset(out, 0, sizeof *out);
    if (stat(rom, &st) != 0 || scan(rom, (long)st.st_size, id, &dl) != 1) return 0;
    fill(out, id, dl);
    return 1;
}

/* ---- the kept answers: "size mtime dlplay code path" a line; a newer line for a path wins ---- */
typedef struct { char *path; long long size, mtime; int dl; char id[4]; int saved; } Kept;
static Kept *kept;
static int nkept, capkept, nlines;

static const char *cache_file(void)
{
    const char *e = getenv("BANANA_PARTY_ROMCAPS");
    return e ? e : ROMCAPS_FILE;
}

static unsigned hash_path(const char *s)
{
    unsigned h = 2166136261u;
    while (*s) h = (h ^ (unsigned char)*s++) * 16777619u;
    return h;
}

/* kept[] by path: an open hash of indexes (2000 games looked up without a walk through the list) */
static int *khash;
static unsigned khcap;

static void khash_put(int i)
{
    unsigned m = khcap - 1, h = hash_path(kept[i].path) & m;
    while (khash[h] >= 0) h = (h + 1) & m;
    khash[h] = i;
}

static int khash_grow(void)
{
    unsigned nc = khcap ? khcap * 2 : 1024;
    int *nh = malloc(sizeof *nh * nc);
    if (!nh) return 0;
    for (unsigned i = 0; i < nc; i++) nh[i] = -1;
    free(khash);
    khash = nh;
    khcap = nc;
    for (int i = 0; i < nkept; i++) khash_put(i);
    return 1;
}

static Kept *find(const char *path)
{
    unsigned m, h;
    if (!khcap) return NULL;
    m = khcap - 1;
    for (h = hash_path(path) & m; khash[h] >= 0; h = (h + 1) & m)
        if (!strcmp(kept[khash[h]].path, path)) return &kept[khash[h]];
    return NULL;
}

static Kept *add(const char *path)
{
    Kept *k = find(path);
    if (k) return k;
    if ((unsigned)(nkept + 1) * 2 > khcap && !khash_grow()) return NULL;
    if (nkept == capkept) {
        int nc = capkept ? capkept * 2 : 256;
        Kept *nk = realloc(kept, sizeof *nk * (size_t)nc);
        if (!nk) return NULL;
        kept = nk;
        capkept = nc;
    }
    k = &kept[nkept];
    memset(k, 0, sizeof *k);
    if (!(k->path = strdup(path))) return NULL;
    khash_put(nkept++);
    return k;
}

static void load(void)
{
    const char *fn = cache_file();
    char line[1024];
    FILE *f;
    if (!fn[0] || !(f = fopen(fn, "r"))) return;
    while (fgets(line, sizeof line, f)) {
        long long size, mtime;
        int dl, off = 0;
        char id[5];
        size_t n = strlen(line);
        if (n && line[n - 1] == '\n') line[--n] = 0;
        nlines++;
        if (sscanf(line, "%lld %lld %d %4s %n", &size, &mtime, &dl, id, &off) != 4 || !off || strlen(id) != 4) continue;
        Kept *k = add(line + off);
        if (!k) break;
        k->size = size;
        k->mtime = mtime;
        k->dl = dl ? 1 : 0;
        memcpy(k->id, id, 4);
        k->saved = 1;
    }
    fclose(f);
}

static int keepable(const Kept *k)
{
    for (int i = 0; i < 4; i++) if (k->id[i] <= ' ' || k->id[i] > '~') return 0;  /* a line must read back */
    return !strchr(k->path, '\n');
}

/* the new answers added to the file; all of it written again when most of its lines are old ones */
static void save(int all)
{
    const char *fn = cache_file();
    char tmp[1100];
    FILE *f;
    int i, unsaved = 0;
    if (!fn[0]) return;
    for (i = 0; i < nkept; i++) if (!kept[i].saved) unsaved++;
    if (all && nlines > nkept + 16 && strlen(fn) < 1000) {
        snprintf(tmp, sizeof tmp, "%s.new", fn);
        if (!(f = fopen(tmp, "w"))) return;
        for (i = 0; i < nkept; i++)
            if (keepable(&kept[i]))
                fprintf(f, "%lld %lld %d %.4s %s\n", kept[i].size, kept[i].mtime, kept[i].dl, kept[i].id, kept[i].path);
        if (fclose(f) == 0 && rename(tmp, fn) == 0) {
            nlines = nkept;
            for (i = 0; i < nkept; i++) kept[i].saved = 1;
        } else unlink(tmp);
        return;
    }
    if (!unsaved || !(f = fopen(fn, "a"))) return;
    for (i = 0; i < nkept; i++) {
        if (kept[i].saved) continue;
        kept[i].saved = 1;
        if (!keepable(&kept[i])) continue;
        fprintf(f, "%lld %lld %d %.4s %s\n", kept[i].size, kept[i].mtime, kept[i].dl, kept[i].id, kept[i].path);
        nlines++;
    }
    fclose(f);
}

/* ---- the reader thread ---- */
enum { RS_WAIT, RS_DONE };
static pthread_mutex_t mu = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cv = PTHREAD_COND_INITIALIZER;
static pthread_t th;
static int started, stopping, paused, loaded;
static char **lp;              /* the list's paths */
static unsigned char *lst;     /* RS_WAIT / RS_DONE */
static RomCaps *lres;
static int ln, lgen, focus = -1, radius, sweep;

static long long now_ms(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (long long)t.tv_sec * 1000 + t.tv_nsec / 1000000;
}

/* the next game to read: the picked one, then outward from it, then the list in order. -1: none (all read) */
static int next_job(void)
{
    if (focus >= 0 && focus < ln) {
        for (int d = 0; d <= radius && d < ln; d++) {
            int a = (focus + d) % ln, b = ((focus - d) % ln + ln) % ln;
            if (lst[a] == RS_WAIT) return a;
            if (lst[b] == RS_WAIT) return b;
        }
    }
    while (sweep < ln && lst[sweep] != RS_WAIT) sweep++;
    return sweep < ln ? sweep : -1;
}

/* the thread at the lowest priority, and its reads after everyone else's (the idle I/O class) */
static void lowest_priority(void)
{
#if defined(__linux__) && defined(SYS_ioprio_set)
    syscall(SYS_ioprio_set, 1 /* IOPRIO_WHO_PROCESS */, 0, (3 << 13) /* IOPRIO_CLASS_IDLE */);
#endif
#if defined(__linux__) && defined(SYS_gettid)
    setpriority(PRIO_PROCESS, (id_t)syscall(SYS_gettid), 19);
#endif
}

static void *worker(void *arg)
{
    long long last_save = now_ms();
    (void)arg;
    lowest_priority();
    pthread_mutex_lock(&mu);
    while (!stopping) {
        int i, gen, r, dl = 0, have = 0;
        char *path, id[4];
        struct stat st;
        RomCaps caps;
        Kept *k;
        if (!loaded) {
            load();
            loaded = 1;
        }
        i = paused ? -1 : next_job();
        if (i < 0) {
            struct timespec t;
            save(0);
            last_save = now_ms();
            clock_gettime(CLOCK_REALTIME, &t);
            t.tv_sec += 2;
            pthread_cond_timedwait(&cv, &mu, &t);
            continue;
        }
        if (!lp[i]) { lst[i] = RS_DONE; continue; }
        path = strdup(lp[i]);
        gen = lgen;
        pthread_mutex_unlock(&mu);
        memset(&caps, 0, sizeof caps);
        r = path && stat(path, &st) == 0;
        pthread_mutex_lock(&mu);
        if (r && (k = find(path)) && k->size == (long long)st.st_size && k->mtime == (long long)st.st_mtime) {
            fill(&caps, k->id, k->dl);
            have = 1;
        }
        pthread_mutex_unlock(&mu);
        if (r && !have) {
            r = scan(path, (long)st.st_size, id, &dl);
            if (r == 1) fill(&caps, id, dl);
        }
        pthread_mutex_lock(&mu);
        if (r == 1 && !have && (k = add(path))) {
            k->size = (long long)st.st_size;
            k->mtime = (long long)st.st_mtime;
            k->dl = dl;
            memcpy(k->id, id, 4);
            k->saved = 0;
        }
        /* given up (paused or stopping): read again later */
        if (r != -1 && gen == lgen && i < ln) {
            lst[i] = RS_DONE;
            lres[i] = caps;
        }
        free(path);
        if (now_ms() - last_save > 3000) { save(0); last_save = now_ms(); }
    }
    save(1);
    pthread_mutex_unlock(&mu);
    return NULL;
}

typedef struct { const char *path; int i; } Old;
static int old_cmp(const void *a, const void *b) { return strcmp(((const Old *)a)->path, ((const Old *)b)->path); }

void rcw_set_list(const char *paths, int stride, int n)
{
    Old *old = NULL;
    int nold = 0;
    char **np = n > 0 ? calloc((size_t)n, sizeof *np) : NULL;
    unsigned char *ns = n > 0 ? calloc((size_t)n, 1) : NULL;
    RomCaps *nr = n > 0 ? calloc((size_t)n, sizeof *nr) : NULL;
    int i;
    if (n > 0 && (!np || !ns || !nr)) { free(np); free(ns); free(nr); return; }
    for (i = 0; i < n; i++) np[i] = strdup(paths + (size_t)i * (size_t)stride);
    pthread_mutex_lock(&mu);
    /* a game already answered keeps its answer where it is now (a sort only moves the list around: no badge
       goes away and comes back) */
    if (ln > 0 && n > 0 && (old = malloc(sizeof *old * (size_t)ln))) {
        for (i = 0; i < ln; i++) if (lp[i] && lst[i] == RS_DONE) old[nold++] = (Old){ lp[i], i };
        qsort(old, (size_t)nold, sizeof *old, old_cmp);
        for (i = 0; i < n; i++) {
            Old key = { np[i], 0 }, *o = np[i] ? bsearch(&key, old, (size_t)nold, sizeof *old, old_cmp) : NULL;
            if (o) { ns[i] = RS_DONE; nr[i] = lres[o->i]; }
        }
        free(old);
    }
    for (i = 0; i < ln; i++) free(lp[i]);
    free(lp);
    free(lst);
    free(lres);
    lp = np;
    lst = ns;
    lres = nr;
    ln = n;
    lgen++;
    sweep = 0;
    if (focus >= ln) focus = -1;
    if (!started && !stopping) started = pthread_create(&th, NULL, worker, NULL) == 0;
    pthread_cond_signal(&cv);
    pthread_mutex_unlock(&mu);
}

void rcw_focus(int idx, int r)
{
    pthread_mutex_lock(&mu);
    focus = idx;
    radius = r;
    pthread_cond_signal(&cv);
    pthread_mutex_unlock(&mu);
}

int rcw_get(int idx, RomCaps *out)
{
    int ok;
    memset(out, 0, sizeof *out);
    pthread_mutex_lock(&mu);
    ok = idx >= 0 && idx < ln && lst[idx] == RS_DONE;
    if (ok) *out = lres[idx];
    pthread_mutex_unlock(&mu);
    return ok;
}

void rcw_pause(int on)
{
    pthread_mutex_lock(&mu);
    if (paused != !!on) {
        paused = !!on;
        g_abort = paused;
        pthread_cond_signal(&cv);
    }
    pthread_mutex_unlock(&mu);
}

void rcw_stop(void)
{
    pthread_mutex_lock(&mu);
    stopping = 1;
    g_abort = 1;
    pthread_cond_signal(&cv);
    pthread_mutex_unlock(&mu);
    if (started) pthread_join(th, NULL);
    started = 0;
}
