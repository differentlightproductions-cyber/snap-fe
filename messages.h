#ifndef SNAP_FE_MESSAGES_H
#define SNAP_FE_MESSAGES_H
/* SNAP Chat -- local messaging between nearby SNAP FE devices.
 *
 * No accounts, no server, nothing leaves the Wi-Fi you are already on: every
 * device broadcasts a small beacon carrying its nickname, and anyone who
 * answers appears in the list. Conversations are either a direct message to one
 * device or one of a few fixed rooms everybody can talk in.
 *
 * A message is one of three things -- typed text, a drawn picture, or a single
 * retro emoji. The picture is the reason several sizes here look arbitrary:
 * the canvas is 96x48 at two bits a pixel, which is 1152 bytes, so a whole
 * drawing fits inside one datagram well under the 1500-byte MTU. Nothing has to
 * be split, reassembled, or retried piece by piece, and four states (blank plus
 * three inks) is what a D-pad and one button can realistically produce anyway.
 *
 * Drawing colours are fixed rather than themed on purpose. A picture has to
 * look the same on the device that receives it as on the device that drew it,
 * and the receiver's theme is not the sender's.
 *
 * The device identity comes from friends.h -- one device, one id, one file --
 * so this app is included after it. Nothing here writes back into that module.
 */

#define SM_PORT     38433
#define SM_MAGIC    0x534d5347u        /* "SMSG" */
#define SM_VERSION  1
enum { SM_BEACON = 1, SM_SAY, SM_ACK, SM_READ };

#define SM_PEERS      24               /* nearby devices plus saved contacts */
#define SM_ROOMS       4
#define SM_NAME_MAX   24
#define SM_TEXT_MAX  120
#define SM_LOG_MAX   140
#define SM_INK_SLOTS  24               /* how many drawings we keep, oldest out */
#define SM_SEEN_TOKENS 6               /* replay window per peer */
/* A room is a live channel, not an archive. What is said in one is dropped a
   day later; a conversation with a person is kept for good. */
#define SM_ROOM_KEEP_SECS (24 * 60 * 60)
#define SM_SAVE_SETTLE_MS 2500         /* write the log this long after it changes */
#define SM_EXPIRE_EVERY_MS 60000

#define SM_CANVAS_W 96
#define SM_CANVAS_H 48
#define SM_CANVAS_BYTES ((SM_CANVAS_W * SM_CANVAS_H) / 4)
#define SM_CANVAS_SCALE 6
#define SM_INKS 3

enum { SM_KIND_TEXT = 0, SM_KIND_EMOJI, SM_KIND_DRAW };
enum { SM_VIEW_LIST = 0, SM_VIEW_CHAT, SM_VIEW_EMOJI, SM_VIEW_DRAW, SM_VIEW_PHOTO };

typedef struct {
    uint32_t magic, token;
    uint8_t  type, version, kind, room;
    int16_t  emoji;
    char     from[33], to[33], name[SM_NAME_MAX + 1];
    char     text[SM_TEXT_MAX + 1];
    uint8_t  ink[SM_CANVAS_BYTES];
} SmPacket;

typedef struct {
    char id[33], name[SM_NAME_MAX + 1];
    struct sockaddr_in addr;
    Uint32 seen;
    uint32_t recent[SM_SEEN_TOKENS];
    int recent_i;
} SmPeer;

/* One line of a conversation. `key` is the peer id for a direct message, or
   "#0".."#3" for a room, so both kinds live in one log and one file. */
typedef struct {
    char    key[34], name[SM_NAME_MAX + 1];
    uint8_t mine, kind, seen;
    uint8_t read;                      /* one of mine the other device has opened */
    int16_t emoji;
    int     ink;                       /* slot index, -1 when not a drawing */
    long    when;
    char    text[SM_TEXT_MAX + 1];
} SmLog;

static const char *sm_room_names[SM_ROOMS] = { "LOBBY", "GAMES", "TRADING", "QUIET" };

static struct {
    int fd, initialized, loaded;
    int n; SmPeer peers[SM_PEERS];
    int log_n; SmLog log[SM_LOG_MAX];
    unsigned char ink[SM_INK_SLOTS][SM_CANVAS_BYTES];
    int ink_next;

    int view, tab, sel, sel_tab[2], scroll, emoji_sel;
    int pick;                          /* browsing history: conversation index, -1 = composing */
    int photo;                         /* ink slot open in the viewer, -1 = none */
    char key[34], title[SM_NAME_MAX + 1];   /* the open conversation */

    unsigned char canvas[SM_CANVAS_BYTES];
    int cx, cy, brush, color, marked;
    float fx, fy;
    int px, py, painting;

    SmPacket out; struct sockaddr_in out_addr;
    int out_pending, out_row;
    Uint32 out_tx, out_deadline;

    uint32_t serial;
    Uint32 beacon, last, dirty_at, expire_at;
    int dirty;
    char status[96];
} sm = { .fd = -1, .brush = 1, .color = 1, .pick = -1, .photo = -1 };

static AppState sm_return = STATE_HOME;

/* ---------------------------------------------------------------- canvas -- */
/* Two bits a pixel, four pixels to a byte, low bits first. */
static int sm_px_get(const unsigned char *ink, int x, int y) {
    if (x < 0 || y < 0 || x >= SM_CANVAS_W || y >= SM_CANVAS_H) return 0;
    int i = y * SM_CANVAS_W + x;
    return (ink[i >> 2] >> ((i & 3) * 2)) & 3;
}
static void sm_px_set(unsigned char *ink, int x, int y, int v) {
    if (x < 0 || y < 0 || x >= SM_CANVAS_W || y >= SM_CANVAS_H) return;
    int i = y * SM_CANVAS_W + x, shift = (i & 3) * 2;
    ink[i >> 2] = (unsigned char)((ink[i >> 2] & ~(3 << shift)) | ((v & 3) << shift));
}
static void sm_dab(unsigned char *ink, int x, int y, int v, int brush) {
    int r = mgx_clampi(brush, 1, 3) - 1;
    for (int dy = -r; dy <= r; dy++)
        for (int dx = -r; dx <= r; dx++) {
            if (r == 2 && abs(dx) == 2 && abs(dy) == 2) continue;   /* round the corners off */
            sm_px_set(ink, x + dx, y + dy, v);
        }
}
/* Joining the last point to this one matters: at any usable cursor speed the
   per-frame steps are several pixels apart, and dabs alone draw a dotted line. */
static void sm_stroke(unsigned char *ink, int x0, int y0, int x1, int y1, int v, int brush) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1, err = dx + dy;
    for (int guard = 0; guard < SM_CANVAS_W * SM_CANVAS_H; guard++) {
        sm_dab(ink, x0, y0, v, brush);
        if (x0 == x1 && y0 == y1) return;
        int e2 = err * 2;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}
static int sm_canvas_blank(const unsigned char *ink) {
    for (int i = 0; i < SM_CANVAS_BYTES; i++) if (ink[i]) return 0;
    return 1;
}
/* Paper and ink are deliberately fixed, not themed -- see the file header. */
static SDL_Color sm_paper = { 236, 238, 228, 255 };
static SDL_Color sm_ink_color[1 + SM_INKS] = {
    { 236, 238, 228, 255 },            /* 0 is the paper itself: "erased" */
    {  40,  42,  52, 255 },
    { 214,  72,  90, 255 },
    {  58, 124, 214, 255 }
};

/* A drawing as an ordinary image. The canvas is tiny, so this is a plain
   nearest-neighbour blow-up: every canvas pixel becomes a hard square, which
   is what the drawing actually is and what it should still look like once it
   is a PNG sitting in a folder. */
static SDL_Surface *sm_drawing_surface(const unsigned char *ink, int scale) {
    SDL_Surface *s = SDL_CreateRGBSurfaceWithFormat(0, SM_CANVAS_W * scale, SM_CANVAS_H * scale,
                                                    32, SDL_PIXELFORMAT_ARGB8888);
    if (!s) return NULL;
    SDL_FillRect(s, NULL, SDL_MapRGBA(s->format, sm_paper.r, sm_paper.g, sm_paper.b, 255));
    for (int y = 0; y < SM_CANVAS_H; y++)
        for (int x = 0; x < SM_CANVAS_W; x++) {
            int v = sm_px_get(ink, x, y);
            if (!v) continue;
            SDL_Color c = sm_ink_color[v];
            SDL_FillRect(s, &(SDL_Rect){ x * scale, y * scale, scale, scale },
                         SDL_MapRGBA(s->format, c.r, c.g, c.b, 255));
        }
    return s;
}
#define SM_SAVE_SCALE 8                /* 96x48 becomes 768x384 on disk */
/* Writes the drawing into `dir` under a dated name, and reports the full path
   it used. Two drawings saved in the same second get a suffix rather than one
   quietly replacing the other. */
