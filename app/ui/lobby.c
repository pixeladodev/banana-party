/* Lobby UI driven by headless bananads + wifi.lobbyctl_dir status files. */
#include "view.h"
#include "lobby.h"
#include "room_ctl.h"

#include <stdio.h>
#include <string.h>

#define T(s) px_tr(s)

extern Hit g_hits[MAX_HITS];
extern int g_nhits;

static void add_hit(const PxCanvas *c, float x, float y, float w, float h, int id, int arg)
{
    if (g_nhits >= MAX_HITS) return;
    g_hits[g_nhits].r.x = px_u(c, x);
    g_hits[g_nhits].r.y = px_u(c, y);
    g_hits[g_nhits].r.w = px_u(c, x + w) - g_hits[g_nhits].r.x;
    g_hits[g_nhits].r.h = px_u(c, y + h) - g_hits[g_nhits].r.y;
    g_hits[g_nhits].id = id;
    g_hits[g_nhits].arg = arg;
    g_nhits++;
}

static void add_face_hit(PxRect r, int id)
{
    if (g_nhits >= MAX_HITS) return;
    g_hits[g_nhits].r = r;
    g_hits[g_nhits].id = id;
    g_hits[g_nhits].arg = 0;
    g_nhits++;
}

/* The room file's own status. game_host and idle are what the emulator writes; anything else is not shown. */
static int member_badge(const char *st, const char **text)
{
    if (!strcmp(st, "game_host")) { *text = T("Host"); return PX_BADGE_PRIMARY; }
    if (!strcmp(st, "idle")) { *text = T("Connected"); return PX_BADGE_SUCCESS; }
    *text = NULL;
    return PX_BADGE_NEUTRAL;
}

static void lobby_latency_warn(PxCanvas *c, const Model *m)
{
    if (!m->ip[0] || !strncmp(m->ip, "10.42.0.", 8)) return;
    px_text_wrap(c, PX_REGULAR, 16, 42, 350,
                 T("LAN needs low latency on your network. It will not work well through a VPN or any tunnel."), px.muted,
                 556, 2, 20);
}

static void face_b(PxCanvas *c, const char *label, int library, int variant)
{
    PxRect r;
    px_face(c, 18, LIB_FACE_Y, PX_LEFT, "B", label, 0, &r, variant);
    add_face_hit(r, library ? HIT_LOBBY : HIT_BACK);
}

static void face_leave_room(PxCanvas *c)
{
    PxRect r;
    px_face(c, 622, LIB_FACE_Y, PX_RIGHT, "X", T("Leave Party"), 0, &r, PX_FACE_DANGER);
    add_face_hit(r, HIT_LOBBY_DISCONNECT);
}

static int member_is_me(const Lobby *lb, int i)
{
    int k, any = 0;
    for (k = 0; k < lb->nmembers; k++)
        if (lb->member_me[k]) any = 1;
    if (any) return lb->member_me[i];
    if (lb->role == 0) return lb->roles[i] == 0;
    return 0;
}

static int slots_pick(int cursor) { return cursor == 0 ? 2 : cursor == 1 ? 3 : 4; }

void lobby_reset(Lobby *lb)
{
    memset(lb, 0, sizeof *lb);
    lb->max_slots = 2;
}

void lobby_leave(Lobby *lb)
{
    room_leave(NULL);
    lobby_reset(lb);
}

void lobby_tick(Lobby *lb, int view)
{
    if (view != V_LOBBY && view != V_GAMES && !lb->in_lobby) return;
    if (lb->phase == LP_JOIN_SEARCH && lb->in_lobby && lb->nhosts > 0) {
        lb->phase = LP_JOIN_PICK;
        lb->join_sel = 0;
    }
}

