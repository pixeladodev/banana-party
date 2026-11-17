#ifndef LOBBY_H
#define LOBBY_H

#include "pixel.h"

#define LOBBY_MEMBERS 4
#define LOBBY_NAME 40

enum {
    LP_CHOOSE = 0,
    LP_HOST_SLOTS,
    LP_HOST_WAIT,
    LP_JOIN_SEARCH,
    LP_JOIN_PICK,
    LP_GUEST_WAIT
};

typedef struct {
    int in_lobby;
    int role;
    int phase;
    int cursor;
    int max_slots;
    int nmembers;
    char members[LOBBY_MEMBERS][LOBBY_NAME];
    int roles[LOBBY_MEMBERS];
    int member_me[LOBBY_MEMBERS];
    int member_ping[LOBBY_MEMBERS];
    char member_ip[LOBBY_MEMBERS][48];
    char member_status[LOBBY_MEMBERS][24];
    char room[LOBBY_NAME];
    char joined[48];
    int join_sel;
    int search_done;
    int nhosts;
    char hosts_name[LOBBY_MEMBERS][LOBBY_NAME];
    char hosts_addr[LOBBY_MEMBERS][48];
    int hosts_players[LOBBY_MEMBERS];
    int hosts_max[LOBBY_MEMBERS];
} Lobby;

extern void room_set_app(const char *dir);

void lobby_reset(Lobby *lb);
void lobby_tick(Lobby *lb, int view);
void lobby_leave(Lobby *lb);
struct Model;
/* the library's room band. Returns the y under it. */
float lobby_library_status(PxCanvas *c, const struct Model *m, float y);
void lobby_top(PxCanvas *c, const struct Model *m);
void lobby_bottom(PxCanvas *c, const struct Model *m);
int lobby_handle(struct Model *m, int act, int arg);

#endif