static int sm_write_drawing(const unsigned char *ink, const char *dir, char *out, size_t n) {
    if (!ink || !dir || !dir[0]) return 0;
    time_t now = time(NULL);
    struct tm parts;
    char stamp[32] = "drawing";
    if (localtime_r(&now, &parts)) strftime(stamp, sizeof stamp, "%Y%m%d-%H%M%S", &parts);
    for (int attempt = 1; attempt <= 9; attempt++) {
        if (attempt == 1) snprintf(out, n, "%s/drawing-%s.png", dir, stamp);
        else snprintf(out, n, "%s/drawing-%s-%d.png", dir, stamp, attempt);
        FILE *taken = fopen(out, "rb");
        if (taken) { fclose(taken); continue; }
        SDL_Surface *s = sm_drawing_surface(ink, SM_SAVE_SCALE);
        if (!s) return 0;
        int ok = IMG_SavePNG(s, out) == 0;
        SDL_FreeSurface(s);
        return ok;
    }
    return 0;
}

/* ----------------------------------------------------------------- emoji -- */
/* Eight-by-eight pixel art kept as text so the shapes are editable in place.
   '.' leaves the paper alone, 'X' is the main colour, 'o' the shade. */
typedef struct { const char *rows[8]; SDL_Color main, shade; const char *name; } SmEmojiArt;
#define SM_EMOJI_COUNT 12
static const SmEmojiArt sm_emoji[SM_EMOJI_COUNT] = {
 {{"..XXXX..",".XXXXXX.","XXoXXoXX","XXXXXXXX","XXXXXXXX","XoXXXXoX",".XooooX.","..XXXX.."},
  {250,205, 70,255},{ 60, 46, 20,255},"Smile"},
 {{"..XXXX..",".XXXXXX.","XXoXXXXX","XXXXooXX","XXXXXXXX","XoXXXXoX",".XooooX.","..XXXX.."},
  {250,205, 70,255},{ 60, 46, 20,255},"Wink"},
 {{"..XXXX..",".XXXXXX.","XXoXXoXX","XXXXXXXX","XXXXXXXX",".XooooX.","XoXXXXoX","..XXXX.."},
  {250,205, 70,255},{ 60, 46, 20,255},"Sad"},
 {{"..XXXX..",".XXXXXX.","XooooooX","XoXooXoX","XXoXXoXX","XXXXXXXX",".XooooX.","..XXXX.."},
  {250,205, 70,255},{ 34, 34, 44,255},"Cool"},
 {{".XX..XX.","XXXXXXXX","XXXXXXXX","XXXXXXXX",".XXXXXX.","..XXXX..","...XX...","........"},
  {228, 74, 96,255},{255,255,255,255},"Heart"},
 {{"...XX...","...XX...","XXXXXXXX",".XXXXXX.","..XXXX..",".XXXXXX.",".XX..XX.","........"},
  {248,196, 62,255},{255,255,255,255},"Star"},
 {{"...XXXX.","...XXXX.","...XX.X.","...XX...","...XX...",".XXXX...","XXXXX...",".XXX...."},
  {120,168,255,255},{255,255,255,255},"Note"},
 {{".XXXXXX.","XXXXXXXX","XooXXooX","XooXXooX","XXXXXXXX","XXoXoXXX",".XXXXXX.","..X.X..."},
  {158,166,184,255},{ 34, 34, 44,255},"Skull"},
 {{"X......X","XX....XX","XXXXXXXX","XoXXXXoX","XXXXXXXX","XXooooXX","XXXXXXXX",".XXXXXX."},
  {224,150, 88,255},{ 54, 38, 26,255},"Cat"},
 {{"...XX...","..XXXX..","..XXXX..","XXXXXX..","XXXXXXX.","XXXXXXX.","XXXXXXX.",".XXXXXX."},
  {238,192,142,255},{110, 74, 46,255},"Nice"},
 {{"X..XX..X",".X.XX.X.","..XXXX..","XXXXXXXX","XXXXXXXX","..XXXX..",".X.XX.X.","X..XX..X"},
  {252,196, 66,255},{255,255,255,255},"Sun"},
 {{"...X....","X..X..X.",".X.X.X..","..XXX...","XXXXXXX.","..XXX...",".X.X.X..","X..X..X."},
  { 92,198,242,255},{255,255,255,255},"Spark"}
};

static void sm_emoji_draw(SDL_Renderer *ren, int idx, int x, int y, int scale) {
    if (idx < 0 || idx >= SM_EMOJI_COUNT) return;
    const SmEmojiArt *e = &sm_emoji[idx];
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
    for (int r = 0; r < 8; r++)
        for (int c = 0; c < 8; c++) {
            char ch = e->rows[r][c];
            if (ch == '.') continue;
            SDL_Color col = ch == 'X' ? e->main : e->shade;
            SDL_SetRenderDrawColor(ren, col.r, col.g, col.b, 255);
            SDL_RenderFillRect(ren, &(SDL_Rect){ x + c * scale, y + r * scale, scale, scale });
        }
}

/* ------------------------------------------------------------------- log -- */
static void sm_path(char *out, size_t n, const char *name) {
    snprintf(out, n, "%s/%s", sn_data_root(), name);
}
static void sm_clean(char *s, size_t n) {
    s[n - 1] = 0;
    for (size_t i = 0; s[i]; i++) if ((unsigned char)s[i] < 32 || s[i] == 127) s[i] = ' ';
}
/* One place sets the app's status line, including main.c's wallpaper step,
   which finishes a job this app started and should report back into it. */
static void sm_note(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(sm.status, sizeof sm.status, fmt, ap);
    va_end(ap);
}
static const char *sm_me_name(void) {
    if (player_name[0]) return player_name;
    if (link_my_name[0]) return link_my_name;
    return "SNAP Player";
}
static int sm_room_key(const char *key) {
    if (key[0] != '#' || !isdigit((unsigned char)key[1])) return -1;
    int r = key[1] - '0';
    return r >= 0 && r < SM_ROOMS ? r : -1;
}
/* The on-screen keyboard is shared by the whole frontend and titles itself
   from what it was opened for. Without this it falls through to its default,
   which is "API KEY" -- a strange thing to read while writing to a friend. */
static const char *sm_compose_title(void) {
    static char title[72];
    int room = sm_room_key(sm.key);
    if (room >= 0) snprintf(title, sizeof title, "SAY SOMETHING IN #%s", sm_room_names[room]);
    else snprintf(title, sizeof title, "MESSAGE TO %.24s", sm.title[0] ? sm.title : "FRIEND");
    return title;
}
static int sm_find_peer(const char *id) {
    for (int i = 0; i < sm.n; i++) if (!strcmp(sm.peers[i].id, id)) return i;
    return -1;
}
static int sm_peer_online(int i) {
    return i >= 0 && i < sm.n && sm.peers[i].seen && SDL_GetTicks() - sm.peers[i].seen < 6500;
}

/* The log changed. Note when, so sm_tick can write it out once things settle
   rather than waiting for the user to leave the app -- which is how messages
   that arrived in the background used to get lost on a power-off. */
static void sm_touch(void) {
    sm.dirty = 1;
    sm.dirty_at = SDL_GetTicks();
}

/* Taking an ink slot retires whatever used to be in it. The line stays in the
   conversation -- it just stops claiming to have a picture attached. */
static int sm_ink_claim(const unsigned char *src) {
    int slot = sm.ink_next;
    sm.ink_next = (sm.ink_next + 1) % SM_INK_SLOTS;
    for (int i = 0; i < sm.log_n; i++)
        if (sm.log[i].kind == SM_KIND_DRAW && sm.log[i].ink == slot) {
            sm.log[i].kind = SM_KIND_TEXT; sm.log[i].ink = -1;
            snprintf(sm.log[i].text, sizeof sm.log[i].text, "(older drawing)");
        }
    memcpy(sm.ink[slot], src, SM_CANVAS_BYTES);
    return slot;
}

static SmLog *sm_log_push(const char *key, const char *name, int mine, int kind,
                          int emoji, const char *text, const unsigned char *ink, long when) {
    if (sm.log_n == SM_LOG_MAX) {
        memmove(sm.log, sm.log + 1, sizeof(SmLog) * (SM_LOG_MAX - 1));
        sm.log_n--;
        if (sm.out_row >= 0) sm.out_row--;
    }
    /* Claim the picture slot before the new row exists. The claim retires every
       row still pointing at that slot, and a half-built row whose ink field is
       still zero looks exactly like one pointing at slot 0 -- so a drawing that
       landed there would retire itself on the way in. */
    int slot = ink ? sm_ink_claim(ink) : -1;
    SmLog *e = &sm.log[sm.log_n++];
    memset(e, 0, sizeof *e);
    snprintf(e->key, sizeof e->key, "%.33s", key);
    snprintf(e->name, sizeof e->name, "%.*s", SM_NAME_MAX, name ? name : "");
    e->mine = (unsigned char)!!mine; e->kind = (unsigned char)kind; e->emoji = (short)emoji;
    e->seen = (unsigned char)(mine ? 1 : 0);
    e->when = when ? when : (long)time(NULL);
    e->ink = slot;
    if (text) snprintf(e->text, sizeof e->text, "%.*s", SM_TEXT_MAX, text);
    sm_touch();
    return e;
}