float lobby_library_status(PxCanvas *c, const Model *m, float y)
{
    char buf[240];
    const Lobby *lb = &m->lobby;
    float x = 42;
    int i;
    px_label(c, x, y, T("Party"), px.muted);
    if (!lb->in_lobby) {
        const char *phrase = T("Auto discovery");
        const char *state = m->discover ? T("On") : T("Off");
        uint32_t phrase_rgb = px.light ? px.accent.dark : px.accent.base;
        uint32_t state_rgb = m->discover ? px.green : px.muted;
        float sw = px_text_w(c, PX_MEDIUM, 18, state, 0);
        px_text_fit(c, PX_MEDIUM, 18, x, y + 22, phrase, phrase_rgb, 0, 556 - sw - 12, PX_LEFT);
        px_text(c, PX_MEDIUM, 18, x + 556 - sw, y + 22, state, state_rgb, 0);
        return y + 22 + 22 + 8;
    }
    {
        int total = lb->max_slots > 0 ? lb->max_slots : 2;
        int filled = lb->nmembers, k;
        float tw, px0;
        if (total > 4) total = 4;
        if (filled < 0) filled = 0;
        if (filled > total) filled = total;
        snprintf(buf, sizeof buf, "%d/%d", filled, total);
        tw = px_text_w(c, PX_REGULAR, 17, buf, 0);
        px_text(c, PX_REGULAR, 17, 598 - tw, y, buf, px.ink, 0);
        px0 = 598 - tw - 10 - (total * 14 - 4);
        for (k = 0; k < total; k++)
            px_rect(c, px0 + k * 14, y + 4, 10, 10, k < filled ? px.accent.base : px.line);
    }
    {
        /* Host takes a third of the row, Guests the other two. Label, then the value under it. */
        int host = -1, g, any = 0;
        const float span = 556, gap = 16, host_w = (span - gap) / 3.0f, guest_w = span - gap - host_w;
        const float left = 42, right = left + host_w + gap, row = y + 36;
        float gx = right, gy = row;
        uint32_t you_rgb = px.light ? px.accent.dark : px.accent.base;
        for (i = 0; i < lb->nmembers; i++)
            if (lb->members[i][0] && lb->roles[i] == 0) { host = i; break; }
        px_label(c, left, y + 20, T("Host"), px.muted);
        px_label(c, right, y + 20, T("Guests"), px.muted);
        if (host >= 0) {
            int me = member_is_me(lb, host);
            float you_w = me ? px_text_w(c, PX_MEDIUM, 16, T("(you)"), 0) + 6 : 0;
            float nw = px_text_fit(c, PX_MEDIUM, 16, left, row, lb->members[host], px.ink, 0, host_w - you_w, PX_LEFT);
            if (me) px_text(c, PX_MEDIUM, 16, left + nw + 6, row, T("(you)"), you_rgb, 0);
        } else
            px_text(c, PX_MEDIUM, 16, left, row, "—", px.muted, 0);
        for (g = 0; g < lb->nmembers; g++) {
            char ping[16];
            float nw, pw, you_w, need;
            int me;
            uint32_t prgb;
            if (!lb->members[g][0] || g == host) continue;
            me = member_is_me(lb, g);
            if (lb->member_ping[g] >= 0) snprintf(ping, sizeof ping, "%d ms", lb->member_ping[g]);
            else snprintf(ping, sizeof ping, "—");
            you_w = me ? px_text_w(c, PX_MEDIUM, 16, T("(you)"), 0) + 6 : 0;
            nw = px_text_w(c, PX_MEDIUM, 16, lb->members[g], 0);
            pw = px_text_w(c, PX_MEDIUM, 16, ping, 0);
            need = nw + you_w + 8 + pw;
            if (need > guest_w) need = guest_w;
            if (gx > right && gx + need > right + guest_w) {
                gx = right;
                gy += 20;
                if (gy > row + 20) break;
            }
            nw = px_text_fit(c, PX_MEDIUM, 16, gx, gy, lb->members[g], px.ink, 0,
                             guest_w - you_w - 8 - pw, PX_LEFT);
            if (me) px_text(c, PX_MEDIUM, 16, gx + nw + 6, gy, T("(you)"), you_rgb, 0);
            if (lb->member_ping[g] >= 0)
                prgb = view_ping_rgb(lb->member_ping[g]);
            else
                prgb = px.muted;
            px_text(c, PX_MEDIUM, 16, gx + nw + you_w + 8, gy, ping, prgb, 0);
            gx += need + 14;
            any = 1;
        }
        if (!any) px_text(c, PX_MEDIUM, 16, right, row, "—", px.muted, 0);
    }
    return y + 64;
}

