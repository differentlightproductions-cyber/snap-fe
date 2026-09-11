#ifndef SNAP_TEST_MESSAGES_131_H
#define SNAP_TEST_MESSAGES_131_H
#include <poll.h>
/* SNAP Chat. Everything here runs over loopback on an ephemeral port, so the
   suite never announces itself onto the machine's real network. */

static int sm_test_peerfd = -1;
static struct sockaddr_in sm_test_local;

static int sm_test_painted(const unsigned char *ink) {
    int n = 0;
    for (int y = 0; y < SM_CANVAS_H; y++)
        for (int x = 0; x < SM_CANVAS_W; x++) if (sm_px_get(ink, x, y)) n++;
    return n;
}
/* Loopback delivery is prompt but not synchronous, and these datagrams are
   1380 bytes. Waiting for the socket to actually become readable keeps the
   test measuring the app rather than the kernel's timing. */
static void sm_test_settle(int fd, int ms) {
    struct pollfd pfd = { .fd = fd, .events = POLLIN, .revents = 0 };
    poll(&pfd, 1, ms);
}
/* Deliver one packet to the app and let it process it. Stamping the beacon
   clock keeps sm_tick from broadcasting while the tests run. */
static void sm_test_deliver(SmPacket *p, uint32_t token) {
    p->magic = htonl(SM_MAGIC); p->version = SM_VERSION; p->token = token;
    assert(sendto(sm_test_peerfd, p, sizeof *p, 0,
                  (struct sockaddr *)&sm_test_local, sizeof sm_test_local) == (ssize_t)sizeof *p);
    sm_test_settle(sm.fd, 500);
    sm.beacon = SDL_GetTicks();
    sm_tick();
}
static void sm_test_blank(SmPacket *p, int type, const char *from, const char *to) {
    memset(p, 0, sizeof *p);
    p->type = (unsigned char)type;
    p->room = 0xff;
    p->emoji = -1;
    snprintf(p->from, sizeof p->from, "%s", from);
    snprintf(p->to, sizeof p->to, "%s", to);
    snprintf(p->name, sizeof p->name, "Pixel");
}
static int sm_test_drain_ack(uint32_t token) {
    for (int i = 0; i < 8; i++) {
        SmPacket got; struct sockaddr_in a; socklen_t len = sizeof a;
        sm_test_settle(sm_test_peerfd, 500);
        ssize_t n = recvfrom(sm_test_peerfd, &got, sizeof got, 0, (struct sockaddr *)&a, &len);
        if (n != (ssize_t)sizeof got) return 0;
        if (got.type == SM_ACK && got.token == token) return 1;
    }
    return 0;
}

/* Did a packet of this type come back to the other device? `ms` is short when
   the point is to prove nothing was sent. */
static int sm_test_saw(int type, int ms) {
    for (int i = 0; i < 8; i++) {
        SmPacket got; struct sockaddr_in a; socklen_t len = sizeof a;
        sm_test_settle(sm_test_peerfd, ms);
        ssize_t n = recvfrom(sm_test_peerfd, &got, sizeof got, 0, (struct sockaddr *)&a, &len);
        if (n != (ssize_t)sizeof got) return 0;
        if (got.type == type) return 1;
    }
    return 0;
}