static int sm_unread_for(const char *key) {
    int n = 0;
    for (int i = 0; i < sm.log_n; i++)
        if (!sm.log[i].mine && !sm.log[i].seen && !strcmp(sm.log[i].key, key)) n++;
    return n;
}
static int sm_unread_total(void) {
    int n = 0;
    for (int i = 0; i < sm.log_n; i++) if (!sm.log[i].mine && !sm.log[i].seen) n++;
    return n;
}

/* Drop room lines older than a day. Direct messages are never touched here:
   a conversation with a person is the thing this app is for keeping. */
static void sm_expire_rooms(void) {
    long cutoff = (long)time(NULL) - SM_ROOM_KEEP_SECS;
    int out = 0;
    for (int i = 0; i < sm.log_n; i++) {
        if (sm_room_key(sm.log[i].key) >= 0 && sm.log[i].when < cutoff) continue;
        if (out != i) sm.log[out] = sm.log[i];
        out++;
    }
    if (out == sm.log_n) return;
    sm.log_n = out;
    /* Rows moved, so anything holding a row index now points at the wrong
       line. Neither is worth reconstructing. */
    sm.out_row = -1;
    if (sm.pick >= sm.log_n) sm.pick = -1;
    sm_touch();
}

/* People you have talked to stay in the list whether or not their device is
   switched on -- a conversation is a thing you keep, the way a phone keeps it.
   Only people you have actually exchanged something with are kept; a device
   that beaconed past once is not a contact. */
static int sm_peer_has_history(int i) {
    if (i < 0 || i >= sm.n) return 0;
    for (int k = 0; k < sm.log_n; k++)
        if (!strcmp(sm.log[k].key, sm.peers[i].id)) return 1;
    return 0;
}
static void sm_people_save(void) {
    char path[768], tmp[790];
    sm_path(path, sizeof path, "messages-people.cfg");
    snprintf(tmp, sizeof tmp, "%s.tmp", path);
    FILE *f = fopen(tmp, "w");
    if (!f) return;
    fprintf(f, "SNAPPEOPLE 1\n");
    for (int i = 0; i < sm.n; i++)
        if (sm_peer_has_history(i))
            fprintf(f, "%s\t%s\n", sm.peers[i].id, sm.peers[i].name);
    int ok = !ferror(f);
    if (fclose(f) != 0) ok = 0;
    if (!ok) { remove(tmp); return; }
    if (rename(tmp, path) != 0) { remove(path); if (rename(tmp, path) != 0) remove(tmp); }
}
static void sm_people_load(void) {
    char path[768], line[220];
    sm_path(path, sizeof path, "messages-people.cfg");
    FILE *f = fopen(path, "r");
    if (!f) return;
    if (!fgets(line, sizeof line, f) || strncmp(line, "SNAPPEOPLE", 10)) { fclose(f); return; }
    while (sm.n < SM_PEERS && fgets(line, sizeof line, f)) {
        line[strcspn(line, "\r\n")] = 0;
        char *tab = strchr(line, '\t');
        if (!tab) continue;
        *tab++ = 0;
        if (!sf_valid_id(line) || !strcmp(line, sf.id) || sm_find_peer(line) >= 0) continue;
        sm_clean(tab, SM_NAME_MAX + 1);
        SmPeer *p = &sm.peers[sm.n++];
        memset(p, 0, sizeof *p);
        snprintf(p->id, sizeof p->id, "%s", line);
        snprintf(p->name, sizeof p->name, "%.*s", SM_NAME_MAX, tab);
        /* seen stays zero: known, but not here right now. */
    }
    fclose(f);
}

static void sm_save(void) {
    static char line[SM_CANVAS_BYTES * 2 + 400];
    char path[768], tmp[790];
    sm_path(path, sizeof path, "messages.cfg");
    snprintf(tmp, sizeof tmp, "%s.tmp", path);
    FILE *f = fopen(tmp, "w");
    if (!f) { sm.dirty_at = SDL_GetTicks(); return; }
    fprintf(f, "SNAPCHAT 2\n");
    long cutoff = (long)time(NULL) - SM_ROOM_KEEP_SECS;
    for (int i = 0; i < sm.log_n; i++) {
        SmLog *e = &sm.log[i];
        if (sm_room_key(e->key) >= 0 && e->when < cutoff) continue;
        int n = snprintf(line, sizeof line, "%s\t%d\t%d\t%d\t%ld\t%s\t%d\t",
                         e->key, e->mine, e->kind, e->emoji, e->when, e->name, e->read);
        if (e->kind == SM_KIND_DRAW && e->ink >= 0 && e->ink < SM_INK_SLOTS) {
            for (int b = 0; b < SM_CANVAS_BYTES && n + 3 < (int)sizeof line; b++)
                n += snprintf(line + n, sizeof line - n, "%02x", sm.ink[e->ink][b]);
        } else {
            snprintf(line + n, sizeof line - n, "%s", e->text);
        }
        fprintf(f, "%s\n", line);
    }
    int ok = !ferror(f);
    if (fclose(f) != 0) ok = 0;
    if (!ok) { remove(tmp); sm.dirty_at = SDL_GetTicks(); return; }
    /* rename() was not being checked, and dirty was cleared either way -- a
       failed commit threw the log away silently. Some SD-card filesystems also
       refuse to rename onto a name that already exists, so clear the way and
       try once more before giving up and keeping the log in memory. */
    if (rename(tmp, path) != 0) {
        remove(path);
        if (rename(tmp, path) != 0) {
            remove(tmp);
            sm.dirty_at = SDL_GetTicks();   /* retry later, not every frame */
            return;
        }
    }
    sm.dirty = 0;
    sm_people_save();
}

static int sm_hexval(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}
static void sm_load(void) {
    static char line[SM_CANVAS_BYTES * 2 + 400];
    char path[768];
    sm_path(path, sizeof path, "messages.cfg");
    FILE *f = fopen(path, "r");
    if (!f) return;
    if (!fgets(line, sizeof line, f) || strncmp(line, "SNAPCHAT", 8)) { fclose(f); return; }
    /* Version 2 added the read-receipt column. Version 1 logs are still read,
       so upgrading never costs anyone their conversations. */
    int version = atoi(line + 8) >= 2 ? 2 : 1;
    int fields = version >= 2 ? 8 : 7;
    long cutoff = (long)time(NULL) - SM_ROOM_KEEP_SECS;
    while (sm.log_n < SM_LOG_MAX && fgets(line, sizeof line, f)) {
        line[strcspn(line, "\r\n")] = 0;
        char *field[8], *p = line;
        int nf = 0;
        while (nf < fields) {
            field[nf++] = p;
            char *tab = strchr(p, '\t');
            if (!tab) break;
            *tab = 0; p = tab + 1;
        }
        if (nf < fields) continue;
        int read_flag = version >= 2 ? atoi(field[6]) != 0 : 0;
        char *payload = field[fields - 1];
        if (sm_room_key(field[0]) >= 0 && atol(field[4]) < cutoff) continue;
        int kind = atoi(field[2]);
        if (kind < SM_KIND_TEXT || kind > SM_KIND_DRAW) continue;
        unsigned char ink[SM_CANVAS_BYTES];
        const unsigned char *inkp = NULL;
        if (kind == SM_KIND_DRAW) {
            if (strlen(payload) < SM_CANVAS_BYTES * 2) continue;
            for (int b = 0; b < SM_CANVAS_BYTES; b++) {
                int hi = sm_hexval(payload[b * 2]), lo = sm_hexval(payload[b * 2 + 1]);
                if (hi < 0 || lo < 0) { hi = 0; lo = 0; }
                ink[b] = (unsigned char)(hi * 16 + lo);
            }
            inkp = ink;
        }
        sm_clean(field[5], SM_NAME_MAX + 1);
        if (kind != SM_KIND_DRAW) sm_clean(payload, SM_TEXT_MAX + 1);
        SmLog *e = sm_log_push(field[0], field[5], atoi(field[1]) != 0, kind,
                               atoi(field[3]), kind == SM_KIND_DRAW ? NULL : payload,
                               inkp, atol(field[4]));
        e->read = (unsigned char)read_flag;
        /* Anything already on disk has been looked at; a restart should not
           announce the whole history as new. */
        e->seen = 1;
    }
    fclose(f);
    sm.dirty = 0;
}

