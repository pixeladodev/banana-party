/* Banana Party: the two panels in the menu's look (pixel.c).
   Top: what the app sees (local multiplayer on or off, this console, BananaOS, the network, its own BananaDS, the version).
   Bottom: a list as Settings and Bshare draw theirs, with the keys along the bottom. */
#include "view.h"
#include "lobby.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>

Hit g_hits[MAX_HITS];
int g_nhits;

/* Spanish: neutral, and the words BananaOS's own translation uses (Encendido, Atrás, Consola...) */
const PxTr view_tr[] = {
    { "Local multiplayer", "Multijugador local" },
    { "Updates", "Actualizaciones" },
    { "Uninstall", "Desinstalar" },
    { "On", "Encendido" },
    { "Off", "Apagado" },
    { "Exit", "Salir" },
    { "Change", "Cambiar" },
    { "Cancel", "Cancelar" },
    { "Back", "Atrás" },
    { "Ready", "Listo" },
    { "This console", "Esta consola" },
    { "Network", "Red" },
    { "DS emulator", "Emulador de DS" },
    { "App version", "Versión de la app" },
    { "No network", "Sin red" },
    { "Unknown", "Desconocida" },
    { "Games you open in Play look for other consoles on your Wi-Fi as they start. A game's menu can change it too.",
      "Los juegos que abras en Jugar buscan otras consolas en tu Wi-Fi al empezar. También puedes cambiarlo en el menú del juego." },
    { "Games you open in Play start with local multiplayer off. Turn it on in the game's menu when you need it.",
      "Los juegos que abras en Jugar empiezan sin multijugador local. Actívalo en el menú del juego cuando lo necesites." },
    { "Open Play, pick the same game on each console and go to its wireless mode: the consoles find each other.",
      "Entra a Jugar, elige el mismo juego en cada consola y entra a su modo inalámbrico: las consolas se encuentran solas." },
    { "About", "Acerca de" },
    { "Version", "Versión" },
    { "Made by", "Hecha por" },
    { "Source code", "Código fuente" },
    { "Credits", "Créditos" },
    { "Local wireless multiplayer for DS games on BananaOS: two consoles on the same Wi-Fi play together, as on a real DS.",
      "Multijugador inalámbrico local para juegos de DS en BananaOS: dos consolas en el mismo Wi-Fi juegan juntas, como en una DS de verdad." },
    { "BananaOS and BananaLinux by JorreFog and the Banana contributors (MIT). BananaDS (SuperDrastic) and melonDS (GPLv3). Pixelify Sans (OFL).",
      "BananaOS y BananaLinux, de JorreFog y los colaboradores de Banana (MIT). BananaDS (SuperDrastic) y melonDS (GPLv3). Pixelify Sans (OFL)." },
    { "An independent app, not part of BananaOS. Built and tested on the RG DS and the RG DS Plus. Free software under the GPLv3.",
      "Una app independiente, no es parte de BananaOS. Hecha y probada en la RG DS y la RG DS Plus. Software libre bajo la GPLv3." },
    { "Uninstall Banana Party?", "¿Desinstalar Banana Party?" },
    { "Turns it off and deletes this app.", "Lo apaga y borra esta app." },
    { "Play", "Jugar" },
    { "Games", "Juegos" },
    { "%d games", "%d juegos" },
    { "1 game", "1 juego" },
    { "No games", "Sin juegos" },
    { "Game", "Juego" },
    { "Save", "Partida guardada" },
    { "None yet", "Todavía no" },
    { "Emulator", "Emulador" },
    { "Open", "Abrir" },
    { "Missing", "No está" },
    { "Folder", "Carpeta" },
    { "Your DS library on the shelf. Games run on the bundled BananaDS build; local multiplayer is supported. BananaOS's system BananaDS is unchanged.",
      "Tu biblioteca de DS en el estante. Los juegos usan el BananaDS incluido; hay multijugador local. El BananaDS del sistema de BananaOS no cambia." },
    { "Saves are shared with BananaOS's BananaDS. Save states are kept apart.",
      "Las partidas guardadas se comparten con el BananaDS de BananaOS. Los estados guardados van aparte." },
    { "No DS games in roms/nds.", "No hay juegos de DS en roms/nds." },
    { "Auto discovery", "Descubrimiento automático" },
    { "DS Download Play", "Descarga DS" },
    { "Download", "Descarga" },
    { "Open the DS menu?", "¿Abrir el menú de DS?" },
    { "This starts your console's DS firmware. In the DS menu, choose DS Download Play to join the game another console is hosting.",
      "Se cargará el firmware de DS de tu consola. En el menú de DS, elige Descarga DS para unirte a la partida que aloja otra consola." },
    { "BIOS files missing", "Faltan archivos de la BIOS" },
    { "Missing: %s", "Faltan: %s" },
    { "Copy your own DS BIOS and firmware to the bios folder at the root of the card (/bios): bios7.bin, bios9.bin and firmware.bin.",
      "Copia la BIOS y el firmware de tu propia DS en la carpeta bios de la raíz de la tarjeta (/bios): bios7.bin, bios9.bin y firmware.bin." },
    { "Optional: DS Download Play", "Opcional: Descarga DS" },
    { "To join a game with DS Download Play, copy your own DS files to the bios folder at the root of the card (/bios, next to banana, bananads and roms): bios7.bin (16 KB), bios9.bin (4 KB) and firmware.bin (256 KB). If they're already on the SD card, copy them there with the BananaOS file explorer. Everything else works without them.",
      "Para unirte a una partida con Descarga DS, copia los archivos de tu propia DS en la carpeta bios de la raíz de la tarjeta (/bios, junto a banana, bananads y roms): bios7.bin (16 KB), bios9.bin (4 KB) y firmware.bin (256 KB). Si ya los tienes en la tarjeta SD, cópialos ahí con el explorador de archivos de BananaOS. Todo lo demás funciona sin ellos." },
    { "OK", "Aceptar" },
    { "Reset settings", "Restablecer" },
    { "Reset", "Restablecer" },
    { "Reset the DS settings?", "¿Restablecer los ajustes de DS?" },
    { "Your name, language and the other DS menu settings go back to those in your firmware.bin in /bios.",
      "Tu nombre, idioma y los demás ajustes del menú de DS vuelven a los de tu firmware.bin de /bios." },
    { "Ready", "Lista" },
    { "Opens the DS menu with your BIOS and firmware. Choose DS Download Play there to join a game another console hosts with one card.",
      "Abre el menú de DS con tu BIOS y firmware. Elige Descarga DS ahí para unirte a una partida que otra consola aloja con una sola tarjeta." },
    { "Needs your DS BIOS and firmware in the bios folder at the root of the card (/bios): bios7.bin, bios9.bin and firmware.bin.",
      "Necesita tu BIOS y firmware de DS en la carpeta bios de la raíz de la tarjeta (/bios): bios7.bin, bios9.bin y firmware.bin." },
    { "Auto discovery is on. Entering multiplayer tries to pair with someone else.",
      "Descubrimiento automático activo. Al entrar a multijugador se intentará emparejar con otra persona." },
    { "Auto discovery is off. The game plays on its own, unless you change it in BananaDS LAN's settings.",
      "Descubrimiento automático desactivado. El juego irá solo, salvo que lo cambies en los ajustes de BananaDS LAN." },
    { "Page", "Página" },
    { "Settings", "Ajustes" },
    { "List", "Lista" },
    { "Grid", "Grilla" },
    { "Order: A-Z", "Orden: A-Z" },
    { "Order: Favorites", "Orden: Favoritos" },
    { "Order: Recent", "Orden: Recientes" },
    { "Add favorite", "Añadir a favoritos" },
    { "Remove favorite", "Quitar de favoritos" },
    { "Favorite", "Favorito" },
    { "Play time", "Tiempo de juego" },
    { "Last played", "Última vez" },
    { "Not played yet", "Sin jugar todavía" },
    { "Never", "Nunca" },
    { "Under a minute", "Menos de un minuto" },
    { "Today, %s", "Hoy, %s" },
    { "Yesterday, %s", "Ayer, %s" },
    { "Jan", "ene" }, { "Feb", "feb" }, { "Mar", "mar" }, { "Apr", "abr" }, { "May", "may" }, { "Jun", "jun" },
    { "Jul", "jul" }, { "Aug", "ago" }, { "Sep", "sept" }, { "Oct", "oct" }, { "Nov", "nov" }, { "Dec", "dic" },
    { "Put your .nds games in roms/nds on the card (or the second card) and open the app again.",
      "Pon tus juegos .nds en roms/nds de la tarjeta (o de la segunda tarjeta) y abre la app de nuevo." },
    { "v%s is out: START, Updates.", "Ya salió la v%s: START, Actualizaciones." },
    { "Pick the same game on each console and go to its wireless mode: the consoles find each other.",
      "Elige el mismo juego en cada consola y entra a su modo inalámbrico: las consolas se encuentran solas." },
    { "Games you open here look for other consoles on your Wi-Fi as they start. A game's menu can change it too.",
      "Los juegos que abras aquí buscan otras consolas en tu Wi-Fi al empezar. También puedes cambiarlo en el menú del juego." },
    { "Games you open here start with local multiplayer off. Turn it on in the game's menu when you need it.",
      "Los juegos que abras aquí empiezan sin multijugador local. Actívalo en el menú del juego cuando lo necesites." },
    { "Lobby", "Sala" },
    { "Host", "Anfitrión" },
    { "You", "Tú" },
    { "(you)", "(tú)" },
    { "Guest", "Invitado" },
    { "Guests", "Invitados" },
    { "Join", "Unirse" },
    { "Library", "Biblioteca" },
    { "Not in a lobby", "Sin sala" },
    { "Hosting", "Anfitrión" },
    { "Joined as guest", "Invitado en sala" },
    { "Host a room for friends\nor join one on your network.",
      "Abre una sala para tus amigos\no únete a una en tu red." },
    { "Pick Host or Join on the bottom screen.", "Elige Anfitrión o Unirse en la pantalla inferior." },
    { "How many can be in this lobby?", "¿Cuántos pueden estar en esta sala?" },
    { "Up to %d consoles, including this one.", "Hasta %d consolas, incluida esta." },
    { "Choose 2, 3, or 4. Then wait for guests on the next screen.",
      "Elige 2, 3 o 4. Luego espera invitados en la siguiente pantalla." },
    { "Room created", "Sala creada" },
    { "Waiting for guests", "Esperando invitados" },
    { "Guests appear here when they connect on the network.",
      "Los invitados aparecen aquí cuando se conectan en la red." },
    { "Join", "Unirse" },
    { "Room", "Sala" },
    { "Members", "Miembros" },
    { "Guests appear in the list when they connect. The room service is not wired yet, so the list stays honest.",
      "Los invitados aparecen en la lista al conectarse. El servicio de sala aún no está conectado; la lista no inventa jugadores." },
    { "Searching", "Buscando" },
    { "Waiting", "Esperando" },
    { "Searching for hosts", "Buscando anfitriones" },
    { "Looking on your local network.", "Buscando en tu red local." },
    { "Hosts nearby", "Anfitriones cercanos" },
    { "No hosts found.", "No se encontró ningún anfitrión." },
    { "Check Wi-Fi or Direct Link, or ask a friend to host. Joining needs the emulator room API.",
      "Revisa Wi-Fi o Direct Link, o pide a un amigo que abra sala. Unirse requiere la API de sala del emulador." },
    { "In the lobby", "En la sala" },
    { "Waiting for the host to start.", "Esperando a que el anfitrión empiece." },
    { "Find a host on your network.", "Busca un anfitrión en tu red." },
    { "Let friends join this console.", "Deja que otros se unan a esta consola." },
    { "members", "miembros" },
    { "%d players", "%d jugadores" },
    { "Choose", "Elegir" },
    { "Leave", "Salir" },
    { "Leave room", "Salir de la sala" },
    { "Back", "Atrás" },
    { "Cancel", "Cancelar" },
    { "No guests yet", "Aún no hay invitados" },
    { "Searching for room", "Buscando sala" },
    { "Game host", "Anfitrión del juego" },
    { "Idle", "En espera" },
    { "#", "#" },
    { "Player", "Jugador" },
    { "Status", "Estado" },
    { "Ping", "Ping" },
    { "IP", "IP" },
    { "LAN needs low latency on your network. It will not work well through a VPN or any tunnel.",
      "El modo LAN necesita poca latencia en tu red. No funcionará bien con VPN ni túneles." },
    { "Leave the lobby?", "¿Salir de la sala?" },
    { "This ends your lobby session for everyone.", "Cierra la sala para todos." },
    { "Leave the app?", "¿Salir de la app?" },
    { "The room will close.", "Se cerrará la sala." },
    { "Stay", "Quedarme" },
    { "Exit app", "Salir" },
    { "Waiting for a guest…", "Esperando un invitado…" },
    { "Searching…", "Buscando…" },
    { "No hosts found", "Sin anfitriones" },
    { "Rooms", "Salas" },
    { "On your network", "En tu red" },
    { "Your room", "Tu sala" },
    { "Connected", "Conectado" },
    { "%d of %d", "%d de %d" },
    { "Name", "Nombre" },
    { "You are the only one so far. Others see this name on the network.",
      "Solo tú, por ahora. Los demás ven este nombre en la red." },
    { "Pick a room. Each person opens their own game.",
      "Elige una sala. Cada quien abre su juego." },
    { "You leave this room. The host's room stays open.",
      "Sales tú. La sala del anfitrión sigue." },
    { NULL, NULL }
};

