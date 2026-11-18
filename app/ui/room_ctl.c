#include "room_ctl.h"
#include "lobby.h"
#include "view.h"

#include <ctype.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static char g_app[512];
/* Leave is in flight. Status can still say host for a moment; do not
   put the screen back in the room until the mode is off. */
static int g_left;
/* Host or join was just asked. Status can still say off for a moment. */
static int g_hold;

void room_set_app(const char *dir) {
    if (dir && dir[0]) snprintf(g_app, sizeof g_app, "%s", dir);
}

static void copy_app(const char *app) {
    if (app && app[0]) snprintf(g_app, sizeof g_app, "%s", app);
}

static int run_room_sh(const char *args) {
    char cmd[640];
    if (!g_app[0]) return 0;
    snprintf(cmd, sizeof cmd, "sh '%s/room.sh' %s", g_app, args);
    return system(cmd) == 0;
}

static int write_cmd(const char *line) {
    char path[256];
    FILE *f;
    snprintf(path, sizeof path, "%s/cmd", ROOM_CTL_DIR);
    f = fopen(path, "w");
    if (!f) return 0;
    fputs(line, f);
    fputc('\n', f);
    return fclose(f) == 0;
}

void room_snap_clear(RoomSnap *s) {
    memset(s, 0, sizeof *s);
}

int room_running(void) {
    char path[256], buf[16];
    FILE *f;
    int pid;
    snprintf(path, sizeof path, "%s/room.pid", ROOM_CTL_DIR);
    f = fopen(path, "r");
    if (!f) return 0;
    if (!fgets(buf, sizeof buf, f)) {
        fclose(f);
        return 0;
    }
    fclose(f);
    pid = atoi(buf);
    return pid > 0 && kill(pid, 0) == 0;
}

int room_poll(RoomSnap *s) {
    char path[512], line[512];
    FILE *f;
    room_snap_clear(s);
    s->room_on = room_running();
    s->room_pid = s->room_on; /* nonzero means a room process is running */
    snprintf(path, sizeof path, "%s/status", ROOM_CTL_DIR);
    f = fopen(path, "r");
    if (!f) return 0;
    while (fgets(line, sizeof line, f)) {
        char *nl = strchr(line, '\n');
        if (nl) *nl = 0;
        if (!strncmp(line, "mode=", 5)) snprintf(s->mode, sizeof s->mode, "%s", line + 5);
        else if (!strncmp(line, "state=", 6)) snprintf(s->state, sizeof s->state, "%s", line + 6);
        else if (!strncmp(line, "max=", 4)) s->max_players = atoi(line + 4);
        else if (!strncmp(line, "host_players=", 13)) s->host_players = atoi(line + 13);
        else if (!strncmp(line, "nplayers=", 9) || !strncmp(line, "nhosts=", 7)) {
            /* The count is how many lines follow. Using it as the write index left a blank row in front. */
        } else if (!strncmp(line, "msg=", 4)) snprintf(s->msg, sizeof s->msg, "%s", line + 4);
        else if (!strncmp(line, "player\t", 7)) {
            char name[40];
            int host, me, ping;
            char st[24], ip[48];
            int n;
            name[0] = st[0] = ip[0] = 0;
            host = me = 0;
            ping = -1;
            n = sscanf(line + 7, "%39[^\t]\t%d\t%d\t%d\t%23[^\t]\t%47s", name, &host, &me, &ping, st, ip);
            if (n >= 3 && name[0] && s->nplayers < ROOM_HOST_MAX) {
                int ix = s->nplayers;
                snprintf(s->players[ix], sizeof s->players[0], "%s", name);
                s->proles[ix] = host ? 0 : 1;
                s->pme[ix] = me;
                s->player_ping[ix] = ping;
                snprintf(s->player_status[ix], sizeof s->player_status[0], "%s", n >= 5 ? st : "");
                snprintf(s->player_ip[ix], sizeof s->player_ip[0], "%s", n >= 6 ? ip : "");
                s->nplayers++;
            }
        } else if (!strncmp(line, "host\t", 5)) {
            char name[64], addr[48];
            int players, max;
            name[0] = addr[0] = 0;
            if (s->nhosts < ROOM_HOST_MAX &&
                sscanf(line + 5, "%63[^\t]\t%47[^\t]\t%d\t%d", name, addr, &players, &max) >= 4 &&
                addr[0]) {
                snprintf(s->host_name[s->nhosts], sizeof s->host_name[0], "%s", name);
                snprintf(s->host_addr[s->nhosts], sizeof s->host_addr[0], "%s", addr);
                s->host_players_n[s->nhosts] = players;
                s->host_max[s->nhosts] = max;
                s->nhosts++;
            }
        }
    }
    fclose(f);
    return 1;
}

int room_start_host(const char *app, int max) {
    char args[32];
    copy_app(app);
    g_left = 0;
    g_hold = 1;
    snprintf(args, sizeof args, "host %d", max);
    return run_room_sh(args);
}

int room_start_join(const char *app) {
    copy_app(app);
    g_left = 0;
    g_hold = 1;
    return run_room_sh("join");
}

int room_join_host(const char *host_addr) {
    if (!host_addr || !host_addr[0]) return 0;
    char line[64];
    g_left = 0;
    g_hold = 1;
    snprintf(line, sizeof line, "join %s", host_addr);
    return write_cmd(line);
}

int room_leave(const char *app) {
    copy_app(app);
    g_hold = 0;
    g_left = 1;
    return run_room_sh("off");
}

int room_library_back(void) { return 1; }