/* speaker grille beside a screen: one hole, then two, then one, each row centered */
static void speaker(PxCanvas *c, float x, float y, float s, uint32_t hole)
{
    float d = 3 * s, gap = 2 * s, row = 5 * s, span = d * 2 + gap, mid = (span - d) / 2;
    px_rect(c, x + mid, y, d, d, hole);
    px_rect(c, x, y + row, d, d, hole);
    px_rect(c, x + d + gap, y + row, d, d, hole);
    px_rect(c, x + mid, y + row * 2, d, d, hole);
}

/* an open DS. Both screens are the same 32 x 24 (4:3). The top one sits between the speaker grilles.
   The bottom shell stops just under that screen, with the pad and the buttons beside it.
   w is the body width (64 at scale 1). Returns the caption baseline. */
static float handheld(PxCanvas *c, float x, float y, float w, int lit)
{
    float s = w / 64.0f;
    uint32_t body = px.light ? 0xe9e3d6 : 0x2e2b33;
    uint32_t rim = lit ? px.accent.base : px.edge;
    uint32_t glass = lit ? px_mix(px.accent.dark, px.accent.base, 0.35f) : (px.light ? 0xc9c0ae : 0x100f14);
    uint32_t hole = px.light ? 0xc9c0ae : px.screen_bg;
    px_round(c, x - 2 * s, y - 2 * s, 68 * s, 38 * s, 6 * s, rim, 255);
    px_round(c, x, y, 64 * s, 34 * s, 5 * s, body, 255);
    px_rect(c, x + 16 * s, y + 5 * s, 32 * s, 24 * s, glass);
    if (lit) px_rect(c, x + 16 * s, y + 5 * s, 32 * s, 3 * s, px.accent.base);
    speaker(c, x + 4 * s, y + 9 * s, s, hole);
    speaker(c, x + 52 * s, y + 9 * s, s, hole);
    px_rect(c, x + 6 * s, y + 36 * s, 52 * s, 4 * s, rim);
    px_round(c, x - 2 * s, y + 40 * s, 68 * s, 36 * s, 6 * s, rim, 255);
    px_round(c, x, y + 42 * s, 64 * s, 32 * s, 5 * s, body, 255);
    px_rect(c, x + 16 * s, y + 46 * s, 32 * s, 24 * s, glass);
    px_rect(c, x + 4 * s, y + 57 * s, 10 * s, 3 * s, px.muted);
    px_rect(c, x + 7 * s, y + 54 * s, 3 * s, 9 * s, px.muted);
    px_rect(c, x + 54 * s, y + 54 * s, 4 * s, 4 * s, px.muted);
    px_rect(c, x + 59 * s, y + 59 * s, 4 * s, 4 * s, px.muted);
    px_rect(c, x + 49 * s, y + 59 * s, 4 * s, 4 * s, px.muted);
    px_rect(c, x + 54 * s, y + 64 * s, 4 * s, 4 * s, px.muted);
    return y + 82 * s;
}

static float hand_hinge(float y, float w) { return y + 38.0f * (w / 64.0f); }

static void waves(PxCanvas *c, float x, float y, int dir, uint32_t rgb)
{
    int k, i;
    for (k = 0; k < 3; k++) {
        float r = 8 + (float)k * 8;
        for (i = -2; i <= 2; i++) {
            float dy = (float)i * r / 3.0f;
            float dx = r - (float)(i * i) * r / 16.0f;
            px_rect(c, x + (float)dir * dx - 2, y + dy - 2, 4, 4, rgb);
        }
    }
}

static void hand_caption(PxCanvas *c, float x, float y, float w, const char *s, int lit)
{
    if (!s || !s[0]) return;
    px_text_fit(c, PX_MEDIUM, 15, x + w / 2, y + 2, s, lit ? px.ink : px.muted, PX_UPPER | PX_TRACK, w, PX_CENTER);
}

/* filled seats of a room you might join: one pip per console the host advertised */
static void seat_pips(PxCanvas *c, float x, float y, float w, int filled, int total)
{
    float pw = 10, gap = 4, span, sx;
    int i;
    if (total < 1) return;
    if (total > 4) total = 4;
    if (filled < 0) filled = 0;
    if (filled > total) filled = total;
    span = total * pw + (total - 1) * gap;
    sx = x + (w - span) / 2;
    for (i = 0; i < total; i++)
        px_rect(c, sx + i * (pw + gap), y, pw, pw, i < filled ? px.accent.base : px.line);
}