/* --------------------------------------------------------------- network -- */
static void sm_packet(SmPacket *p, int type, const char *to) {
    memset(p, 0, sizeof *p);
    p->magic = htonl(SM_MAGIC);
    p->version = SM_VERSION;
    p->type = (unsigned char)type;
    p->token = htonl(++sm.serial);
    p->emoji = -1;
    p->room = 0xff;
    snprintf(p->from, sizeof p->from, "%s", sf.id);
    snprintf(p->to, sizeof p->to, "%s", to ? to : "");
    snprintf(p->name, sizeof p->name, "%.*s", SM_NAME_MAX, sm_me_name());
}
static void sm_raw_send(const SmPacket *p, const struct sockaddr_in *to) {
    if (sm.fd >= 0 && to) sendto(sm.fd, p, sizeof *p, 0, (const struct sockaddr *)to, sizeof *to);
}
static void sm_broadcast(const SmPacket *p) {
    struct sockaddr_in a; memset(&a, 0, sizeof a);
    a.sin_family = AF_INET; a.sin_port = htons(SM_PORT);
    a.sin_addr.s_addr = htonl(INADDR_BROADCAST);
    sm_raw_send(p, &a);
}

static void sm_init(void) {
    if (sm.initialized) return;
    sm.initialized = 1;
    sf_init();                          /* one device identity, owned by friends.h */
    sm.out_row = -1;
    if (!sm.loaded) { sm.loaded = 1; sm_load(); sm_expire_rooms(); sm_people_load(); }
    sm.fd = mgx_net_socket(SM_PORT);
    sm.serial = (uint32_t)time(NULL) ^ ((uint32_t)getpid() << 8);
}

/* Sending: a room goes out as a broadcast twice and is not acknowledged, since
   there is no single recipient to acknowledge it. A direct message is repeated
   until the other device acknowledges that exact token, or gives up saying so
   rather than pretending it arrived. */
static void sm_send(int kind, int emoji, const char *text, const unsigned char *ink) {
    sm_init();
    if (!sm.key[0]) return;
    int room = sm_room_key(sm.key);
    SmPacket p;
    sm_packet(&p, SM_SAY, room >= 0 ? NULL : sm.key);
    p.kind = (unsigned char)kind;
    p.room = (unsigned char)(room >= 0 ? room : 0xff);
    p.emoji = (short)emoji;
    if (text) snprintf(p.text, sizeof p.text, "%.*s", SM_TEXT_MAX, text);
    if (ink) memcpy(p.ink, ink, SM_CANVAS_BYTES);

    sm_log_push(sm.key, sm_me_name(), 1, kind, emoji, text, ink, 0);
    sm.scroll = 0; sm.pick = -1;

    if (room >= 0) {
        sm_broadcast(&p); sm_broadcast(&p);
        snprintf(sm.status, sizeof sm.status, "Sent to %s", sm_room_names[room]);
        return;
    }
    int i = sm_find_peer(sm.key);
    if (i < 0 || !sm_peer_online(i)) {
        snprintf(sm.status, sizeof sm.status, "%.20s is not nearby right now", sm.title);
        sm.log[sm.log_n - 1].seen = 2;
        return;
    }
    sm.out = p; sm.out_addr = sm.peers[i].addr;
    sm.out_pending = 1; sm.out_row = sm.log_n - 1;
    sm.out_tx = 0; sm.out_deadline = SDL_GetTicks() + 6000;
    snprintf(sm.status, sizeof sm.status, "Sending...");
}

/* The keyboard commits from above this file, where the message kinds are
   not yet declared, so text sending gets its own name. */
static void sm_send_text(const char *text) { sm_send(SM_KIND_TEXT, -1, text, NULL); }

/* Read receipts, on for everyone, no setting. Opening a conversation with a
   person tells that person you opened it; there is nothing to send for a room,
   because a room has no one in particular to report back to. */
static void sm_send_read(const char *peer_id) {
    if (!peer_id || sm_room_key(peer_id) >= 0) return;
    int i = sm_find_peer(peer_id);
    if (i < 0 || !sm_peer_online(i)) return;
    SmPacket p;
    sm_packet(&p, SM_READ, peer_id);
    sm_raw_send(&p, &sm.peers[i].addr);
}
/* Everything already sent to that peer has now been seen by them. */
static int sm_mark_read(const char *peer_id) {
    int changed = 0;
    for (int i = 0; i < sm.log_n; i++)
        if (sm.log[i].mine && !sm.log[i].read && !strcmp(sm.log[i].key, peer_id)) {
            sm.log[i].read = 1; changed = 1;
        }
    if (changed) sm_touch();
    return changed;
}

static int sm_seen_token(SmPeer *peer, uint32_t token) {
    for (int i = 0; i < SM_SEEN_TOKENS; i++) if (peer->recent[i] == token) return 1;
    peer->recent[peer->recent_i] = token;
    peer->recent_i = (peer->recent_i + 1) % SM_SEEN_TOKENS;
    return 0;
}

static void sm_tick(void) {
    sm_init();
    if (sm.fd < 0) return;
    Uint32 now = SDL_GetTicks();

    if (now - sm.beacon >= 1500) {
        sm.beacon = now;
        SmPacket p; sm_packet(&p, SM_BEACON, NULL);
        sm_broadcast(&p);
    }

    for (int budget = 0; budget < 16; budget++) {
        SmPacket p; struct sockaddr_in a; socklen_t len = sizeof a;
        ssize_t n = recvfrom(sm.fd, &p, sizeof p, 0, (struct sockaddr *)&a, &len);
        if (n < 0) break;
        if (n != (ssize_t)sizeof p || ntohl(p.magic) != SM_MAGIC || p.version != SM_VERSION) continue;
        p.from[32] = 0; p.to[32] = 0;
        sm_clean(p.name, sizeof p.name);
        sm_clean(p.text, sizeof p.text);
        if (!sf_valid_id(p.from) || !strcmp(p.from, sf.id)) continue;

        int i = sm_find_peer(p.from);
        if (i < 0) {
            if (sm.n == SM_PEERS) continue;
            i = sm.n++;
            memset(&sm.peers[i], 0, sizeof sm.peers[i]);
            snprintf(sm.peers[i].id, sizeof sm.peers[i].id, "%s", p.from);
        }
        SmPeer *peer = &sm.peers[i];
        peer->addr = a; peer->seen = now;
        if (p.name[0]) snprintf(peer->name, sizeof peer->name, "%.*s", SM_NAME_MAX, p.name);

        if (p.type == SM_READ) {
            if (!strcmp(p.to, sf.id)) sm_mark_read(p.from);
            continue;
        }
        if (p.type == SM_ACK) {
            if (sm.out_pending && p.token == sm.out.token && !strcmp(p.from, sm.out.to)) {
                sm.out_pending = 0;
                snprintf(sm.status, sizeof sm.status, "Delivered");
            }
            continue;
        }
        if (p.type != SM_SAY || p.kind > SM_KIND_DRAW) continue;

        int room = p.room < SM_ROOMS ? p.room : -1;
        if (room < 0 && strcmp(p.to, sf.id)) continue;   /* someone else's direct message */

        /* Acknowledge before the replay check, so a resend caused by a lost
           acknowledgement is answered instead of silently dropped. */
        if (room < 0) {
            SmPacket ack; sm_packet(&ack, SM_ACK, p.from);
            ack.token = p.token;
            sm_raw_send(&ack, &a);
        }
        if (sm_seen_token(peer, p.token)) continue;

        char key[34];
        if (room >= 0) snprintf(key, sizeof key, "#%d", room);
        else snprintf(key, sizeof key, "%s", p.from);
        sm_log_push(key, peer->name[0] ? peer->name : "Someone", 0, p.kind,
                    p.kind == SM_KIND_EMOJI ? p.emoji : -1,
                    p.kind == SM_KIND_DRAW ? NULL : p.text,
                    p.kind == SM_KIND_DRAW ? p.ink : NULL, 0);
        /* Already reading that conversation? Then it is not unread. */
        if (sm.view == SM_VIEW_CHAT && !strcmp(sm.key, key)) {
            sm.log[sm.log_n - 1].seen = 1;
            sm.scroll = 0;
            if (room < 0) sm_send_read(p.from);
        }
    }

    if (sm.out_pending) {
        if (SDL_TICKS_PASSED(now, sm.out_deadline)) {
            sm.out_pending = 0;
            snprintf(sm.status, sizeof sm.status, "Not delivered - moved out of range?");
            if (sm.out_row >= 0 && sm.out_row < sm.log_n) sm.log[sm.out_row].seen = 2;
        } else if (!sm.out_tx || now - sm.out_tx >= 400) {
            sm.out_tx = now;
            sm_raw_send(&sm.out, &sm.out_addr);
        }
    }
    /* Anything that changed the log gets written once it settles. Waiting for
       the user to leave the app meant a message that arrived in the background
       was lost if the handheld was simply switched off. */
    if (sm.dirty && now - sm.dirty_at >= SM_SAVE_SETTLE_MS) sm_save();
    if (!sm.expire_at || SDL_TICKS_PASSED(now, sm.expire_at)) {
        sm.expire_at = now + SM_EXPIRE_EVERY_MS;
        sm_expire_rooms();
    }
    /* Peers that stopped beaconing drop off the list rather than lingering as
       permanently unreachable rows. */
    for (int i = 0; i < sm.n; i++)
        if (sm.peers[i].seen && now - sm.peers[i].seen > 45000 && !sm_peer_has_history(i)) {
            memmove(sm.peers + i, sm.peers + i + 1, sizeof(SmPeer) * (sm.n - i - 1));
            sm.n--; i--;
            if (sm.sel >= SM_ROOMS + sm.n) sm.sel = SM_ROOMS + sm.n - 1;
        }
}

