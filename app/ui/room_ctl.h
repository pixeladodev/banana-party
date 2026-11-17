#ifndef ROOM_CTL_H
#define ROOM_CTL_H

#include "lobby.h"

#define ROOM_CTL_DIR "/data/bananads-lan/lobby"
#define ROOM_HOST_MAX 16

typedef struct {
    char mode[12];
    char state[12];
    int max_players;
    int host_players;
    int nplayers;
    char players[ROOM_HOST_MAX][40];
    int proles[ROOM_HOST_MAX]; /* 0 host, 1 guest */
    int pme[ROOM_HOST_MAX];
    int player_ping[ROOM_HOST_MAX];
    char player_status[ROOM_HOST_MAX][24];
    char player_ip[ROOM_HOST_MAX][48];
    int nhosts;
    char host_name[ROOM_HOST_MAX][64];
    char host_addr[ROOM_HOST_MAX][48];
    int host_players_n[ROOM_HOST_MAX];
    int host_max[ROOM_HOST_MAX];
    char msg[96];
    int room_pid;
    int room_on;
} RoomSnap;

void room_set_app(const char *dir);
void room_snap_clear(RoomSnap *s);
int room_poll(RoomSnap *s);
int room_running(void);
int room_start_host(const char *app, int max);
int room_start_join(const char *app);
int room_join_host(const char *host_addr);
int room_leave(const char *app);
int room_library_back(void); /* B: stay in room */
void room_ui_present(void);
void room_ui_away(void);
void room_sync_lobby(const RoomSnap *s, Lobby *lb, const struct Model *m);
int room_handoff_play(const char *app);
int room_resume(const char *app);
int room_quit(const char *app); /* close the app: end the room, do not keep it */

#endif