static void test_messages_131(SDL_Renderer *ren) {
    const char *them = "dddddddddddddddddddddddddddddddd";
    const char *other = "eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee";
    sf_init();
    assert(sf_valid_id(sf.id));

    /* A drawing has to survive as one datagram. If the packet ever grows past
       the usual MTU it starts being fragmented, and the whole "no reassembly"
       design in this app quietly stops being true. */
    assert(SM_CANVAS_BYTES == 1152);
    assert(sizeof(SmPacket) <= 1400);

    /* ---- the canvas: two bits a pixel, packed four to a byte ------------- */
    unsigned char c[SM_CANVAS_BYTES];
    memset(c, 0, sizeof c);
    assert(sm_canvas_blank(c));
    sm_px_set(c, 5, 7, 2);
    assert(sm_px_get(c, 5, 7) == 2 && sm_px_get(c, 4, 7) == 0 && sm_px_get(c, 6, 7) == 0);
    assert(!sm_canvas_blank(c));
    sm_px_set(c, 6, 7, 3);
    assert(sm_px_get(c, 5, 7) == 2 && sm_px_get(c, 6, 7) == 3);  /* neighbours share a byte */
    sm_px_set(c, 5, 7, 0); sm_px_set(c, 6, 7, 0);
    assert(sm_canvas_blank(c));                                  /* erasing really erases */
    /* Off the edge is dropped, never wrapped into the next row. */
    sm_px_set(c, -1, 0, 3); sm_px_set(c, SM_CANVAS_W, 0, 3); sm_px_set(c, 0, SM_CANVAS_H, 3);
    assert(sm_canvas_blank(c));

    /* A stroke joins its endpoints. Without this the cursor's per-frame jump
       leaves a dotted line instead of a drawn one. */
    sm_stroke(c, 2, 2, 40, 2, 1, 1);
    for (int x = 2; x <= 40; x++) assert(sm_px_get(c, x, 2) == 1);
    assert(sm_px_get(c, 41, 2) == 0 && sm_px_get(c, 1, 2) == 0);
    /* A fatter brush covers more, and the eraser takes all of it back. */
    memset(c, 0, sizeof c);
    sm_dab(c, 20, 20, 3, 1); int thin = sm_test_painted(c);
    memset(c, 0, sizeof c);
    sm_dab(c, 20, 20, 3, 3); int fat = sm_test_painted(c);
    assert(thin == 1 && fat > thin);
    sm_dab(c, 20, 20, 0, 3);
    assert(sm_canvas_blank(c));

    /* ---- drawing with a held button ------------------------------------- */
    sm_draw_open();
    assert(sm.view == SM_VIEW_DRAW && sm_canvas_blank(sm.canvas) && !sm.marked);
    int startx = sm.cx, starty = sm.cy;
    sm.last = SDL_GetTicks() - 300;
    sm_step(1, 0, 1, 0);                       /* right, holding A */
    /* The first frame of a press marks where the cursor is; it does not reach
       back to wherever the cursor had been sitting unpressed. */
    assert(sm.cx > startx && sm.marked && sm_px_get(sm.canvas, sm.cx, starty));
    int midx = sm.cx;
    sm.last = SDL_GetTicks() - 300;
    sm_step(1, 0, 1, 0);                       /* still held: the line joins up */
    assert(sm.cx > midx);
    for (int x = midx; x <= sm.cx; x++) assert(sm_px_get(sm.canvas, x, starty));
    int drawn = sm_test_painted(sm.canvas);
    /* Moving with nothing held lays down nothing at all. */
    sm.last = SDL_GetTicks() - 300;
    sm_step(1, 0, 0, 0);
    assert(sm.cx > startx && sm_test_painted(sm.canvas) == drawn);
    /* B walks back over the line and lifts ink off it. */
    sm.last = SDL_GetTicks() - 300;
    sm_step(-1, 0, 0, 1);
    assert(sm_test_painted(sm.canvas) < drawn);
    /* The cursor stays on the canvas however long you push. */
    for (int i = 0; i < 30; i++) { sm.last = SDL_GetTicks() - 300; sm_step(-1, -1, 0, 0); }
    assert(sm.cx == 0 && sm.cy == 0);
    for (int i = 0; i < 60; i++) { sm.last = SDL_GetTicks() - 300; sm_step(1, 1, 0, 0); }
    assert(sm.cx == SM_CANVAS_W - 1 && sm.cy == SM_CANVAS_H - 1);

    /* The shoulder buttons cycle brush and colour, and Y wipes the sheet. */
    AppState state = STATE_MESSAGES;
    int brush0 = sm.brush;
    sm_key(SDLK_q, &state); assert(sm.brush != brush0);
    for (int i = 0; i < 2; i++) sm_key(SDLK_q, &state);
    assert(sm.brush == brush0 && sm.brush >= 1 && sm.brush <= 3);
    int color0 = sm.color;
    sm_key(SDLK_e, &state); assert(sm.color != color0 && sm.color >= 1 && sm.color <= SM_INKS);
    for (int i = 0; i < SM_INKS - 1; i++) sm_key(SDLK_e, &state);
    assert(sm.color == color0);
    sm_key(SDLK_f, &state); assert(sm_canvas_blank(sm.canvas));
    /* An empty sheet is not worth sending, and saying so beats sending blank
       paper to somebody. */
    sm_key(SDLK_s, &state);
    assert(sm.view == SM_VIEW_DRAW && strstr(sm.status, "Nothing"));
    /* B is the eraser here, so Select is the way out. */
    sm_key(SDLK_SLASH, &state); assert(sm.view == SM_VIEW_CHAT && state == STATE_MESSAGES);

    /* ---- loopback network ----------------------------------------------- */
    sm_init();
    if (sm.fd >= 0) close(sm.fd);
    sm.fd = mgx_net_socket(0); assert(sm.fd >= 0);
    sm_test_peerfd = mgx_net_socket(0); assert(sm_test_peerfd >= 0);
    socklen_t sz = sizeof sm_test_local;
    assert(getsockname(sm.fd, (struct sockaddr *)&sm_test_local, &sz) == 0);
    sm_test_local.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    sm.n = 0; sm.log_n = 0; sm.ink_next = 0; sm.view = SM_VIEW_LIST; sm.sel = 0;

    SmPacket p;
    /* A beacon is all it takes to be discovered -- no pairing, no accounts. */
    sm_test_blank(&p, SM_BEACON, them, "");
    sm_test_deliver(&p, 1);
    assert(sm.n == 1 && !strcmp(sm.peers[0].name, "Pixel") && sm_peer_online(0));
    /* People and rooms are two separate lists, each with its own length and
       its own cursor. */
    sm.tab = SM_TAB_PEOPLE;  assert(sm_rows() == 1);
    sm_tab_set(SM_TAB_ROOMS); assert(sm.tab == SM_TAB_ROOMS && sm_rows() == SM_ROOMS);
    sm.sel = 2;
    sm_tab_set(SM_TAB_PEOPLE); assert(sm.sel == 0);
    sm_tab_set(SM_TAB_ROOMS);  assert(sm.sel == 2);   /* each list remembers */
    sm_tab_set(SM_TAB_PEOPLE);

    /* A room message reaches everyone and lands under that room. */
    sm_test_blank(&p, SM_SAY, them, "");
    p.room = 0; p.kind = SM_KIND_TEXT;
    snprintf(p.text, sizeof p.text, "hello everyone");
    sm_test_deliver(&p, 2);
    assert(sm.log_n == 1 && !strcmp(sm.log[0].key, "#0") && !sm.log[0].mine &&
           !strcmp(sm.log[0].text, "hello everyone") && !sm.log[0].seen);
    assert(sm_unread_for("#0") == 1 && sm_unread_total() == 1);
    /* Rooms are broadcast twice, so the same token must not say it twice. */
    sm_test_deliver(&p, 2);
    assert(sm.log_n == 1);

    /* A direct message is acknowledged, so the sender can tell it arrived. */
    sm_test_blank(&p, SM_SAY, them, sf.id);
    p.kind = SM_KIND_TEXT;
    snprintf(p.text, sizeof p.text, "just for you");
    sm_test_deliver(&p, 3);
    assert(sm.log_n == 2 && !strcmp(sm.log[1].key, them));
    assert(sm_test_drain_ack(3));
    /* A repeat -- what a sender does when our acknowledgement went missing --
       is acknowledged again but never logged twice. */
    sm_test_deliver(&p, 3);
    assert(sm.log_n == 2 && sm_test_drain_ack(3));

    /* Somebody else's direct message is not ours to read. */
    sm_test_blank(&p, SM_SAY, them, other);
    p.kind = SM_KIND_TEXT; snprintf(p.text, sizeof p.text, "private");
    sm_test_deliver(&p, 4);
    assert(sm.log_n == 2);
    /* Nor is a packet claiming to come from this very device. */
    sm_test_blank(&p, SM_SAY, sf.id, sf.id);
    p.room = 0; p.kind = SM_KIND_TEXT; snprintf(p.text, sizeof p.text, "spoofed");
    sm_test_deliver(&p, 5);
    assert(sm.log_n == 2 && sm.n == 1);
    /* Control characters in a name or message are scrubbed before they can
       wreck a line of the log file or the screen. */
    sm_test_blank(&p, SM_SAY, them, sf.id);
    p.kind = SM_KIND_TEXT;
    snprintf(p.text, sizeof p.text, "tab\there");
    snprintf(p.name, sizeof p.name, "New\nLine");
    sm_test_deliver(&p, 6);
    assert(!strchr(sm.log[2].text, '\t') && !strchr(sm.log[2].name, '\n'));

    /* A drawing arrives whole, pixel for pixel. */
    memset(c, 0, sizeof c);
    sm_stroke(c, 4, 4, 80, 40, 2, 2);
    sm_stroke(c, 80, 4, 4, 40, 3, 2);
    sm_test_blank(&p, SM_SAY, them, sf.id);
    p.kind = SM_KIND_DRAW;
    memcpy(p.ink, c, SM_CANVAS_BYTES);
    sm_test_deliver(&p, 7);
    assert(sm.log_n == 4 && sm.log[3].kind == SM_KIND_DRAW && sm.log[3].ink >= 0);
    assert(!memcmp(sm.ink[sm.log[3].ink], c, SM_CANVAS_BYTES));

    /* An emoji is its own little message, not a character inside one. */
    sm_test_blank(&p, SM_SAY, them, sf.id);
    p.kind = SM_KIND_EMOJI; p.emoji = 4;
    sm_test_deliver(&p, 8);
    assert(sm.log_n == 5 && sm.log[4].kind == SM_KIND_EMOJI && sm.log[4].emoji == 4);

    /* ---- reading, and what that does to the unread counts ---------------- */
    assert(sm_unread_for(them) == 4 && sm_unread_for("#0") == 1);
    /* The counts split the same way the lists do. */
    assert(sm_unread_people() == 4 && sm_unread_rooms() == 1);
    assert(sm_unread_people() + sm_unread_rooms() == sm_unread_total());
    sm.tab = SM_TAB_PEOPLE; sm.sel = 0;      /* the first nearby device */
    sm_key(SDLK_RETURN, &state);
    assert(sm.view == SM_VIEW_CHAT && !strcmp(sm.key, them));
    assert(sm_unread_for(them) == 0 && sm_unread_total() == 1);   /* the room is still unread */
    assert(sm_conv_count() == 4);
    /* A message that lands while you are looking at that conversation is not
       waiting to be read. */
    sm_test_blank(&p, SM_SAY, them, sf.id);
    p.kind = SM_KIND_TEXT; snprintf(p.text, sizeof p.text, "still here?");
    sm_test_deliver(&p, 9);
    assert(sm_conv_count() == 5 && sm_unread_for(them) == 0);

    /* ---- sending -------------------------------------------------------- */
    int before = sm.log_n;
    sm_send_text("on my way");
    assert(sm.log_n == before + 1 && sm.log[before].mine &&
           !strcmp(sm.log[before].text, "on my way"));
    assert(sm.out_pending);                  /* waiting to be acknowledged */
    /* Acknowledging that exact token settles it. */
    SmPacket ack;
    sm_test_blank(&ack, SM_ACK, them, sf.id);
    sm_test_deliver(&ack, sm.out.token);
    assert(!sm.out_pending && strstr(sm.status, "Deliver"));

    /* Talking to a device that has gone says so rather than pretending. */
    sm.peers[0].seen = SDL_GetTicks() - 60000;
    assert(!sm_peer_online(0));
    before = sm.log_n;
    sm_send_text("anyone there");
    assert(sm.log_n == before + 1 && sm.log[before].seen == 2 && !sm.out_pending);
    sm.peers[0].seen = SDL_GetTicks();

    /* A room message goes out without waiting for anyone to answer, because in
       a room there is nobody in particular to answer. */
    snprintf(sm.key, sizeof sm.key, "#1");
    snprintf(sm.title, sizeof sm.title, "%s", sm_room_names[1]);
    before = sm.log_n;
    sm_send_text("anyone up for a race");
    assert(sm.log_n == before + 1 && !sm.out_pending && !strcmp(sm.log[before].key, "#1"));

    /* ---- keeping conversations across a restart ------------------------- */
    int saved_n = sm.log_n, saved_slot = sm.log[3].ink;
    unsigned char saved_ink[SM_CANVAS_BYTES];
    memcpy(saved_ink, sm.ink[saved_slot], SM_CANVAS_BYTES);
    sm_save();
    assert(!sm.dirty);
    sm.log_n = 0; sm.ink_next = 0;
    memset(sm.ink, 0, sizeof sm.ink);
    sm_load();
    assert(sm.log_n == saved_n);
    assert(!strcmp(sm.log[0].key, "#0") && !strcmp(sm.log[0].text, "hello everyone"));
    assert(sm.log[3].kind == SM_KIND_DRAW && sm.log[3].ink >= 0);
    assert(!memcmp(sm.ink[sm.log[3].ink], saved_ink, SM_CANVAS_BYTES));
    assert(sm.log[4].kind == SM_KIND_EMOJI && sm.log[4].emoji == 4);
    /* History that was already on disk is history, not a pile of new mail. */
    assert(sm_unread_total() == 0);

    /* ---- drawings age out, and say so instead of showing someone else's -- */
    int recycled = sm.log[3].ink;
    for (int i = 0; i < SM_INK_SLOTS; i++) {
        memset(c, 0, sizeof c);
        sm_px_set(c, i % SM_CANVAS_W, 1, 1);
        sm_log_push("#0", "Pixel", 0, SM_KIND_DRAW, -1, NULL, c, 0);
    }
    assert(sm.log[3].kind == SM_KIND_TEXT && sm.log[3].ink == -1 &&
           strstr(sm.log[3].text, "older"));
    (void)recycled;
    /* The log is a ring: it fills up and drops the oldest rather than growing
       without limit or refusing new messages. */
    while (sm.log_n < SM_LOG_MAX)
        sm_log_push("#0", "Pixel", 0, SM_KIND_TEXT, -1, "filler", NULL, 0);
    snprintf(sm.key, sizeof sm.key, "#0");
    int room_lines = sm_conv_count();
    sm_log_push("#0", "Pixel", 0, SM_KIND_TEXT, -1, "newest", NULL, 0);
    assert(sm.log_n == SM_LOG_MAX && sm_conv_count() == room_lines);
    assert(!strcmp(sm.log[SM_LOG_MAX - 1].text, "newest"));

    /* Room keys are read strictly -- a peer id must never be mistaken for one. */
    assert(sm_room_key("#0") == 0 && sm_room_key("#3") == 3);
    assert(sm_room_key("#9") < 0 && sm_room_key(them) < 0 && sm_room_key("#x") < 0);

    /* ---- read receipts, on for everyone, no setting --------------------- */
    sm.log_n = 0; sm.ink_next = 0; sm.view = SM_VIEW_LIST;
    snprintf(sm.key, sizeof sm.key, "%s", them);
    snprintf(sm.title, sizeof sm.title, "Pixel");
    sm_log_push(them, sm_me_name(), 1, SM_KIND_TEXT, -1, "did you see this", NULL, 0);
    assert(!sm.log[0].read);
    /* Their device reports that it opened the conversation. */
    sm_test_blank(&p, SM_READ, them, sf.id);
    sm_test_deliver(&p, 40);
    assert(sm.log[0].read);
    /* A receipt addressed to somebody else is not ours to act on. */
    sm_log_push(them, sm_me_name(), 1, SM_KIND_TEXT, -1, "and this", NULL, 0);
    sm_test_blank(&p, SM_READ, them, other);
    sm_test_deliver(&p, 41);
    assert(!sm.log[1].read);
    /* Opening a conversation with a person sends one back... */
    while (sm_test_saw(SM_ACK, 40)) { }           /* clear anything still queued */
    sm.tab = SM_TAB_PEOPLE; sm.sel = 0; sm.view = SM_VIEW_LIST;
    sm_key(SDLK_RETURN, &state);
    assert(sm.view == SM_VIEW_CHAT && sm_test_saw(SM_READ, 500));
    /* ...and opening a room sends nothing, because a room has nobody in
       particular to report back to. */
    sm.view = SM_VIEW_LIST; sm.tab = SM_TAB_ROOMS; sm.sel = 0;
    sm_key(SDLK_RETURN, &state);
    assert(sm.view == SM_VIEW_CHAT && !sm_test_saw(SM_READ, 120));
    sm.tab = SM_TAB_PEOPLE;

    /* ---- rooms are a day long; conversations with people are forever ---- */
    sm.log_n = 0; sm.ink_next = 0;
    long now_s = (long)time(NULL);
    sm_log_push("#0", "Pixel", 0, SM_KIND_TEXT, -1, "yesterday", NULL, now_s - SM_ROOM_KEEP_SECS - 60);
    sm_log_push("#0", "Pixel", 0, SM_KIND_TEXT, -1, "just now", NULL, now_s - 5);
    sm_log_push(them, "Pixel", 0, SM_KIND_TEXT, -1, "an old direct message", NULL,
                now_s - SM_ROOM_KEEP_SECS * 30);
    sm_expire_rooms();
    assert(sm.log_n == 2);
    assert(!strcmp(sm.log[0].text, "just now"));
    assert(!strcmp(sm.log[1].text, "an old direct message"));
    /* A stale room line does not come back off the disk either. */
    sm_log_push("#1", "Pixel", 0, SM_KIND_TEXT, -1, "stale", NULL, now_s - SM_ROOM_KEEP_SECS - 1);
    sm_save();
    sm.log_n = 0; sm.ink_next = 0;
    sm_load();
    assert(sm.log_n == 2);
    for (int i = 0; i < sm.log_n; i++) assert(strcmp(sm.log[i].text, "stale"));

    /* A receipt survives a restart -- it is part of the conversation. */
    sm.log_n = 0; sm.ink_next = 0;
    sm_log_push(them, sm_me_name(), 1, SM_KIND_TEXT, -1, "kept", NULL, 0);
    sm.log[0].read = 1;
    sm_save();
    sm.log_n = 0; sm.ink_next = 0;
    sm_load();
    assert(sm.log_n == 1 && sm.log[0].read && sm.log[0].mine);

    /* ---- the log writes itself ------------------------------------------ */
    /* This is the bug behind "messages are not saved": a message that arrived
       while the app was closed used to sit unwritten until somebody happened to
       open Messages and back out of it again. */
    {
        char path[800];
        sm_path(path, sizeof path, "messages.cfg");
        remove(path);
        sm.log_n = 0; sm.ink_next = 0; sm.dirty = 0;
        sm_log_push(them, "Pixel", 0, SM_KIND_TEXT, -1, "while you were out", NULL, 0);
        assert(sm.dirty);
        sm.beacon = SDL_GetTicks();
        sm_tick();
        assert(sm.dirty);                          /* not written the instant it lands */
        sm.dirty_at = SDL_GetTicks() - SM_SAVE_SETTLE_MS - 1;
        sm.beacon = SDL_GetTicks();
        sm_tick();
        assert(!sm.dirty);
        FILE *written = fopen(path, "r");
        assert(written);
        fclose(written);
    }

    /* ---- the status bar badge ------------------------------------------- */
    sm.log_n = 0;
    assert(sm_unread_total() == 0 && sm_hud_badge(ren, 300, 14) == 0);
    sm_log_push(them, "Pixel", 0, SM_KIND_TEXT, -1, "unread", NULL, 0);
    assert(sm_unread_total() == 1 && sm_hud_badge(ren, 300, 14) > 0);

    /* ---- a conversation outlives the other device being switched off ---- */
    /* The list used to be built from live beacons alone, so a thread vanished
       the moment its device went quiet -- which reads exactly like the history
       having been lost, even though the file on disk was fine. */
    sm.log_n = 0; sm.ink_next = 0; sm.n = 0;
    sm_test_blank(&p, SM_BEACON, them, "");
    sm_test_deliver(&p, 60);
    assert(sm.n == 1);
    sm_log_push(them, "Pixel", 0, SM_KIND_TEXT, -1, "see you tomorrow", NULL, 0);
    /* Their handheld goes off: no beacon for a good while. */
    sm.peers[0].seen = SDL_GetTicks() - 60000;
    sm.beacon = SDL_GetTicks();
    sm_tick();
    assert(sm.n == 1 && !sm_peer_online(0));      /* still listed, marked away */
    /* Someone we have never spoken to does drop off. */
    sm_test_blank(&p, SM_BEACON, other, "");
    snprintf(p.name, sizeof p.name, "Passer By");
    sm_test_deliver(&p, 61);
    assert(sm.n == 2);
    sm.peers[1].seen = SDL_GetTicks() - 60000;
    sm.beacon = SDL_GetTicks();
    sm_tick();
    assert(sm.n == 1 && !strcmp(sm.peers[0].id, them));

    /* A restart with nobody nearby at all: the contact and its history are
       both back, and the thread opens. */
    sm_save();
    sm.n = 0; sm.log_n = 0; sm.ink_next = 0;
    sm_load(); sm_people_load();
    assert(sm.n == 1 && !strcmp(sm.peers[0].id, them) && !strcmp(sm.peers[0].name, "Pixel"));
    assert(!sm_peer_online(0));                   /* known, but not here */
    sm.tab = SM_TAB_PEOPLE; sm.sel = 0; sm.view = SM_VIEW_LIST;
    assert(sm_rows() == 1);
    sm_key(SDLK_RETURN, &state);
    assert(sm.view == SM_VIEW_CHAT && !strcmp(sm.key, them) && sm_conv_count() == 1);
    assert(!strcmp(sm_conv_at(0)->text, "see you tomorrow"));
    sm.view = SM_VIEW_LIST;

    /* Threads are listed newest first. */
    {
        long now_o = (long)time(NULL);
        sm_test_blank(&p, SM_BEACON, other, "");
        snprintf(p.name, sizeof p.name, "Rook");
        sm_test_deliver(&p, 62);
        assert(sm.n == 2);
        sm_log_push(other, "Rook", 0, SM_KIND_TEXT, -1, "newer", NULL, now_o);
        int order[SM_PEERS], n = sm_people_order(order, SM_PEERS);
        assert(n == 2);
        assert(!strcmp(sm.peers[order[0]].id, other));     /* most recent first */
        assert(sm_peer_last_at(order[0]) >= sm_peer_last_at(order[1]));
    }

    /* ---- a save that cannot be committed keeps the log ------------------- */
    /* rename() was not being checked and the dirty flag was cleared either
       way, so a filesystem that refused the commit silently ate the log. */
    {
        /* Standing a non-empty directory where the file belongs lets the write
           succeed and the commit fail, which is the case the old code got
           wrong -- it is not reachable by making the whole path unwritable. */
        char path[800], blocker[900];
        sm_path(path, sizeof path, "messages.cfg");
        remove(path);
        assert(mkdir(path, 0755) == 0);
        snprintf(blocker, sizeof blocker, "%s/in-the-way", path);
        FILE *b = fopen(blocker, "w");
        assert(b); fclose(b);

        sm.log_n = 0; sm.ink_next = 0; sm.dirty = 0;
        sm_log_push(them, "Pixel", 0, SM_KIND_TEXT, -1, "must survive", NULL, 0);
        assert(sm.dirty);
        sm_save();
        assert(sm.dirty);                          /* still owed to the disk */
        assert(sm.log_n == 1 && !strcmp(sm.log[0].text, "must survive"));

        /* Clear the way and it lands, with nothing lost in between. */
        remove(blocker);
        assert(rmdir(path) == 0);
        sm_save();
        assert(!sm.dirty);
        sm.log_n = 0; sm.ink_next = 0;
        sm_load();
        assert(sm.log_n == 1 && !strcmp(sm.log[0].text, "must survive"));
    }

    /* ---- browsing, and opening a drawing at a size worth looking at ------ */
    sm.log_n = 0; sm.ink_next = 0; sm.pick = -1; sm.photo = -1; sm.status[0] = 0;
    snprintf(sm.key, sizeof sm.key, "%s", them);
    snprintf(sm.title, sizeof sm.title, "Pixel");
    memset(c, 0, sizeof c);
    sm_stroke(c, 8, 38, 46, 8, 1, 2); sm_stroke(c, 46, 8, 86, 38, 1, 2);
    sm_log_push(them, "Pixel", 0, SM_KIND_TEXT, -1, "look at this", NULL, 0);
    sm_log_push(them, "Pixel", 0, SM_KIND_DRAW, -1, NULL, c, 0);
    sm.view = SM_VIEW_CHAT;
    state = STATE_MESSAGES;

    /* With nothing highlighted, A is still the composer -- opening a picture
       must not cost you the button you press most. */
    sm_key(SDLK_RETURN, &state);
    assert(state == STATE_KEYBOARD && kb_purpose == KB_PURPOSE_MESSAGE);
    /* The keyboard is shared and titles itself from what opened it. Left to
       its own default it reads "API KEY", which is not what you are typing. */
    assert(strstr(sm_compose_title(), "MESSAGE TO") && strstr(sm_compose_title(), "Pixel"));
    {   char keep[34];
        snprintf(keep, sizeof keep, "%s", sm.key);
        snprintf(sm.key, sizeof sm.key, "#0");
        assert(strstr(sm_compose_title(), "LOBBY"));
        snprintf(sm.key, sizeof sm.key, "%s", keep);
    }
    state = STATE_MESSAGES;
    /* Up steps into the history at the newest line. */
    sm_key(SDLK_UP, &state);
    assert(sm.pick == sm_conv_count() - 1);
    /* A opens the drawing, full size and pixel for pixel. */
    sm_key(SDLK_RETURN, &state);
    assert(sm.view == SM_VIEW_PHOTO && sm_photo_ink());
    assert(!memcmp(sm_photo_ink(), c, SM_CANVAS_BYTES));
    /* Bigger than the thumbnail in the bubble, or there was no point opening. */
    assert(sm_photo_scale() > 3);
    sm_key(SDLK_ESCAPE, &state);
    assert(sm.view == SM_VIEW_CHAT && sm.pick >= 0);
    /* A line of text says so rather than opening an empty viewer. */
    sm_key(SDLK_UP, &state);
    assert(sm.pick == 0);
    sm_key(SDLK_RETURN, &state);
    assert(sm.view == SM_VIEW_CHAT && strstr(sm.status, "Only drawings"));
    /* Up at the oldest line stays there instead of running off the end. */
    sm_key(SDLK_UP, &state);
    assert(sm.pick == 0);
    /* Down past the newest hands the buttons back to the composer. */
    sm_key(SDLK_DOWN, &state); sm_key(SDLK_DOWN, &state);
    assert(sm.pick == -1 && sm.scroll == 0);
    sm_key(SDLK_RETURN, &state);
    assert(state == STATE_KEYBOARD);
    state = STATE_MESSAGES;

    /* ---- keeping one as an ordinary file -------------------------------- */
    sm.view = SM_VIEW_PHOTO; sm.photo = sm.log[1].ink; sm.status[0] = 0;
    sm_key(SDLK_s, &state);
    assert(strstr(sm.status, "Saved to messages/"));
    {
        char dir[800], path[1000], first[200];
        snprintf(dir, sizeof dir, "%s/messages", sn_data_root());
        snprintf(first, sizeof first, "%s", strstr(sm.status, "messages/") + 9);
        snprintf(path, sizeof path, "%s/%s", dir, first);
        SDL_Surface *back = IMG_Load(path);
        assert(back);
        /* A real image at a whole-number blow-up, not the packed 1152 bytes. */
        assert(back->w == SM_CANVAS_W * SM_SAVE_SCALE && back->h == SM_CANVAS_H * SM_SAVE_SCALE);
        SDL_FreeSurface(back);
        /* Saving the same drawing twice in one second keeps both. */
        sm.status[0] = 0;
        sm_key(SDLK_s, &state);
        assert(strstr(sm.status, "Saved to messages/"));
        assert(strcmp(strstr(sm.status, "messages/") + 9, first));
        char second[1000];
        snprintf(second, sizeof second, "%s/%s", dir, strstr(sm.status, "messages/") + 9);
        remove(path); remove(second);
    }
    /* Y hands the drawing to the wallpaper picker rather than guessing which
       system the user meant. */
    sm.status[0] = 0;
    sm_key(SDLK_f, &state);
    assert(state == STATE_BG_TARGET && bg_target_purpose == BG_TARGET_DRAWING);
    bg_target_purpose = BG_TARGET_ONLINE;
    state = STATE_MESSAGES;
    sm.view = SM_VIEW_PHOTO;
    sm_render(ren); capture(ren, "messages-drawing-open");
    sm.view = SM_VIEW_CHAT; sm.pick = -1; sm.photo = -1; sm.status[0] = 0;

    /* ---- screens -------------------------------------------------------- */
    sm.view = SM_VIEW_LIST; sm.tab = SM_TAB_PEOPLE; sm.sel = 0;
    sm_render(ren); capture(ren, "messages-nearby");
    sm.tab = SM_TAB_ROOMS; sm.sel = 1;
    sm_render(ren); capture(ren, "messages-rooms");
    sm.tab = SM_TAB_PEOPLE; sm.sel = 0;
    sm.log_n = 0; sm.ink_next = 0;
    snprintf(sm.key, sizeof sm.key, "%s", them);
    snprintf(sm.title, sizeof sm.title, "Pixel");
    sm_log_push(them, "Pixel", 0, SM_KIND_TEXT, -1, "hey! made you something", NULL, 0);
    memset(c, 0, sizeof c);
    sm_stroke(c, 8, 38, 46, 8, 1, 2); sm_stroke(c, 46, 8, 86, 38, 1, 2);
    sm_stroke(c, 14, 30, 80, 30, 3, 1);
    sm_log_push(them, "Pixel", 0, SM_KIND_DRAW, -1, NULL, c, 0);
    sm_log_push(them, sm_me_name(), 1, SM_KIND_TEXT, -1, "that is excellent, thank you", NULL, 0);
    sm_log_push(them, sm_me_name(), 1, SM_KIND_EMOJI, 5, NULL, NULL, 0);
    sm.log[sm.log_n - 1].read = 1;           /* they have opened it */
    sm.view = SM_VIEW_CHAT; sm.scroll = 0;
    sm_render(ren); capture(ren, "messages-chat");
    sm.view = SM_VIEW_EMOJI; sm.emoji_sel = 6;
    sm_render(ren); capture(ren, "messages-emoji");
    memcpy(sm.canvas, c, SM_CANVAS_BYTES);
    sm.view = SM_VIEW_DRAW; sm.cx = 46; sm.cy = 20; sm.brush = 2; sm.color = 2;
    sm.status[0] = 0;
    sm_render(ren); capture(ren, "messages-draw");
    /* The room view names who said what; a direct message does not need to. */
    snprintf(sm.key, sizeof sm.key, "#0");
    snprintf(sm.title, sizeof sm.title, "%s", sm_room_names[0]);
    sm_log_push("#0", "Pixel", 0, SM_KIND_TEXT, -1, "who is around?", NULL, 0);
    sm_log_push("#0", "Rook", 0, SM_KIND_TEXT, -1, "me, just finished a run", NULL, 0);
    sm_log_push("#0", sm_me_name(), 1, SM_KIND_TEXT, -1, "same, that last stage was brutal", NULL, 0);
    sm.view = SM_VIEW_CHAT;
    sm_render(ren); capture(ren, "messages-room");

    /* ---- leave things as we found them ---------------------------------- */
    close(sm_test_peerfd); sm_test_peerfd = -1;
    if (sm.fd >= 0) close(sm.fd);
    sm.fd = -1; sm.initialized = 0;
    sm.log_n = 0; sm.n = 0; sm.ink_next = 0; sm.dirty = 0;
    sm.view = SM_VIEW_LIST; sm.sel = 0; sm.key[0] = 0;
    { char path[768]; sm_path(path, sizeof path, "messages.cfg"); remove(path); }

    puts("PASS: SNAP Chat discovers nearby devices, keeps rooms and direct messages apart, acknowledges without duplicating, "
         "carries a whole drawing in one datagram, and survives a restart");
}
#endif