void room_ui_present(void)
{
    char path[256];
    FILE *f;
    snprintf(path, sizeof path, "%s/ui_present", ROOM_CTL_DIR);
    f = fopen(path, "w");
    if (!f) return;
    fputc('1', f);
    fclose(f);
    snprintf(path, sizeof path, "%s/away_since", ROOM_CTL_DIR);
    remove(path);
}

void room_ui_away(void)
{
    char path[256];
    FILE *f;
    time_t now = time(NULL);
    snprintf(path, sizeof path, "%s/ui_present", ROOM_CTL_DIR);
    remove(path);
    snprintf(path, sizeof path, "%s/away_since", ROOM_CTL_DIR);
    f = fopen(path, "w");
    if (!f) return;
    fprintf(f, "%ld\n", (long)now);
    fclose(f);
}

int room_resume(const char *app) {
    copy_app(app);
    return run_room_sh("boot");
}

int room_quit(const char *app) {
    copy_app(app);
    return run_room_sh("quit");
}

int room_handoff_play(const char *app) {
    (void)app;
    /* The game is the same process. Nothing here is killed. */
    return 1;
}

static int room_phase_live(int phase)
{
    return phase == LP_HOST_WAIT || phase == LP_GUEST_WAIT ||
           phase == LP_JOIN_SEARCH || phase == LP_JOIN_PICK;
}

void room_sync_lobby(const RoomSnap *rs, Lobby *lb, const struct Model *m)
{
    int i, host, join, live;
    if (g_left) {
        lb->in_lobby = 0;
        lb->role = 0;
        lb->nmembers = 0;
        /* Leave the choose screen and the player list alone. Forcing CHOOSE here
           put Host back to the list before a count could be picked. */
        if (room_phase_live(lb->phase))
            lb->phase = LP_CHOOSE;
        if (!rs->room_on || !strcmp(rs->mode, "off"))
            g_left = 0;
        return;
    }
    host = !strcmp(rs->mode, "host");
    join = !strcmp(rs->mode, "join");
    if (host || join) g_hold = 0;
    live = rs->room_on && (host || join || !strcmp(rs->mode, "auto"));
    if (!live) {
        lb->in_lobby = 0;
        if (!g_hold && room_phase_live(lb->phase))
            lb->phase = LP_CHOOSE;
        if (!rs->room_on) {
            lb->role = 0;
            lb->nmembers = 0;
        }
        return;
    }
    if (rs->room_on || lb->in_lobby) {
        if (host) lb->role = 0;
        else if (join || !strcmp(rs->mode, "auto")) lb->role = 1;
        lb->in_lobby = 1;
        if (lb->in_lobby) {
            lb->max_slots = rs->max_players > 0 ? rs->max_players : lb->max_slots;
            lb->nmembers = 0;
            for (i = 0; i < rs->nplayers && lb->nmembers < LOBBY_MEMBERS; i++) {
                int ix = lb->nmembers;
                if (!rs->players[i][0]) continue;
                snprintf(lb->members[ix], LOBBY_NAME, "%s", rs->players[i]);
                lb->roles[ix] = rs->proles[i];
                lb->member_me[ix] = rs->pme[i];
                lb->member_ping[ix] = rs->player_ping[i];
                snprintf(lb->member_status[ix], sizeof lb->member_status[0], "%s", rs->player_status[i]);
                if (rs->pme[i] && m->ip[0])
                    snprintf(lb->member_ip[ix], sizeof lb->member_ip[0], "%s", m->ip);
                else if (rs->player_ip[i][0])
                    snprintf(lb->member_ip[ix], sizeof lb->member_ip[0], "%s", rs->player_ip[i]);
                else if (rs->proles[i] == 0 && lb->joined[0])
                    snprintf(lb->member_ip[ix], sizeof lb->member_ip[0], "%s", lb->joined);
                else lb->member_ip[ix][0] = 0;
                lb->nmembers++;
            }
            if (!strcmp(rs->mode, "host") && lb->nmembers > 0) {
                const char *who = lb->members[0];
                for (i = 0; i < lb->nmembers; i++)
                    if (lb->roles[i] == 0) { who = lb->members[i]; break; }
                snprintf(lb->room, LOBBY_NAME, "%s's Party", who);
            }
            lb->nhosts = 0;
            for (i = 0; i < rs->nhosts && lb->nhosts < LOBBY_MEMBERS; i++) {
                int ix = lb->nhosts;
                if (!rs->host_addr[i][0]) continue;
                snprintf(lb->hosts_name[ix], LOBBY_NAME, "%s", rs->host_name[i]);
                snprintf(lb->hosts_addr[ix], sizeof lb->hosts_addr[0], "%s", rs->host_addr[i]);
                lb->hosts_players[ix] = rs->host_players_n[i];
                lb->hosts_max[ix] = rs->host_max[i];
                lb->nhosts++;
            }
            if (lb->join_sel >= lb->nhosts) lb->join_sel = lb->nhosts > 0 ? lb->nhosts - 1 : 0;
            if (lb->phase == LP_JOIN_SEARCH && rs->nhosts > 0)
                lb->phase = LP_JOIN_PICK;
            if (!strcmp(rs->mode, "join") && !strcmp(rs->state, "ready") && rs->nplayers > 0)
                lb->phase = LP_GUEST_WAIT;
            /* Back from a game: the room process is already this mode. The library opens on the choose
               screen, so land on the room that is actually open. */
            if (rs->room_on && lb->phase == LP_CHOOSE) {
                if (!strcmp(rs->mode, "host")) lb->phase = LP_HOST_WAIT;
                else if (!strcmp(rs->mode, "join"))
                    lb->phase = (!strcmp(rs->state, "ready") && rs->nplayers > 0) ? LP_GUEST_WAIT : LP_JOIN_SEARCH;
            }
        }
    }
}