/* ------------------------------------------------------------ navigation -- */
/* People and rooms are two lists, not one. They behave differently enough --
   one is a conversation that is kept, the other a channel that expires -- that
   mixing them in a single column made the screen read as one kind of thing. */
enum { SM_TAB_PEOPLE = 0, SM_TAB_ROOMS, SM_TAB_COUNT };
static const char *sm_tab_name[SM_TAB_COUNT] = { "FRIENDS", "ROOMS" };
static int sm_rows(void) { return sm.tab == SM_TAB_ROOMS ? SM_ROOMS : sm.n; }
static long sm_peer_last_at(int i) {
    long newest = 0;
    for (int k = 0; k < sm.log_n; k++)
        if (!strcmp(sm.log[k].key, sm.peers[i].id) && sm.log[k].when > newest)
            newest = sm.log[k].when;
    return newest;
}
/* Newest conversation first, the way a phone lists threads. People you have
   never messaged fall in behind those you have, nearby ones ahead of the rest.
   The order is derived rather than stored, so it cannot drift out of step with
   the log it describes. */
static int sm_people_order(int *out, int max) {
    int n = 0;
    for (int i = 0; i < sm.n && n < max; i++) out[n++] = i;
    for (int a = 1; a < n; a++) {
        int v = out[a];
        long va = sm_peer_last_at(v);
        int vo = sm_peer_online(v), b = a - 1;
        while (b >= 0) {
            long ba = sm_peer_last_at(out[b]);
            int bo = sm_peer_online(out[b]);
            if (!(va > ba || (va == ba && vo > bo))) break;
            out[b + 1] = out[b]; b--;
        }
        out[b + 1] = v;
    }
    return n;
}
static int sm_unread_rooms(void) {
    int n = 0;
    for (int i = 0; i < sm.log_n; i++)
        if (!sm.log[i].mine && !sm.log[i].seen && sm_room_key(sm.log[i].key) >= 0) n++;
    return n;
}
static int sm_unread_people(void) { return sm_unread_total() - sm_unread_rooms(); }
static void sm_tab_set(int tab) {
    tab = mgx_clampi(tab, 0, SM_TAB_COUNT - 1);
    if (tab == sm.tab) return;
    sm.sel_tab[sm.tab] = sm.sel;          /* each list keeps its own cursor */
    sm.tab = tab;
    sm.sel = sm.sel_tab[tab];
    int rows = sm_rows();
    if (sm.sel < 0 || sm.sel >= rows) sm.sel = 0;
}

static void sm_enter(AppState back) {
    sm_init();
    sm_return = back;
    sm.view = SM_VIEW_LIST;
    sm.scroll = 0;
    sm.status[0] = 0;
    int rows = sm_rows();
    if (sm.sel < 0 || sm.sel >= rows) sm.sel = 0;
}

static void sm_open_row(int row) {
    if (row < 0 || row >= sm_rows()) return;
    if (sm.tab == SM_TAB_ROOMS) {
        snprintf(sm.key, sizeof sm.key, "#%d", row);
        snprintf(sm.title, sizeof sm.title, "%s", sm_room_names[row]);
    } else {
        int order[SM_PEERS], n = sm_people_order(order, SM_PEERS);
        if (row >= n) return;
        SmPeer *p = &sm.peers[order[row]];
        snprintf(sm.key, sizeof sm.key, "%s", p->id);
        snprintf(sm.title, sizeof sm.title, "%.*s", SM_NAME_MAX, p->name[0] ? p->name : "Nearby device");
    }
    for (int i = 0; i < sm.log_n; i++)
        if (!strcmp(sm.log[i].key, sm.key) && !sm.log[i].seen) { sm.log[i].seen = 1; sm_touch(); }
    /* Opening it is what tells the other device you read it. */
    if (sm.tab != SM_TAB_ROOMS) sm_send_read(sm.key);
    sm.view = SM_VIEW_CHAT;
    sm.scroll = 0;
    sm.pick = -1;
    sm.status[0] = 0;
}

/* How many lines the open conversation has, and where its i-th one lives. */
static int sm_conv_count(void) {
    int n = 0;
    for (int i = 0; i < sm.log_n; i++) if (!strcmp(sm.log[i].key, sm.key)) n++;
    return n;
}
static SmLog *sm_conv_at(int index) {
    int n = 0;
    for (int i = 0; i < sm.log_n; i++)
        if (!strcmp(sm.log[i].key, sm.key) && n++ == index) return &sm.log[i];
    return NULL;
}

static const unsigned char *sm_photo_ink(void) {
    return sm.photo >= 0 && sm.photo < SM_INK_SLOTS ? sm.ink[sm.photo] : NULL;
}

static void sm_draw_open(void) {
    memset(sm.canvas, 0, sizeof sm.canvas);
    sm.cx = SM_CANVAS_W / 2; sm.cy = SM_CANVAS_H / 2;
    sm.fx = (float)sm.cx; sm.fy = (float)sm.cy;
    sm.px = sm.cx; sm.py = sm.cy; sm.painting = 0;
    sm.marked = 0;
    sm.view = SM_VIEW_DRAW;
    sm.last = SDL_GetTicks();
}

static void sm_leave(AppState *state) {
    if (sm.dirty) sm_save();
    *state = sm_return;
}