/* 90% of the four-across row that filled the sheet. The same width for 2, 3 or 4, and for the mode screen. */
#define HAND_W 114.0f

/* this console and the other side of a link. filled/total draw the other room's seats; 0 skips them. */
static void link_pair(PxCanvas *c, float y, int left_lit, const char *left, int right_lit, const char *right,
                      int linked, int filled, int total)
{
    const float w = HAND_W, gap = 48;
    float x = (640 - (w * 2 + gap)) / 2;
    float cap = handheld(c, x, y, w, left_lit);
    float hy = hand_hinge(y, w);
    uint32_t wire = linked ? px.accent.base : px.muted;
    waves(c, x + w + 6, hy, 1, wire);
    if (linked) waves(c, x + w + gap - 6, hy, -1, wire);
    handheld(c, x + w + gap, y, w, right_lit);
    hand_caption(c, x, cap, w, left, left_lit);
    hand_caption(c, x + w + gap, cap, w, right, right_lit);
    if (total > 0) seat_pips(c, x + w + gap, cap + 18, w, filled, total);
}

/* Host on the mode screen: this console, then open seats a friend can take. */
static void host_offer(PxCanvas *c, float y)
{
    const float w = HAND_W, wave = 36, gap = 12;
    float total = w * 3 + wave + gap;
    float x = (640 - total) / 2;
    float cap = handheld(c, x, y, w, 1);
    waves(c, x + w + 4, hand_hinge(y, w), 1, px.accent.base);
    handheld(c, x + w + wave, y, w, 0);
    handheld(c, x + w + wave + w + gap, y, w, 0);
    hand_caption(c, x, cap, w, T("You"), 1);
}

/* Searching: this console calling, and nobody answering yet. */
static void searching(PxCanvas *c, float y)
{
    const float w = HAND_W;
    float x = (640 - w) / 2 - 36;
    float cap = handheld(c, x, y, w, 1);
    waves(c, x + w + 8, hand_hinge(y, w), 1, px.muted);
    hand_caption(c, x, cap, w, T("You"), 1);
}

/* Alone on the room list: the search finished and there is no host to draw. */
static void alone(PxCanvas *c, float y)
{
    const float w = 76;
    float x = (640 - w) / 2;
    float cap = handheld(c, x, y, w, 1);
    hand_caption(c, x, cap, w, T("You"), 1);
}

/* n consoles in a row, one fixed width. The first lit_n are taken.
   Names come from the room when label_members is set. Returns the y under the captions. */
static float seat_row(PxCanvas *c, float y, int n, int lit_n, const Lobby *lb, int label_members)
{
    float gap = 16, w = HAND_W, total, x, cap = y;
    int i;
    if (n < 1) n = 1;
    if (n > 4) n = 4;
    if (lit_n < 0) lit_n = 0;
    if (lit_n > n) lit_n = n;
    total = n * w + (n - 1) * gap;
    x = (640 - total) / 2;
    for (i = 0; i < n; i++) {
        int lit = i < lit_n;
        const char *name = NULL;
        cap = handheld(c, x, y, w, lit);
        if (label_members && lit && lb && i < LOBBY_MEMBERS && lb->members[i][0]) name = lb->members[i];
        else if (!label_members && i == 0) name = T("You");
        hand_caption(c, x, cap, w, name, lit);
        x += w + gap;
    }
    return cap + 22;
}

static void seat_count(PxCanvas *c, int filled, int total, float y)
{
    char buf[16];
    float tw;
    if (total < 1) total = 1;
    if (total > 4) total = 4;
    if (filled < 0) filled = 0;
    if (filled > total) filled = total;
    snprintf(buf, sizeof buf, "%d/%d", filled, total);
    tw = px_text_w(c, PX_MEDIUM, 22, buf, 0);
    px_text(c, PX_MEDIUM, 22, 598 - tw, y, buf, px.ink, 0);
}