#define T(s) px_tr(s)

static void games_top(PxCanvas *c, const Model *m);
static void games_bottom(PxCanvas *c, const Model *m);

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

/* a file in BIOS_DIR under one of `names`, of `size` bytes (0: 128, 256 or 512 KB, a firmware) */
static int bios_has(const char *const *names, long size)
{
    char path[256];
    for (; *names; names++) {
        FILE *f;
        long n;
        snprintf(path, sizeof path, "%s/%s", BIOS_DIR, *names);
        if (!(f = fopen(path, "rb"))) continue;
        n = fseek(f, 0, SEEK_END) == 0 ? ftell(f) : -1;
        fclose(f);
        if (size ? n == size : n == 0x20000 || n == 0x40000 || n == 0x80000) return 1;
    }
    return 0;
}

/* the DS's own BIOS and firmware are there (the names BananaDS looks for) */
int view_firmware_ok(void)
{
    static const char *const b9[] = { "bios9.bin", "biosnds9.rom", "bios9.rom", "nds_bios_arm9.bin", NULL };
    static const char *const b7[] = { "bios7.bin", "biosnds7.rom", "bios7.rom", "nds_bios_arm7.bin", NULL };
    static const char *const fw[] = { "firmware.bin", "nds_firmware.bin", "firmware.rom", "fw.bin", NULL };
    return bios_has(b9, 4096) && bios_has(b7, 16384) && bios_has(fw, 0);
}