static void sm_key(SDL_Keycode k, AppState *state) {
    sm_init();
    switch (sm.view) {
    case SM_VIEW_LIST: {
        int rows = sm_rows();
        if (k == SDLK_ESCAPE) { sm_leave(state); play_click(); return; }
        if (k == SDLK_q || k == SDLK_LEFT)  { sm_tab_set(sm.tab - 1); play_click(); return; }
        if (k == SDLK_e || k == SDLK_RIGHT) { sm_tab_set(sm.tab + 1); play_click(); return; }
        if (k == SDLK_UP && rows)   { sm.sel = (sm.sel + rows - 1) % rows; play_click(); return; }
        if (k == SDLK_DOWN && rows) { sm.sel = (sm.sel + 1) % rows; play_click(); return; }
        if (k == SDLK_RETURN && rows) { sm_open_row(sm.sel); play_click(); return; }
        if (k == SDLK_f) {                     /* Y -- nickname */
            snprintf(kb_buffer, sizeof kb_buffer, "%s", player_name);
            kb_len = (int)strlen(kb_buffer); kb_cursor = kb_len; kb_row = 0; kb_col = 0;
            kb_purpose = KB_PURPOSE_PLAYER_NAME;
            kb_return_state = STATE_MESSAGES;
            *state = STATE_KEYBOARD; play_click();
            return;
        }
        return;
    }
    case SM_VIEW_CHAT: {
        int count = sm_conv_count();
        int top = count > 0 ? count - 1 : 0;
        if (sm.pick >= count) sm.pick = count - 1;
        /* Browsing the history. The highlighted line is kept at the bottom of
           the window, so stepping back never leaves the cursor off screen. */
        if (sm.pick >= 0) {
            if (k == SDLK_ESCAPE) { sm.pick = -1; sm.scroll = 0; play_click(); return; }
            if (k == SDLK_UP || k == SDLK_q) {
                if (sm.pick > 0) sm.pick--;
                sm.scroll = top - sm.pick; play_click(); return;
            }
            if (k == SDLK_DOWN || k == SDLK_e) {
                if (sm.pick >= top) { sm.pick = -1; sm.scroll = 0; }
                else { sm.pick++; sm.scroll = top - sm.pick; }
                play_click(); return;
            }
            if (k == SDLK_RETURN) {
                SmLog *e = sm_conv_at(sm.pick);
                if (e && e->kind == SM_KIND_DRAW && e->ink >= 0 && e->ink < SM_INK_SLOTS) {
                    sm.photo = e->ink; sm.view = SM_VIEW_PHOTO; sm.status[0] = 0;
                } else snprintf(sm.status, sizeof sm.status, "Only drawings open");
                play_click(); return;
            }
            return;                            /* writing waits until you are done reading */
        }
        if (k == SDLK_ESCAPE) { sm.view = SM_VIEW_LIST; sm.scroll = 0; sm.pick = -1; if (sm.dirty) sm_save(); play_click(); return; }
        if (k == SDLK_UP) {                    /* into the history */
            if (count) { sm.pick = top; sm.scroll = 0; }
            play_click(); return;
        }
        if (k == SDLK_q)                   { sm.scroll = mgx_clampi(sm.scroll + 1, 0, top); play_click(); return; }
        if (k == SDLK_DOWN || k == SDLK_e) { sm.scroll = mgx_clampi(sm.scroll - 1, 0, top); play_click(); return; }
        if (k == SDLK_RETURN) {                /* A -- type */
            kb_buffer[0] = 0; kb_len = 0; kb_cursor = 0; kb_row = 0; kb_col = 0;
            kb_purpose = KB_PURPOSE_MESSAGE;
            kb_return_state = STATE_MESSAGES;
            *state = STATE_KEYBOARD; play_click();
            return;
        }
        if (k == SDLK_s) { sm.view = SM_VIEW_EMOJI; play_click(); return; }   /* X */
        if (k == SDLK_f) { sm_draw_open(); play_click(); return; }            /* Y */
        return;
    }
    case SM_VIEW_EMOJI: {
        const int cols = 4;
        if (k == SDLK_ESCAPE) { sm.view = SM_VIEW_CHAT; play_click(); return; }
        if (k == SDLK_LEFT)  { sm.emoji_sel = (sm.emoji_sel + SM_EMOJI_COUNT - 1) % SM_EMOJI_COUNT; play_click(); return; }
        if (k == SDLK_RIGHT) { sm.emoji_sel = (sm.emoji_sel + 1) % SM_EMOJI_COUNT; play_click(); return; }
        if (k == SDLK_UP)    { sm.emoji_sel = (sm.emoji_sel + SM_EMOJI_COUNT - cols) % SM_EMOJI_COUNT; play_click(); return; }
        if (k == SDLK_DOWN)  { sm.emoji_sel = (sm.emoji_sel + cols) % SM_EMOJI_COUNT; play_click(); return; }
        if (k == SDLK_RETURN) { sm_send(SM_KIND_EMOJI, sm.emoji_sel, NULL, NULL); sm.view = SM_VIEW_CHAT; play_click(); return; }
        return;
    }
    case SM_VIEW_PHOTO: {
        if (k == SDLK_ESCAPE) { sm.view = SM_VIEW_CHAT; sm.status[0] = 0; play_click(); return; }
        if (k == SDLK_s) {                     /* X -- keep it as a real file */
            char dir[800], path[900];
            snprintf(dir, sizeof dir, "%s/messages", sn_data_root());
            mkdir(dir, 0755);
            const unsigned char *ink = sm_photo_ink();
            if (ink && sm_write_drawing(ink, dir, path, sizeof path)) {
                const char *name = strrchr(path, '/');
                snprintf(sm.status, sizeof sm.status, "Saved to messages/%s", name ? name + 1 : path);
            } else snprintf(sm.status, sizeof sm.status, "Could not save that drawing");
            play_click(); return;
        }
        if (k == SDLK_f) {                     /* Y -- hand it to the wallpaper picker */
            if (!sm_photo_ink()) return;
            bg_target_purpose = BG_TARGET_DRAWING;
            bg_target_selected = platform_selected;
            *state = STATE_BG_TARGET; play_click();
            return;
        }
        return;
    }
    case SM_VIEW_DRAW: {
        /* A and B are held down to paint, so they are handled in the step, not
           here. Exit is on Select because B is the eraser. */
        if (k == SDLK_SLASH) { sm.view = SM_VIEW_CHAT; play_click(); return; }
        if (k == SDLK_q) { sm.brush = sm.brush % 3 + 1; play_click(); return; }        /* L1 */
        if (k == SDLK_e) { sm.color = sm.color % SM_INKS + 1; play_click(); return; }  /* R1 */
        if (k == SDLK_f) { memset(sm.canvas, 0, sizeof sm.canvas); sm.marked = 0; play_click(); return; }
        if (k == SDLK_s) {                     /* X -- send */
            if (sm_canvas_blank(sm.canvas)) {
                snprintf(sm.status, sizeof sm.status, "Nothing drawn yet");
            } else {
                sm_send(SM_KIND_DRAW, -1, NULL, sm.canvas);
                sm.view = SM_VIEW_CHAT;
            }
            play_click();
            return;
        }
        return;
    }
    default: return;
    }
}

/* Held input, delivered per frame the way the mini-games get it: the pad bridge
   turns a button into an instant keydown+keyup, so a "hold to draw" control has
   to read the held state rather than the event. */
static void sm_step(int mvx, int mvy, int hold_draw, int hold_erase) {
    Uint32 now = SDL_GetTicks();
    float dt = sm.last ? (now - sm.last) / 1000.0f : 0.0f;
    sm.last = now;
    if (sm.view != SM_VIEW_DRAW) return;
    if (dt > 0.25f) dt = 0.25f;

    const float speed = 34.0f;             /* canvas pixels a second */
    sm.fx = mgx_clampf(sm.fx + mvx * speed * dt, 0.0f, (float)(SM_CANVAS_W - 1));
    sm.fy = mgx_clampf(sm.fy + mvy * speed * dt, 0.0f, (float)(SM_CANVAS_H - 1));
    sm.cx = (int)(sm.fx + 0.5f); sm.cy = (int)(sm.fy + 0.5f);

    int ink = hold_draw ? sm.color : hold_erase ? 0 : -1;
    if (ink < 0) { sm.painting = 0; sm.px = sm.cx; sm.py = sm.cy; return; }
    if (!sm.painting) { sm.painting = 1; sm.px = sm.cx; sm.py = sm.cy; }
    sm_stroke(sm.canvas, sm.px, sm.py, sm.cx, sm.cy, ink, sm.brush);
    sm.px = sm.cx; sm.py = sm.cy;
    if (hold_draw) sm.marked = 1;
}

/* ---------------------------------------------------------------- render -- */
static void sm_header(SDL_Renderer *ren, const char *title, const char *right) {
    Theme *th = mg_theme();
    SDL_Texture *t = render_text_fit(ren, font_small, title, th->accent2, WIN_W - 220);
    if (t) { int w, h; SDL_QueryTexture(t, NULL, NULL, &w, &h);
             SDL_RenderCopy(ren, t, NULL, &(SDL_Rect){ 30, 52, w, h }); }
    if (right) {
        SDL_Texture *r = render_text_fit(ren, font_label, right, g_ui_dim, 200);
        if (r) { int w, h; SDL_QueryTexture(r, NULL, NULL, &w, &h);
                 SDL_RenderCopy(ren, r, NULL, &(SDL_Rect){ WIN_W - w - 30, 58, w, h }); }
    }
}
static void sm_hint(SDL_Renderer *ren, const char *hint) {
    SDL_Texture *t = render_text_fit(ren, font_label, hint, g_ui_dim, WIN_W - 52);
    if (!t) return;
    int w, h; SDL_QueryTexture(t, NULL, NULL, &w, &h);
    SDL_RenderCopy(ren, t, NULL, &(SDL_Rect){ WIN_W / 2 - w / 2, WIN_H - h - 12, w, h });
}
/* Two short lines beat one long line that gets ellipsised on a 640-wide panel. */
static void sm_hint2(SDL_Renderer *ren, const char *upper, const char *lower) {
    int lw = 0, lh = 0;
    SDL_Texture *l = render_text_fit(ren, font_label, lower, g_ui_dim, WIN_W - 52);
    if (l) { SDL_QueryTexture(l, NULL, NULL, &lw, &lh);
             SDL_RenderCopy(ren, l, NULL, &(SDL_Rect){ WIN_W / 2 - lw / 2, WIN_H - lh - 12, lw, lh }); }
    SDL_Texture *u = render_text_fit(ren, font_label, upper, g_ui_dim, WIN_W - 52);
    if (u) { int uw, uh; SDL_QueryTexture(u, NULL, NULL, &uw, &uh);
             SDL_RenderCopy(ren, u, NULL, &(SDL_Rect){ WIN_W / 2 - uw / 2, WIN_H - lh - uh - 14, uw, uh }); }
}
static void sm_status_line(SDL_Renderer *ren, int y) {
    if (!sm.status[0]) return;
    mgx_text_center(ren, font_fixed ? font_fixed : font_label, sm.status, mg_theme()->accent2, WIN_W / 2, y);
}

/* A drawing, scaled up from the packed canvas. Paper goes down first so erased
   pixels read as paper rather than as whatever is behind the bubble. */