void lobby_top(PxCanvas *c, const Model *m)
{
    char buf[128];
    const Lobby *lb = &m->lobby;
    const float art = 142, art_room = 184;
    px_screen(c);
    px_status(c, "Banana Party", m->battery, m->charging, m->clock);
    px_sheet(c, 18, 60, 604, 406);
    switch (lb->phase) {
    case LP_CHOOSE:
        px_label(c, 44, 84, T("Party"), px.ink);
        px_text_wrap(c, PX_MEDIUM, 22, 42, 108, T("Host a Party for friends\nor join one on your network."), px.ink, 556, 2, 28);
        if (lb->cursor == 0) host_offer(c, 176);
        else link_pair(c, 176, 1, T("You"), 1, T("Host"), 1, 0, 0);
        px_text_wrap(c, PX_REGULAR, 18, 42, 360, T("Pick Host or Join on the bottom screen."), px.muted, 556, 2, 24);
        break;
    case LP_HOST_SLOTS: {
        float below;
        px_label(c, 44, 84, T("Party"), px.ink);
        px_text_fit(c, PX_MEDIUM, 22, 42, 108, T("How many can be in this Party?"), px.ink, 0, 556, PX_LEFT);
        below = seat_row(c, art_room, slots_pick(lb->cursor), 1, NULL, 0);
        snprintf(buf, sizeof buf, T("Up to %d consoles, including this one."), slots_pick(lb->cursor));
        px_text_wrap(c, PX_REGULAR, 18, 42, below, buf, px.muted, 556, 2, 24);
        break;
    }
    case LP_HOST_WAIT: {
        int total = lb->max_slots > 0 ? lb->max_slots : 2;
        int filled = lb->nmembers > 0 ? lb->nmembers : 1;
        float below;
        px_label(c, 44, 84, T("Your Party"), px.muted);
        px_text_fit(c, PX_MEDIUM, 26, 42, 108, lb->room[0] ? lb->room : T("Your Party"), px.ink, 0, 460, PX_LEFT);
        seat_count(c, filled, total, 108);
        below = seat_row(c, art_room, total, filled, lb, 1);
        if (lb->nmembers <= 1)
            px_text_wrap(c, PX_REGULAR, 18, 42, below, T("You are the only one so far. Others see this name on the network."),
                         px.muted, 556, 2, 24);
        break;
    }
    case LP_JOIN_SEARCH:
        px_label(c, 44, 84, T("On your network"), px.muted);
        px_text_dots(c, PX_MEDIUM, 22, 42, 108, T("Searching for a Party"), m->dots, px.ink, 0, PX_LEFT);
        searching(c, art);
        px_text_wrap(c, PX_REGULAR, 18, 42, 312, T("Looking on your local network."), px.muted, 556, 2, 24);
        lobby_latency_warn(c, m);
        break;
    case LP_JOIN_PICK:
        px_label(c, 44, 84, T("On your network"), px.muted);
        px_text_fit(c, PX_MEDIUM, 26, 42, 108, T("Parties"), px.ink, 0, 540, PX_LEFT);
        if (lb->nhosts <= 0) {
            alone(c, art);
            px_text_wrap(c, PX_REGULAR, 18, 42, 312, T("No hosts found."), px.muted, 556, 2, 24);
        } else {
            int sel = lb->join_sel;
            const char *name;
            if (sel < 0 || sel >= lb->nhosts) sel = 0;
            name = lb->hosts_name[sel][0] ? lb->hosts_name[sel] : lb->hosts_addr[sel];
            link_pair(c, art, 1, T("You"), 1, name, 1, lb->hosts_players[sel], lb->hosts_max[sel]);
            px_text_wrap(c, PX_REGULAR, 18, 42, 328, T("Pick a Party. Each person opens their own game."), px.muted, 556, 2, 24);
        }
        lobby_latency_warn(c, m);
        break;
    case LP_GUEST_WAIT: {
        int total = lb->max_slots > 0 ? lb->max_slots : 2;
        int filled = lb->nmembers > 0 ? lb->nmembers : 1;
        px_label(c, 44, 84, T("In the Party"), px.muted);
        px_text_fit(c, PX_MEDIUM, 26, 42, 108, lb->room[0] ? lb->room : T("In the Party"), px.ink, 0, 460, PX_LEFT);
        seat_count(c, filled, total, 108);
        seat_row(c, art_room, total, filled, lb, 1);
        break;
    }
    default:
        break;
    }
    if (m->ip[0]) {
        if (!strncmp(m->ip, "10.42.0.", 8)) snprintf(buf, sizeof buf, "Direct Link  ·  %s", m->ip);
        else snprintf(buf, sizeof buf, "Wi-Fi  ·  %s", m->ip);
        px_text_fit(c, PX_REGULAR, 16, 598, 418, buf, px.muted, 0, 556, PX_RIGHT);
    }
}