int view_firmware_copy(void)
{
    return access(FW_COPY, F_OK) == 0 || getenv("PREVIEW_FW_COPY") != NULL;  /* (preview.c: as if there) */
}

int view_firmware_missing(char *buf, int n)
{
    static const char *const b9[] = { "bios9.bin", "biosnds9.rom", "bios9.rom", "nds_bios_arm9.bin", NULL };
    static const char *const b7[] = { "bios7.bin", "biosnds7.rom", "bios7.rom", "nds_bios_arm7.bin", NULL };
    static const char *const fw[] = { "firmware.bin", "nds_firmware.bin", "firmware.rom", "fw.bin", NULL };
    int k = 0;
    if (n > 0) buf[0] = 0;
    if (!bios_has(b7, 16384)) { snprintf(buf + strlen(buf), (size_t)(n - (int)strlen(buf)), "%sbios7.bin", k++ ? ", " : ""); }
    if (!bios_has(b9, 4096)) { snprintf(buf + strlen(buf), (size_t)(n - (int)strlen(buf)), "%sbios9.bin", k++ ? ", " : ""); }
    if (!bios_has(fw, 0)) { snprintf(buf + strlen(buf), (size_t)(n - (int)strlen(buf)), "%sfirmware.bin", k++ ? ", " : ""); }
    return k;
}

int view_row_enabled(const Model *m, int row)
{
    (void)m;
    (void)row;
    return 1;
}

static const char *model_name(const char *id)
{
    if (!strcmp(id, "rgds")) return "RG DS";
    if (!strcmp(id, "rgds_plus")) return "RG DS Plus";
    return T("Unknown");
}

/* a value over its label, as the game page's stat block */
static void stat(PxCanvas *c, float x, float y, float w, const char *label, const char *value, uint32_t rgb)
{
    px_text_fit(c, PX_MEDIUM, 22, x, y, value, rgb, 0, w, PX_LEFT);
    px_label(c, x, y + 28, label, px.muted);
}

void view_top(PxCanvas *c, const Model *m)
{
    char buf[128], net[96];
    const char *hero = m->enabled ? T("On") : T("Off");
    const char *msg = NULL;
    uint32_t msg_rgb = px.ink;
    float hw, cw;
    /* the cover shine belongs to the library's top screen: any other view forgets its rectangle, so main.c's
       sweep frames don't paint it over that view */
    if (m->view != V_GAMES) px_cover_shine(NULL, 0, 0, 0, 0);
    if (m->view == V_LOBBY) { lobby_top(c, m); return; }
    if (m->view == V_LOBBY_EXIT || m->view == V_LOBBY_DISCONNECT || m->view == V_FW_CONFIRM || m->view == V_FW_MISSING ||
        m->view == V_BIOS_NOTICE || m->view == V_FW_RESET) {
        px_screen(c);
        px_status(c, "Banana Party", m->battery, m->charging, m->clock);
        return;
    }
    if (m->view == V_GAMES) { games_top(c, m); return; }
    px_screen(c);
    px_status(c, "Banana Party", m->battery, m->charging, m->clock);
    px_sheet(c, 18, 60, 604, 406);

    px_label(c, 44, 84, T("Local multiplayer"), px.ink);
    /* the state, big: green on, grey off */
    hw = px_badge(c, 42, 106, 52, hero, m->enabled ? PX_BADGE_SUCCESS : PX_BADGE_NEUTRAL);
    /* beside it: ready, or why not */
    snprintf(buf, sizeof buf, "%s", T("Ready"));
    cw = px_text_w(c, PX_MEDIUM, 15, buf, PX_UPPER | PX_TRACK) + 24;
    px_chip(c, 42 + hw + 18, 117, cw, 30, 0);
    px_text(c, PX_MEDIUM, 15, 42 + hw + 30, 122, buf, px.ink, PX_UPPER | PX_TRACK);

    px_line_h(c, 42, 168, 556, 2, px.edge, 70);

    stat(c, 42, 186, 260, T("This console"), model_name(m->model), px.ink);
    stat(c, 330, 186, 260, "BananaOS", m->os[0] ? m->os : "?", px.ink);

    if (!m->ip[0]) snprintf(net, sizeof net, "%s", T("No network"));
    else if (!strncmp(m->ip, "10.42.0.", 8)) snprintf(net, sizeof net, "Direct Link  ·  %s", m->ip);
    else snprintf(net, sizeof net, "Wi-Fi  ·  %s", m->ip);
    if (!m->ip[0]) {
        px_label(c, 42, 282, T("Network"), px.muted);
        px_badge(c, 42, 248, 32, T("No network"), PX_BADGE_WARNING);
    } else stat(c, 42, 254, 260, T("Network"), net, px.ink);
    stat(c, 330, 254, 260, T("DS emulator"), "Banana Party", px.ink);

    if (m->upd.state == UPD_AVAILABLE) {
        char avail[64];
        snprintf(avail, sizeof avail, T("v%s available"), m->upd.latest);
        snprintf(buf, sizeof buf, "%s  ·  %s", m->version[0] ? m->version : "?", avail);
        stat(c, 42, 322, 556, T("App version"), buf, px.ink);
    } else stat(c, 42, 322, 260, T("App version"), m->version[0] ? m->version : "?", px.ink);

    px_line_h(c, 42, 380, 556, 2, px.edge, 70);
      if (m->err[0]) { msg = m->err; msg_rgb = px.warn; }
    else msg = T("Pick the same game on each console and go to its wireless mode: the consoles find each other.");
    px_text_wrap(c, PX_REGULAR, 18, 42, 392, msg, msg_rgb, 556, 2, 24);
}