static void sm_canvas_render(SDL_Renderer *ren, const unsigned char *ink, SDL_Rect box, int scale) {
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(ren, sm_paper.r, sm_paper.g, sm_paper.b, 255);
    SDL_RenderFillRect(ren, &box);
    for (int y = 0; y < SM_CANVAS_H; y++) {
        int x = 0;
        while (x < SM_CANVAS_W) {
            int v = sm_px_get(ink, x, y), run = 1;
            while (x + run < SM_CANVAS_W && sm_px_get(ink, x + run, y) == v) run++;
            if (v) {
                SDL_Color c = sm_ink_color[v];
                SDL_SetRenderDrawColor(ren, c.r, c.g, c.b, 255);
                SDL_RenderFillRect(ren, &(SDL_Rect){ box.x + x * scale, box.y + y * scale, run * scale, scale });
            }
            x += run;
        }
    }
    SDL_SetRenderDrawColor(ren, 90, 92, 104, 255);
    SDL_RenderDrawRect(ren, &box);
}

/* The two lists, as a pair of tabs across the top. Each carries its own unread
   count so the one you are not looking at can still say it wants you. */
static void sm_render_tabs(SDL_Renderer *ren, int y) {
    Theme *th = mg_theme();
    int gap = 10, w = (WIN_W - 52 - gap) / 2;
    for (int t = 0; t < SM_TAB_COUNT; t++) {
        SDL_Rect r = { 26 + t * (w + gap), y, w, 30 };
        int on = t == sm.tab;
        mgx_panel(ren, r, on ? th->accent2 : th->bg, on ? th->accent2 : th->dim, on ? 235 : 175);
        int unread = t == SM_TAB_ROOMS ? sm_unread_rooms() : sm_unread_people();
        char label[48];
        if (unread) snprintf(label, sizeof label, "%s (%d)", sm_tab_name[t], unread);
        else        snprintf(label, sizeof label, "%s", sm_tab_name[t]);
        mgx_text_center(ren, font_label, label, on ? th->bg : g_ui_dim, r.x + r.w / 2, r.y + 4);
    }
}

static void sm_render_list(SDL_Renderer *ren) {
    Theme *th = mg_theme();
    char who[64]; snprintf(who, sizeof who, "You: %.*s", SM_NAME_MAX, sm_me_name());
    sm_header(ren, "MESSAGES", who);
    sm_render_tabs(ren, 86);

    const int top = 126, rowh = 40, gap = 6;
    int order[SM_PEERS], order_n = sm_people_order(order, SM_PEERS);
    int rows = sm_rows();
    int fit = (WIN_H - top - 58) / (rowh + gap);
    if (fit < 1) fit = 1;
    int first = 0;
    if (rows > fit) {
        first = sm.sel - fit / 2;
        if (first < 0) first = 0;
        if (first > rows - fit) first = rows - fit;
    }
    for (int i = first; i < rows && i < first + fit; i++) {
        SDL_Rect r = { 26, top + (i - first) * (rowh + gap), WIN_W - 52, rowh };
        int sel = i == sm.sel;
        mgx_panel(ren, r, sel ? th->select_bg : th->bg, sel ? th->accent2 : th->dim, sel ? 245 : 200);
        char key[34], label[96], right[40];
        if (sm.tab == SM_TAB_ROOMS) {
            snprintf(key, sizeof key, "#%d", i);
            snprintf(label, sizeof label, "# %s", sm_room_names[i]);
            snprintf(right, sizeof right, "24h");
        } else {
            int pi = i < order_n ? order[i] : i;
            SmPeer *p = &sm.peers[pi];
            snprintf(key, sizeof key, "%s", p->id);
            snprintf(label, sizeof label, "%.*s", SM_NAME_MAX, p->name[0] ? p->name : "Nearby device");
            snprintf(right, sizeof right, "%s", sm_peer_online(pi) ? "nearby" : "away");
        }
        int unread = sm_unread_for(key);
        if (unread) snprintf(right, sizeof right, "%d new", unread);
        mgx_text(ren, font_label, label, sel ? g_ui_text : g_ui_dim, 40, r.y + 9);
        SDL_Texture *rt = render_text_fit(ren, font_fixed ? font_fixed : font_label, right,
                                          unread ? th->accent2 : g_ui_dim, 120);
        if (rt) { int w, h; SDL_QueryTexture(rt, NULL, NULL, &w, &h);
                  SDL_RenderCopy(ren, rt, NULL, &(SDL_Rect){ r.x + r.w - w - 14, r.y + (rowh - h) / 2, w, h }); }
    }
    /* Without the socket nothing is nearby and nothing ever will be, so say
       that rather than leaving the honest-looking empty-room message up. */
    if (sm.fd < 0)
        mgx_text_center(ren, font_fixed ? font_fixed : font_label,
                        "Messaging could not open its network port - check Wi-Fi.",
                        th->accent2, WIN_W / 2, WIN_H - 54);
    else if (sm.tab == SM_TAB_PEOPLE && !sm.n)
        mgx_text_center(ren, font_fixed ? font_fixed : font_label,
                        "Nobody yet. Devices appear here as they come within range.",
                        g_ui_dim, WIN_W / 2, WIN_H - 54);
    sm_hint(ren, "A Open   L1/R1 List   Y Nickname   B Back");
}

static void sm_render_chat(SDL_Renderer *ren) {
    Theme *th = mg_theme();
    int room = sm_room_key(sm.key);
    char title[64];
    snprintf(title, sizeof title, "%s%.*s", room >= 0 ? "# " : "", SM_NAME_MAX, sm.title);
    int peer = room >= 0 ? -1 : sm_find_peer(sm.key);
    sm_header(ren, title, room >= 0 ? "everyone nearby" : (sm_peer_online(peer) ? "nearby" : "away"));

    int count = sm_conv_count();
    /* One receipt, under the newest of mine they have opened. Marking every
       bubble would say the same thing four times. */
    int read_upto = -1;
    for (int i = 0; i < count; i++) {
        SmLog *e = sm_conv_at(i);
        if (e && e->mine && e->read) read_upto = i;
    }
    int top = 88, bottom = WIN_H - 76;
    const int draw_scale = 3;
    /* Newest at the bottom: walk backwards from the last visible line, laying
       bubbles upward until the next one would not fit. */
    int y = bottom;
    for (int i = count - 1 - sm.scroll; i >= 0 && y > top; i--) {
        SmLog *e = sm_conv_at(i);
        if (!e) continue;
        int maxw = (WIN_W - 72) * 3 / 4;
        int bw, bh, nlines = 0;
        char lines[MAX_LINES][128];
        if (e->kind == SM_KIND_DRAW) {
            bw = SM_CANVAS_W * draw_scale + 16;
            bh = SM_CANVAS_H * draw_scale + 16;
        } else if (e->kind == SM_KIND_EMOJI) {
            bw = 8 * 4 + 20; bh = 8 * 4 + 16;
        } else {
            nlines = wrap_text(font_label, e->text[0] ? e->text : " ", maxw - 24, lines);
            if (nlines < 1) nlines = 1;
            bw = 0;
            for (int l = 0; l < nlines; l++) {
                int w = 0, h = 0;
                if (TTF_SizeUTF8(font_label, lines[l], &w, &h) == 0 && w > bw) bw = w;
            }
            bw += 24;
            if (bw > maxw) bw = maxw;
            bh = nlines * (TTF_FontHeight(font_label) + 2) + 14;
        }
        int name_h = (!e->mine && room >= 0) ? 16 : 0;
        bh += name_h;
        y -= bh;
        if (y < top) break;
        SDL_Rect b = { e->mine ? WIN_W - 26 - bw : 26, y, bw, bh };
        mgx_panel(ren, b, e->mine ? th->accent2 : th->select_bg,
                  e->mine ? th->accent2 : th->dim, e->mine ? 235 : 220);
        if (i == sm.pick) {
            SDL_SetRenderDrawColor(ren, th->accent3.r, th->accent3.g, th->accent3.b, 255);
            SDL_RenderDrawRect(ren, &(SDL_Rect){ b.x - 3, b.y - 3, b.w + 6, b.h + 6 });
            SDL_RenderDrawRect(ren, &(SDL_Rect){ b.x - 2, b.y - 2, b.w + 4, b.h + 4 });
        }
        if (name_h)
            mgx_text(ren, font_fixed ? font_fixed : font_label, e->name, g_ui_dim, b.x + 12, b.y + 3);
        if (e->kind == SM_KIND_DRAW && e->ink >= 0 && e->ink < SM_INK_SLOTS) {
            sm_canvas_render(ren, sm.ink[e->ink],
                             (SDL_Rect){ b.x + 8, b.y + 8 + name_h,
                                         SM_CANVAS_W * draw_scale, SM_CANVAS_H * draw_scale },
                             draw_scale);
        } else if (e->kind == SM_KIND_EMOJI) {
            sm_emoji_draw(ren, e->emoji, b.x + 10, b.y + 8 + name_h, 4);
        } else {
            SDL_Color tc = e->mine ? th->bg : g_ui_text;
            for (int l = 0; l < nlines; l++) {
                SDL_Texture *t = render_text_fit(ren, font_label, lines[l], tc, bw - 20);
                if (!t) continue;
                int w, h; SDL_QueryTexture(t, NULL, NULL, &w, &h);
                SDL_RenderCopy(ren, t, NULL, &(SDL_Rect){ b.x + 12,
                    b.y + 7 + name_h + l * (TTF_FontHeight(font_label) + 2), w, h });
            }
        }
        /* A message that never got acknowledged says so, instead of sitting
           there looking delivered. */
        if (e->mine && e->seen == 2) {
            SDL_SetRenderDrawColor(ren, 214, 72, 90, 255);
            SDL_RenderFillRect(ren, &(SDL_Rect){ b.x - 5, b.y + 4, 3, b.h - 8 });
        }
        if (i == read_upto) {
            SDL_Texture *r = render_text(ren, font_fixed ? font_fixed : font_label, "Read", g_ui_dim);
            if (r) {
                int rw, rh; SDL_QueryTexture(r, NULL, NULL, &rw, &rh);
                SDL_RenderCopy(ren, r, NULL, &(SDL_Rect){ b.x + b.w - rw, b.y + b.h + 1, rw, rh });
                y -= rh + 1;
            }
        }
        y -= 8;
    }
    if (!count)
        mgx_text_center(ren, font_label,
                        room >= 0 ? "Nothing said in here yet. Say hello."
                                  : "No messages yet. A types, X sends an emoji, Y draws.",
                        g_ui_dim, WIN_W / 2, WIN_H / 2 - 10);
    sm_status_line(ren, WIN_H - 70);
    if (sm.pick >= 0) {
        SmLog *held = sm_conv_at(sm.pick);
        sm_hint(ren, held && held->kind == SM_KIND_DRAW
                     ? "A Open Drawing   Up/Down Browse   B Done"
                     : "Up/Down Browse   B Done");
    } else sm_hint(ren, "A Write   X Emoji   Y Draw   Up Browse   B Back");
}