static void lobby_choice_cards(PxCanvas *c, const Model *m, float y)
{
    const char *labels[2] = { T("Host"), T("Join") };
    int i;
    for (i = 0; i < 2; i++) {
        float h = 118, card_y = y + i * (h + 14);
        int sel = m->lobby.cursor == i;
        px_card(c, 18, card_y, 604, h, sel, 0);
        px_text(c, PX_MEDIUM, 28, 40, card_y + 38, labels[i], sel ? px.on_pick : px.ink, 0);
        px_text_wrap(c, PX_REGULAR, 18, 40, card_y + 72,
                     i ? T("Find a host on your network.") : T("Let friends join this console."),
                     sel ? px.on_pick_sub : px.muted, 560, 2, 22);
        add_hit(c, 18, card_y, 604, h, HIT_LOBBY_ROW, i);
    }
}

static void lobby_slot_cards(PxCanvas *c, const Model *m, float y)
{
    int slots[3] = { 2, 3, 4 };
    int i;
    for (i = 0; i < 3; i++) {
        char buf[32];
        float h = 76, card_y = y + i * (h + 10);
        int sel = m->lobby.cursor == i;
        snprintf(buf, sizeof buf, T("%d players"), slots[i]);
        px_card(c, 18, card_y, 604, h, sel, 0);
        px_text(c, PX_MEDIUM, 26, 40, card_y + 22, buf, sel ? px.on_pick : px.ink, 0);
        add_hit(c, 18, card_y, 604, h, HIT_LOBBY_ROW, slots[i]);
    }
}

static void lobby_host_list(PxCanvas *c, const Model *m, float y)
{
    int i;
    for (i = 0; i < m->lobby.nhosts; i++) {
        char line[120], seats[16];
        float row_y = y + i * 62;
        int sel = m->lobby.join_sel == i;
        snprintf(line, sizeof line, "%s  ·  %s", m->lobby.hosts_name[i], m->lobby.hosts_addr[i]);
        snprintf(seats, sizeof seats, "%d/%d", m->lobby.hosts_players[i], m->lobby.hosts_max[i]);
        px_card(c, 18, row_y, 604, 54, sel, 0);
        px_text_fit(c, PX_MEDIUM, 18, 598, row_y + 15, seats, sel ? px.on_pick : px.ink, 0, 80, PX_RIGHT);
        px_text_fit(c, PX_MEDIUM, 18, 38, row_y + 15, line, sel ? px.on_pick : px.ink, 0, 480, PX_LEFT);
        add_hit(c, 18, row_y, 604, 54, HIT_LOBBY_ROW, i);
    }
}

static void lobby_member_list(PxCanvas *c, const Model *m, float y)
{
    int i, row = 0;
    px_label(c, 36, y, "#", px.muted);
    px_label(c, 78, y, T("Name"), px.muted);
    px_label(c, 250, y, T("Status"), px.muted);
    px_label(c, 390, y, T("Ping"), px.muted);
    px_label(c, 478, y, "IP", px.muted);
    y += 22;
    for (i = 0; i < m->lobby.nmembers; i++) {
        char num[8], ping[16];
        float row_y;
        if (!m->lobby.members[i][0]) continue;
        row_y = y + row * 54;
        row++;
        if (m->lobby.member_ping[i] >= 0) snprintf(ping, sizeof ping, "%d ms", m->lobby.member_ping[i]);
        else snprintf(ping, sizeof ping, "-");
        snprintf(num, sizeof num, "%d", row);
        px_card(c, 18, row_y, 604, 48, 0, 0);
        px_text(c, PX_MEDIUM, 18, 36, row_y + 13, num, px.ink, 0);
        {
            int me = member_is_me(&m->lobby, i);
            float you_w = me ? px_text_w(c, PX_MEDIUM, 18, T("(you)"), 0) + 8 : 0;
            float nw = px_text_fit(c, PX_MEDIUM, 18, 78, row_y + 13, m->lobby.members[i], px.ink, 0, 160 - you_w, PX_LEFT);
            if (me)
                px_text(c, PX_MEDIUM, 18, 78 + nw + 6, row_y + 13, T("(you)"),
                        px.light ? px.accent.dark : px.accent.base, 0);
        }
        {
            const char *st = NULL;
            int sk = member_badge(m->lobby.member_status[i], &st);
            if (st) px_badge(c, 250, row_y + 10, 26, st, sk);
        }
        px_text_fit(c, PX_MEDIUM, 16, 390, row_y + 14, ping,
                    m->lobby.member_ping[i] >= 0 ? view_ping_rgb(m->lobby.member_ping[i]) : px.ink, 0, 80, PX_LEFT);
        px_text_fit(c, PX_MEDIUM, 16, 478, row_y + 14, m->lobby.member_ip[i][0] ? m->lobby.member_ip[i] : "-", px.ink, 0,
                    130, PX_LEFT);
    }
}