void view_game_name(const char *path, char *dst, int n)
{
    const char *b = strrchr(path, '/'), *dot;
    int len;
    b = b ? b + 1 : path;
    dot = strrchr(b, '.');
    len = dot && dot > b ? (int)(dot - b) : (int)strlen(b);
    if (len > n - 1) len = n - 1;
    memcpy(dst, b, (size_t)len);
    dst[len] = 0;
}

/* "3 h 12 min", "45 min", "Under a minute" */
static void fmt_time(char *buf, size_t n, long secs)
{
    if (secs >= 3600) snprintf(buf, n, "%ld h %ld min", secs / 3600, secs % 3600 / 60);
    else if (secs >= 60) snprintf(buf, n, "%ld min", secs / 60);
    else snprintf(buf, n, "%s", T("Under a minute"));
}

/* today and yesterday by the hour (the clock as settings.conf has it), older by the date */
static void fmt_when(char *buf, size_t n, time_t t, int clock12)
{
    static const char *mon[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
    time_t now = time(NULL);
    struct tm a, b;
    char hm[16];
    long days;
    localtime_r(&t, &a);
    localtime_r(&now, &b);
    if (clock12) {
        int h = a.tm_hour % 12;
        snprintf(hm, sizeof hm, "%d:%02d %s", h ? h : 12, a.tm_min, a.tm_hour < 12 ? "AM" : "PM");
    } else snprintf(hm, sizeof hm, "%02d:%02d", a.tm_hour, a.tm_min);
    {
        struct tm da = a, db = b;
        da.tm_hour = db.tm_hour = 12;
        da.tm_min = db.tm_min = da.tm_sec = db.tm_sec = 0;
        days = (long)((mktime(&db) - mktime(&da) + 43200) / 86400);
    }
    if (days == 0) snprintf(buf, n, T("Today, %s"), hm);
    else if (days == 1) snprintf(buf, n, T("Yesterday, %s"), hm);
    else snprintf(buf, n, "%d %s %d", a.tm_mday, T(mon[a.tm_mon]), a.tm_year + 1900);
}

/* a short shadow under the picture and to its right, like a small CSS box-shadow: offset, then a few softer pixels */
static void cover_shadow(PxCanvas *c, float x, float y, float w, float h)
{
    static const int dark[] = { 110, 74, 42, 20, 8 };
    static const int lite[] = { 64, 42, 24, 12, 5 };
    const int *fall = px.light ? lite : dark;
    int i;
    if (w < 8 || h < 8) return;
    for (i = 0; i < 5; i++) {
        float o = (float)(i + 1);
        px_rect_a(c, x + o, y + h + (float)i, w, 1, 0x000000, fall[i]);
        if (h > o) px_rect_a(c, x + w + (float)i, y + o, 1, h - o, 0x000000, fall[i]);
    }
}

/* a cover, or the tile's place for one: the game's icon, else its initial on the card */
static void game_image(PxCanvas *c, const PxImage *img, float x, float y, float w, float h, const char *name, int sel)
{
    if (img && img->px) { px_image_draw(c, x, y, w, h, img); return; }
    {
        char ch[2] = { name[0] ? (char)toupper((unsigned char)name[0]) : '?', 0 };
        float sz = h * 0.5f;
        px_rect_a(c, x, y, w, h, sel ? 0xffffff : px.ink, 22);
        px_text(c, PX_MEDIUM, sz, x + (w - px_text_w(c, PX_MEDIUM, sz, ch, 0)) / 2, y + (h - sz) / 2 - sz * 0.12f, ch,
                sel ? px.on_pick_sub : px.muted, 0);
    }
}

/* top: the room first, then the picked game's cover, name, play time and last played */
static void games_top(PxCanvas *c, const Model *m)
{
    char name[GAME_PATH], buf[96], when[48];
    float room_y, game_y;
    px_screen(c);
    px_status(c, "Banana Party", m->battery, m->charging, m->clock);
    px_sheet(c, 18, 60, 604, 406);
    if (m->ngames <= 0) {
        px_cover_shine(NULL, 0, 0, 0, 0);
        px_label(c, 44, 84, T("Games"), px.ink);
        px_text_wrap(c, PX_MEDIUM, 26, 42, 110, T("No DS games in roms/nds."), px.warn, 556, 2, 32);
        px_text_wrap(c, PX_REGULAR, 18, 42, 190,
                     T("Put your .nds games in roms/nds on the card (or the second card) and open the app again."),
                     px.muted, 556, 3, 24);
        return;
    }
    room_y = lobby_library_status(c, m, 76);
    px_line_h(c, 42, room_y + 6, 556, 2, px.edge, 70);
    game_y = room_y + 18;
    view_game_name(m->games[m->gsel], name, (int)sizeof name);
    {
        /* the old inset: the cover starts at 40, and the text a gap past it */
        const float art_x = ART_X;
        const PxImage *art = m->art_game == m->gsel && m->art.px ? &m->art : NULL;
        int sx = 0, sy = 0, sw = 0, sh = 0;
        if (art && px_image_dest(c, art_x, game_y, ART_W, ART_H, art, &sx, &sy, &sw, &sh) && c->s > 0) {
            float inv = 1.0f / c->s;
            cover_shadow(c, sx * inv, sy * inv, sw * inv, sh * inv);
        }
        game_image(c, art, art_x, game_y, ART_W, ART_H, name, 0);
        if (art && sw >= 2)
            px_cover_shine(c, sx, sy, sw, sh);
        else
            px_cover_shine(c, 0, 0, 0, 0);
    }

    /* the title sits in two reserved lines and grows upward, so a second line never pushes the stats */
    {
        const float tx = ART_TEXT_X, tw = 598 - tx, lh = 28, slot = game_y + 22;
        int lines = px_text_lines(c, PX_MEDIUM, 22, name, tw, 2, lh);
        if (lines < 1) lines = 1;
        /* the "Game" label is gone; slot still starts a label-height under game_y */
        px_text_wrap(c, PX_MEDIUM, 22, tx, slot + (2 - lines) * lh, name, px.ink, tw, 2, lh);
        if (m->gsecs > 0) {
            fmt_time(buf, sizeof buf, m->gsecs);
            stat(c, tx, slot + 2 * lh + 10, tw, T("Play time"), buf, px.ink);
        } else stat(c, tx, slot + 2 * lh + 10, tw, T("Play time"), T("Not played yet"), px.muted);
        if (m->glast > 0) {
            fmt_when(when, sizeof when, m->glast, m->clock12);
            stat(c, tx, slot + 2 * lh + 66, tw, T("Last played"), when, px.ink);
        } else stat(c, tx, slot + 2 * lh + 66, tw, T("Last played"), T("Never"), px.muted);
    }
    if (m->err[0]) px_text_wrap(c, PX_REGULAR, 17, 42, 430, m->err, px.warn, 556, 2, 22);
    if (m->upd.state == UPD_AVAILABLE) {
        snprintf(buf, sizeof buf, T("v%s is out: START, Updates."), m->upd.latest);
        px_text_fit(c, PX_MEDIUM, 17, 42, 432, buf, px.accent.base, 0, 556, PX_LEFT);
    }
}

static const char *sort_label(int sort)
{
    if (sort == SORT_FAV) return T("Order: Favorites");
    if (sort == SORT_RECENT) return T("Order: Recent");
    return T("Order: A-Z");
}

/* top of the library panel: navigation hints and the game count in the middle */
static void library_top_legend(PxCanvas *c, const Model *m)
{
    char count[24];
    const char *sort = sort_label(m->sort);
    const char *view = m->grid ? T("List") : T("Grid");
    float y = 12, cy = y + 18, mid = 320, cw;
    uint32_t ink = px.ink, dim = px.dim;
    px_bubble(c, 18, y, 604, 42, 0);
    if (m->ngames > 0) {
        int vert = !m->grid;
        px_keycap(c, 34, y + 8, "L");
        px_triangle(c, 62, cy, 10, vert ? -2 : -1, ink);
        px_keycap(c, 78, y + 8, "L2");
        px_text_fit(c, PX_MEDIUM, 15, 114, y + 12, sort, ink, 0, 168, PX_LEFT);
        add_hit(c, 74, y + 4, 210, 34, HIT_SORT, 0);
        snprintf(count, sizeof count, "%02d / %02d", m->gsel + 1, m->ngames);
        cw = px_text_w(c, PX_MEDIUM, 19, count, 0);
        px_text(c, PX_MEDIUM, 19, mid - cw / 2, y + 11, count, px.muted, 0);
        {
            float x, vw, kw, hx0;
            x = 614;
            kw = px_keycap_w(c, "R");
            x -= kw;
            px_keycap(c, x, y + 8, "R");
            x -= 8;
            px_triangle(c, x - 5, cy, 10, vert ? 2 : 1, ink);
            x -= 16;
            kw = px_keycap_w(c, "R2");
            x -= kw;
            px_keycap(c, x, y + 8, "R2");
            x -= 8;
            vw = px_text_w(c, PX_MEDIUM, 15, view, 0);
            hx0 = x - vw - 4;
            x -= vw;
            px_text(c, PX_MEDIUM, 15, x, y + 12, view, ink, 0);
            add_hit(c, hx0, y + 4, 614 - hx0, 34, HIT_VIEW, 0);
        }
    } else {
        snprintf(count, sizeof count, "%s", T("No games"));
        cw = px_text_w(c, PX_MEDIUM, 19, count, 0);
        px_text(c, PX_MEDIUM, 19, mid - cw / 2, y + 11, count, dim, 0);
    }
    (void)dim;
}

/* bottom face row only */
static void library_bottom_faces(PxCanvas *c, const Model *m)
{
    PxRect r;
    float y = LIB_FACE_Y, bw, aw;
    int disfav = m->ngames <= 0;
    int fav_on = m->gfav && !disfav;
    float x = 18;
    /* no B Exit face on any panel, so SELECT shows the same everywhere (B still leaves) */
    px_face(c, x, y, PX_LEFT, "X", T("Lobby"), 0, &r, PX_FACE_PRIMARY);
    add_face_hit(r, HIT_LOBBY);
    bw = px_face_w(c, "X", T("Lobby"));
    x += bw + 8;
    aw = px_face_w(c, "A", T("Play"));
    {
        /* SELECT, DS Download Play, between Lobby and Favorite: the short label where the long one doesn't fit */
        const float room = 622 - aw - 8 - px_face_w(c, "Y", T("Favorite")) - 8 - x;
        const char *dl = T("DS Download Play");
        if (px_face_w(c, "SELECT", dl) > room) dl = T("Download");
        if (px_face_w(c, "SELECT", dl) > room) dl = "";
        px_face(c, x, y, PX_LEFT, "SELECT", dl, 0, &r, PX_FACE_NEUTRAL);
        add_face_hit(r, HIT_DLPLAY);
    }
    px_face(c, 622, y, PX_RIGHT, "A", T("Play"), m->ngames <= 0, &r, PX_FACE_NEUTRAL);
    add_face_hit(r, HIT_OK);
    px_face(c, 622 - aw - 8, y, PX_RIGHT, "Y", T("Favorite"), disfav, &r, fav_on ? PX_FACE_FAV : PX_FACE_NEUTRAL);
    add_face_hit(r, HIT_Y);
}

/* bottom: the library, as a grid scrolled sideways or as a list */
static void games_bottom(PxCanvas *c, const Model *m)
{
    char name[GAME_PATH];
    PxRect rail;
    int i;
    float grid_bot;
    g_nhits = 0;
    px_screen(c);
    library_top_legend(c, m);
    if (m->ngames <= 0) {
        px_card(c, 18, LIB_TOP, 604, 120, 0, 0);
        px_text_wrap(c, PX_MEDIUM, 22, 40, LIB_TOP + 20, T("No DS games in roms/nds."), px.ink, 560, 1, 28);
        px_text_wrap(c, PX_REGULAR, 17, 40, LIB_TOP + 56,
                     T("Put your .nds games in roms/nds on the card (or the second card) and open the app again."),
                     px.muted, 560, 2, 23);
    } else if (m->grid) {
        int cols = (m->ngames + GRID_ROWS - 1) / GRID_ROWS;
        for (i = 0; i < GRID_COLS * GRID_ROWS; i++) {
            int col = m->gleft + i / GRID_ROWS, rw = i % GRID_ROWS, g = col * GRID_ROWS + rw, sel;
            float x, y;
            if (col >= cols || g >= m->ngames) continue;
            sel = g == m->gsel;
            x = 18 + (i / GRID_ROWS) * (TILE_W + TILE_GAP);
            y = LIB_TOP + rw * (TILE_H + TILE_GAP);
            view_game_name(m->games[g], name, (int)sizeof name);
            px_card(c, x, y, TILE_W, TILE_H, sel, 0);
            game_image(c, m->tstate && m->tstate[g] == 1 ? &m->thumbs[g] : NULL, x + 8, y + 8, TILE_IMG_W, TILE_IMG_H,
                       name, sel);
            if (game_is_favorite(m->games[g]))
                px_fav_mark(c, x + TILE_W - FAV_MARK - 6, y + 6, FAV_MARK);
            add_hit(c, x, y, TILE_W, TILE_H, HIT_ROW, g);
        }
        grid_bot = LIB_TOP + GRID_TILES_H + LIB_GRID_GAP;
        if (cols > GRID_COLS || m->ngames > 1) {
            PxRect track, thumb;
            px_hrail(c, 18, grid_bot, LIB_W, m->gsel, m->ngames, NULL, &track, &thumb);
            g_hits[g_nhits].r = track;
            g_hits[g_nhits].id = HIT_RAIL_TRACK;
            g_hits[g_nhits].arg = 0;
            g_nhits++;
            g_hits[g_nhits].r = thumb;
            g_hits[g_nhits].id = HIT_RAIL_THUMB;
            g_hits[g_nhits].arg = 0;
            g_nhits++;
        }
    } else {
        static int mq_gsel = -1;
        float list_h = LIB_FACE_Y - LIST_Y0 - 8;
        float text_max = LIST_CARD_W - 86 - FAV_MARK - 18 - LIST_TEXT_INSET;
        px_marquee_begin();
        if (m->gsel != mq_gsel) {
            mq_gsel = m->gsel;
            if (!px_marquee_consume_seed()) px_marquee_reset();
        }
        for (i = 0; i < GAME_ROWS && m->gtop + i < m->ngames; i++) {
            int g = m->gtop + i, sel = g == m->gsel;
            float y = LIST_Y0 + i * LIST_ROW_PITCH;
            view_game_name(m->games[g], name, (int)sizeof name);
            px_card(c, 18, y, LIST_CARD_W, 54, sel, 0);
            game_image(c, m->tstate && m->tstate[g] == 1 ? &m->thumbs[g] : NULL, 32, y + 7, LIST_IMG, LIST_IMG, name,
                       sel);
            px_text_label(c, PX_MEDIUM, 20, 86, y + 14, name, sel ? px.on_pick : px.ink, 0, text_max, sel);
            if (game_is_favorite(m->games[g]))
                px_fav_mark(c, 18 + LIST_CARD_W - FAV_MARK - 6, y + 6, FAV_MARK);
            add_hit(c, 18, y, LIST_CARD_W, 54, HIT_ROW, g);
        }
        if (m->ngames > GAME_ROWS) {
            PxRect track, thumb;
            px_rail(c, LIST_RAIL_X, LIST_Y0, list_h, m->gtop, GAME_ROWS, m->ngames, &track, &thumb);
            g_hits[g_nhits].r = track;
            g_hits[g_nhits].id = HIT_VRAIL_TRACK;
            g_hits[g_nhits].arg = 0;
            g_nhits++;
            g_hits[g_nhits].r = thumb;
            g_hits[g_nhits].id = HIT_VRAIL_THUMB;
            g_hits[g_nhits].arg = 0;
            g_nhits++;
        }
    }
    library_bottom_faces(c, m);
}

static void row(PxCanvas *c, const Model *m, int i, float y)
{
    int sel = m->cursor == i && m->view == V_MAIN, dis = !view_row_enabled(m, i);
    uint32_t ink = sel && !dis ? px.on_pick : dis ? px.dim : px.ink;
    uint32_t sub = sel && !dis ? px.on_pick_sub : dis ? px.dim : px.muted;
    const char *title = i == ROW_DISCOVER ? T("Auto discovery") : i == ROW_DLPLAY ? T("DS Download Play")
                      : i == ROW_ABOUT ? T("About") : T("Updates");
    px_card(c, 18, y, 588, 54, sel, dis);
    px_text_fit(c, PX_MEDIUM, 20, 38, y + 14, title, ink, 0, 330, PX_LEFT);
    if (i == ROW_DISCOVER) {
        px_text_fit(c, PX_MEDIUM, 20, 584, y + 14, m->discover ? T("On") : T("Off"), ink, 0, 180, PX_RIGHT);
    } else if (i == ROW_DLPLAY) {
        px_text_fit(c, PX_MEDIUM, 20, 584, y + 14, view_firmware_ok() ? T("Ready") : T("Missing"), sub, 0, 180, PX_RIGHT);
    } else if (i == ROW_ABOUT) {
        /* opens with A; nothing on the right */
    } else if (i == ROW_UPDATES) {
        char buf[64];
        int d;
        uint32_t v = upd_lit(&m->upd) || upd_running(&m->upd) ? (sel ? px.on_pick : px.ink)
                                                              : sel ? px.on_pick_sub : px.muted;
        upd_value(&m->upd, buf, sizeof buf, &d, m->dots);
        if (m->upd.state == UPD_DOWNLOADING) {
            /* the percentage in a box of its own (the word beside it stays put), and a bar along the card */
            char pct[8];
            int p = m->upd.pct < 0 ? 0 : m->upd.pct > 100 ? 100 : m->upd.pct;
            float pw = px_text_w(c, PX_MEDIUM, 20, "100%", 0);
            snprintf(pct, sizeof pct, "%d%%", p);
            px_text(c, PX_MEDIUM, 20, 584 - px_text_w(c, PX_MEDIUM, 20, pct, 0), y + 14, pct, v, 0);
            px_text_fit(c, PX_MEDIUM, 20, 584 - pw - 10, y + 14, T("Downloading"), v, 0, 240, PX_RIGHT);
            px_rect(c, 30, y + 46, 564, 3, sel ? px.on_pick_sub : px.line);
            px_rect(c, 30, y + 46, 564.0f * p / 100, 3, sel ? px.on_pick : px.accent.base);
        } else if (d >= 0) px_text_dots(c, PX_MEDIUM, 20, 584, y + 14, buf, d, v, 0, PX_RIGHT);
        else px_text_fit(c, PX_MEDIUM, 20, 584, y + 14, buf, v, 0, 300, PX_RIGHT);
    }
    (void)sub;
    add_hit(c, 18, y, 588, 54, HIT_ROW, i);
}

static void help(PxCanvas *c, const Model *m)
{
    char buf[320];
    const char *s;
    if (m->cursor == ROW_DISCOVER)
        s = m->discover
            ? T("Auto discovery is on. Entering multiplayer tries to pair with someone else.")
            : T("Auto discovery is off. The game plays on its own, unless you change it in BananaDS LAN's settings.");
    else if (m->cursor == ROW_DLPLAY)
        s = view_firmware_ok()
            ? T("Opens the DS menu with your BIOS and firmware. Choose DS Download Play there to join a game another console hosts with one card.")
            : T("Needs your DS BIOS and firmware in the bios folder at the root of the card (/bios): bios7.bin, bios9.bin and firmware.bin.");
    else if (m->cursor == ROW_ABOUT)
        s = T("Local wireless multiplayer for DS games on BananaOS: two consoles on the same Wi-Fi play together, as on a real DS.");
    else {
        upd_help(&m->upd, "Banana Party", buf, sizeof buf);
        s = buf;
    }
    px_text_wrap(c, PX_REGULAR, 18, 30, 84 + ROW_COUNT * ROW_PITCH + 4, s, px.muted, 574, 3, 24);
}

/* a question over the list: a card in the middle, its two answers on the face row */
static void dialog(PxCanvas *c, const char *title, const char *sub, const char *mono)
{
    px_rect_a(c, 0, 0, 640, 480, 0x000000, px.light ? 70 : 150);
    px_card(c, 50, 120, 540, 200, 0, 0);
    px_text_wrap(c, PX_MEDIUM, 26, 76, 146, title, px.ink, 488, 2, 32);
    if (mono) px_text_fit(c, PX_REGULAR, 18, 76, 222, mono, px.muted, 0, 488, PX_LEFT);
    if (sub) px_text_wrap(c, PX_REGULAR, 18, 76, mono ? 252 : 222, sub, px.muted, 488, 3, 25);
}

static float dialog_exit_app(PxCanvas *c)
{
    float y;
    px_rect_a(c, 0, 0, 640, 480, 0x000000, px.light ? 70 : 150);
    px_card(c, 40, 88, 560, 272, 0, 0);
    px_text_wrap(c, PX_MEDIUM, 24, 64, 108, T("Leave the app?"), px.ink, 512, 2, 30);
    y = 158;
    y += 24 * px_text_wrap(c, PX_REGULAR, 17, 64, y, T("The room will close."), px.muted, 512, 2, 24);
    return y;
}

/* SELECT: who made it, where its source is, what it is built on */
static void view_about(PxCanvas *c, const Model *m)
{
    char buf[96];
    PxRect r;
    float y;
    g_nhits = 0;
    px_screen(c);
    px_bubble(c, 18, 14, 604, 54, 0);
    px_text_fit(c, PX_MEDIUM, 26, 38, 25, T("About"), px.ink, 0, 440, PX_LEFT);
    px_card(c, 18, 82, 604, 326, 0, 0);
    px_text(c, PX_MEDIUM, 28, 40, 98, "Banana Party", px.ink, 0);
    snprintf(buf, sizeof buf, "v%s", m->version[0] ? m->version : "?");
    px_text_fit(c, PX_MEDIUM, 20, 600, 104, buf, px.muted, 0, 160, PX_RIGHT);
    y = 138;
    y += 25 * px_text_wrap(c, PX_REGULAR, 18, 40, y,
                           T("Local wireless multiplayer for DS games on BananaOS: two consoles on the same Wi-Fi play together, as on a real DS."),
                           px.ink, 560, 3, 25) + 10;
    px_label(c, 40, y + 4, T("Made by"), px.muted);
    px_text(c, PX_MEDIUM, 18, 200, y, "Pixelado Dev", px.ink, 0);
    y += 26;
    px_label(c, 40, y + 4, T("Source code"), px.muted);
    px_text_fit(c, PX_MEDIUM, 18, 200, y, "github.com/pixeladodev/banana-party", px.ink, 0, 400, PX_LEFT);
    y += 32;
    px_label(c, 40, y, T("Credits"), px.muted);
    y += 20;
    y += 23 * px_text_wrap(c, PX_REGULAR, 16, 40, y,
                           T("BananaOS and BananaLinux by JorreFog and the Banana contributors (MIT). BananaDS (SuperDrastic) and melonDS (GPLv3). Pixelify Sans (OFL)."),
                           px.muted, 560, 3, 23) + 6;
    px_text_wrap(c, PX_REGULAR, 16, 40, y,
                 T("An independent app, not part of BananaOS. Built and tested on the RG DS and the RG DS Plus. Free software under the GPLv3."),
                 px.muted, 560, 3, 23);
    px_face(c, 18, 422, PX_LEFT, "B", T("Back"), 0, &r, PX_FACE_NEUTRAL);
    add_face_hit(r, HIT_BACK);
}

void view_bottom(PxCanvas *c, const Model *m)
{
    char count[16];
    PxRect r;
    int i;
    if (m->view == V_LOBBY) { lobby_bottom(c, m); return; }
    if (m->view == V_LOBBY_EXIT) {
        g_nhits = 0;
        px_screen(c);
        dialog_exit_app(c);
        px_face(c, 18, 422, PX_LEFT, "B", T("Stay"), 0, &r, PX_FACE_NEUTRAL);
        add_face_hit(r, HIT_BACK);
        px_face(c, 622, 422, PX_RIGHT, "A", T("Exit app"), 0, &r, PX_FACE_DANGER);
        add_face_hit(r, HIT_OK);
        return;
    }
    if (m->view == V_LOBBY_DISCONNECT) {
        g_nhits = 0;
        px_screen(c);
        dialog(c, T("Leave the lobby?"),
               m->lobby.role == 0 ? T("This ends your lobby session for everyone.")
                                  : T("You leave this room. The host's room stays open."),
               NULL);
        px_face(c, 18, 422, PX_LEFT, "B", T("Stay"), 0, &r, PX_FACE_NEUTRAL);
        add_face_hit(r, HIT_BACK);
        px_face(c, 622, 422, PX_RIGHT, "A", T("Leave room"), 0, &r, PX_FACE_DANGER);
        add_face_hit(r, HIT_OK);
        return;
    }
    if (m->view == V_GAMES) { games_bottom(c, m); return; }
    if (m->view == V_FW_CONFIRM) {
        g_nhits = 0;
        px_screen(c);
        dialog(c, T("Open the DS menu?"),
               T("This starts your console's DS firmware. In the DS menu, choose DS Download Play to join the game another console is hosting."),
               NULL);
        px_face(c, 18, 422, PX_LEFT, "B", T("Cancel"), 0, &r, PX_FACE_NEUTRAL);
        add_face_hit(r, HIT_BACK);
        if (view_firmware_copy()) {  /* the DS menu's settings kept on this console: X puts the user's own back */
            px_face(c, 18 + px_face_w(c, "B", T("Cancel")) + 8, 422, PX_LEFT, "X", T("Reset settings"), 0, &r,
                    PX_FACE_NEUTRAL);
            add_face_hit(r, HIT_LOBBY);
        }
        px_face(c, 622, 422, PX_RIGHT, "A", T("Open"), 0, &r, PX_FACE_PRIMARY);
        add_face_hit(r, HIT_OK);
        return;
    }
    if (m->view == V_FW_RESET) {
        g_nhits = 0;
        px_screen(c);
        dialog(c, T("Reset the DS settings?"),
               T("Your name, language and the other DS menu settings go back to those in your firmware.bin in /bios."),
               NULL);
        px_face(c, 18, 422, PX_LEFT, "B", T("Cancel"), 0, &r, PX_FACE_NEUTRAL);
        add_face_hit(r, HIT_BACK);
        px_face(c, 622, 422, PX_RIGHT, "A", T("Reset"), 0, &r, PX_FACE_DANGER);
        add_face_hit(r, HIT_OK);
        return;
    }
    if (m->view == V_BIOS_NOTICE || m->view == V_FW_MISSING) {
        /* where the DS's BIOS and firmware go: on the first launch without them (once, main.c's
           BIOS_NOTICE_FILE), and when DS Download Play is asked for without them */
        const int notice = m->view == V_BIOS_NOTICE;
        char miss[64], line[96];
        float y;
        g_nhits = 0;
        px_screen(c);
        view_firmware_missing(miss, (int)sizeof miss);
        snprintf(line, sizeof line, T("Missing: %s"), miss);
        px_rect_a(c, 0, 0, 640, 480, 0x000000, px.light ? 70 : 150);
        px_card(c, 34, 70, 572, 290, 0, 0);
        px_text_wrap(c, PX_MEDIUM, 24, 58, 92, notice ? T("Optional: DS Download Play") : T("BIOS files missing"),
                     px.ink, 524, 1, 30);
        y = 140;
        y += 24 * px_text_wrap(c, PX_REGULAR, 17, 58, y,
                               notice ? T("To join a game with DS Download Play, copy your own DS files to the bios folder at the root of the card (/bios, next to banana, bananads and roms): bios7.bin (16 KB), bios9.bin (4 KB) and firmware.bin (256 KB). If they're already on the SD card, copy them there with the BananaOS file explorer. Everything else works without them.")
                                      : T("Copy your own DS BIOS and firmware to the bios folder at the root of the card (/bios): bios7.bin, bios9.bin and firmware.bin."),
                               px.muted, 524, 8, 24);
        px_text_fit(c, PX_REGULAR, 17, 58, y + 10, line, px.ink, 0, 524, PX_LEFT);
        px_face(c, 622, 422, PX_RIGHT, "A", T("OK"), 0, &r, notice ? PX_FACE_PRIMARY : PX_FACE_NEUTRAL);
        add_face_hit(r, HIT_OK);
        return;
    }
    g_nhits = 0;
    px_screen(c);
    px_bubble(c, 18, 14, 604, 54, 0);
    px_text_fit(c, PX_MEDIUM, 26, 38, 25, T("Settings"), px.ink, 0, 440, PX_LEFT);
    snprintf(count, sizeof count, "%02d/%02d", m->cursor + 1, ROW_COUNT);
    px_text(c, PX_MEDIUM, 19, 602 - px_text_w(c, PX_MEDIUM, 19, count, 0), 30, count, px.muted, 0);
    for (i = 0; i < ROW_COUNT; i++) row(c, m, i, 84 + i * ROW_PITCH);
    px_rail(c, 615, 84, ROW_COUNT * ROW_PITCH - 12, m->cursor, 1, ROW_COUNT, NULL, NULL);
    help(c, m);

    if (m->view == V_MAIN) {
        PxFace f[4];
        PxRect hr[4];
        int inst = upd_installing(&m->upd);
        f[0].key = "B"; f[0].label = T("Back"); f[0].dis = 0;
        f[1].key = "Y"; f[1].label = T("Uninstall"); f[1].dis = inst;
        f[2].key = "A"; f[2].label = m->cursor == ROW_DISCOVER ? T("Change") : m->cursor == ROW_UPDATES ? upd_action(&m->upd) : T("Open");
        f[2].dis = inst || (m->cursor == ROW_UPDATES && upd_running(&m->upd));
        px_faces(c, 422, 3, 2, f, hr);
        add_face_hit(hr[0], HIT_BACK);
        add_face_hit(hr[1], HIT_Y);
        add_face_hit(hr[2], HIT_OK);
        return;
    }
    if (m->view == V_ABOUT) {
        view_about(c, m);
        return;
    }
    g_nhits = 0;                     /* the list under the question takes no touch */
    if (m->view == V_UPDATE || m->view == V_UPDATED) {
        char title[96], sub[320];
        int done = m->view == V_UPDATED;
        snprintf(title, sizeof title, done ? T("Updated to v%s") : T("Update to v%s?"),
                 done ? m->upd.current : m->upd.latest);
        snprintf(sub, sizeof sub, "%s%s%s",
                 done ? T("A opens the new version.")
                      : T("It downloads from GitHub and checks it before anything changes. Your settings stay."),
                 done && m->upd.msg[0] ? " " : "", done ? m->upd.msg : "");
        dialog(c, title, sub, NULL);
        if (!done) {
            px_face(c, 18, 422, PX_LEFT, "B", T("Cancel"), 0, &r, PX_FACE_NEUTRAL);
            add_face_hit(r, HIT_BACK);
        }
        px_face(c, 622, 422, PX_RIGHT, "A", done ? T("Restart") : T("Update"), 0, &r, PX_FACE_NEUTRAL);
        add_face_hit(r, HIT_OK);
    } else if (m->view == V_UNINSTALL) {
        dialog(c, T("Uninstall Banana Party?"), m->err[0] ? m->err : T("Turns it off and deletes this app."), NULL);
        px_face(c, 18, 422, PX_LEFT, "B", T("Cancel"), 0, &r, PX_FACE_NEUTRAL);
        add_face_hit(r, HIT_BACK);
        px_face(c, 622, 422, PX_RIGHT, "A", T("Uninstall"), 0, &r, PX_FACE_NEUTRAL);
        add_face_hit(r, HIT_OK);
    }
}