static void sm_render_emoji(SDL_Renderer *ren) {
    Theme *th = mg_theme();
    sm_header(ren, "EMOJI", NULL);
    const int cols = 4, rows = SM_EMOJI_COUNT / 4;
    int cw = (WIN_W - 80) / cols, chh = (WIN_H - 190) / rows;
    for (int i = 0; i < SM_EMOJI_COUNT; i++) {
        int c = i % cols, r = i / cols;
        SDL_Rect box = { 40 + c * cw, 96 + r * chh, cw - 12, chh - 12 };
        int sel = i == sm.emoji_sel;
        mgx_panel(ren, box, sel ? th->select_bg : th->bg, sel ? th->accent2 : th->dim, sel ? 245 : 190);
        int scale = mgx_clampi((box.h - 26) / 8, 2, 5);
        sm_emoji_draw(ren, i, box.x + box.w / 2 - scale * 4, box.y + 6, scale);
        mgx_text_center(ren, font_fixed ? font_fixed : font_label, sm_emoji[i].name,
                        sel ? g_ui_text : g_ui_dim, box.x + box.w / 2, box.y + box.h - 18);
    }
    sm_hint(ren, "D-pad Choose   A Send   B Back");
}

static void sm_render_draw(SDL_Renderer *ren) {
    Theme *th = mg_theme();
    char right[48];
    snprintf(right, sizeof right, "Brush %d", sm.brush);
    sm_header(ren, "DRAW", right);

    int cw = SM_CANVAS_W * SM_CANVAS_SCALE, ch = SM_CANVAS_H * SM_CANVAS_SCALE;
    SDL_Rect box = { WIN_W / 2 - cw / 2, 96, cw, ch };
    sm_canvas_render(ren, sm.canvas, box, SM_CANVAS_SCALE);

    /* The cursor is a hollow square the size of the brush, so what you are
       about to lay down is what you can see. */
    int r = mgx_clampi(sm.brush, 1, 3);
    SDL_Rect cur = { box.x + (sm.cx - r / 2) * SM_CANVAS_SCALE - 1,
                     box.y + (sm.cy - r / 2) * SM_CANVAS_SCALE - 1,
                     r * SM_CANVAS_SCALE + 2, r * SM_CANVAS_SCALE + 2 };
    SDL_Color c = sm_ink_color[sm.color];
    SDL_SetRenderDrawColor(ren, c.r, c.g, c.b, 255);
    SDL_RenderDrawRect(ren, &cur);
    SDL_SetRenderDrawColor(ren, 30, 30, 38, 255);
    SDL_RenderDrawRect(ren, &(SDL_Rect){ cur.x - 1, cur.y - 1, cur.w + 2, cur.h + 2 });

    int sw = 22, y = box.y + ch + 12;
    for (int i = 1; i <= SM_INKS; i++) {
        SDL_Rect s = { box.x + (i - 1) * (sw + 8), y, sw, sw };
        SDL_Color ic = sm_ink_color[i];
        SDL_SetRenderDrawColor(ren, ic.r, ic.g, ic.b, 255);
        SDL_RenderFillRect(ren, &s);
        if (i == sm.color) {
            SDL_SetRenderDrawColor(ren, th->accent2.r, th->accent2.g, th->accent2.b, 255);
            SDL_RenderDrawRect(ren, &(SDL_Rect){ s.x - 3, s.y - 3, s.w + 6, s.h + 6 });
        }
    }
    sm_status_line(ren, y + 2);
    sm_hint2(ren, "D-pad Move   A Draw   B Erase   Y Clear",
                  "L1 Brush   R1 Colour   X Send   Select Back");
}

/* The biggest whole-number blow-up that fits. Whole numbers only: half a
   canvas pixel of smoothing turns hand-drawn pixel art to mush. */
static int sm_photo_scale(void) {
    int by_width = (WIN_W - 48) / SM_CANVAS_W;
    int by_height = (WIN_H - 156) / SM_CANVAS_H;
    return mgx_clampi(by_width < by_height ? by_width : by_height, 1, 10);
}
static void sm_render_photo(SDL_Renderer *ren) {
    sm_header(ren, "DRAWING", NULL);
    const unsigned char *ink = sm_photo_ink();
    if (!ink) { sm_hint(ren, "B Back"); return; }
    int scale = sm_photo_scale();
    int w = SM_CANVAS_W * scale, h = SM_CANVAS_H * scale;
    int top = 88, room = WIN_H - 68 - top;
    SDL_Rect box = { WIN_W / 2 - w / 2, top + (room > h ? (room - h) / 2 : 0), w, h };
    sm_canvas_render(ren, ink, box, scale);
    sm_status_line(ren, WIN_H - 64);
    sm_hint(ren, "X Save to Device   Y Use as Background   B Back");
}

/* Unread mail in the top status bar, so a message that arrived while you were
   somewhere else says so without the app being open. Drawn only when there is
   something waiting -- a badge that is always present stops meaning anything. */
static int sm_hud_badge(SDL_Renderer *ren, int right_x, int y) {
    int n = sm_unread_total();
    if (n <= 0) return 0;
    Theme *th = mg_theme();
    char count[8]; snprintf(count, sizeof count, "%d", n > 99 ? 99 : n);
    SDL_Texture *t = render_text(ren, font_fixed ? font_fixed : font_label, count, th->bg);
    int tw = 0, thh = 0;
    if (t) SDL_QueryTexture(t, NULL, NULL, &tw, &thh);
    const int gw = 15, gh = 11, pad = 7, gap = 6;
    int h = (thh > gh ? thh : gh) + 8;
    int w = pad + gw + gap + tw + pad;
    SDL_Rect pill = { right_x - w, y, w, h };
    fill_rounded(ren, pill, h / 2, th->accent2.r, th->accent2.g, th->accent2.b, 240);
    /* A closed envelope: the body, then the flap as two strokes in from the
       top corners. Legible at eleven pixels tall, which an icon would not be. */
    int ex = pill.x + pad, ey = pill.y + (h - gh) / 2;
    SDL_SetRenderDrawColor(ren, th->bg.r, th->bg.g, th->bg.b, 255);
    SDL_RenderDrawRect(ren, &(SDL_Rect){ ex, ey, gw, gh });
    SDL_RenderDrawLine(ren, ex, ey, ex + gw / 2, ey + gh / 2);
    SDL_RenderDrawLine(ren, ex + gw - 1, ey, ex + gw / 2, ey + gh / 2);
    if (t) SDL_RenderCopy(ren, t, NULL, &(SDL_Rect){ ex + gw + gap, pill.y + (h - thh) / 2, tw, thh });
    return w;
}

static void sm_render(SDL_Renderer *ren) {
    switch (sm.view) {
        case SM_VIEW_CHAT:  sm_render_chat(ren);  break;
        case SM_VIEW_EMOJI: sm_render_emoji(ren); break;
        case SM_VIEW_DRAW:  sm_render_draw(ren);  break;
        case SM_VIEW_PHOTO: sm_render_photo(ren); break;
        default:            sm_render_list(ren);  break;
    }
}
#endif