void lobby_bottom(PxCanvas *c, const Model *m)
{
    PxRect r;
    const char *title = T("Party");
    g_nhits = 0;
    px_screen(c);
    if (m->lobby.phase == LP_JOIN_PICK) title = T("Parties");
    else if (m->lobby.phase == LP_HOST_WAIT) title = T("Your Party");
    else if (m->lobby.phase == LP_GUEST_WAIT) title = T("In the Party");
    px_bubble(c, 18, 14, 604, 54, 0);
    px_text_fit(c, PX_MEDIUM, 26, 38, 25, title, px.ink, 0, 440, PX_LEFT);
    switch (m->lobby.phase) {
    case LP_CHOOSE:
        lobby_choice_cards(c, m, 82);
        face_b(c, T("Library"), 1, PX_FACE_NEUTRAL);
        px_face(c, 622, LIB_FACE_Y, PX_RIGHT, "A", T("Choose"), 0, &r, PX_FACE_NEUTRAL);
        add_face_hit(r, HIT_OK);
        break;
    case LP_HOST_SLOTS:
        lobby_slot_cards(c, m, 82);
        face_b(c, T("Back"), 0, PX_FACE_NEUTRAL);
        px_face(c, 622, LIB_FACE_Y, PX_RIGHT, "A", T("Choose"), 0, &r, PX_FACE_NEUTRAL);
        add_face_hit(r, HIT_OK);
        break;
    case LP_HOST_WAIT:
        lobby_member_list(c, m, 78);
        face_b(c, T("Library"), 0, PX_FACE_PRIMARY);
        face_leave_room(c);
        break;
    case LP_JOIN_SEARCH:
        px_card(c, 18, 120, 604, 120, 0, 0);
        px_text_dots(c, PX_MEDIUM, 22, 40, 158, T("Searching for a Party"), m->dots, px.ink, 0, PX_LEFT);
        face_b(c, T("Cancel"), 0, PX_FACE_NEUTRAL);
        break;
    case LP_JOIN_PICK:
        if (m->lobby.nhosts > 0) {
            lobby_host_list(c, m, 82);
            face_b(c, T("Back"), 0, PX_FACE_NEUTRAL);
            px_face(c, 622, LIB_FACE_Y, PX_RIGHT, "A", T("Join"), m->lobby.nhosts <= 0, &r, PX_FACE_NEUTRAL);
            add_face_hit(r, HIT_OK);
        } else {
            px_card(c, 18, 120, 604, 80, 0, 1);
            px_text_fit(c, PX_MEDIUM, 20, 38, 145, T("No hosts found"), px.muted, 0, 560, PX_LEFT);
            face_b(c, T("Back"), 0, PX_FACE_NEUTRAL);
        }
        break;
    case LP_GUEST_WAIT:
        lobby_member_list(c, m, 78);
        face_b(c, T("Library"), 0, PX_FACE_PRIMARY);
        face_leave_room(c);
        break;
    default:
        break;
    }
}

int lobby_handle(Model *m, int act, int arg)
{
    Lobby *lb = &m->lobby;
    if (act == ACT_LOBBY) {
        /* X leaves a room you are already in. On the room list it does nothing. */
        if (lb->phase == LP_HOST_WAIT || lb->phase == LP_GUEST_WAIT) {
            m->view = V_LOBBY_DISCONNECT;
            return 1;
        }
        if (lb->phase == LP_JOIN_PICK || lb->phase == LP_JOIN_SEARCH || lb->phase == LP_HOST_SLOTS) return 1;
        m->view = V_GAMES;
        return 1;
    }
    if (act == ACT_LOBBY_DISCONNECT) {
        m->view = V_LOBBY_DISCONNECT;
        return 1;
    }
    if ((act == ACT_BACK || act == ACT_FAV) && lb->phase == LP_CHOOSE) {
        m->view = V_GAMES;
        return 1;
    }
    switch (lb->phase) {
    case LP_CHOOSE:
        if (act == ACT_UP) { if (lb->cursor > 0) lb->cursor--; return 1; }
        if (act == ACT_DOWN) { if (lb->cursor < 1) lb->cursor++; return 1; }
        if (act == ACT_LOBBY_ROW) { lb->cursor = arg; act = ACT_OK; }
        if (act == ACT_OK) {
            if (lb->cursor == 0) {
                lb->phase = LP_HOST_SLOTS;
                lb->cursor = 0;
                lb->max_slots = 2;
            } else {
                lb->in_lobby = 1;
                lb->role = 1;
                lb->phase = LP_JOIN_SEARCH;
                room_start_join(NULL);
            }
            return 1;
        }
        break;
    case LP_HOST_SLOTS:
        if (act == ACT_UP) {
            if (lb->cursor > 0) lb->cursor--;
            lb->max_slots = slots_pick(lb->cursor);
            return 1;
        }
        if (act == ACT_DOWN) {
            if (lb->cursor < 2) lb->cursor++;
            lb->max_slots = slots_pick(lb->cursor);
            return 1;
        }
        if (act == ACT_LOBBY_ROW) {
            lb->max_slots = arg;
            for (lb->cursor = 0; lb->cursor < 3; lb->cursor++)
                if (slots_pick(lb->cursor) == arg) break;
            act = ACT_OK;
        }
        if (act == ACT_OK) {
            lb->max_slots = slots_pick(lb->cursor);
            lb->in_lobby = 1;
            lb->role = 0;
            lb->phase = LP_HOST_WAIT;
            room_start_host(NULL, lb->max_slots);
            return 1;
        }
        if (act == ACT_BACK) { lb->phase = LP_CHOOSE; lb->cursor = 0; return 1; }
        break;
    case LP_HOST_WAIT:
        if (act == ACT_BACK) {
            m->view = V_GAMES;
            return 1;
        }
        break;
    case LP_JOIN_SEARCH:
        if (act == ACT_BACK) {
            lobby_leave(lb);
            lb->phase = LP_CHOOSE;
            lb->cursor = 1;
            return 1;
        }
        break;
    case LP_JOIN_PICK:
        if (act == ACT_UP) { if (lb->join_sel > 0) lb->join_sel--; return 1; }
        if (act == ACT_DOWN) {
            if (lb->join_sel < lb->nhosts - 1) lb->join_sel++;
            return 1;
        }
        if (act == ACT_LOBBY_ROW && arg < lb->nhosts) lb->join_sel = arg;
        if ((act == ACT_OK || act == ACT_LOBBY_ROW) && lb->nhosts > 0) {
            snprintf(lb->room, LOBBY_NAME, "%s",
                     lb->hosts_name[lb->join_sel][0] ? lb->hosts_name[lb->join_sel] : lb->hosts_addr[lb->join_sel]);
            snprintf(lb->joined, sizeof lb->joined, "%s", lb->hosts_addr[lb->join_sel]);
            room_join_host(lb->hosts_addr[lb->join_sel]);
            lb->phase = LP_GUEST_WAIT;
            return 1;
        }
        if (act == ACT_BACK) {
            lobby_leave(lb);
            lb->cursor = 1;
            return 1;
        }
        break;
    case LP_GUEST_WAIT:
        if (act == ACT_BACK) {
            m->view = V_GAMES;
            return 1;
        }
        break;
    default:
        break;
    }
    return 0;
}