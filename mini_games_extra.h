#ifndef SNAP_FE_MINI_GAMES_EXTRA_H
#define SNAP_FE_MINI_GAMES_EXTRA_H

/*
 * SNAP FE mini-games expansion (1.2.9)
 *
 * This header deliberately contains implementation: include it once in main.c
 * immediately after Duck Dash and before the mini-games picker.  It expects the
 * existing mini-game helpers/state (mg_theme, mg_chrome, mg_icon, mg_games,
 * mg_best, mg_set_best, render_text, render_text_fit, fill_rounded, pg and fb)
 * to have been declared already.
 *
 * Suggested IDs in main.c:
 *   MG_RUNNER 7, MG_ROAD 8, MG_SWAP 9, MG_REACTION 10,
 *   MG_TANK 11, MG_TIDEPOOL 12, MG_COUNT 13.
 *
 * All names, level layouts and procedural artwork here are original.  The
 * aquarium chapter uses the broad feed/grow/earn/defend loop requested by the
 * project, but does not use third-party characters, names, art or level data.
 */

#ifndef MG_RUNNER
#define MG_RUNNER   7
#define MG_ROAD     8
#define MG_SWAP     9
#define MG_REACTION 10
#define MG_TANK     11
#define MG_TIDEPOOL 12
#endif

#define MGX_PI 3.14159265358979323846f

static int mgx_clampi(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }
static float mgx_clampf(float v, float lo, float hi) { return v < lo ? lo : v > hi ? hi : v; }
static float mgx_len2(float x, float y) { return x*x + y*y; }
static int mgx_hit(float ax, float ay, float aw, float ah,
                   float bx, float by, float bw, float bh) {
    return ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by;
}

static void mgx_text(SDL_Renderer *ren, TTF_Font *font, const char *s,
                     SDL_Color c, int x, int y) {
    SDL_Texture *t = render_text_fit(ren, font, s, c, WIN_W-x-26);
    if (!t) return;
    int w = 0, h = 0; SDL_QueryTexture(t, NULL, NULL, &w, &h);
    SDL_RenderCopy(ren, t, NULL, &(SDL_Rect){x, y, w, h});
}

static void mgx_text_center(SDL_Renderer *ren, TTF_Font *font, const char *s,
                            SDL_Color c, int cx, int y) {
    SDL_Texture *t = render_text_fit(ren, font, s, c, WIN_W - 52);
    if (!t) return;
    int w = 0, h = 0; SDL_QueryTexture(t, NULL, NULL, &w, &h);
    SDL_RenderCopy(ren, t, NULL, &(SDL_Rect){cx - w/2, y, w, h});
}

static void mgx_panel(SDL_Renderer *ren, SDL_Rect r, SDL_Color fill,
                      SDL_Color edge, int alpha) {
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
    fill_rounded(ren, r, 10, fill.r, fill.g, fill.b, (Uint8)mgx_clampi(alpha, 0, 255));
    SDL_SetRenderDrawColor(ren, edge.r, edge.g, edge.b, 255);
    SDL_RenderDrawRect(ren, &r);
}

static void mgx_overlay_message(SDL_Renderer *ren, const char *title, const char *sub) {
    Theme *th = mg_theme();
    int w = WIN_W > 680 ? 500 : 460, h = sub ? 122 : 62;
    SDL_Rect p = { WIN_W/2 - w/2, WIN_H/2 - h/2, w, h };
    mgx_panel(ren, p, th->bg, th->accent2, 255);
    SDL_Texture *head=render_text_fit(ren,font_small_bold?font_small_bold:font_small,title,th->accent2,w-32);
    int tw,hh;if(head){SDL_QueryTexture(head,NULL,NULL,&tw,&hh);SDL_RenderCopy(ren,head,NULL,&(SDL_Rect){p.x+(w-tw)/2,p.y+14,tw,hh});}
    if(sub){
        char lines[MAX_LINES][128];int n=wrap_text(font_label,sub,w-36,lines);
        for(int i=0;i<n&&i<2;i++){
            SDL_Texture *body=render_text_fit(ren,font_label,lines[i],g_ui_dim,w-36);
            if(body){SDL_QueryTexture(body,NULL,NULL,&tw,&hh);SDL_RenderCopy(ren,body,NULL,&(SDL_Rect){p.x+(w-tw)/2,p.y+55+i*28,tw,hh});}
        }
    }
}

static void mgx_diamond(SDL_Renderer *ren, int cx, int cy, int r, SDL_Color c) {
    SDL_SetRenderDrawColor(ren, c.r, c.g, c.b, 255);
    for (int y = -r; y <= r; y++) {
        int hw = r - abs(y);
        SDL_RenderDrawLine(ren, cx - hw, cy + y, cx + hw, cy + y);
    }
}

static void mgx_circle(SDL_Renderer *ren, int cx, int cy, int r, SDL_Color c, int alpha) {
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(ren, c.r, c.g, c.b, (Uint8)mgx_clampi(alpha, 0, 255));
    for (int y = -r; y <= r; y++) {
        int hw = (int)sqrtf((float)(r*r - y*y));
        SDL_RenderDrawLine(ren, cx - hw, cy + y, cx + hw, cy + y);
    }
}

/* ---------------------- Genre-organized picker ----------------------- */

#define MGX_GENRE_COUNT 4
static const char *mgx_genre_names[MGX_GENRE_COUNT] = {
    "ARCADE", "PUZZLE", "ACTION", "STRATEGY"
};
static int mgx_genre_sel = 0;

static int mgx_game_genre(int id) {
    if (id == MG_TTT || id == MG_BLOXORZ || id == MG_SWAP) return 1;
    if (id == MG_RUNNER || id == MG_ROAD || id == MG_REACTION || id == MG_TANK) return 2;
    if (id == MG_TIDEPOOL) return 3;
    return 0;
}

static int mgx_genre_list(int genre, int *out) {
    int n = 0;
    for (int i = 0; i < MG_COUNT; i++) if (mgx_game_genre(i) == genre) out[n++] = i;
    return n;
}

static void mgx_menu_enter(void) {
    mgx_genre_sel = mgx_game_genre(mg_menu_sel);
}

/* Returns the selected game ID for A, otherwise -1. */
static int mgx_menu_key(SDL_Keycode k) {
    int ids[MG_COUNT], n = mgx_genre_list(mgx_genre_sel, ids), pos = 0;
    for (int i = 0; i < n; i++) if (ids[i] == mg_menu_sel) pos = i;
    if (k == SDLK_LEFT || k == SDLK_RIGHT || k == SDLK_q || k == SDLK_e) {
        int d = (k == SDLK_RIGHT || k == SDLK_e) ? 1 : -1;
        mgx_genre_sel = (mgx_genre_sel + d + MGX_GENRE_COUNT) % MGX_GENRE_COUNT;
        n = mgx_genre_list(mgx_genre_sel, ids);
        if (n > 0) mg_menu_sel = ids[0];
    } else if (k == SDLK_UP || k == SDLK_DOWN) {
        if (n > 0) {
            pos = (pos + (k == SDLK_DOWN ? 1 : -1) + n) % n;
            mg_menu_sel = ids[pos];
        }
    } else if (k == SDLK_RETURN) return mg_menu_sel;
    return -1;
}

static void mgx_icon(SDL_Renderer *ren, int id, int x, int y, int s, SDL_Color c) {
    if (id < MG_RUNNER) { mg_icon(ren, id, x, y, s, c); return; }
    SDL_SetRenderDrawColor(ren, c.r, c.g, c.b, 255);
    int q = s/4; if (q < 3) q = 3;
    if (id == MG_RUNNER) {
        SDL_RenderFillRect(ren, &(SDL_Rect){x+2, y+s-q-2, s-4, 2});
        SDL_RenderFillRect(ren, &(SDL_Rect){x+q, y+s-2*q-2, q, q});
        SDL_RenderDrawLine(ren, x+2*q+2, y+s-q-3, x+3*q, y+q);
        SDL_RenderDrawLine(ren, x+3*q, y+q, x+s-2, y+s-q-3);
    } else if (id == MG_ROAD) {
        SDL_RenderDrawLine(ren, x+q, y+2, x+q, y+s-2);
        SDL_RenderDrawLine(ren, x+s-q, y+2, x+s-q, y+s-2);
        SDL_RenderFillRect(ren, &(SDL_Rect){x+2, y+s/2-3, s-4, 6});
    } else if (id == MG_SWAP) {
        for (int yy=0; yy<2; yy++) for (int xx=0; xx<2; xx++)
            SDL_RenderDrawRect(ren, &(SDL_Rect){x+xx*(s/2)+2,y+yy*(s/2)+2,s/2-4,s/2-4});
    } else if (id == MG_REACTION) {
        SDL_RenderDrawLine(ren,x+s/2,y+1,x+q,y+s/2);
        SDL_RenderDrawLine(ren,x+q,y+s/2,x+s/2,y+s-1);
        SDL_RenderDrawLine(ren,x+s/2,y+s-1,x+s-q,y+s/2);
        SDL_RenderDrawLine(ren,x+s-q,y+s/2,x+s/2,y+1);
        SDL_RenderFillRect(ren,&(SDL_Rect){x+s/2-2,y+q,4,s-2*q});
    } else if (id == MG_TANK) {
        SDL_RenderFillRect(ren,&(SDL_Rect){x+q/2,y+q,s-q,2*q});
        SDL_RenderFillRect(ren,&(SDL_Rect){x+s/2-3,y+2,6,s-q});
        SDL_RenderDrawLine(ren,x+s/2,y+q,x+s-2,y+2);
    } else {
        SDL_RenderDrawRect(ren,&(SDL_Rect){x+2,y+q,s-4,s-2*q});
        SDL_RenderDrawLine(ren,x+q,y+s/2,x+2*q,y+q);
        SDL_RenderDrawLine(ren,x+q,y+s/2,x+2*q,y+s-q);
        mgx_circle(ren,x+3*q,y+s/2,q/2,c,255);
    }
}

static void mgx_render_menu(SDL_Renderer *ren) {
    Theme *th = mg_theme();
    int ids[MG_COUNT], n=mgx_genre_list(mgx_genre_sel,ids), pos=0;
    draw_dock_logo(ren, font_small);
    mgx_text(ren, font_small, "MINI GAMES", th->accent2, 34, 51);
    char total[48];
    snprintf(total,sizeof total,"%d %s in %s",n,n==1?"game":"games",mgx_genre_names[mgx_genre_sel]);
    SDL_Texture *count_label=render_text_fit(ren,font_label,total,g_ui_dim,WIN_W/2-42);
    if(count_label){int w,h;SDL_QueryTexture(count_label,NULL,NULL,&w,&h);float scale=fminf(1,22.0f/h);w=(int)(w*scale);h=(int)(h*scale);SDL_RenderCopy(ren,count_label,NULL,&(SDL_Rect){WIN_W-34-w,55,w,h});}

    int tabx = 30, tabw = (WIN_W - 60) / MGX_GENRE_COUNT;
    for (int i=0; i<MGX_GENRE_COUNT; i++) {
        SDL_Rect tr={tabx+i*tabw,82,tabw-5,30};
        SDL_Color bg=i==mgx_genre_sel?th->select_bg:th->bg;
        mgx_panel(ren,tr,bg,i==mgx_genre_sel?th->accent2:th->dim,i==mgx_genre_sel?255:90);
        SDL_Texture *label=render_text_fit(ren,font_fixed?font_fixed:font_label,mgx_genre_names[i],i==mgx_genre_sel?g_ui_text:g_ui_dim,tr.w-16);
        if(label){int w,h;SDL_QueryTexture(label,NULL,NULL,&w,&h);float scale=fminf(1,20.0f/h);w=(int)(w*scale);h=(int)(h*scale);SDL_RenderCopy(ren,label,NULL,&(SDL_Rect){tr.x+(tr.w-w)/2,tr.y+(tr.h-h)/2,w,h});}
    }

    for(int i=0;i<n;i++) if(ids[i]==mg_menu_sel) pos=i;
    int visible=4, first=pos-visible+1; if(first<0)first=0;
    if(first>n-visible)first=n-visible;
    if(first<0)first=0;
    int top=124,rowh=70,lx=34,rw=WIN_W-68;
    for(int j=0;j<visible && first+j<n;j++) {
        int id=ids[first+j], sel=id==mg_menu_sel;
        SDL_Rect r={lx,top+j*rowh,rw,rowh-8};
        mgx_panel(ren,r,th->select_bg,sel?th->accent2:th->dim,sel?248:75);
        SDL_Rect ib={r.x+10,r.y+9,44,44};
        SDL_SetRenderDrawColor(ren,th->bg.r,th->bg.g,th->bg.b,245); SDL_RenderFillRect(ren,&ib);
        mgx_icon(ren,id,ib.x+5,ib.y+5,ib.w-10,sel?th->accent2:g_ui_dim);
        int tx=ib.x+ib.w+13;
        char best[28]; snprintf(best,sizeof best,"%s %d",mg_games[id].best_label,mg_best[id]);
        int best_w=0,best_h=0;TTF_SizeUTF8(font_label,best,&best_w,&best_h);
        int title_max=r.x+r.w-best_w-24-tx;
        SDL_Texture *name=render_text_fit(ren,font_label,mg_games[id].name,sel?g_ui_text:g_ui_dim,title_max);
        if(name){int w,h;SDL_QueryTexture(name,NULL,NULL,&w,&h);SDL_RenderCopy(ren,name,NULL,&(SDL_Rect){tx,r.y+9,w,h});}
        SDL_Texture *tag=render_text_fit(ren,font_fixed?font_fixed:font_label,
                                         mg_games[id].tagline,g_ui_dim,r.w-84);
        if(tag){int w,h;SDL_QueryTexture(tag,NULL,NULL,&w,&h);SDL_RenderCopy(ren,tag,NULL,&(SDL_Rect){tx,r.y+34,w,h});}
        SDL_Texture *bt=render_text(ren,font_label,best,th->accent2);
        if(bt){int w,h;SDL_QueryTexture(bt,NULL,NULL,&w,&h);SDL_RenderCopy(ren,bt,NULL,&(SDL_Rect){r.x+r.w-w-12,r.y+9,w,h});}
    }
    if(n>visible){
        SDL_Rect track={WIN_W-23,top,5,visible*rowh-8};
        int thumb_h=track.h*visible/n;if(thumb_h<14)thumb_h=14;
        int thumb_y=track.y+(track.h-thumb_h)*first/(n-visible);
        SDL_SetRenderDrawColor(ren,th->dim.r,th->dim.g,th->dim.b,75);SDL_RenderFillRect(ren,&track);
        SDL_SetRenderDrawColor(ren,th->accent2.r,th->accent2.g,th->accent2.b,255);
        SDL_RenderFillRect(ren,&(SDL_Rect){track.x,thumb_y,track.w,thumb_h});
    }
    SDL_Texture *controls=render_text_fit(ren,font_label,"Left/Right Genre    A Play    B Back",g_ui_dim,WIN_W-68);
    if(controls){int w,h;SDL_QueryTexture(controls,NULL,NULL,&w,&h);SDL_RenderCopy(ren,controls,NULL,&(SDL_Rect){(WIN_W-w)/2,WIN_H-h-13,w,h});}
}

/* ---------------- Mini-game-only nearby UDP transport ---------------- */

#define MGX_NET_HOST_PORT 38429
#define MGX_NET_DISC_PORT 38430
#define MGX_NET_MAGIC 0x534E4D47u /* SNMG */
#define MGX_NET_VERSION 3
static int mgx_exit_confirm=0;
#define MGX_NET_WORDS 48
enum { MGX_PKT_BEACON=1, MGX_PKT_HELLO, MGX_PKT_ACK, MGX_PKT_INPUT, MGX_PKT_STATE };
enum { MGX_NET_OFF=0, MGX_NET_HOST, MGX_NET_JOIN };

typedef struct {
    uint32_t magic;
    unsigned char version, type, game, reserved;
    uint32_t session, seq;
    int16_t d[MGX_NET_WORDS];
} MgxPacket;

typedef struct {
    int fd, role, game, connected;
    struct sockaddr_in peer;
    uint32_t session, seq, rx_seq, last_beacon, last_tx, last_rx;
    int remote_x, remote_y, remote_action, remote_restart;
    char status[64];
} MgxNet;

static MgxNet mgx_net = { .fd = -1 };

static void mgx_net_close(void) {
    if (mgx_net.fd >= 0) close(mgx_net.fd);
    memset(&mgx_net,0,sizeof mgx_net); mgx_net.fd=-1;
}

static int mgx_net_socket(int port) {
    int fd=socket(AF_INET,SOCK_DGRAM,0); if(fd<0)return -1;
    int one=1; setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&one,sizeof one);
    setsockopt(fd,SOL_SOCKET,SO_BROADCAST,&one,sizeof one);
    /* A tiny interactive datagram should not sit behind bulk transfers. */
    int tos=0x10;setsockopt(fd,IPPROTO_IP,IP_TOS,&tos,sizeof tos);
    int fl=fcntl(fd,F_GETFL,0); if(fl>=0)fcntl(fd,F_SETFL,fl|O_NONBLOCK);
    struct sockaddr_in a; memset(&a,0,sizeof a); a.sin_family=AF_INET;
    a.sin_addr.s_addr=htonl(INADDR_ANY); a.sin_port=htons((unsigned short)port);
    if(bind(fd,(struct sockaddr*)&a,sizeof a)!=0){close(fd);return -1;}
    return fd;
}

static void mgx_net_packet(MgxPacket *p,int type,int game) {
    memset(p,0,sizeof *p); p->magic=htonl(MGX_NET_MAGIC); p->version=MGX_NET_VERSION;
    p->type=(unsigned char)type; p->game=(unsigned char)game;
    p->session=htonl(mgx_net.session); p->seq=htonl(++mgx_net.seq);
}

static void mgx_net_send(const MgxPacket *p,const struct sockaddr_in *to) {
    if(mgx_net.fd>=0 && to) sendto(mgx_net.fd,p,sizeof *p,0,(const struct sockaddr*)to,sizeof *to);
}

static int mgx_net_open(int role,int game) {
    mgx_net_close();
    int port=role==MGX_NET_HOST?MGX_NET_HOST_PORT:MGX_NET_DISC_PORT;
    mgx_net.fd=mgx_net_socket(port); if(mgx_net.fd<0)return 0;
    mgx_net.role=role; mgx_net.game=game; mgx_net.session=(uint32_t)SDL_GetTicks() ^ (uint32_t)getpid()*2654435761u;
    snprintf(mgx_net.status,sizeof mgx_net.status,role==MGX_NET_HOST?"Waiting for nearby player...":"Searching local network...");
    return 1;
}

static int mgx_net_peer_ok(const struct sockaddr_in *a) {
    return mgx_net.connected && a->sin_addr.s_addr==mgx_net.peer.sin_addr.s_addr && a->sin_port==mgx_net.peer.sin_port;
}

/* Handles discovery/handshake and returns the newest gameplay packet type. */
static int mgx_net_poll(int game,MgxPacket *latest) {
    if(mgx_net.fd<0)return 0;
    Uint32 now=SDL_GetTicks();
    if(mgx_net.role==MGX_NET_HOST && !mgx_net.connected && now-mgx_net.last_beacon>=500){
        mgx_net.last_beacon=now; MgxPacket p; mgx_net_packet(&p,MGX_PKT_BEACON,game);
        struct sockaddr_in b;memset(&b,0,sizeof b);b.sin_family=AF_INET;b.sin_port=htons(MGX_NET_DISC_PORT);b.sin_addr.s_addr=htonl(INADDR_BROADCAST);
        mgx_net_send(&p,&b);
    }
    if(mgx_net.role==MGX_NET_JOIN && !mgx_net.connected && mgx_net.peer.sin_port &&
       now-mgx_net.last_beacon>=300){
        mgx_net.last_beacon=now;MgxPacket h;mgx_net_packet(&h,MGX_PKT_HELLO,game);mgx_net_send(&h,&mgx_net.peer);
    }
    int got=0;
    for(int budget=0;budget<32;budget++){
        MgxPacket p; struct sockaddr_in from; socklen_t fn=sizeof from;
        ssize_t n=recvfrom(mgx_net.fd,&p,sizeof p,0,(struct sockaddr*)&from,&fn);
        if(n<0){if(errno==EINTR)continue;break;}
        if(n!=(ssize_t)sizeof p || ntohl(p.magic)!=MGX_NET_MAGIC || p.version!=MGX_NET_VERSION || p.game!=game)continue;
        int type=p.type;
        if(mgx_net.role==MGX_NET_JOIN && type==MGX_PKT_BEACON && !mgx_net.connected && !mgx_net.peer.sin_port){
            mgx_net.peer=from; mgx_net.peer.sin_port=htons(MGX_NET_HOST_PORT); mgx_net.session=ntohl(p.session);
            MgxPacket h;mgx_net_packet(&h,MGX_PKT_HELLO,game);mgx_net_send(&h,&mgx_net.peer);
            snprintf(mgx_net.status,sizeof mgx_net.status,"Found host - connecting...");
        } else if(mgx_net.role==MGX_NET_HOST && type==MGX_PKT_HELLO && ntohl(p.session)==mgx_net.session &&
                  (!mgx_net.connected || mgx_net_peer_ok(&from))){
            mgx_net.peer=from; mgx_net.connected=1;mgx_net.last_rx=now;
            MgxPacket a;mgx_net_packet(&a,MGX_PKT_ACK,game);mgx_net_send(&a,&mgx_net.peer);
            snprintf(mgx_net.status,sizeof mgx_net.status,"Player connected");
        } else if(mgx_net.role==MGX_NET_JOIN && type==MGX_PKT_ACK && ntohl(p.session)==mgx_net.session &&
                  from.sin_addr.s_addr==mgx_net.peer.sin_addr.s_addr && from.sin_port==mgx_net.peer.sin_port){
            mgx_net.peer=from;mgx_net.connected=1;mgx_net.last_rx=now;
            snprintf(mgx_net.status,sizeof mgx_net.status,"Connected to host");
        } else if(mgx_net_peer_ok(&from) && ntohl(p.session)==mgx_net.session &&
                  type==(mgx_net.role==MGX_NET_HOST?MGX_PKT_INPUT:MGX_PKT_STATE) &&
                  (int32_t)(ntohl(p.seq)-mgx_net.rx_seq)>0){
            mgx_net.rx_seq=ntohl(p.seq);mgx_net.last_rx=now;if(latest)*latest=p;got=type;
        }
    }
    if(mgx_net.connected && now-mgx_net.last_rx>4200){close(mgx_net.fd);mgx_net.fd=-1;mgx_net.connected=0;snprintf(mgx_net.status,sizeof mgx_net.status,"Connection lost - B to leave");}
    return got;
}

static void mgx_net_send_input(int game,int x,int y,int action,int restart) {
    if(!mgx_net.connected || mgx_net.fd<0)return;
    Uint32 now=SDL_GetTicks(); if(now-mgx_net.last_tx<16)return; mgx_net.last_tx=now;
    MgxPacket p;mgx_net_packet(&p,MGX_PKT_INPUT,game);
    p.d[0]=htons((int16_t)x);p.d[1]=htons((int16_t)y);p.d[2]=htons((int16_t)action);p.d[3]=htons((int16_t)restart);
    mgx_net_send(&p,&mgx_net.peer);
}

static int mgx_pkt_i(const MgxPacket *p,int i){return (int)(int16_t)ntohs((uint16_t)p->d[i]);}
static void mgx_pkt_set(MgxPacket *p,int i,int v){p->d[i]=htons((int16_t)mgx_clampi(v,-32768,32767));}
static uint32_t mgx_pkt_u32(const MgxPacket *p,int i){return ((uint32_t)ntohs((uint16_t)p->d[i])<<16)|ntohs((uint16_t)p->d[i+1]);}
static void mgx_pkt_set_u32(MgxPacket *p,int i,uint32_t v){p->d[i]=htons((uint16_t)(v>>16));p->d[i+1]=htons((uint16_t)v);}

/* ---------------- Existing Pong: CPU / Nearby Host / Nearby Join ------ */

static int mgx_pong_ready=0,mgx_pong_remote_ready=0;
static Uint32 mgx_pong_launch=0,mgx_pong_frame=0;
static int mgx_pong_round=1;
static int mgx_pong_lobby=1,mgx_pong_choice=0,mgx_pong_mode=0;
static int mgx_pong_remote_mv=0,mgx_pong_action=0,mgx_pong_remote_action=0,mgx_pong_scored=0;
typedef struct {
    Uint32 peer_sent,peer_received,remote_at;
    double clock_offset,best_rtt;
    int clock_valid,remote_mv;
    float ball_dx,ball_dy,paddle_dy;
} MgxPongSync;
static MgxPongSync mgx_pong_sync;

/* Four timestamps measure transit time without assuming matching device
   clocks. The lowest RTT sample avoids learning Wi-Fi queueing as clock skew. */
static float mgx_pong_packet_age(const MgxPacket *p,int sent_word,int echo_word,int received_word,Uint32 now){
    Uint32 sent=mgx_pkt_u32(p,sent_word),echo=mgx_pkt_u32(p,echo_word),received=mgx_pkt_u32(p,received_word);
    int32_t turnaround=(int32_t)(sent-received),elapsed=(int32_t)(now-echo);
    double rtt=(double)elapsed-turnaround;
    if(echo&&received&&turnaround>=0&&elapsed>=0&&rtt>=0&&rtt<=1000 &&
       (!mgx_pong_sync.clock_valid||rtt<=mgx_pong_sync.best_rtt+2)){
        mgx_pong_sync.best_rtt=rtt;
        mgx_pong_sync.clock_offset=((double)(int32_t)(received-echo)+(double)(int32_t)(sent-now))*.5;
        mgx_pong_sync.clock_valid=1;
    }
    mgx_pong_sync.peer_sent=sent;mgx_pong_sync.peer_received=now;
    return mgx_pong_sync.clock_valid?mgx_clampf((float)((double)(int32_t)(now-sent)+mgx_pong_sync.clock_offset),0,180):0;
}
static void mgx_pong_visual_decay(float ms){
    float keep=expf(-ms/45.0f);
    mgx_pong_sync.ball_dx*=keep;mgx_pong_sync.ball_dy*=keep;mgx_pong_sync.paddle_dy*=keep;
}
static void mgx_pong_ball_advance(float ms){
    /* Small steps preserve paddle collisions when a frame or packet is late.
       Prediction never awards points: only the host changes the score. */
    for(float left=mgx_clampf(ms,0,180);left>.001f;){
        float step=fminf(left,4.0f),t=step/16.6667f;left-=step;
        pg.bx+=pg.bvx*t;pg.by+=pg.bvy*t;
        if(pg.by<pg.top+pg.br){pg.by=2*(pg.top+pg.br)-pg.by;pg.bvy=fabsf(pg.bvy);}
        if(pg.by>pg.bot-pg.br){pg.by=2*(pg.bot-pg.br)-pg.by;pg.bvy=-fabsf(pg.bvy);}
        if(pg.bvx<0&&pg.bx-pg.br<=pg.left+pg.pw&&pg.bx>pg.left-6&&fabsf(pg.by-pg.you)<=pg.ph/2.0f+pg.br){
            pg.bx=pg.left+pg.pw+pg.br;pg.bvx=fminf(9.5f,fabsf(pg.bvx)+.3f);pg.bvy=mgx_clampf(pg.bvy+(pg.by-pg.you)*.13f,-12,12);}
        if(pg.bvx>0&&pg.bx+pg.br>=pg.right-pg.pw&&pg.bx<pg.right+6&&fabsf(pg.by-pg.cpu)<=pg.ph/2.0f+pg.br){
            pg.bx=pg.right-pg.pw-pg.br;pg.bvx=-fminf(9.5f,fabsf(pg.bvx)+.3f);pg.bvy=mgx_clampf(pg.bvy+(pg.by-pg.cpu)*.13f,-12,12);}
        pg.bx=mgx_clampf(pg.bx,pg.left-40,pg.right+40);
    }
}

static void mgx_pong_reset(void){memset(&mgx_pong_sync,0,sizeof mgx_pong_sync);mgx_pong_round=1;mgx_pong_remote_mv=0;mgx_pong_ready=mgx_pong_remote_ready=0;mgx_pong_launch=0;mgx_pong_frame=SDL_GetTicks();mgx_net_close();pong_reset();mgx_pong_lobby=1;mgx_pong_choice=0;mgx_pong_mode=0;mgx_pong_action=mgx_pong_remote_action=0;mgx_pong_scored=0;}

static void mgx_pong_key(SDL_Keycode k){
    if(mgx_pong_lobby){
        if(k==SDLK_LEFT)mgx_pong_choice=(mgx_pong_choice+2)%3;
        else if(k==SDLK_RIGHT)mgx_pong_choice=(mgx_pong_choice+1)%3;
        else if(k==SDLK_RETURN){
            mgx_pong_mode=mgx_pong_choice;mgx_pong_lobby=0;pong_reset();
            if(mgx_pong_mode==0)pong_key(SDLK_RETURN);
            else if(!mgx_net_open(mgx_pong_mode==1?MGX_NET_HOST:MGX_NET_JOIN,MG_PONG)){mgx_pong_lobby=1;}
        }
        return;
    }
    if(mgx_pong_mode==0){pong_key(k);return;}
    if(k==SDLK_RETURN && mgx_net.connected){mgx_pong_ready=1;}
}

static void mgx_pong_host_advance(int mine,int other,float ms){
    float t=mgx_clampf(ms,0,100)/16.6667f;
    if(!pg.started||pg.over)return;
    float lo=pg.top+pg.ph/2.0f,hi=pg.bot-pg.ph/2.0f;
    pg.you=mgx_clampf(pg.you+mine*6.4f*t,lo,hi);pg.cpu=mgx_clampf(pg.cpu+other*6.4f*t,lo,hi);
    mgx_pong_ball_advance(t*16.6667f);
    if(pg.bx<pg.left-18){pg.sc++;pong_serve(0);}else if(pg.bx>pg.right+18){pg.sy++;pong_serve(1);}
    if(pg.sy>=7||pg.sc>=7){pg.over=1;pg.winner=pg.sy>=7?1:2;if(pg.winner==1&&!mgx_pong_scored){mg_set_best(MG_PONG,mg_best[MG_PONG]+1);mgx_pong_scored=1;}}
}

static void mgx_pong_write_state(MgxPacket *p,Uint32 now,int mine){
    mgx_pkt_set(p,0,pg.sy);mgx_pkt_set(p,1,pg.sc);mgx_pkt_set(p,2,(int)(pg.bx*10));mgx_pkt_set(p,3,(int)(pg.by*10));
    mgx_pkt_set(p,4,(int)(pg.bvx*100));mgx_pkt_set(p,5,(int)(pg.bvy*100));mgx_pkt_set(p,6,(int)(pg.you*10));mgx_pkt_set(p,7,(int)(pg.cpu*10));
    mgx_pkt_set(p,8,pg.started);mgx_pkt_set(p,9,pg.over);mgx_pkt_set(p,10,pg.winner);mgx_pkt_set(p,11,mgx_pong_ready);mgx_pkt_set(p,12,mgx_pong_remote_ready);mgx_pkt_set(p,14,mgx_pong_round);
    mgx_pkt_set(p,13,mgx_pong_launch?(int)fmaxf(0,(float)(int32_t)(mgx_pong_launch-now)):0);
    mgx_pkt_set_u32(p,15,now);mgx_pkt_set_u32(p,17,mgx_pong_sync.peer_sent);mgx_pkt_set_u32(p,19,mgx_pong_sync.peer_received);mgx_pkt_set(p,21,mine);
}

static void mgx_pong_take_state_at(const MgxPacket *p,Uint32 now){
    float local=pg.cpu,oldx=pg.bx+mgx_pong_sync.ball_dx,oldy=pg.by+mgx_pong_sync.ball_dy,oldp=pg.you+mgx_pong_sync.paddle_dy;
    int old_started=pg.started,old_round=mgx_pong_round,old_sy=pg.sy,old_sc=pg.sc;
    float age=mgx_pong_packet_age(p,15,17,19,now);
    pg.sy=mgx_clampi(mgx_pkt_i(p,0),0,7);pg.sc=mgx_clampi(mgx_pkt_i(p,1),0,7);
    pg.bx=mgx_clampi(mgx_pkt_i(p,2),-200,(WIN_W+20)*10)/10.0f;
    pg.by=mgx_clampi(mgx_pkt_i(p,3),0,WIN_H*10)/10.0f;
    pg.bvx=mgx_clampi(mgx_pkt_i(p,4),-3000,3000)/100.0f;pg.bvy=mgx_clampi(mgx_pkt_i(p,5),-3000,3000)/100.0f;
    pg.you=mgx_clampi(mgx_pkt_i(p,6),0,WIN_H*10)/10.0f;pg.cpu=mgx_clampi(mgx_pkt_i(p,7),0,WIN_H*10)/10.0f;
    mgx_pong_remote_ready=!!mgx_pkt_i(p,11);
    int round=mgx_clampi(mgx_pkt_i(p,14),1,30000);if(round!=mgx_pong_round){mgx_pong_round=round;mgx_pong_ready=0;}
    int countdown=mgx_clampi(mgx_pkt_i(p,13),0,3000);mgx_pong_launch=countdown?now+(Uint32)fmaxf(1,countdown-age):0;
    pg.started=!!mgx_pkt_i(p,8);pg.over=!!mgx_pkt_i(p,9);pg.winner=mgx_clampi(mgx_pkt_i(p,10),0,2);
    mgx_pong_sync.remote_mv=mgx_clampi(mgx_pkt_i(p,21),-1,1);mgx_pong_sync.remote_at=now;
    if(old_started&&pg.started&&old_round==mgx_pong_round)pg.cpu=local;
    if(pg.started&&!pg.over){
        pg.you=mgx_clampf(pg.you+mgx_pong_sync.remote_mv*6.4f*age/16.6667f,pg.top+pg.ph/2.0f,pg.bot-pg.ph/2.0f);
        mgx_pong_ball_advance(age);
    }
    int continuous=old_started&&pg.started&&!pg.over&&old_round==mgx_pong_round&&old_sy==pg.sy&&old_sc==pg.sc;
    mgx_pong_sync.ball_dx=continuous&&fabsf(oldx-pg.bx)<120?oldx-pg.bx:0;
    mgx_pong_sync.ball_dy=continuous&&fabsf(oldy-pg.by)<120?oldy-pg.by:0;
    mgx_pong_sync.paddle_dy=continuous?mgx_clampf(oldp-pg.you,-70,70):0;
    if(!pg.over)mgx_pong_scored=0;
    if(pg.over&&pg.winner==2&&!mgx_pong_scored){mg_set_best(MG_PONG,mg_best[MG_PONG]+1);mgx_pong_scored=1;}
}
static void mgx_pong_take_state(const MgxPacket *p){mgx_pong_take_state_at(p,SDL_GetTicks());}

static void mgx_pong_take_input(const MgxPacket *p,Uint32 now){
    float age=mgx_pong_packet_age(p,4,6,8,now);
    if(mgx_pkt_i(p,3)!=mgx_pong_round)return;
    mgx_pong_remote_mv=mgx_clampi(mgx_pkt_i(p,1),-1,1);mgx_pong_remote_ready=!!mgx_pkt_i(p,2);
    mgx_pong_sync.remote_at=now;
    if(pg.started&&!pg.over&&mgx_pkt_i(p,10)){
        float old=pg.cpu+mgx_pong_sync.paddle_dy;
        pg.cpu=mgx_clampf(mgx_pkt_i(p,0)/10.0f+mgx_pong_remote_mv*6.4f*age/16.6667f,pg.top+pg.ph/2.0f,pg.bot-pg.ph/2.0f);
        mgx_pong_sync.paddle_dy=mgx_clampf(old-pg.cpu,-70,70);
    }
}
static void mgx_pong_client_advance(int mv,float ms,Uint32 now){
    if(!pg.started||pg.over)return;
    float t=mgx_clampf(ms,0,100)/16.6667f,lo=pg.top+pg.ph/2.0f,hi=pg.bot-pg.ph/2.0f;
    /* Our paddle never waits for a host echo, including on release. */
    pg.cpu=mgx_clampf(pg.cpu+mv*6.4f*t,lo,hi);
    if(now-mgx_pong_sync.remote_at<180){
        pg.you=mgx_clampf(pg.you+mgx_pong_sync.remote_mv*6.4f*t,lo,hi);
        mgx_pong_ball_advance(ms);
    }
}

static void mgx_pong_step(int mv){
    Uint32 now=SDL_GetTicks();float ms=fminf(now-mgx_pong_frame,100);mgx_pong_frame=now;
    if(mgx_pong_lobby)return;
    if(mgx_pong_mode==0){pong_step(mv);return;}
    mgx_pong_visual_decay(ms);
    MgxPacket p;int got=mgx_net_poll(MG_PONG,&p);
    if(mgx_pong_mode==1){
        if(got==MGX_PKT_INPUT)mgx_pong_take_input(&p,now);
        if(mgx_net.connected && mgx_pong_ready && mgx_pong_remote_ready && !mgx_pong_launch && !pg.started){
            pong_reset();mgx_pong_scored=0;mgx_pong_launch=now+3000;mgx_pong_sync.ball_dx=mgx_pong_sync.ball_dy=mgx_pong_sync.paddle_dy=0;
        }
        if(mgx_pong_launch && SDL_TICKS_PASSED(now,mgx_pong_launch)){pg.started=1;pg.last=now;mgx_pong_launch=0;}
        if(mgx_net.connected){int was_over=pg.over;mgx_pong_host_advance(mv,got!=MGX_PKT_INPUT&&now-mgx_pong_sync.remote_at<180?mgx_pong_remote_mv:0,ms);
            if(!was_over&&pg.over){mgx_pong_round=mgx_pong_round%30000+1;pg.started=0;mgx_pong_ready=mgx_pong_remote_ready=0;}}
        if(mgx_net.connected&&now-mgx_net.last_tx>=16){mgx_net.last_tx=now;mgx_net_packet(&p,MGX_PKT_STATE,MG_PONG);mgx_pong_write_state(&p,now,mv);mgx_net_send(&p,&mgx_net.peer);}
    }else{
        mgx_pong_client_advance(mv,ms,now);
        if(got==MGX_PKT_STATE)mgx_pong_take_state_at(&p,now);
        if(mgx_net.connected&&mgx_net.fd>=0&&now-mgx_net.last_tx>=16){
            mgx_net.last_tx=now;mgx_net_packet(&p,MGX_PKT_INPUT,MG_PONG);
            mgx_pkt_set(&p,0,(int)(pg.cpu*10));mgx_pkt_set(&p,1,mv);mgx_pkt_set(&p,2,mgx_pong_ready);mgx_pkt_set(&p,3,mgx_pong_round);
            mgx_pkt_set_u32(&p,4,now);mgx_pkt_set_u32(&p,6,mgx_pong_sync.peer_sent);mgx_pkt_set_u32(&p,8,mgx_pong_sync.peer_received);mgx_pkt_set(&p,10,pg.started);
            mgx_net_send(&p,&mgx_net.peer);
        }
    }
}

static void mgx_pong_render(SDL_Renderer *ren){
    int started=pg.started,over=pg.over;float bx=pg.bx,by=pg.by,you=pg.you,cpu=pg.cpu;
    if(mgx_pong_mode==2){pg.bx+=mgx_pong_sync.ball_dx;pg.by+=mgx_pong_sync.ball_dy;pg.you+=mgx_pong_sync.paddle_dy;}
    else if(mgx_pong_mode==1)pg.cpu+=mgx_pong_sync.paddle_dy;
    if(mgx_pong_lobby||mgx_pong_mode){pg.started=1;pg.over=0;}pong_render(ren);
    pg.bx=bx;pg.by=by;pg.you=you;pg.cpu=cpu;pg.started=started;pg.over=over;Theme *th=mg_theme();
    if(mgx_pong_lobby){
        const char *m[3]={"CPU BATTLE","LOCAL LINK HOST","LOCAL LINK JOIN"};
        mgx_overlay_message(ren,m[mgx_pong_choice],"Left/Right Mode    A Choose");
    }else if(mgx_pong_mode!=0&&!mgx_net.connected)mgx_overlay_message(ren,mgx_net.status,"Same Wi-Fi / local network required");
    else if(mgx_pong_mode!=0 && mgx_pong_launch){char c[32];snprintf(c,sizeof c,"%d",mgx_clampi(((int32_t)(mgx_pong_launch-SDL_GetTicks())+999)/1000,1,3));mgx_overlay_message(ren,c,"Get ready!");}
    else if(mgx_pong_mode!=0 && !pg.started)mgx_overlay_message(ren,mgx_pong_ready?"YOU ARE READY":"CONFIRM MATCH",mgx_pong_ready?"Waiting for your friend to confirm":"A Ready - both players must confirm");
    else if(mgx_pong_mode!=0)mgx_text_center(ren,font_label,mgx_pong_mode==1?"Nearby match - you are LEFT":"Nearby match - you are RIGHT",th->accent2,WIN_W/2,82);
}

/* Nearby Connect 4, using the same discovery transport as Tank and Pong. */
static int mgx_c4_lobby=1,mgx_c4_choice=0,mgx_c4_mode=0,mgx_c4_pending=-1;
static int mgx_c4_ply(void){int n=0;for(int i=0;i<42;i++)n+=tt.cell[i]!=0;return n;}
static void mgx_c4_reset(void){mgx_net_close();ttt_reset();mgx_c4_lobby=1;mgx_c4_choice=mgx_c4_mode=0;mgx_c4_pending=-1;}
static void mgx_c4_drop(int col,int who){
    if(tt.over||tt.turn!=who||c4_drop(tt.cell,col,who)<0)return;
    tt.winner=ttt_result(tt.cell);tt.over=tt.winner!=0;tt.turn=3-who;
}
static void mgx_c4_key(SDL_Keycode k){
    if(mgx_c4_lobby){
        if(k==SDLK_LEFT)mgx_c4_choice=(mgx_c4_choice+2)%3;
        if(k==SDLK_RIGHT)mgx_c4_choice=(mgx_c4_choice+1)%3;
        if(k==SDLK_RETURN){mgx_c4_mode=mgx_c4_choice;ttt_reset();mgx_c4_lobby=0;
            if(mgx_c4_mode&&!mgx_net_open(mgx_c4_mode==1?MGX_NET_HOST:MGX_NET_JOIN,MG_TTT))mgx_c4_lobby=1;}
        return;
    }
    if(!mgx_c4_mode){ttt_key(k);return;}
    if(!mgx_net.connected)return;
    if(k==SDLK_LEFT)tt.cur=(tt.cur+6)%7;
    if(k==SDLK_RIGHT)tt.cur=(tt.cur+1)%7;
    if(k==SDLK_RETURN){
        if(tt.over){if(mgx_c4_mode==1)ttt_reset();return;}
        if(mgx_c4_mode==1)mgx_c4_drop(tt.cur,1);
        else if(tt.turn==2 && !tt.cell[tt.cur])mgx_c4_pending=tt.cur;
    }
}
static void mgx_c4_step(void){
    if(mgx_c4_lobby)return;if(!mgx_c4_mode){ttt_step();return;}
    MgxPacket p;int got=mgx_net_poll(MG_TTT,&p);
    if(mgx_c4_mode==1){
        if(got==MGX_PKT_INPUT && mgx_pkt_i(&p,2)==mgx_c4_ply()+1)mgx_c4_drop(mgx_pkt_i(&p,0),2);
        if(mgx_net.connected && SDL_GetTicks()-mgx_net.last_tx>=30){mgx_net.last_tx=SDL_GetTicks();mgx_net_packet(&p,MGX_PKT_STATE,MG_TTT);
            for(int i=0;i<42;i++)mgx_pkt_set(&p,i,tt.cell[i]);mgx_pkt_set(&p,42,tt.turn);mgx_pkt_set(&p,43,tt.winner);mgx_net_send(&p,&mgx_net.peer);}
    }else{
        if(got==MGX_PKT_STATE){int old=mgx_c4_ply();for(int i=0;i<42;i++)tt.cell[i]=mgx_clampi(mgx_pkt_i(&p,i),0,2);
            tt.turn=mgx_clampi(mgx_pkt_i(&p,42),1,2);tt.winner=mgx_clampi(mgx_pkt_i(&p,43),0,3);tt.over=tt.winner!=0;
            if(mgx_c4_ply()!=old||tt.turn!=2)mgx_c4_pending=-1;}
        mgx_net_send_input(MG_TTT,mgx_c4_pending,0,mgx_c4_pending>=0?mgx_c4_ply()+1:0,0);
    }
}
static void mgx_c4_render(SDL_Renderer *ren){
    ttt_render(ren);
    if(mgx_c4_lobby){const char *m[]={"CPU BATTLE","LOCAL LINK HOST","LOCAL LINK JOIN"};mgx_overlay_message(ren,m[mgx_c4_choice],"Left/Right Mode    A Choose");}
    else if(mgx_c4_mode){
        SDL_SetRenderDrawColor(ren,mg_theme()->bg.r,mg_theme()->bg.g,mg_theme()->bg.b,255);SDL_RenderFillRect(ren,&(SDL_Rect){0,410,WIN_W,70});
        const char *label=tt.over?(tt.winner==3?"Draw - Host A Rematch":tt.winner==(mgx_c4_mode==1?1:2)?"You win - Host A Rematch":"Friend wins - Host A Rematch"):tt.turn==(mgx_c4_mode==1?1:2)?"Your turn - Left/Right + A":"Friend's turn";
        mgx_text_center(ren,font_label,label,g_ui_text,WIN_W/2,420);
        if(!mgx_net.connected)mgx_overlay_message(ren,mgx_net.status,"Same Wi-Fi / local network required");
    }
}

/* ---------------- Flapping Bird golden collectibles ------------------ */

static int mgx_flap_gold[FLAP_PIPES];
static float mgx_flap_prev_x[FLAP_PIPES];
static int mgx_flap_gold_count=0;
static void mgx_flap_reset(void){flap_reset();mgx_flap_gold_count=0;for(int i=0;i<FLAP_PIPES;i++){mgx_flap_gold[i]=1;mgx_flap_prev_x[i]=fb.px[i];}}
static void mgx_flap_step(void){
    flap_step();if(!fb.started||fb.dead){for(int i=0;i<FLAP_PIPES;i++)mgx_flap_prev_x[i]=fb.px[i];return;}
    for(int i=0;i<FLAP_PIPES;i++){
        if(fb.px[i]>mgx_flap_prev_x[i]+80)mgx_flap_gold[i]=1;
        float gx=fb.px[i]+fb.pw/2.0f,gy=(float)fb.pgc[i];
        if(mgx_flap_gold[i]&&mgx_len2(gx-fb.bx,gy-fb.by)<18.0f*18.0f){mgx_flap_gold[i]=0;mgx_flap_gold_count++;fb.score+=2;}
        mgx_flap_prev_x[i]=fb.px[i];
    }
}
static void mgx_flap_render(SDL_Renderer *ren){
    flap_render(ren);SDL_Color gold={245,190,45,255};
    for(int i=0;i<FLAP_PIPES;i++)if(mgx_flap_gold[i]){int x=(int)(fb.px[i]+fb.pw/2),y=fb.pgc[i];mgx_circle(ren,x,y,8,gold,80);mgx_diamond(ren,x,y,6,gold);}
    char s[40];snprintf(s,sizeof s,"Gold %d  (+2)",mgx_flap_gold_count);mgx_text(ren,font_label,s,gold,30,82);
}

/* -------------------- Pulse Runner: ten stages ----------------------- */

typedef struct { int x,w,h,type; } MgxRunObj; /* 0 spike, 1 block/platform, 2 ground gap */
typedef struct { int length,speed,n; const MgxRunObj *o; const char *name; } MgxRunLevel;

#define RSP(x) {x,30,32,0}
#define RBL(x,w,h) {x,w,h,1}
#define RGP(x,w) {x,w,0,2}
static const MgxRunObj mgx_rl0[]={RSP(420),RSP(720),RBL(980,42,46),RSP(1260),RSP(1320),RGP(1600,90),RSP(1950),RBL(2240,55,58)};
static const MgxRunObj mgx_rl1[]={RSP(360),RSP(540),RSP(720),RBL(900,50,48),RBL(1110,50,64),RGP(1370,100),RSP(1660),RSP(1710),RBL(2040,80,48),RSP(2360)};
static const MgxRunObj mgx_rl2[]={RGP(430,80),RSP(700),RBL(870,85,42),RSP(1090),RSP(1140),RGP(1390,120),RBL(1700,55,70),RSP(1920),RSP(1990),RGP(2260,90),RSP(2500)};
/* Stage four used to demand three increasingly high, frame-tight landings.
 * Keep its staircase rhythm, but make every ledge and gap forgiving enough
 * to read on the handheld screen and clear without a near-perfect tap. */
static const MgxRunObj mgx_rl3[]={RSP(330),RSP(470),RBL(650,45,40),RBL(810,45,54),RBL(980,45,66),RGP(1220,105),RSP(1530),RSP(1590),RBL(1910,110,38),RGP(2250,90),RSP(2540)};
static const MgxRunObj mgx_rl4[]={RGP(390,90),RBL(650,70,45),RSP(820),RSP(880),RGP(1080,105),RBL(1370,45,82),RSP(1590),RBL(1770,100,48),RSP(2010),RSP(2070),RGP(2320,135),RBL(2640,50,70)};
static const MgxRunObj mgx_rl5[]={RSP(300),RSP(420),RSP(540),RGP(750,120),RBL(1050,60,50),RBL(1190,60,75),RSP(1430),RGP(1660,90),RSP(1900),RSP(1950),RBL(2190,130,44),RGP(2510,110),RSP(2790)};
static const MgxRunObj mgx_rl6[]={RBL(350,55,50),RSP(520),RGP(690,95),RSP(940),RSP(990),RBL(1210,45,90),RGP(1450,140),RBL(1750,80,45),RSP(1950),RGP(2140,95),RSP(2400),RSP(2460),RSP(2520),RBL(2780,60,74)};
static const MgxRunObj mgx_rl7[]={RGP(330,110),RSP(620),RBL(790,45,58),RSP(950),RGP(1140,145),RBL(1450,50,90),RSP(1690),RSP(1750),RGP(1970,120),RBL(2260,90,46),RSP(2480),RSP(2540),RGP(2760,100),RSP(3010)};
static const MgxRunObj mgx_rl8[]={RSP(280),RSP(390),RGP(570,100),RBL(820,50,80),RSP(1020),RSP(1070),RBL(1270,100,46),RGP(1510,145),RSP(1820),RSP(1870),RSP(1920),RBL(2190,45,96),RGP(2440,120),RBL(2730,100,44),RSP(2990)};
static const MgxRunObj mgx_rl9[]={RSP(270),RGP(440,105),RSP(850),RGP(1080,140),RBL(1390,50,95),RSP(1610),RGP(1790,115),RSP(2040),RSP(2100),RBL(2320,120,48),RGP(2610,150),RSP(2930),RSP(2990),RBL(3210,55,82)};
#undef RSP
#undef RBL
#undef RGP

#define MGX_ARRN(a) ((int)(sizeof(a)/sizeof((a)[0])))
static const MgxRunLevel mgx_run_levels[10]={
    {2600,4,MGX_ARRN(mgx_rl0),mgx_rl0,"FIRST LIGHT"},{2700,4,MGX_ARRN(mgx_rl1),mgx_rl1,"THREE BEATS"},
    {2850,4,MGX_ARRN(mgx_rl2),mgx_rl2,"SKY BRIDGE"},{2900,4,MGX_ARRN(mgx_rl3),mgx_rl3,"STAIR SIGNAL"},
    {3000,5,MGX_ARRN(mgx_rl4),mgx_rl4,"LONG NOTE"},{3150,5,MGX_ARRN(mgx_rl5),mgx_rl5,"TRIPLE TAP"},
    {3250,5,MGX_ARRN(mgx_rl6),mgx_rl6,"OFFSET"},{3350,6,MGX_ARRN(mgx_rl7),mgx_rl7,"NIGHT DRIVE"},
    {3450,6,MGX_ARRN(mgx_rl8),mgx_rl8,"UPBEAT"},{3650,6,MGX_ARRN(mgx_rl9),mgx_rl9,"FINAL PULSE"}
};

#define MGX_RUN_JUMP_VY    -12.4f
#define MGX_RUN_GRAVITY       .58f
#define MGX_RUN_COYOTE_MS      110u
#define MGX_RUN_BUFFER_MS      150u

static struct {
    int level,dead,won,started,on_floor,inverted;
    float scroll,y,vy,rot;
    Uint32 last,grounded_at,jump_until;
} mgx_run;

static int mgx_run_length(void){return mgx_run_levels[mgx_run.level].length*3;}
static int mgx_run_progress(void){return mgx_run.won?100:mgx_clampi((int)(mgx_run.scroll*100/(mgx_run_length()-120)),0,99);}
static MgxRunObj mgx_run_object(int i){
    const MgxRunLevel *l=&mgx_run_levels[mgx_run.level];MgxRunObj o=l->o[i%l->n];o.x+=(i/l->n)*l->length;return o;
}
static int mgx_run_screen_y(int y,int h){return mgx_run.inverted?532-y-h:y;}
static int mgx_run_in_gap(float wx){wx=fmodf(wx,(float)mgx_run_levels[mgx_run.level].length);const MgxRunLevel*l=&mgx_run_levels[mgx_run.level];for(int i=0;i<l->n;i++)if(l->o[i].type==2&&wx>=l->o[i].x&&wx<=l->o[i].x+l->o[i].w)return 1;return 0;}
static void mgx_runner_retry(void){
    mgx_run.dead=mgx_run.won=mgx_run.started=mgx_run.inverted=0;mgx_run.on_floor=1;
    mgx_run.scroll=0;mgx_run.y=373;mgx_run.vy=mgx_run.rot=0;
    mgx_run.last=mgx_run.grounded_at=SDL_GetTicks();mgx_run.jump_until=0;
}
static void mgx_runner_reset(void){mgx_run.level=mg_best[MG_RUNNER]>0&&mg_best[MG_RUNNER]<10?mg_best[MG_RUNNER]:0;mgx_runner_retry();}
static void mgx_runner_take_buffered_jump(Uint32 now){
    if(!mgx_run.jump_until)return;
    if(SDL_TICKS_PASSED(now,mgx_run.jump_until)){mgx_run.jump_until=0;return;}
    if(mgx_run.on_floor || now-mgx_run.grounded_at<=MGX_RUN_COYOTE_MS){
        mgx_run.vy=MGX_RUN_JUMP_VY;mgx_run.on_floor=0;mgx_run.jump_until=0;
        /* One press must produce one jump; do not let the same coyote window
         * accept a second event after takeoff. */
        mgx_run.grounded_at=now-MGX_RUN_COYOTE_MS-1u;
    }
}
static void mgx_runner_key(SDL_Keycode k){
    if(k==SDLK_s && mgx_run.level>=5 && mgx_run.started && !mgx_run.dead && !mgx_run.won){
        mgx_run.inverted=!mgx_run.inverted;mgx_run.y=508-mgx_run.y;mgx_run.vy=-mgx_run.vy;mgx_run.on_floor=0;mgx_run.grounded_at=SDL_GetTicks()-MGX_RUN_COYOTE_MS-1;return;
    }
    if(k!=SDLK_RETURN&&k!=SDLK_UP)return;
    if(mgx_run.dead){mgx_runner_retry();return;}
    if(mgx_run.won){mgx_run.level=(mgx_run.level+1)%10;mgx_runner_retry();return;}
    if(!mgx_run.started){mgx_run.started=1;return;}
    Uint32 now=SDL_GetTicks();mgx_run.jump_until=now+MGX_RUN_BUFFER_MS;
    mgx_runner_take_buffered_jump(now);
}

static void mgx_runner_step(void){
    Uint32 now=SDL_GetTicks(),dt=now-mgx_run.last;mgx_run.last=now;if(dt>50)dt=50;float t=dt/16.6667f;
    if(!mgx_run.started||mgx_run.dead||mgx_run.won)return;
    if(mgx_run.on_floor)mgx_run.grounded_at=now;
    mgx_runner_take_buffered_jump(now);
    const MgxRunLevel*l=&mgx_run_levels[mgx_run.level];
    float oldy=mgx_run.y;mgx_run.scroll+=l->speed*t;mgx_run.vy+=MGX_RUN_GRAVITY*t;mgx_run.y+=mgx_run.vy*t;mgx_run.rot+=7.5f*t;
    float wx=mgx_run.scroll+120,pbottom=mgx_run.y+24;mgx_run.on_floor=0;
    float floor_y=397;
    if(!mgx_run_in_gap(wx)&&pbottom>=floor_y&&oldy+24<=floor_y+mgx_run.vy*t+4&&mgx_run.vy>=0){mgx_run.y=floor_y-24;mgx_run.vy=0;mgx_run.on_floor=1;}
    for(int i=0;i<l->n*3&&!mgx_run.dead;i++){
        MgxRunObj object=mgx_run_object(i);const MgxRunObj*o=&object;if(o->type==2)continue;float ox=o->x-mgx_run.scroll,oy=floor_y-o->h;
        if(o->type==1&&mgx_run.vy>=0&&120+22>ox&&120<ox+o->w&&oldy+24<=oy+4&&mgx_run.y+24>=oy){mgx_run.y=oy-24;mgx_run.vy=0;mgx_run.on_floor=1;continue;}
        if(mgx_hit(123,mgx_run.y+3,18,18,ox+(o->type==0?5:0),oy+(o->type==0?8:0),o->w-(o->type==0?10:0),o->h-(o->type==0?8:0)))mgx_run.dead=1;
    }
    if(!mgx_run.dead&&mgx_run.on_floor){mgx_run.grounded_at=now;mgx_runner_take_buffered_jump(now);}
    if(mgx_run.y>WIN_H+30)mgx_run.dead=1;
    if(!mgx_run.dead && wx>=mgx_run_length()){mgx_run.scroll=mgx_run_length()-120;mgx_run.won=1;mgx_run.started=0;mg_set_best(MG_RUNNER,mgx_run.level+1);}
}

static void mgx_runner_spike(SDL_Renderer*ren,int x,int base,int w,int h,SDL_Color c){SDL_SetRenderDrawColor(ren,c.r,c.g,c.b,255);for(int xx=0;xx<w;xx++){int yy=(xx<w/2)?h*xx/(w/2):h*(w-xx)/(w-w/2);SDL_RenderDrawLine(ren,x+xx,base,x+xx,base-yy);}}
static void mgx_runner_render(SDL_Renderer*ren){
    Theme*th=mg_theme();mg_chrome(ren,"PULSE RUNNER");const MgxRunLevel*l=&mgx_run_levels[mgx_run.level];int ground=397;
    for(int y=100;y<ground;y+=36){SDL_SetRenderDrawColor(ren,th->accent2.r,th->accent2.g,th->accent2.b,18);SDL_RenderDrawLine(ren,18,y,WIN_W-18,y);}
    for(int x=-(int)mgx_run.scroll%40;x<WIN_W;x+=40){SDL_SetRenderDrawColor(ren,th->dim.r,th->dim.g,th->dim.b,60);SDL_RenderDrawLine(ren,x,100,x,ground);}
    SDL_SetRenderDrawColor(ren,th->accent2.r,th->accent2.g,th->accent2.b,255);SDL_RenderDrawLine(ren,18,ground,WIN_W-18,ground);
    for(int i=0;i<l->n*3;i++){MgxRunObj object=mgx_run_object(i);const MgxRunObj*o=&object;int x=o->x-(int)mgx_run.scroll;if(x>WIN_W||x+o->w<0)continue;
        if(o->type==0){if(!mgx_run.inverted)mgx_runner_spike(ren,x,ground,o->w,o->h,th->accent3);
            else{SDL_SetRenderDrawColor(ren,th->accent3.r,th->accent3.g,th->accent3.b,255);for(int xx=0;xx<o->w;xx++){int h=o->h*(o->w/2-abs(xx-o->w/2))/(o->w/2);SDL_RenderDrawLine(ren,x+xx,135,x+xx,135+h);}}}
        else if(o->type==1){SDL_SetRenderDrawColor(ren,th->select_bg.r,th->select_bg.g,th->select_bg.b,255);SDL_RenderFillRect(ren,&(SDL_Rect){x,mgx_run_screen_y(ground-o->h,o->h),o->w,o->h});SDL_SetRenderDrawColor(ren,th->accent1.r,th->accent1.g,th->accent1.b,255);SDL_RenderDrawRect(ren,&(SDL_Rect){x,mgx_run_screen_y(ground-o->h,o->h),o->w,o->h});}
        else{SDL_SetRenderDrawColor(ren,th->bg.r,th->bg.g,th->bg.b,255);SDL_RenderFillRect(ren,&(SDL_Rect){x,mgx_run_screen_y(ground-2,8),o->w,8});}
    }
    if(mgx_run.level>=5){SDL_SetRenderDrawColor(ren,th->accent2.r,th->accent2.g,th->accent2.b,150);SDL_RenderDrawLine(ren,18,135,WIN_W-18,135);mgx_text(ren,font_fixed?font_fixed:font_label,"X Flip gravity",th->accent2,30,106);}
    SDL_Rect cube={120,mgx_run_screen_y((int)mgx_run.y,24),24,24};SDL_SetRenderDrawColor(ren,th->accent1.r,th->accent1.g,th->accent1.b,255);SDL_RenderFillRect(ren,&cube);SDL_SetRenderDrawColor(ren,th->text.r,th->text.g,th->text.b,255);SDL_RenderDrawRect(ren,&cube);SDL_RenderDrawLine(ren,cube.x+5,cube.y+6,cube.x+19,cube.y+18);SDL_RenderDrawLine(ren,cube.x+19,cube.y+6,cube.x+5,cube.y+18);
    char hud[120];snprintf(hud,sizeof hud,"Stage %d/10  %s   %d%%   Cleared %d/10",mgx_run.level+1,l->name,mgx_run_progress(),mg_best[MG_RUNNER]);mgx_text(ren,font_label,hud,g_ui_text,30,82);
    if(!mgx_run.started&&!mgx_run.dead&&!mgx_run.won)mgx_overlay_message(ren,"A TO START",mgx_run.level>=5?"A / Up Jump   X Flip gravity - jump from ceilings":"A / Up Jump - three sections with ledges and gaps");
    else if(mgx_run.dead)mgx_overlay_message(ren,"SIGNAL LOST","A retry     B exit");
    else if(mgx_run.won)mgx_overlay_message(ren,mgx_run.level==9?"ALL TEN PULSES CLEARED":"STAGE CLEAR",mgx_run.level==9?"A restart the set":"A next stage     B exit");
}

/* -------------------------- Pocket Crossing ------------------------- */
#define MGX_ROAD_ROWS 10
#define MGX_ROAD_COIN_HISTORY 32
enum { MGX_LAND=0,MGX_TRAFFIC,MGX_FOREST,MGX_RIVER,MGX_TRACK,MGX_BUILD,
       MGX_MUD,MGX_NIGHT,MGX_TUNNEL,MGX_INTERSECTION,MGX_BRIDGE };
enum { MGX_ZONE_ROADS=0,MGX_ZONE_HIGHWAY,MGX_ZONE_WOODS,MGX_ZONE_RIVER,
       MGX_ZONE_STATION,MGX_ZONE_CONSTRUCTION,MGX_ZONE_SWAMP,MGX_ZONE_NIGHT,
       MGX_ZONE_TUNNEL,MGX_ZONE_CITY,MGX_ZONE_FARM,MGX_ZONE_BRIDGE };
static struct {
    float x,clock; int row,distance,farthest,score,coins,dead,started,scroll_base;
    int coin_taken[MGX_ROAD_COIN_HISTORY],coin_cursor; Uint32 last,move_ready;
} mgx_road;

/* A small integer hash makes every strip reproducible without ever storing an
   endless level.  That keeps memory flat even after a very long run. */
static unsigned mgx_road_hash(unsigned x) {
    x^=x>>16;x*=0x7feb352du;x^=x>>15;x*=0x846ca68bu;x^=x>>16;return x;
}
/* Select one authored six-row zone with deterministic weighted variety.
   The first twelve rows teach roads/woods before water, rail and mixed
   hazards enter the pool. */
static int mgx_road_zone(int world_row) {
    if(world_row<0)return MGX_ZONE_ROADS;
    int section=world_row/6,difficulty=world_row/24;
    unsigned roll=mgx_road_hash((unsigned)section*911u+73u)%100u;
    if(section<2)return roll<65?MGX_ZONE_ROADS:MGX_ZONE_WOODS;
    if(difficulty<2){
        if(roll<32)return MGX_ZONE_ROADS;
        if(roll<53)return MGX_ZONE_WOODS;
        if(roll<68)return MGX_ZONE_HIGHWAY;
        if(roll<80)return MGX_ZONE_FARM;
        if(roll<90)return MGX_ZONE_RIVER;
        return MGX_ZONE_STATION;
    }
    if(roll<14)return MGX_ZONE_ROADS;
    if(roll<25)return MGX_ZONE_HIGHWAY;
    if(roll<35)return MGX_ZONE_WOODS;
    if(roll<45)return MGX_ZONE_RIVER;
    if(roll<54)return MGX_ZONE_STATION;
    if(roll<63)return MGX_ZONE_CONSTRUCTION;
    if(roll<71)return MGX_ZONE_SWAMP;
    if(roll<79)return MGX_ZONE_NIGHT;
    if(roll<86)return MGX_ZONE_TUNNEL;
    if(roll<92)return MGX_ZONE_CITY;
    if(roll<96)return MGX_ZONE_FARM;
    return MGX_ZONE_BRIDGE;
}
static const char *mgx_road_zone_name(int zone){
    static const char *n[]={"COUNTRY ROAD","HIGHWAY INTERCHANGE","PINE FOREST","RIVER CROSSING",
        "RAILROAD STATION","CONSTRUCTION SITE","MUDDY SWAMP","NIGHT DISTRICT",
        "MOUNTAIN TUNNEL","CITY BLOCK","FARM COUNTRY","OLD BRIDGE"};
    return n[mgx_clampi(zone,0,MGX_ZONE_BRIDGE)];
}
/* Each six-row section begins with a guaranteed safe planning row. Hazard
   sections then follow authored patterns selected by a deterministic weighted
   generator, so the run varies without ever creating an inescapable wall. */
static int mgx_road_kind(int world_row) {
    if(world_row<=0||world_row%6==0)return MGX_LAND;
    int local=world_row%6,zone=mgx_road_zone(world_row);
    switch(zone){
      case MGX_ZONE_ROADS:return local==3?MGX_LAND:MGX_TRAFFIC;
      case MGX_ZONE_HIGHWAY:return MGX_TRAFFIC;
      case MGX_ZONE_WOODS:return local==5?MGX_LAND:MGX_FOREST;
      case MGX_ZONE_RIVER:return local==5?MGX_LAND:MGX_RIVER;
      case MGX_ZONE_STATION:return (local==2||local==4)?MGX_TRACK:MGX_LAND;
      case MGX_ZONE_CONSTRUCTION:return local==5?MGX_LAND:MGX_BUILD;
      case MGX_ZONE_SWAMP:return local==5?MGX_FOREST:MGX_MUD;
      case MGX_ZONE_NIGHT:return local==5?MGX_LAND:MGX_NIGHT;
      case MGX_ZONE_TUNNEL:return local==5?MGX_LAND:MGX_TUNNEL;
      case MGX_ZONE_CITY:return local==5?MGX_LAND:MGX_INTERSECTION;
      case MGX_ZONE_FARM:return local&1?MGX_FOREST:MGX_MUD;
      case MGX_ZONE_BRIDGE:return local==5?MGX_LAND:MGX_BRIDGE;
      default:return MGX_LAND;
    }
}
static int mgx_road_world_at(int lane) {
    return mgx_road.scroll_base+MGX_ROAD_ROWS-1-lane;
}
static int mgx_road_forest_gap(int world_row) {
    int span=WIN_W-180;if(span<1)return WIN_W/2;
    /* Consecutive rows share a broad route; small deterministic offsets keep
       the path interesting while remaining traversable one hop at a time. */
    int base=90+(int)(mgx_road_hash((unsigned)(world_row/6)*109u+17u)%(unsigned)span);
    return mgx_clampi(base+(int)(mgx_road_hash((unsigned)world_row*31u)%41u)-20,70,WIN_W-70);
}
static int mgx_road_tree_x(int world_row,int item) {
    int span=mgx_clampi(WIN_W-84,1,9999);
    return 42+(int)(mgx_road_hash((unsigned)world_row*977u+(unsigned)item*131u)%span);
}
static int mgx_road_fixed_blocked(int world_row,float x) {
    int kind=mgx_road_kind(world_row);
    if(kind==MGX_BRIDGE)return fabsf(x-WIN_W/2)>58;
    if(kind!=MGX_FOREST&&kind!=MGX_BUILD)return 0;
    int gap=mgx_road_forest_gap(world_row);
    if(fabsf(x-gap)<31)return 0;
    for(int i=0;i<8;i++){
        int tx=mgx_road_tree_x(world_row,i);
        if(abs(tx-gap)<38)continue;
        if(fabsf(x-tx)<(kind==MGX_BUILD?21:17))return 1;
    }
    return 0;
}
static int mgx_road_coin_x(int world_row) {
    int kind=mgx_road_kind(world_row);
    if(kind==MGX_FOREST||kind==MGX_BUILD)return mgx_road_forest_gap(world_row);
    if(kind==MGX_BRIDGE)return WIN_W/2;
    return 34+(int)(mgx_road_hash((unsigned)world_row*313u+71u)%
                    (unsigned)mgx_clampi(WIN_W-68,1,9999));
}
static int mgx_road_has_coin(int world_row) {
    int kind=mgx_road_kind(world_row);
    if(world_row<=0||kind==MGX_RIVER||kind==MGX_TRACK||mgx_road_hash((unsigned)world_row*59u+5u)%3u)return 0;
    for(int i=0;i<MGX_ROAD_COIN_HISTORY;i++)if(mgx_road.coin_taken[i]==world_row)return 0;
    return 1;
}
static float mgx_road_speed(int world_row) {
    int zone=mgx_road_zone(world_row),difficulty=mgx_clampi(mgx_road.farthest/20,0,9);
    float base=zone==MGX_ZONE_HIGHWAY?92:zone==MGX_ZONE_TUNNEL?86:zone==MGX_ZONE_NIGHT?73:58;
    return base+(mgx_road_hash((unsigned)world_row*23u)%32u)+difficulty*7.0f;
}
static float mgx_road_object(int world_row,int item) {
    float direction=(world_row&1)?1.0f:-1.0f;
    float span=WIN_W+190.0f;
    float offset=(mgx_road_hash((unsigned)world_row*193u+(unsigned)item*43u)%1000u)/1000.0f*span;
    float x=fmodf(offset+mgx_road.clock*mgx_road_speed(world_row)*direction,span);
    if(x<0)x+=span;
    return x-100.0f;
}
static int mgx_road_car_width(int world_row,int item) {
    int shape=(int)(mgx_road_hash((unsigned)world_row*17u+(unsigned)item*89u)%5u);
    return shape==0?86:shape==1?72:shape==2?58:shape==3?44:34;
}
static int mgx_road_vehicle_count(int world_row){
    int zone=mgx_road_zone(world_row),difficulty=mgx_clampi(mgx_road.farthest/28,0,3);
    return mgx_clampi((zone==MGX_ZONE_HIGHWAY?4:3)+difficulty,3,6);
}
#define MGX_ROAD_LOG_COUNT 5
#define MGX_ROAD_LOG_WIDTH 108
static float mgx_road_log_x(int world_row,int item){
    float dir=(world_row&1)?1.0f:-1.0f,span=WIN_W+260.0f;
    /* Even spacing guarantees recurring, readable boarding windows. The row
       seed moves that cadence without letting random generation bunch every
       platform off-screen at once. */
    float seed=(mgx_road_hash((unsigned)world_row*331u)%1000u)/1000.0f*span+
               item*(span/MGX_ROAD_LOG_COUNT);
    float speed=25.0f+(mgx_road_hash((unsigned)world_row*19u)%22u)+mgx_clampi(mgx_road.farthest/40,0,4)*3;
    float x=fmodf(seed+mgx_road.clock*speed*dir,span);
    if(x<0)x+=span;
    return x-130.0f;
}
static float mgx_road_train_phase(int world_row){
    float period=fmaxf(4.8f,7.1f-mgx_clampi(mgx_road.farthest/35,0,8)*.25f);
    float seed=(mgx_road_hash((unsigned)world_row*71u)%1000u)/1000.0f*period;
    return fmodf(mgx_road.clock+seed,period);
}
static int mgx_road_train_warning(int world_row){
    float period=fmaxf(4.8f,7.1f-mgx_clampi(mgx_road.farthest/35,0,8)*.25f),p=mgx_road_train_phase(world_row);
    return p>period-1.65f&&p<=period-.58f;
}
static float mgx_road_train_x(int world_row,int *width){
    float period=fmaxf(4.8f,7.1f-mgx_clampi(mgx_road.farthest/35,0,8)*.25f),p=mgx_road_train_phase(world_row);
    *width=230;if(p<=period-.58f)return -9999;
    float t=(p-(period-.58f))/.58f;
    return (world_row&1)?-240+t*(WIN_W+480):WIN_W+10-t*(WIN_W+480);
}
static int mgx_road_intersection_hit(int world_row,float x){
    float phase=fmodf(mgx_road.clock+(mgx_road_hash((unsigned)world_row*47u)%300u)/100.0f,4.4f);
    int column=90+(int)(mgx_road_hash((unsigned)(world_row/6)*83u)%(unsigned)(WIN_W-180));
    return phase<.72f&&fabsf(x-column)<17;
}
static void mgx_road_collect(void) {
    int world_row=mgx_road_world_at(mgx_road.row);
    if(mgx_road_has_coin(world_row)&&fabsf(mgx_road.x-mgx_road_coin_x(world_row))<22){
        mgx_road.coin_taken[mgx_road.coin_cursor++%MGX_ROAD_COIN_HISTORY]=world_row;
        mgx_road.coins++;mgx_road.score=mgx_road.farthest*10+mgx_road.coins*50;
        mg_set_best(MG_ROAD,mgx_road.score);
    }
}
static void mgx_road_reset(void) {
    memset(&mgx_road, 0, sizeof mgx_road);
    for(int i=0;i<MGX_ROAD_COIN_HISTORY;i++)mgx_road.coin_taken[i]=-1;
    mgx_road.x=WIN_W/2;mgx_road.row=MGX_ROAD_ROWS-1;mgx_road.last=SDL_GetTicks();
}
static void mgx_road_key(SDL_Keycode k) {
    if (mgx_road.dead) { if (k == SDLK_RETURN) mgx_road_reset(); return; }
    if (k != SDLK_UP && k != SDLK_DOWN && k != SDLK_LEFT && k != SDLK_RIGHT) return;
    Uint32 now=SDL_GetTicks();int current=mgx_road_world_at(mgx_road.row);
    if(mgx_road_kind(current)==MGX_MUD&&now<mgx_road.move_ready)return;
    mgx_road.started = 1;
    float nx=mgx_road.x;int nr=mgx_road.row,nb=mgx_road.scroll_base,nd=mgx_road.distance;
    if(k==SDLK_LEFT)nx-=28;
    if(k==SDLK_RIGHT)nx+=28;
    if(k==SDLK_UP){nr--;nd++;}
    if(k==SDLK_DOWN&&nd>0){nr++;nd--;}
    nx=mgx_clampf(nx,30,WIN_W-30);
    if(nr<4){nb++;nr++;}
    if(nr>=MGX_ROAD_ROWS-1&&nb>0){nb--;nr--;}
    nr=mgx_clampi(nr,0,MGX_ROAD_ROWS-1);
    int destination=nb+MGX_ROAD_ROWS-1-nr;
    if(!mgx_road_fixed_blocked(destination,nx)){
        mgx_road.x=nx;mgx_road.row=nr;mgx_road.scroll_base=nb;mgx_road.distance=nd;
        if(nd>mgx_road.farthest){mgx_road.farthest=nd;mgx_road.score=nd*10+mgx_road.coins*50;mg_set_best(MG_ROAD,mgx_road.score);}
        if(mgx_road_kind(destination)==MGX_MUD)mgx_road.move_ready=now+145;
        mgx_road_collect();
    }
}
static void mgx_road_step(void) {
    Uint32 now = SDL_GetTicks(); float dt = fminf((now - mgx_road.last) / 1000.0f, .05f);
    mgx_road.last = now;
    if (!mgx_road.started || mgx_road.dead) return;
    mgx_road.clock += dt;
    int world_row=mgx_road_world_at(mgx_road.row),kind=mgx_road_kind(world_row);
    if(kind==MGX_TRAFFIC||kind==MGX_NIGHT||kind==MGX_TUNNEL||kind==MGX_INTERSECTION){
        int count=mgx_road_vehicle_count(world_row);
        for(int i=0;i<count;i++){
            float x=mgx_road_object(world_row,i);int width=mgx_road_car_width(world_row,i);
            if(mgx_road.x+10>x&&mgx_road.x-10<x+width){mgx_road.dead=1;break;}
        }
        if(!mgx_road.dead&&kind==MGX_INTERSECTION&&mgx_road_intersection_hit(world_row,mgx_road.x))mgx_road.dead=1;
    }else if(kind==MGX_TRACK){
        int width=0;float x=mgx_road_train_x(world_row,&width);
        if(mgx_road.x+10>x&&mgx_road.x-10<x+width)mgx_road.dead=1;
    }else if(kind==MGX_RIVER){
        int riding=-1;
        for(int i=0;i<MGX_ROAD_LOG_COUNT;i++){
            float x=mgx_road_log_x(world_row,i);
            if(mgx_road.x+8>x&&mgx_road.x-8<x+MGX_ROAD_LOG_WIDTH){riding=i;break;}
        }
        if(riding<0)mgx_road.dead=1;
        else{
            float dir=(world_row&1)?1.0f:-1.0f;
            float speed=25.0f+(mgx_road_hash((unsigned)world_row*19u)%22u)+mgx_clampi(mgx_road.farthest/40,0,4)*3;
            mgx_road.x+=dir*speed*dt;
            if(mgx_road.x<20||mgx_road.x>WIN_W-20)mgx_road.dead=1;
        }
    }
}
static void mgx_road_render(SDL_Renderer *ren) {
    Theme *th = mg_theme(); mg_chrome(ren, "POCKET CROSSING");
    int top=112,cell=(WIN_H-164)/MGX_ROAD_ROWS;
    SDL_Rect clip={20,top,WIN_W-40,cell*MGX_ROAD_ROWS};SDL_RenderSetClipRect(ren,&clip);
    for(int lane=0;lane<MGX_ROAD_ROWS;lane++){
        int world_row=mgx_road_world_at(lane),kind=mgx_road_kind(world_row),zone=mgx_road_zone(world_row);
        int shade=(MGX_ROAD_ROWS-lane)*2;
        SDL_Color c=(kind==MGX_TRAFFIC||kind==MGX_NIGHT||kind==MGX_TUNNEL||kind==MGX_INTERSECTION)?(SDL_Color){47+shade/3,49+shade/3,59+shade/3,255}:
                    kind==MGX_FOREST?(SDL_Color){24,66+shade,43,255}:
                    kind==MGX_RIVER?(SDL_Color){18,81+shade,126+shade,255}:
                    kind==MGX_TRACK?(SDL_Color){72,67,62,255}:
                    kind==MGX_BUILD?(SDL_Color){108,91,61,255}:
                    kind==MGX_MUD?(SDL_Color){83,71,54,255}:
                    kind==MGX_BRIDGE?(SDL_Color){17,76,112,255}:(SDL_Color){45,92+shade,58,255};
        SDL_SetRenderDrawColor(ren, c.r, c.g, c.b, 255);
        SDL_RenderFillRect(ren, &(SDL_Rect){20,top+lane*cell,WIN_W-40,cell-1});
        if(kind==MGX_TRAFFIC||kind==MGX_NIGHT||kind==MGX_TUNNEL||kind==MGX_INTERSECTION){
            SDL_SetRenderDrawColor(ren,126,127,129,120);
            for(int x=30+(world_row*23%42);x<WIN_W-24;x+=62)SDL_RenderFillRect(ren,&(SDL_Rect){x,top+lane*cell+cell/2-1,27,2});
            int count=mgx_road_vehicle_count(world_row);
            for(int i=0;i<count;i++){
                int x=(int)mgx_road_object(world_row,i),w=mgx_road_car_width(world_row,i),y=top+lane*cell+4;
                SDL_Color vehicle[5]={th->accent1,th->accent3,{225,159,50,255},{74,158,196,255},{190,85,68,255}};
                SDL_Color car=vehicle[mgx_road_hash((unsigned)world_row*13u+i)%5u];
                SDL_SetRenderDrawColor(ren,12,14,19,190);SDL_RenderFillRect(ren,&(SDL_Rect){x+4,y+cell-11,w-8,5});
                fill_rounded(ren,(SDL_Rect){x,y,w,cell-9},5,car.r,car.g,car.b,255);
                SDL_SetRenderDrawColor(ren,180,224,235,230);SDL_RenderFillRect(ren,&(SDL_Rect){x+w/3,y+4,w/3,5});
                SDL_Color lamp=(world_row&1)?(SDL_Color){255,225,126,255}:(SDL_Color){222,70,74,255};
                SDL_SetRenderDrawColor(ren,lamp.r,lamp.g,lamp.b,255);int lx=(world_row&1)?x+w-3:x;SDL_RenderFillRect(ren,&(SDL_Rect){lx,y+7,3,7});
            }
            if(kind==MGX_INTERSECTION){
                int col=90+(int)(mgx_road_hash((unsigned)(world_row/6)*83u)%(unsigned)(WIN_W-180));
                SDL_SetRenderDrawColor(ren,205,205,193,125);SDL_RenderFillRect(ren,&(SDL_Rect){col-12,top+lane*cell,24,cell});
                if(mgx_road_intersection_hit(world_row,(float)col)){
                    SDL_SetRenderDrawColor(ren,238,112,68,255);SDL_RenderFillRect(ren,&(SDL_Rect){col-9,top+lane*cell+2,18,cell-4});
                }
            }
        }else if(kind==MGX_RIVER){
            SDL_SetRenderDrawColor(ren,111,195,222,80);for(int x=24;x<WIN_W-20;x+=38)SDL_RenderDrawLine(ren,x,top+lane*cell+5,x+15,top+lane*cell+5);
            for(int i=0;i<MGX_ROAD_LOG_COUNT;i++){
                int x=(int)mgx_road_log_x(world_row,i);
                fill_rounded(ren,(SDL_Rect){x,top+lane*cell+5,MGX_ROAD_LOG_WIDTH,cell-10},5,112,73,42,255);
                SDL_SetRenderDrawColor(ren,173,116,61,255);
                for(int n=14;n<MGX_ROAD_LOG_WIDTH-4;n+=22)
                    SDL_RenderDrawLine(ren,x+n,top+lane*cell+7,x+n,top+(lane+1)*cell-7);
            }
        }else if(kind==MGX_TRACK){
            int yy=top+lane*cell+cell/2;SDL_SetRenderDrawColor(ren,188,184,173,255);SDL_RenderFillRect(ren,&(SDL_Rect){20,yy-7,WIN_W-40,3});SDL_RenderFillRect(ren,&(SDL_Rect){20,yy+5,WIN_W-40,3});
            for(int x=24;x<WIN_W-20;x+=24)SDL_RenderFillRect(ren,&(SDL_Rect){x,yy-10,5,21});
            int tw=0,tx=(int)mgx_road_train_x(world_row,&tw);if(tx>-9000){SDL_SetRenderDrawColor(ren,151,48,47,255);SDL_RenderFillRect(ren,&(SDL_Rect){tx,top+lane*cell+2,tw,cell-4});SDL_SetRenderDrawColor(ren,243,204,100,255);for(int w=12;w<tw-8;w+=30)SDL_RenderFillRect(ren,&(SDL_Rect){tx+w,top+lane*cell+6,16,6});}
            int sx=WIN_W-38;SDL_Color sig=mgx_road_train_warning(world_row)?(SDL_Color){255,64,61,255}:(SDL_Color){76,207,112,255};mgx_circle(ren,sx,yy,5,sig,255);
        }else if(kind==MGX_BUILD){
            int gap=mgx_road_forest_gap(world_row);for(int i=0;i<8;i++){int tx=mgx_road_tree_x(world_row,i);if(abs(tx-gap)<38)continue;SDL_SetRenderDrawColor(ren,244,143,43,255);SDL_RenderFillRect(ren,&(SDL_Rect){tx-12,top+lane*cell+5,24,cell-10});SDL_SetRenderDrawColor(ren,37,37,40,255);SDL_RenderDrawLine(ren,tx-10,top+lane*cell+6,tx+10,top+(lane+1)*cell-7);}
        }else if(kind==MGX_MUD){
            SDL_SetRenderDrawColor(ren,54,45,38,120);for(int i=0;i<7;i++){int px=32+(int)(mgx_road_hash((unsigned)world_row*57u+i*19u)%(unsigned)(WIN_W-64));fill_rounded(ren,(SDL_Rect){px-12,top+lane*cell+8,24,cell-16},5,61,52,43,160);}
        }else if(kind==MGX_BRIDGE){
            SDL_SetRenderDrawColor(ren,135,95,58,255);SDL_RenderFillRect(ren,&(SDL_Rect){WIN_W/2-58,top+lane*cell,116,cell-1});for(int x=WIN_W/2-50;x<WIN_W/2+55;x+=18){SDL_SetRenderDrawColor(ren,92,62,44,255);SDL_RenderDrawLine(ren,x,top+lane*cell,x,top+(lane+1)*cell-2);}
        }else{
            SDL_SetRenderDrawColor(ren,83,133,76,100);
            for(int i=0;i<12;i++){int sx=28+(int)(mgx_road_hash(world_row*41u+i*13u)%(WIN_W-56));SDL_RenderDrawPoint(ren,sx,top+lane*cell+5+(i*7%(cell-9)));}
            if(kind==MGX_FOREST)for(int i=0;i<8;i++){
                int tx=mgx_road_tree_x(world_row,i),gap=mgx_road_forest_gap(world_row);if(abs(tx-gap)<38)continue;
                int ty=top+lane*cell+cell/2;
                mgx_circle(ren,tx+3,ty+4,10,(SDL_Color){9,31,24,255},115);
                SDL_SetRenderDrawColor(ren,105,69,43,255);SDL_RenderFillRect(ren,&(SDL_Rect){tx-2,ty,5,cell/2});
                mgx_circle(ren,tx,ty-2,9,(SDL_Color){35,116+(i&1)*18,66,255},255);
                mgx_circle(ren,tx-6,ty,5,(SDL_Color){55,145,76,255},255);
            }
            if(kind==MGX_LAND&&world_row%6==0&&world_row>0){
                const char *zn=mgx_road_zone_name(zone);SDL_Texture *zt=render_text_fit(ren,font_label,zn,(SDL_Color){226,232,194,220},WIN_W/3);
                if(zt){int zw=0,zh=0;SDL_QueryTexture(zt,NULL,NULL,&zw,&zh);fill_rounded(ren,(SDL_Rect){28,top+lane*cell+2,zw+12,zh+2},3,38,57,42,210);SDL_RenderCopy(ren,zt,NULL,&(SDL_Rect){34,top+lane*cell+3,zw,zh});}
            }
        }
        if(mgx_road_has_coin(world_row)){
            int cx=mgx_road_coin_x(world_row),cy=top+lane*cell+cell/2;
            mgx_circle(ren,cx,cy,8,(SDL_Color){255,189,50,255},255);mgx_circle(ren,cx,cy,4,(SDL_Color){255,235,135,255},255);
            SDL_SetRenderDrawColor(ren,255,250,205,230);SDL_RenderDrawLine(ren,cx,cy-11,cx,cy-8);SDL_RenderDrawLine(ren,cx-11,cy,cx-8,cy);
        }
    }
    int player_world=mgx_road_world_at(mgx_road.row),player_kind=mgx_road_kind(player_world);
    if(player_kind==MGX_NIGHT||player_kind==MGX_TUNNEL){
        SDL_SetRenderDrawColor(ren,2,5,13,player_kind==MGX_TUNNEL?155:112);SDL_RenderFillRect(ren,&clip);
        int hx=(int)mgx_road.x,hy=top+mgx_road.row*cell+cell/2;
        mgx_circle(ren,hx,hy,34,(SDL_Color){244,224,143,35},35);mgx_circle(ren,hx,hy,20,(SDL_Color){255,238,166,50},50);
    }
    int px=(int)mgx_road.x,py=top+mgx_road.row*cell+cell/2;
    mgx_circle(ren,px,py,9,(SDL_Color){105,218,112,255},255);mgx_circle(ren,px-6,py-7,4,(SDL_Color){132,239,132,255},255);mgx_circle(ren,px+6,py-7,4,(SDL_Color){132,239,132,255},255);
    mgx_circle(ren,px-6,py-8,1,(SDL_Color){10,30,20,255},255);mgx_circle(ren,px+6,py-8,1,(SDL_Color){10,30,20,255},255);
    SDL_RenderSetClipRect(ren,NULL);
    char line[160];snprintf(line,sizeof line,"Score %d   %dm   Coins %d   %s",mgx_road.score,mgx_road.farthest,mgx_road.coins,mgx_road_zone_name(mgx_road_zone(player_world)));
    mgx_text(ren,font_label,line,th->accent2,30,84);
    mgx_text_center(ren,font_label,"D-pad Hop    Climb forever    Coins +50    B Exit",g_ui_dim,WIN_W/2,WIN_H-30);
    if(!mgx_road.started)mgx_overlay_message(ren,"EVERY RUN CHANGES","Roads, rivers, rails and landmarks get tougher as you climb.");
    if(mgx_road.dead)mgx_overlay_message(ren,"RUN ENDED","A Retry    B Exit");
}

/* -------------------- Personal artwork swap puzzle ------------------ */
static struct {
    SDL_Texture *art; int aw, ah, difficulty, n, cols, rows, tile[16], cursor, held, moves, won;
    int choosing, loading, load_failed, scan_start, scan_checked, scan_limit, fallback_done;
    char source[768];
} mgx_swap;

static void mgx_swap_close(void) { if (mgx_swap.art) SDL_DestroyTexture(mgx_swap.art); mgx_swap.art = NULL; }
static void mgx_swap_scan(const char *folder, int depth, int *budget, int *found, char *chosen, size_t cap) {
    if (depth < 0 || *budget <= 0) return;
    DIR *d = opendir(folder); if (!d) return;
    struct dirent *e;
    while (*budget > 0 && (e = readdir(d))) {
        if (e->d_name[0] == '.') continue;
        (*budget)--;
        char path[900]; if (snprintf(path,sizeof path,"%s/%s",folder,e->d_name) >= (int)sizeof path) continue;
        struct stat st; if (lstat(path,&st)) continue;
        if (S_ISDIR(st.st_mode)) mgx_swap_scan(path,depth-1,budget,found,chosen,cap);
        else if (S_ISREG(st.st_mode) && (has_ext(path,".png") || has_ext(path,".jpg") || has_ext(path,".jpeg"))) {
            if (++*found == 1 || rand() % *found == 0) snprintf(chosen,cap,"%s",path);
        }
    }
    closedir(d);
}
static void mgx_swap_grid(void) {
    /* Every difficulty is a true square. The former 4x2 medium board sliced
       rectangular art into visibly uneven pieces and made movement confusing. */
    mgx_swap.cols = mgx_swap.difficulty==0?2:mgx_swap.difficulty==1?3:4;
    mgx_swap.rows = mgx_swap.cols;
    mgx_swap.n = mgx_swap.cols * mgx_swap.rows;
}
static void mgx_swap_shuffle(void) {
    mgx_swap_grid();
    for (int i=0;i<mgx_swap.n;i++) mgx_swap.tile[i]=i;
    for (int i=mgx_swap.n-1;i>0;i--) { int j=rand()%(i+1), t=mgx_swap.tile[i]; mgx_swap.tile[i]=mgx_swap.tile[j]; mgx_swap.tile[j]=t; }
    int solved=1; for(int i=0;i<mgx_swap.n;i++) if(mgx_swap.tile[i]!=i) solved=0;
    if (solved) { mgx_swap.tile[0]=1; mgx_swap.tile[1]=0; }
    mgx_swap.cursor=0; mgx_swap.held=-1; mgx_swap.moves=mgx_swap.won=0;
}
static void mgx_swap_pick(SDL_Renderer *ren) {
    (void)ren;
    /* This is deliberately only a chooser reset.  Art lookup and decoding do
     * not begin until A confirms the difficulty, so the four-piece version
     * can never reveal the picture before the player's real round starts. */
    mgx_swap_close();mgx_swap.source[0]=0;mgx_swap_grid();
    mgx_swap.choosing=1;mgx_swap.loading=mgx_swap.load_failed=0;
    mgx_swap.cursor=0;mgx_swap.held=-1;mgx_swap.moves=mgx_swap.won=0;
}
static void mgx_swap_begin_load(void) {
    mgx_swap_close();mgx_swap.source[0]=0;
    mgx_swap.choosing=0;mgx_swap.loading=1;mgx_swap.load_failed=0;
    mgx_swap.scan_start=game_count>0?rand()%game_count:0;
    mgx_swap.scan_checked=0;mgx_swap.scan_limit=mgx_clampi(game_count,0,256);
    mgx_swap.fallback_done=0;
}
static int mgx_swap_surface_has_picture(SDL_Surface *s) {
    if(!s||s->w<24||s->h<24||!s->format||!s->pixels)return 0;
    SDL_Surface *c=s->format->format==SDL_PIXELFORMAT_RGBA32?s:SDL_ConvertSurfaceFormat(s,SDL_PIXELFORMAT_RGBA32,0);
    if(!c)return 0;
    int sx=mgx_clampi(c->w/28,1,32),sy=mgx_clampi(c->h/28,1,32);
    int sampled=0,visible=0,min_luma=255,max_luma=0;
    if(SDL_LockSurface(c)!=0){if(c!=s)SDL_FreeSurface(c);return 0;}
    for(int y=sy/2;y<c->h;y+=sy)for(int x=sx/2;x<c->w;x+=sx){
        Uint32 pixel=((Uint32*)((Uint8*)c->pixels+y*c->pitch))[x];
        Uint8 r=0,g=0,b=0,a=0;SDL_GetRGBA(pixel,c->format,&r,&g,&b,&a);sampled++;
        if(a>32){int l=(r*30+g*59+b*11)/100;visible++;if(l<min_luma)min_luma=l;if(l>max_luma)max_luma=l;}
    }
    SDL_UnlockSurface(c);if(c!=s)SDL_FreeSurface(c);
    /* Reject transparent/empty scrape files. A real cover, screenshot or fan
       image easily clears 15%; this deliberately excludes sparse logo files. */
    return sampled>0&&visible*100>=sampled*15&&max_luma-min_luma>=4;
}
static int mgx_swap_try_load(SDL_Renderer *ren,const char *path) {
    SDL_Surface *s=load_scaled_surface(path,512);if(!s)return 0;
    if(!mgx_swap_surface_has_picture(s)){SDL_FreeSurface(s);return 0;}
    SDL_Texture *t=SDL_CreateTextureFromSurface(ren,s);
    int w=s->w,h=s->h;SDL_FreeSurface(s);if(!t)return 0;
    mgx_swap_close();mgx_swap.art=t;mgx_swap.aw=w;mgx_swap.ah=h;
    snprintf(mgx_swap.source,sizeof mgx_swap.source,"%s",path);
    mgx_swap.loading=mgx_swap.load_failed=0;mgx_swap_shuffle();return 1;
}
static void mgx_swap_load_step(SDL_Renderer *ren) {
    if(!mgx_swap.loading)return;
    /* At most two game records are checked per frame.  The old input handler
     * synchronously walked up to 4,096 directory entries, which made Y look
     * frozen and let a queued B unexpectedly kick the player back to menu. */
    for(int step=0;step<2&&mgx_swap.scan_checked<mgx_swap.scan_limit;step++) {
        int idx=(mgx_swap.scan_start+mgx_swap.scan_checked++)%game_count;
        GameEntry *g=&games[idx];
        static const int puzzle_art[]={0,2,3,5,6}; /* full-frame scraped art only */
        for(unsigned ai=0;ai<sizeof puzzle_art/sizeof puzzle_art[0];ai++) {
            int a=puzzle_art[ai];
            char chosen[768];
            if(cached_art_file(chosen,sizeof chosen,g->platform_dir,g->raw_filename,a)&&
               mgx_swap_try_load(ren,chosen))return;
        }
    }
    if(mgx_swap.scan_checked<mgx_swap.scan_limit)return;
    if(!mgx_swap.fallback_done) {
        static const int puzzle_art[]={0,2,3,5,6};
        char folder[768],chosen[768]="";int budget=128,found=0;
        mgx_swap.fallback_done=1;
        int start=PLATFORM_COUNT?rand()%PLATFORM_COUNT:0;
        for(int pi=0;pi<PLATFORM_COUNT&&budget>0;pi++){
            int p=(start+pi)%PLATFORM_COUNT;
            for(unsigned ai=0;ai<sizeof puzzle_art/sizeof puzzle_art[0]&&budget>0;ai++){
                snprintf(folder,sizeof folder,"%s/boxart/%s/%s",sn_data_root(),platform_dirs[p],art_type_slugs[puzzle_art[ai]]);
                mgx_swap_scan(folder,0,&budget,&found,chosen,sizeof chosen);
            }
            /* Flat files are legacy 2D cover scrapes; do not descend into the
               other art-type directories from here. */
            snprintf(folder,sizeof folder,"%s/boxart/%s",sn_data_root(),platform_dirs[p]);
            mgx_swap_scan(folder,0,&budget,&found,chosen,sizeof chosen);
        }
        if(chosen[0]&&mgx_swap_try_load(ren,chosen))return;
    }
    mgx_swap.loading=0;mgx_swap.load_failed=1;
}
static void mgx_swap_key(SDL_Renderer *ren, SDL_Keycode k) {
    if(mgx_swap.choosing) {
        if(k==SDLK_LEFT||k==SDLK_q)mgx_swap.difficulty=(mgx_swap.difficulty+2)%3;
        else if(k==SDLK_RIGHT||k==SDLK_e)mgx_swap.difficulty=(mgx_swap.difficulty+1)%3;
        else if(k==SDLK_RETURN)mgx_swap_begin_load();
        mgx_swap_grid();return;
    }
    /* New Art keeps the chosen difficulty and starts a replacement round
       directly. The chooser is only shown on first entry. */
    if(k==SDLK_f) {mgx_swap_begin_load();return;}
    if(mgx_swap.loading)return;
    if(mgx_swap.load_failed){if(k==SDLK_RETURN)mgx_swap_pick(ren);return;}
    if(!mgx_swap.art)return;
    if(mgx_swap.won) {if(k==SDLK_RETURN)mgx_swap_shuffle();return;}
    int c=mgx_swap.cursor, x=c%mgx_swap.cols,y=c/mgx_swap.cols;
    if(k==SDLK_LEFT)x=(x+mgx_swap.cols-1)%mgx_swap.cols;
    if(k==SDLK_RIGHT)x=(x+1)%mgx_swap.cols;
    if(k==SDLK_UP)y=(y+mgx_swap.rows-1)%mgx_swap.rows;
    if(k==SDLK_DOWN)y=(y+1)%mgx_swap.rows;
    mgx_swap.cursor=y*mgx_swap.cols+x;
    if(k==SDLK_RETURN) {
        if(mgx_swap.held<0) mgx_swap.held=c;
        else if(mgx_swap.held==c) mgx_swap.held=-1;
        else {
            int t=mgx_swap.tile[c];mgx_swap.tile[c]=mgx_swap.tile[mgx_swap.held];mgx_swap.tile[mgx_swap.held]=t;
            mgx_swap.held=-1;mgx_swap.moves++;mgx_swap.won=1;
            for(int i=0;i<mgx_swap.n;i++)if(mgx_swap.tile[i]!=i)mgx_swap.won=0;
            if(mgx_swap.won)mg_set_best(MG_SWAP,mg_best[MG_SWAP]+1);
        }
    }
}
static void mgx_swap_render(SDL_Renderer *ren) {
    Theme *th=mg_theme();mg_chrome(ren,"ART SHUFFLE");
    const char *difficulty=mgx_swap.difficulty==0?"Easy":mgx_swap.difficulty==1?"Medium":"Hard";
    if(mgx_swap.choosing) {
        mgx_text_center(ren,font_small_bold?font_small_bold:font_small,"CHOOSE DIFFICULTY",th->accent2,WIN_W/2,112);
        mgx_text_center(ren,font_label,"The artwork stays hidden until you press A",g_ui_dim,WIN_W/2,143);
        SDL_Rect p={45,180,WIN_W-90,118};mgx_panel(ren,p,th->bg,th->dim,235);
        const char *names[3]={"EASY","MEDIUM","HARD"};const char *pieces[3]={"4 PIECES","9 PIECES","16 PIECES"};
        int cw=(p.w-40)/3;
        for(int i=0;i<3;i++){
            SDL_Rect card={p.x+10+i*cw,p.y+12,cw-10,p.h-24};int selected=i==mgx_swap.difficulty;
            mgx_panel(ren,card,selected?th->select_bg:th->bg,selected?th->accent2:th->dim,selected?255:125);
            mgx_text_center(ren,font_label_bold?font_label_bold:font_label,names[i],selected?g_ui_text:g_ui_dim,card.x+card.w/2,card.y+18);
            mgx_text_center(ren,font_label,pieces[i],selected?th->accent2:g_ui_dim,card.x+card.w/2,card.y+52);
        }
        mgx_text_center(ren,font_label,"Left / Right or L1 / R1: Difficulty",g_ui_dim,WIN_W/2,WIN_H-52);
        mgx_text_center(ren,font_label,"A: Start     B: Exit",g_ui_text,WIN_W/2,WIN_H-28);
        return;
    }
    if(mgx_swap.loading)mgx_swap_load_step(ren);
    if(mgx_swap.loading) {
        mgx_overlay_message(ren,"FINDING ART...","Loading a puzzle in small steps. B still exits immediately.");
        mgx_text_center(ren,font_label,"B: Exit",g_ui_text,WIN_W/2,WIN_H-28);return;
    }
    if(mgx_swap.load_failed) {
        mgx_overlay_message(ren,"NO SCRAPED ART FOUND","Scrape a game first. A returns to difficulty; B exits.");
        mgx_text_center(ren,font_label,"A: Difficulty     B: Exit",g_ui_text,WIN_W/2,WIN_H-28);return;
    }
    char label[96];snprintf(label,sizeof label,"%s: %d pieces    Swaps %d",difficulty,mgx_swap.n,mgx_swap.moves);
    mgx_text(ren,font_label,label,th->accent2,30,84);
    if(mgx_swap.art&&mgx_swap.won) {
        int area_x=28,area_y=112,area_w=WIN_W-270,area_h=WIN_H-158;
        float scale=fminf(area_w/(float)mgx_swap.aw,area_h/(float)mgx_swap.ah);
        int w=(int)(mgx_swap.aw*scale),h=(int)(mgx_swap.ah*scale);
        SDL_Rect dst={area_x+(area_w-w)/2,area_y+(area_h-h)/2,w,h};
        SDL_RenderCopy(ren,mgx_swap.art,NULL,&dst);
        SDL_SetRenderDrawColor(ren,th->accent2.r,th->accent2.g,th->accent2.b,255);SDL_RenderDrawRect(ren,&dst);
        SDL_Rect prompt={WIN_W-224,145,194,166};mgx_panel(ren,prompt,th->bg,th->accent2,242);
        mgx_text(ren,font_small_bold?font_small_bold:font_small,"PICTURE PERFECT",th->accent2,prompt.x+14,prompt.y+16);
        char done[48];snprintf(done,sizeof done,"Solved in %d swaps",mgx_swap.moves);
        mgx_text(ren,font_label,done,g_ui_dim,prompt.x+14,prompt.y+57);
        mgx_text(ren,font_label_bold?font_label_bold:font_label,"A  Shuffle again",g_ui_text,prompt.x+14,prompt.y+96);
        mgx_text(ren,font_label_bold?font_label_bold:font_label,"Y  New artwork",g_ui_text,prompt.x+14,prompt.y+124);
        mgx_text_center(ren,font_label,"The complete artwork stays visible until you choose",g_ui_dim,WIN_W/2,WIN_H-28);
    } else if(mgx_swap.art) {
        float scale=fminf((WIN_W-100)/(float)mgx_swap.aw,(WIN_H-190)/(float)mgx_swap.ah);
        int w=(int)(mgx_swap.aw*scale),h=(int)(mgx_swap.ah*scale),ox=(WIN_W-w)/2,oy=112;
        for(int i=0;i<mgx_swap.n;i++) {
            int id=mgx_swap.tile[i],c=mgx_swap.cols,r=mgx_swap.rows;
            SDL_Rect src={id%c*mgx_swap.aw/c,id/c*mgx_swap.ah/r,(id%c+1)*mgx_swap.aw/c-id%c*mgx_swap.aw/c,(id/c+1)*mgx_swap.ah/r-id/c*mgx_swap.ah/r};
            SDL_Rect dst={ox+i%c*w/c,oy+i/c*h/r,(i%c+1)*w/c-i%c*w/c,(i/c+1)*h/r-i/c*h/r};
            SDL_RenderCopy(ren,mgx_swap.art,&src,&dst);
            SDL_Color edge=i==mgx_swap.held?th->accent3:i==mgx_swap.cursor?th->accent2:th->bg;
            SDL_SetRenderDrawColor(ren,edge.r,edge.g,edge.b,255);
                for(int k=0;k<(i==mgx_swap.cursor||i==mgx_swap.held?3:1);k++)SDL_RenderDrawRect(ren,&(SDL_Rect){dst.x+k,dst.y+k,dst.w-2*k,dst.h-2*k});
        }
        mgx_text_center(ren,font_label,"D-pad: Move     A: Pick / Swap",g_ui_dim,WIN_W/2,WIN_H-52);
        mgx_text_center(ren,font_label,"Y: New artwork     B: Exit",g_ui_text,WIN_W/2,WIN_H-28);
    }
}

/* ------------------------ Button Blitz ------------------------------ */
static struct { int phase, round, score, lives, target, pressed, pair, best_ms; Uint32 due, shown, first; } mgx_react;
static int mgx_react_bit(SDL_Keycode k) {
    return k==SDLK_RETURN?1:k==SDLK_ESCAPE?2:k==SDLK_s?4:k==SDLK_f?8:
           k==SDLK_q?16:k==SDLK_e?32:k==SDLK_PAGEUP?64:k==SDLK_PAGEDOWN?128:0;
}
static void mgx_react_next(void) {
    mgx_react.phase=1;mgx_react.pressed=0;mgx_react.first=0;mgx_react.round++;
    /* Teach the face buttons first, then mix in all four shoulders/triggers.
     * Later pairs may combine either group, but never ask for the same input
     * twice. */
    int pool=mgx_react.round>=3?8:4;
    mgx_react.target=1<<(rand()%pool);
    if(mgx_react.round>5 && rand()%3==0){int extra;do extra=1<<(rand()%pool);while(extra&mgx_react.target);mgx_react.target|=extra;}
    mgx_react.due=SDL_GetTicks()+800+rand()%1300;
}
static void mgx_react_reset(void) { memset(&mgx_react,0,sizeof mgx_react);mgx_react.lives=3; }
static void mgx_react_miss(void) {
    mgx_react.lives--;mgx_react.phase=3;mgx_react.pair=0;mgx_react.due=SDL_GetTicks()+850;
    if(mgx_react.lives<=0){mgx_react.phase=4;mg_set_best(MG_REACTION,mgx_react.score);}
}
static void mgx_react_key(SDL_Keycode k) {
    int b=mgx_react_bit(k);if(!b)return;
    if(mgx_react.phase==0 || mgx_react.phase==4){if(k==SDLK_RETURN){mgx_react_reset();mgx_react_next();}return;}
    if(mgx_react.phase==1){mgx_react_miss();return;}
    if(mgx_react.phase!=2)return;
    Uint32 now=SDL_GetTicks();
    if(!(mgx_react.target&b) || (mgx_react.first && now-mgx_react.first>180)){mgx_react_miss();return;}
    if(!mgx_react.first)mgx_react.first=now;
    mgx_react.pressed|=b;
    if(mgx_react.pressed==mgx_react.target){
        int ms=(int)(now-mgx_react.shown);mgx_react.best_ms=ms;mgx_react.score++;mg_set_best(MG_REACTION,mgx_react.score);
        mgx_react.phase=3;mgx_react.pair=1;mgx_react.due=now+850;
    }
}
static void mgx_react_step(void) {
    Uint32 now=SDL_GetTicks();
    if(mgx_react.phase==1 && SDL_TICKS_PASSED(now,mgx_react.due)) {
        mgx_react.phase=2;mgx_react.shown=now;
        mgx_react.due=now+mgx_clampi(1000-mgx_react.score*20,350,1000);
    }else if(mgx_react.phase==2 && (SDL_TICKS_PASSED(now,mgx_react.due) || (mgx_react.first && now-mgx_react.first>180)))mgx_react_miss();
    else if(mgx_react.phase==3 && SDL_TICKS_PASSED(now,mgx_react.due))mgx_react_next();
}
static void mgx_react_render(SDL_Renderer *ren) {
    Theme *th=mg_theme();mg_chrome(ren,"BUTTON BLITZ");
    // B is a playable prompt in this game. Cover the common B-exit hint.
    SDL_SetRenderDrawColor(ren,th->bg.r,th->bg.g,th->bg.b,255);SDL_RenderFillRect(ren,&(SDL_Rect){WIN_W-180,48,170,39});
    SDL_Rect exit_hint={WIN_W-151,52,121,28};mgx_panel(ren,exit_hint,th->bg,th->dim,245);
    mgx_text_center(ren,font_label_bold?font_label_bold:font_label,"SELECT  EXIT",g_ui_text,exit_hint.x+exit_hint.w/2,exit_hint.y+5);
    char line[96];snprintf(line,sizeof line,"Hits %d    Lives %d    Last %d ms",mgx_react.score,mgx_react.lives,mgx_react.best_ms);
    mgx_text(ren,font_label,line,th->accent2,30,87);
    if(mgx_react.phase==0)mgx_overlay_message(ren,"WAIT FOR THE SIGNAL","A starts. Press the shown control; quick pairs arrive later.");
    else if(mgx_react.phase==1)mgx_overlay_message(ren,"WAIT...","Do not press a button yet");
    else if(mgx_react.phase==2){
        char target[64]="";const char *labels[]={"A","B","X","Y","L1","R1","L2","R2"};
        for(int i=0;i<8;i++)if(mgx_react.target&(1<<i)){if(target[0])strcat(target," + ");strcat(target,labels[i]);}
        mgx_text_center(ren,font_big,target,th->accent2,WIN_W/2,WIN_H/2-40);
        mgx_text_center(ren,font_label,"Pairs: press both within 180 ms",g_ui_dim,WIN_W/2,WIN_H/2+18);
    }else if(mgx_react.phase==3)mgx_overlay_message(ren,mgx_react.pair?"NICE REFLEXES":"MISSED","Next signal in a moment");
    else mgx_overlay_message(ren,"RUN COMPLETE","A Retry    Select Exit");
    mgx_text_center(ren,font_label,"A / B / X / Y / L1 / R1 / L2 / R2",g_ui_dim,WIN_W/2,WIN_H-50);
    mgx_text_center(ren,font_label_bold?font_label_bold:font_label,"Press what appears     Select: Exit",g_ui_text,WIN_W/2,WIN_H-27);
}

/* ------------------------ Tank Duel --------------------------------- */
typedef struct { float x,y,dx,dy; int owner,active; } MgxShot;
#define MGX_TANK_MAP_COUNT 5
#define MGX_TANK_MAX_WALLS 8
typedef struct { const char *name; int wall_count; SDL_Rect wall[MGX_TANK_MAX_WALLS]; float spawn[4]; } MgxTankMap;
static const MgxTankMap mgx_tank_maps[MGX_TANK_MAP_COUNT]={
    {"OPEN RANGE",3,{{218,168,36,92},{386,312,36,84},{296,238,48,24}}, {76,268,-76,268}},
    {"CROSSROADS",4,{{286,145,24,90},{286,325,24,82},{330,145,24,90},{330,325,24,82}}, {76,190,-76,365}},
    {"TWIN FORTS",6,{{150,168,26,88},{150,318,26,70},{176,168,72,24},{392,318,72,24},{464,168,26,70},{464,300,26,88}}, {78,360,-78,190}},
    {"SWITCHBACK",5,{{126,188,152,24},{362,156,152,24},{126,310,152,24},{362,278,152,24},{306,226,28,104}}, {76,157,-76,394}},
    {"FOUR CORNERS",8,{{104,172,92,22},{104,172,22,64},{444,172,92,22},{514,172,22,64},{104,342,92,22},{104,300,22,64},{444,342,92,22},{514,300,22,64}}, {72,268,-72,268}}
};
static struct {
    float x[2],y[2],elapsed,cpu_clock; int dir[2],hp[2],over,started,mode,lobby,choice;
    int map,map_choice,lobby_stage,action,remote_action,remote_x,remote_y,scored;
    int series_target,wins[2],round_recorded;Uint32 next_round;
    int cpu_dx,cpu_dy,cpu_escape;float cpu_still,cpu_last_x,cpu_last_y;
    Uint32 last,fire[2]; MgxShot shot[12];
} mgx_tank;
static const MgxTankMap *mgx_tank_map(int map) {
    return &mgx_tank_maps[mgx_clampi(map,0,MGX_TANK_MAP_COUNT-1)];
}
static int mgx_tank_clear(float x,float y) {
    if(x<42||x>WIN_W-42||y<133||y>WIN_H-58)return 0;
    const MgxTankMap *m=mgx_tank_map(mgx_tank.map);
    for(int i=0;i<m->wall_count;i++){SDL_Rect r=m->wall[i];if(mgx_hit(x-12,y-12,24,24,r.x,r.y,r.w,r.h))return 0;}return 1;
}
static int mgx_tank_line_clear(float x1,float y1,float x2,float y2) {
    const MgxTankMap *m=mgx_tank_map(mgx_tank.map);float dx=x2-x1,dy=y2-y1;
    int steps=(int)(fmaxf(fabsf(dx),fabsf(dy))/7)+1;
    for(int s=1;s<steps;s++){
        float x=x1+dx*s/steps,y=y1+dy*s/steps;
        for(int i=0;i<m->wall_count;i++){SDL_Rect r=m->wall[i];if(x>=r.x-2&&x<=r.x+r.w+2&&y>=r.y-2&&y<=r.y+r.h+2)return 0;}
    }
    return 1;
}
static void mgx_tank_round(void) {
    const MgxTankMap *m=mgx_tank_map(mgx_tank.map);
    mgx_tank.x[0]=m->spawn[0];mgx_tank.y[0]=m->spawn[1];
    mgx_tank.x[1]=m->spawn[2]<0?WIN_W+m->spawn[2]:m->spawn[2];mgx_tank.y[1]=m->spawn[3];
    mgx_tank.dir[0]=1;mgx_tank.dir[1]=3;mgx_tank.hp[0]=mgx_tank.hp[1]=4;
    mgx_tank.over=mgx_tank.scored=mgx_tank.round_recorded=0;mgx_tank.next_round=0;mgx_tank.last=SDL_GetTicks();mgx_tank.cpu_still=0;
    mgx_tank.cpu_last_x=mgx_tank.x[1];mgx_tank.cpu_last_y=mgx_tank.y[1];
    memset(mgx_tank.shot,0,sizeof mgx_tank.shot);memset(mgx_tank.fire,0,sizeof mgx_tank.fire);
}
static void mgx_tank_reset(void) {mgx_net_close();memset(&mgx_tank,0,sizeof mgx_tank);mgx_tank.lobby=1;mgx_tank.series_target=2;mgx_tank_round();}
static void mgx_tank_fire(int owner) {
    Uint32 now=SDL_GetTicks();if(now-mgx_tank.fire[owner]<350 || mgx_tank.over)return;
    static const int dx[]={0,1,0,-1},dy[]={-1,0,1,0};int d=mgx_tank.dir[owner];
    for(int i=0;i<12;i++)if(!mgx_tank.shot[i].active){
        mgx_tank.shot[i]=(MgxShot){mgx_tank.x[owner]+dx[d]*19,mgx_tank.y[owner]+dy[d]*19,dx[d]*240,dy[d]*240,owner,1};
        mgx_tank.fire[owner]=now;break;
    }
}
static void mgx_tank_key(SDL_Keycode k) {
    if(mgx_tank.lobby){
        if(k==SDLK_f){mgx_tank.series_target=mgx_tank.series_target==2?1:2;return;}
        int *pick=mgx_tank.lobby_stage?&mgx_tank.map_choice:&mgx_tank.choice;
        int count=mgx_tank.lobby_stage?MGX_TANK_MAP_COUNT:3;
        if(k==SDLK_LEFT)*pick=(*pick+count-1)%count;
        if(k==SDLK_RIGHT)*pick=(*pick+1)%count;
        if(k==SDLK_q&&mgx_tank.lobby_stage){mgx_tank.lobby_stage=0;return;}
        if(k==SDLK_RETURN){
            if(!mgx_tank.lobby_stage){mgx_tank.lobby_stage=1;return;}
            mgx_tank.mode=mgx_tank.choice;
            mgx_tank.map=mgx_tank.map_choice;mgx_tank_round();
            if(!mgx_tank.mode||mgx_net_open(mgx_tank.mode==1?MGX_NET_HOST:MGX_NET_JOIN,MG_TANK))mgx_tank.lobby=0;
        }return;
    }
    if(k!=SDLK_RETURN)return;
    mgx_tank.action++;
    if(mgx_tank.mode==2)return;
    if(mgx_tank.over){if(mgx_tank.next_round||!mgx_tank.round_recorded)return;mgx_tank.wins[0]=mgx_tank.wins[1]=0;mgx_tank_round();}else mgx_tank_fire(0);
}
static void mgx_tank_move(int i,int dx,int dy,float dt) {
    if(dx)mgx_tank.dir[i]=dx>0?1:3;else if(dy)mgx_tank.dir[i]=dy>0?2:0;
    float scale=dx&&dy?.7071f:1.0f,speed=104*dt*scale;
    if(mgx_tank_clear(mgx_tank.x[i]+dx*speed,mgx_tank.y[i]))mgx_tank.x[i]+=dx*speed;
    if(mgx_tank_clear(mgx_tank.x[i],mgx_tank.y[i]+dy*speed))mgx_tank.y[i]+=dy*speed;
}
/* Grid routing is cheap at this resolution and, unlike single-axis chasing,
   always discovers a route around the current map's walls. */
static void mgx_tank_cpu_route(int *out_x,int *out_y) {
    enum { GW=31,GH=16,GS=18,GX=48,GY=140 };
    short dist[GH][GW];signed char first_x[GH][GW],first_y[GH][GW];short qx[GW*GH],qy[GW*GH];
    for(int y=0;y<GH;y++)for(int x=0;x<GW;x++){dist[y][x]=-1;first_x[y][x]=first_y[y][x]=0;}
    int sx=mgx_clampi((int)((mgx_tank.x[1]-GX+GS/2)/GS),0,GW-1),sy=mgx_clampi((int)((mgx_tank.y[1]-GY+GS/2)/GS),0,GH-1);
    int head=0,tail=0;qx[tail]=sx;qy[tail++]=sy;dist[sy][sx]=0;
    int bestx=sx,besty=sy;float best=mgx_len2(GX+sx*GS-mgx_tank.x[0],GY+sy*GS-mgx_tank.y[0]);
    static const int mx[4]={1,-1,0,0},my[4]={0,0,1,-1};
    while(head<tail){
        int x=qx[head],y=qy[head++];float score=mgx_len2(GX+x*GS-mgx_tank.x[0],GY+y*GS-mgx_tank.y[0]);
        if(score<best){best=score;bestx=x;besty=y;}
        for(int d=0;d<4;d++){int nx=x+mx[d],ny=y+my[d];
            if(nx<0||nx>=GW||ny<0||ny>=GH||dist[ny][nx]>=0||!mgx_tank_clear(GX+nx*GS,GY+ny*GS))continue;
            dist[ny][nx]=dist[y][x]+1;
            first_x[ny][nx]=(x==sx&&y==sy)?mx[d]:first_x[y][x];first_y[ny][nx]=(x==sx&&y==sy)?my[d]:first_y[y][x];
            qx[tail]=nx;qy[tail++]=ny;
        }
    }
    *out_x=first_x[besty][bestx];*out_y=first_y[besty][bestx];
    if(!*out_x&&!*out_y){float xx=mgx_tank.x[0]-mgx_tank.x[1],yy=mgx_tank.y[0]-mgx_tank.y[1];*out_x=fabsf(xx)>fabsf(yy)?(xx>0?1:-1):0;*out_y=*out_x?0:(yy>0?1:-1);}
}
static void mgx_tank_cpu_step(float dt,Uint32 now) {
    int cx=0,cy=0;mgx_tank.cpu_clock+=dt;
    /* Sidestep an incoming round before resuming the route. */
    for(int i=0;i<12;i++)if(mgx_tank.shot[i].active&&mgx_tank.shot[i].owner==0){MgxShot *s=&mgx_tank.shot[i];
        if(fabsf(s->y-mgx_tank.y[1])<15&&fabsf(s->x-mgx_tank.x[1])<115&&s->dx*(mgx_tank.x[1]-s->x)>0){cy=((i+(int)mgx_tank.cpu_clock)&1)?1:-1;break;}
        if(fabsf(s->x-mgx_tank.x[1])<15&&fabsf(s->y-mgx_tank.y[1])<115&&s->dy*(mgx_tank.y[1]-s->y)>0){cx=((i+(int)mgx_tank.cpu_clock)&1)?1:-1;break;}
    }
    if(!cx&&!cy)mgx_tank_cpu_route(&cx,&cy);
    float before_x=mgx_tank.x[1],before_y=mgx_tank.y[1];mgx_tank_move(1,cx,cy,dt*.92f);
    if(mgx_len2(mgx_tank.x[1]-before_x,mgx_tank.y[1]-before_y)<.04f)mgx_tank.cpu_still+=dt;else mgx_tank.cpu_still=0;
    if(mgx_tank.cpu_still>.42f){
        static const int ex[4]={1,0,-1,0},ey[4]={0,1,0,-1};
        for(int n=0;n<4;n++){int d=(mgx_tank.cpu_escape+n)%4;if(mgx_tank_clear(mgx_tank.x[1]+ex[d]*26,mgx_tank.y[1]+ey[d]*26)){mgx_tank_move(1,ex[d],ey[d],dt*1.25f);mgx_tank.cpu_escape=d+1;break;}}
        mgx_tank.cpu_still=0;
    }
    float xx=mgx_tank.x[0]-mgx_tank.x[1],yy=mgx_tank.y[0]-mgx_tank.y[1];
    if(fabsf(yy)<15&&mgx_tank_line_clear(mgx_tank.x[1],mgx_tank.y[1],mgx_tank.x[0],mgx_tank.y[0])){mgx_tank.dir[1]=xx>0?1:3;if(now-mgx_tank.fire[1]>610)mgx_tank_fire(1);}
    else if(fabsf(xx)<15&&mgx_tank_line_clear(mgx_tank.x[1],mgx_tank.y[1],mgx_tank.x[0],mgx_tank.y[0])){mgx_tank.dir[1]=yy>0?2:0;if(now-mgx_tank.fire[1]>610)mgx_tank_fire(1);}
    mgx_tank.cpu_last_x=mgx_tank.x[1];mgx_tank.cpu_last_y=mgx_tank.y[1];
}
static void mgx_tank_state(MgxPacket *p,int write) {
    if(write) {
        for(int i=0;i<2;i++){mgx_pkt_set(p,i*4,(int)mgx_tank.x[i]);mgx_pkt_set(p,i*4+1,(int)mgx_tank.y[i]);mgx_pkt_set(p,i*4+2,mgx_tank.dir[i]);mgx_pkt_set(p,i*4+3,mgx_tank.hp[i]);}
        mgx_pkt_set(p,8,mgx_tank.over);
        for(int i=0;i<12;i++){mgx_pkt_set(p,9+i*3,(int)mgx_tank.shot[i].x);mgx_pkt_set(p,10+i*3,(int)mgx_tank.shot[i].y);mgx_pkt_set(p,11+i*3,mgx_tank.shot[i].active?mgx_tank.shot[i].owner+1:0);}
        mgx_pkt_set(p,45,mgx_tank.map);mgx_pkt_set(p,46,mgx_tank.wins[0]*10+mgx_tank.wins[1]);mgx_pkt_set(p,47,mgx_tank.series_target);
    }else{
        for(int i=0;i<2;i++){mgx_tank.x[i]=mgx_clampi(mgx_pkt_i(p,i*4),0,WIN_W);mgx_tank.y[i]=mgx_clampi(mgx_pkt_i(p,i*4+1),0,WIN_H);mgx_tank.dir[i]=mgx_clampi(mgx_pkt_i(p,i*4+2),0,3);mgx_tank.hp[i]=mgx_clampi(mgx_pkt_i(p,i*4+3),0,4);}
        mgx_tank.over=mgx_clampi(mgx_pkt_i(p,8),0,1);
        for(int i=0;i<12;i++){mgx_tank.shot[i].x=mgx_clampi(mgx_pkt_i(p,9+i*3),0,WIN_W);mgx_tank.shot[i].y=mgx_clampi(mgx_pkt_i(p,10+i*3),0,WIN_H);int a=mgx_clampi(mgx_pkt_i(p,11+i*3),0,2);mgx_tank.shot[i].active=a!=0;mgx_tank.shot[i].owner=a==2;}
        mgx_tank.map=mgx_clampi(mgx_pkt_i(p,45),0,MGX_TANK_MAP_COUNT-1);int wins=mgx_pkt_i(p,46);mgx_tank.wins[0]=mgx_clampi(wins/10,0,2);mgx_tank.wins[1]=mgx_clampi(wins%10,0,2);mgx_tank.series_target=mgx_clampi(mgx_pkt_i(p,47),1,2);
    }
}
static void mgx_tank_step(int dx,int dy) {
    Uint32 now=SDL_GetTicks();float dt=fminf((now-mgx_tank.last)/1000.0f,.04f);mgx_tank.last=now;
    if(mgx_tank.lobby)return;
    MgxPacket p;int got=mgx_tank.mode?mgx_net_poll(MG_TANK,&p):0;
    if(mgx_tank.mode==2){
        mgx_net_send_input(MG_TANK,dx,dy,mgx_tank.action,0);
        if(got==MGX_PKT_STATE)mgx_tank_state(&p,0);
        if(mgx_tank.over&&!mgx_tank.hp[0]&&!mgx_tank.scored){mg_set_best(MG_TANK,mg_best[MG_TANK]+1);mgx_tank.scored=1;}
        if(!mgx_tank.over)mgx_tank.scored=0;
        return;
    }
    if(mgx_tank.mode && !mgx_net.connected)return;
    if(got==MGX_PKT_INPUT){
        mgx_tank.remote_x=mgx_clampi(mgx_pkt_i(&p,0),-1,1);mgx_tank.remote_y=mgx_clampi(mgx_pkt_i(&p,1),-1,1);
        int a=mgx_pkt_i(&p,2);if(a!=mgx_tank.remote_action){mgx_tank.remote_action=a;if(!mgx_tank.over)mgx_tank_fire(1);}
    }
    if(mgx_tank.over && !mgx_tank.round_recorded){
        mgx_tank.round_recorded=1;mgx_tank.wins[mgx_tank.hp[0]>0?0:1]++;
        if(mgx_tank.wins[0]<mgx_tank.series_target && mgx_tank.wins[1]<mgx_tank.series_target)mgx_tank.next_round=now+4000;
    }
    if(mgx_tank.next_round && SDL_TICKS_PASSED(now,mgx_tank.next_round))mgx_tank_round();
    if(!mgx_tank.over){
        mgx_tank_move(0,dx,dy,dt);
        if(mgx_tank.mode)mgx_tank_move(1,mgx_tank.remote_x,mgx_tank.remote_y,dt);
        else mgx_tank_cpu_step(dt,now);
        for(int i=0;i<12;i++)if(mgx_tank.shot[i].active){
            MgxShot *s=&mgx_tank.shot[i];s->x+=s->dx*dt;s->y+=s->dy*dt;
            if(!mgx_tank_clear(s->x,s->y)){s->active=0;continue;}
            int target=1-s->owner;
            if(fabsf(s->x-mgx_tank.x[target])<17&&fabsf(s->y-mgx_tank.y[target])<17){s->active=0;mgx_tank.hp[target]--;if(mgx_tank.hp[target]<=0)mgx_tank.over=1;}
        }
        if(mgx_tank.over&&!mgx_tank.hp[1]&&!mgx_tank.scored){mg_set_best(MG_TANK,mg_best[MG_TANK]+1);mgx_tank.scored=1;}
    }
    if(mgx_tank.mode && now-mgx_net.last_tx>=45){mgx_net.last_tx=now;mgx_net_packet(&p,MGX_PKT_STATE,MG_TANK);mgx_tank_state(&p,1);mgx_net_send(&p,&mgx_net.peer);}
}
static void mgx_tank_render(SDL_Renderer *ren) {
    Theme *th=mg_theme();mg_chrome(ren,"TANK DUEL");
    int shown_map=mgx_tank.lobby&&mgx_tank.lobby_stage?mgx_tank.map_choice:mgx_tank.map;
    const MgxTankMap *m=mgx_tank_map(shown_map);
    SDL_SetRenderDrawColor(ren,th->select_bg.r/2,th->select_bg.g/2,th->select_bg.b/2,255);SDL_RenderFillRect(ren,&(SDL_Rect){24,115,WIN_W-48,WIN_H-155});
    SDL_SetRenderDrawColor(ren,th->accent2.r,th->accent2.g,th->accent2.b,55);for(int x=34;x<WIN_W-24;x+=28)SDL_RenderDrawLine(ren,x,116,x,WIN_H-41);
    SDL_SetRenderDrawColor(ren,th->accent2.r,th->accent2.g,th->accent2.b,150);SDL_RenderDrawRect(ren,&(SDL_Rect){24,115,WIN_W-48,WIN_H-155});
    for(int i=0;i<m->wall_count;i++){mgx_panel(ren,m->wall[i],th->select_bg,th->dim,255);SDL_Rect hi={m->wall[i].x+4,m->wall[i].y+4,m->wall[i].w-8,m->wall[i].h-8};SDL_SetRenderDrawColor(ren,th->accent2.r,th->accent2.g,th->accent2.b,100);SDL_RenderDrawRect(ren,&hi);}
    static const int tx[]={0,1,0,-1},ty[]={-1,0,1,0};
    for(int i=0;i<2;i++){
        SDL_Color c=i?th->accent3:th->accent1;int x=(int)mgx_tank.x[i],y=(int)mgx_tank.y[i],d=mgx_tank.dir[i];
        mgx_panel(ren,(SDL_Rect){x-13,y-13,26,26},c,th->text,255);
        SDL_SetRenderDrawColor(ren,c.r,c.g,c.b,255);for(int t=-2;t<=2;t++)SDL_RenderDrawLine(ren,x+t,y+t,x+tx[d]*23+t,y+ty[d]*23+t);
        for(int hp=0;hp<mgx_tank.hp[i];hp++)SDL_RenderFillRect(ren,&(SDL_Rect){x-14+hp*7,y-22,5,3});
    }
    for(int i=0;i<12;i++)if(mgx_tank.shot[i].active)mgx_circle(ren,mgx_tank.shot[i].x,mgx_tank.shot[i].y,3,th->text,255);
    char foot[110];snprintf(foot,sizeof foot,"%s    D-pad Move/Aim    A Fire    B Exit",m->name);mgx_text_center(ren,font_label,foot,g_ui_dim,WIN_W/2,WIN_H-27);
    if(mgx_tank.lobby&&mgx_tank.lobby_stage==0){const char *labels[]={"CPU BATTLE","LOCAL LINK HOST","LOCAL LINK JOIN"};mgx_overlay_message(ren,labels[mgx_tank.choice],mgx_tank.series_target==2?"Left/Right Mode  A Map  Y Best of 3":"Left/Right Mode  A Map  Y Single match");}
    else if(mgx_tank.lobby){char title[96],sub[100];snprintf(title,sizeof title,"MAP %d/%d: %s",mgx_tank.map_choice+1,MGX_TANK_MAP_COUNT,m->name);snprintf(sub,sizeof sub,"Left/Right Map    A Start    L1 Mode");mgx_overlay_message(ren,title,sub);}
    else if(mgx_tank.mode&&!mgx_net.connected)mgx_overlay_message(ren,mgx_net.status,"Same Wi-Fi / local network    B Exit");
    else if(mgx_tank.over){int me=mgx_tank.mode==2?1:0;char score[100];snprintf(score,sizeof score,"%d - %d   %s",mgx_tank.wins[0],mgx_tank.wins[1],mgx_tank.wins[0]<mgx_tank.series_target&&mgx_tank.wins[1]<mgx_tank.series_target?"Next round in 4 seconds":"Host A New series   B Leave");mgx_overlay_message(ren,mgx_tank.hp[me]>0?"DUEL WON":"TANK DOWN",score);}
}

/* ---------------- Crazy Fish: aquarium survival campaign ------------- */
#define MGX_FISH_MAX 14
#define MGX_FISH_TYPES 7
#define MGX_FISH_DROPS 48
#define MGX_FISH_MONSTERS 3
#define MGX_FISH_PARTICLES 24
enum {MGX_RES_SILVER,MGX_RES_GOLD,MGX_RES_RUBY,MGX_RES_DIAMOND,
      MGX_RES_PEARL,MGX_RES_EMERALD,MGX_RES_RARE,MGX_RES_FOOD,MGX_RES_WASTE};
enum {MGX_PET_GUPPY,MGX_PET_TETRA,MGX_PET_ANGEL,MGX_PET_SHARK,
      MGX_PET_CLAM,MGX_PET_SEAHORSE,MGX_PET_CRAB};
typedef struct {
    float x,y,hunger,coin_at,bob,hurt_flash,flee_time,eat_cooldown;
    int active,growth,dir,type,hp,max_hp;
} MgxFishPet;
typedef struct {
    float x,y,vy,bottom_age,life;
    int active,value,kind,rare_id;
} MgxFishDrop;
typedef struct {
    float x,y,bite_cool,retarget,recoil_x,recoil_y,hit_flash;
    int active,hp,max_hp,type,target,boss;
} MgxFishMonster;
typedef struct {float x,y,vx,vy,life;int active,tint;} MgxFishParticle;
typedef struct {int kills,resources,min_fish,boss;float survive;const char *name;} MgxFishGoal;
static const char *mgx_fish_type_name[MGX_FISH_TYPES]={
    "GUPPY","NEON TETRA","ANGELFISH","REEF SHARK","PEARL CLAM","SEAHORSE","CLEANER CRAB"};
static const int mgx_fish_cost[MGX_FISH_TYPES]={25,55,90,145,70,82,120};
/* Permanent egg collection unlocks these species, in discovery order. */
static const int mgx_fish_unlock[MGX_FISH_TYPES]={0,1,4,6,2,5,3};
static const int mgx_fish_hatch_order[]={MGX_PET_TETRA,MGX_PET_CLAM,MGX_PET_CRAB,MGX_PET_ANGEL,MGX_PET_SEAHORSE,MGX_PET_SHARK};
static const char *mgx_fish_ability[]={"Your starter fish earns silver coins.","Collects a fallen coin for you.","Guides baby fish away from monsters.","Helps keep your fish fed for longer.","Slows falling treasure.","Helps keep your fish fed for longer.","Attacks monsters alongside your ray."};
static const char *mgx_fish_chapter_name[11]={
    "TIDEPOOL","SUNLIT COVE","PEARL SHOALS","RUBY CURRENT","CLEANER REEF",
    "SHARK PASS","STORM GARDEN","MIDNIGHT TRENCH","CROWN REEF","THE LEVIATHAN","ENDLESS REEF"};
static const MgxFishGoal mgx_fish_goals[10]={
    {0,3,2,0,0,"FIRST HARVEST"}, {0,6,2,0,0,"GOLDEN CURRENT"},
    {2,8,2,0,0,"FIRST INVASION"}, {2,11,3,0,0,"PEARL WINDOW"},
    {3,14,4,0,12,"CLEAN WATER"}, {4,17,4,0,16,"EMERALD GUARD"},
    {5,20,4,0,20,"FEED THE SHARK"}, {7,23,5,0,24,"MONSTER SWARM"},
    {8,27,6,0,28,"DEEP DEFENSE"}, {1,12,4,1,20,"LEVIATHAN"}
};
static struct {
    float x,y,clock,next_spawn,ray_time,ray_x,ray_y,ray_from_x,ray_from_y;
    float level_time,waste_tick;Uint32 last,last_shot;
    int coins,chapter,chapter_kills,chapter_goal,total_kills,rare_mask,over,started,between;
    int shop_type,gun_level,level_resources,boss_spawned,boss_defeated,resource_serial;
    int egg_pieces,helpers;float helper_clock,intro;
    int collection,hatch_type,hatch_pending;float hatch_time,notice_time;
    char notice[100];
    MgxFishPet fish[MGX_FISH_MAX];MgxFishDrop drops[MGX_FISH_DROPS];
    MgxFishMonster monster[MGX_FISH_MONSTERS];MgxFishParticle particle[MGX_FISH_PARTICLES];
} mgx_fish;
static int mgx_fish_alive(void){int n=0;for(int i=0;i<MGX_FISH_MAX;i++)n+=mgx_fish.fish[i].active;return n;}
static int mgx_fish_monsters_alive(void){int n=0;for(int i=0;i<MGX_FISH_MONSTERS;i++)n+=mgx_fish.monster[i].active;return n;}
static int mgx_fish_rare_count(void){int n=0;for(unsigned bits=(unsigned)mgx_fish.rare_mask;bits;bits>>=1)n+=(int)(bits&1u);return n;}
static int mgx_fish_is_treasure(int kind){return kind>=MGX_RES_SILVER&&kind<=MGX_RES_RARE;}
static float mgx_fish_full_hunger(int type){static const float h[]={64,70,78,100,95,64,92};return h[mgx_clampi(type,0,MGX_FISH_TYPES-1)];}
static int mgx_fish_max_hp(int type){static const int hp[]={3,4,6,9,7,3,7};return hp[mgx_clampi(type,0,MGX_FISH_TYPES-1)];}
static const MgxFishGoal *mgx_fish_goal(void){return mgx_fish.chapter<=10?&mgx_fish_goals[mgx_fish.chapter-1]:NULL;}
static int mgx_fish_resource_kind(int type){static const int kind[]={MGX_RES_SILVER,MGX_RES_GOLD,MGX_RES_RUBY,MGX_RES_DIAMOND,MGX_RES_PEARL,MGX_RES_EMERALD,-1};return kind[mgx_clampi(type,0,MGX_FISH_TYPES-1)];}
static int mgx_fish_resource_value(int type){static const int value[]={3,6,11,18,9,8,0};return value[mgx_clampi(type,0,MGX_FISH_TYPES-1)];}
static float mgx_fish_resource_interval(int type){static const float secs[]={5.2f,6.8f,9.0f,12.5f,10.5f,8.0f,99};return secs[mgx_clampi(type,0,MGX_FISH_TYPES-1)];}
static void mgx_fish_particle(float x,float y,float vx,float vy,int tint){
    for(int i=0;i<MGX_FISH_PARTICLES;i++)if(!mgx_fish.particle[i].active){
        MgxFishParticle *p=&mgx_fish.particle[i];p->active=1;p->x=x;p->y=y;p->vx=vx;p->vy=vy;p->life=.32f;p->tint=tint;return;
    }
}
static int mgx_fish_add_type(int type){
    type=mgx_clampi(type,0,MGX_FISH_TYPES-1);
    for(int i=0;i<MGX_FISH_MAX;i++)if(!mgx_fish.fish[i].active){
        MgxFishPet *f=&mgx_fish.fish[i];memset(f,0,sizeof *f);f->active=1;f->type=type;
        f->x=86+rand()%mgx_clampi(WIN_W-172,1,9999);f->y=70+rand()%mgx_clampi(WIN_H-150,1,9999);
        if(type==MGX_PET_CLAM||type==MGX_PET_CRAB)f->y=WIN_H-42;
        if(type==MGX_PET_SEAHORSE)f->x=42+(i%7)*96;
        f->hunger=mgx_fish_full_hunger(type);f->max_hp=f->hp=mgx_fish_max_hp(type);
        f->coin_at=mgx_fish_resource_interval(type)+(rand()%20)/10.0f;f->dir=rand()%2?1:-1;f->bob=(rand()%628)/100.0f;return 1;
    }return 0;
}
static void mgx_fish_add(void){mgx_fish_add_type(MGX_PET_GUPPY);}
static void mgx_fish_drop_add(float x,float y,int value,int kind,int rare_id){
    for(int i=0;i<MGX_FISH_DROPS;i++)if(!mgx_fish.drops[i].active){
        MgxFishDrop *d=&mgx_fish.drops[i];memset(d,0,sizeof *d);d->active=1;d->x=x;d->y=y;
        d->vy=(kind==MGX_RES_FOOD?21.0f:kind==MGX_RES_WASTE?8.0f:17.0f)+(float)((mgx_fish.resource_serial++%5)*2);
        d->bottom_age=-1.0f;d->life=kind==MGX_RES_WASTE?28.0f:0;d->value=value;d->kind=kind;d->rare_id=rare_id;return;
    }
}
static int mgx_fish_waste_count(void){int n=0;for(int i=0;i<MGX_FISH_DROPS;i++)n+=mgx_fish.drops[i].active&&mgx_fish.drops[i].kind==MGX_RES_WASTE;return n;}
static int mgx_fish_unlocked(int type){return type>=0&&type<MGX_FISH_TYPES&&mgx_fish.helpers>=mgx_fish_unlock[type];}
static void mgx_fish_notice(const char *message){snprintf(mgx_fish.notice,sizeof mgx_fish.notice,"%s",message);mgx_fish.notice_time=3;}
static void mgx_fish_select_pet(int dir){for(int i=0;i<MGX_FISH_TYPES;i++){mgx_fish.shop_type=(mgx_fish.shop_type+dir+MGX_FISH_TYPES)%MGX_FISH_TYPES;if(mgx_fish_unlocked(mgx_fish.shop_type))break;}}
static void mgx_fish_collect_drop(int i){
    MgxFishDrop *d=&mgx_fish.drops[i];if(!d->active)return;
    if(d->kind==MGX_RES_RARE){int fresh=!(mgx_fish.rare_mask&(1<<d->rare_id));mgx_fish.rare_mask|=1<<d->rare_id;mgx_fish.coins+=fresh?d->value:d->value/3;mgx_fish.level_resources++;}
    else if(mgx_fish_is_treasure(d->kind)){mgx_fish.coins+=d->value;mgx_fish.level_resources++;}
    d->active=0;
}
static void mgx_fish_set_goal(void){
    const MgxFishGoal *g=mgx_fish_goal();mgx_fish.chapter_goal=g?g->kills:mgx_clampi(5+mgx_fish.chapter/2,6,12);
}
static void mgx_fish_save_helpers(void){char path[768],tmp[780];snprintf(path,sizeof path,"%s/fish-helpers.cfg",sn_data_root());snprintf(tmp,sizeof tmp,"%s.tmp",path);FILE *f=fopen(tmp,"w");if(f){fprintf(f,"%d\n",mgx_fish.helpers);if(fclose(f)==0)rename(tmp,path);}}
static void mgx_fish_reset(void){
    memset(&mgx_fish,0,sizeof mgx_fish);mgx_fish.x=WIN_W/2;mgx_fish.y=WIN_H/2;
    mgx_fish.coins=70;mgx_fish.chapter=1;mgx_fish.gun_level=1;mgx_fish_set_goal();
    char path[768];snprintf(path,sizeof path,"%s/fish-helpers.cfg",sn_data_root());FILE *saved=fopen(path,"r");if(saved){int n=0;if(fscanf(saved,"%d",&n)==1)mgx_fish.helpers=mgx_clampi(n,0,40);fclose(saved);}
    mgx_fish.chapter=mgx_fish.helpers+1;mgx_fish_set_goal();
    mgx_fish.next_spawn=90;mgx_fish.last=SDL_GetTicks();mgx_fish_add();mgx_fish_add();
}
static void mgx_fish_spawn_monster(void){
    for(int i=0;i<MGX_FISH_MONSTERS;i++)if(!mgx_fish.monster[i].active){
        MgxFishMonster *m=&mgx_fish.monster[i];memset(m,0,sizeof *m);m->active=1;m->target=-1;
        m->boss=mgx_fish.chapter==10;m->type=m->boss?3:(mgx_fish.chapter+i)%3;
        m->max_hp=m->hp=m->boss?38:4+mgx_fish.chapter+i*2;
        m->x=(i&1)?38:WIN_W-38;m->y=176+rand()%mgx_clampi(WIN_H-265,1,9999);
        if(m->boss){m->x=WIN_W-54;m->y=(158+WIN_H-32)/2;mgx_fish.boss_spawned=1;}return;
    }
}
static void mgx_fish_chapter_advance(float reward_x,float reward_y){
    int rare=(mgx_fish.chapter-1)%8;mgx_fish_drop_add(reward_x,reward_y,90,MGX_RES_RARE,rare);
    mgx_fish.coins+=45+mgx_fish.chapter*12;mg_set_best(MG_TIDEPOOL,mgx_fish.chapter);
    mgx_fish.helpers++;mgx_fish_save_helpers();mgx_fish.egg_pieces=0;mgx_fish.intro=0;
    mgx_fish.chapter++;mgx_fish.chapter_kills=0;mgx_fish.level_resources=0;mgx_fish.level_time=0;
    mgx_fish.boss_spawned=0;mgx_fish.boss_defeated=0;mgx_fish_set_goal();mgx_fish.between=2;
    /* Invasions follow active play time, including across level changes. */
    mgx_fish.next_spawn=fmaxf(mgx_fish.next_spawn,mgx_fish.clock+5);
    mgx_fish.shop_type=mgx_fish.hatch_type;
    for(int i=0;i<MGX_FISH_MONSTERS;i++)mgx_fish.monster[i].active=0;
}
static int mgx_fish_objective_met(void){
    const MgxFishGoal *g=mgx_fish_goal();
    if(!g)return mgx_fish.chapter_kills>=mgx_fish.chapter_goal;
    return mgx_fish.chapter_kills>=g->kills&&mgx_fish.level_resources>=g->resources&&
           mgx_fish_alive()>=g->min_fish&&mgx_fish.level_time>=g->survive&&(!g->boss||mgx_fish.boss_defeated);
}
static void mgx_fish_check_goal(float x,float y){(void)x;(void)y; /* Egg purchase owns level advancement. */}
static int mgx_fish_egg_cost(void){return 70+mgx_fish.chapter*30;}
static void mgx_fish_buy_egg(void){
    int cost=mgx_fish_egg_cost();
    if(!mgx_fish.started||mgx_fish.over||mgx_fish.between||mgx_fish.intro>0||mgx_fish.collection)return;
    if(mgx_fish.coins<cost){char msg[100];snprintf(msg,sizeof msg,"Egg piece costs $%d - need $%d more",cost,cost-mgx_fish.coins);mgx_fish_notice(msg);return;}
    mgx_fish.coins-=cost;
    if(++mgx_fish.egg_pieces==3){mgx_fish.hatch_type=mgx_fish_hatch_order[mgx_fish.helpers%6];mgx_fish.hatch_time=4;mgx_fish.hatch_pending=1;mgx_fish.between=1;}
    else mgx_fish_notice("Egg piece purchased!");
}
static void mgx_fish_monster_defeated(int index){
    MgxFishMonster *m=&mgx_fish.monster[index];float x=m->x,y=m->y;int boss=m->boss;m->active=0;
    mgx_fish.total_kills++;mgx_fish.chapter_kills++;mgx_fish.coins+=18+mgx_fish.chapter*3+(boss?80:0);
    if(boss)mgx_fish.boss_defeated=1;
    if(boss||(mgx_road_hash((unsigned)mgx_fish.total_kills*71u+(unsigned)mgx_fish.chapter)%6u)==0u)
        mgx_fish_drop_add(x,y,boss?120:70,MGX_RES_RARE,(mgx_fish.total_kills+mgx_fish.chapter)%8);
    else mgx_fish_drop_add(x,y,10+mgx_fish.chapter*2,MGX_RES_GOLD,0);
    mgx_fish_check_goal(x,y);
}
static void mgx_fish_feed_pet(MgxFishPet *f){
    if(f->eat_cooldown>0)return;
    f->eat_cooldown=10;
    f->hunger=mgx_fish_full_hunger(f->type);if(f->growth<7)f->growth++;
    if(f->hp<f->max_hp)f->hp++;
}
static void mgx_fish_key(SDL_Keycode k){
    if(mgx_fish.over){if(k==SDLK_RETURN)mgx_fish_reset();return;}
    if(mgx_fish.between){if(mgx_fish.between==2&&k==SDLK_RETURN){mgx_fish.between=0;mgx_fish.intro=5;}return;}
    if(k==SDLK_f){mgx_fish.collection=!mgx_fish.collection;mgx_fish.notice_time=0;return;}
    if(mgx_fish.collection){
        if(k==SDLK_ESCAPE){mgx_fish.collection=0;return;}
        if(k==SDLK_DOWN||k==SDLK_RIGHT){mgx_fish_select_pet(1);return;}
        if(k==SDLK_UP||k==SDLK_LEFT){mgx_fish_select_pet(-1);return;}
        if(k==SDLK_RETURN){mgx_fish.collection=0;return;}
        if(k!=SDLK_s)return;
    }
    if(!mgx_fish.started){if(k==SDLK_RETURN){mgx_fish.started=1;mgx_fish.intro=5;}return;}
    if(mgx_fish.intro>0){if(k==SDLK_RETURN)mgx_fish.intro=0;return;}
    if(k==SDLK_e){mgx_fish_buy_egg();return;}
    if(k==SDLK_s){
        int type=mgx_fish.shop_type,cost=mgx_fish_cost[type];
        if(!mgx_fish_unlocked(type)){mgx_fish_notice("Hatch an egg to discover a new animal");return;}
        if(mgx_fish.coins<cost){char msg[100];snprintf(msg,sizeof msg,"%s costs $%d - need $%d more",mgx_fish_type_name[type],cost,cost-mgx_fish.coins);mgx_fish_notice(msg);return;}
        if(mgx_fish_alive()>=MGX_FISH_MAX){mgx_fish_notice("Tank full - 14 animals maximum");return;}
        if(mgx_fish_add_type(type)){mgx_fish.coins-=cost;mgx_fish_notice("Animal added to your tank");}
        return;
    }
    if(k==SDLK_q){int cost=65+mgx_fish.gun_level*45;
        if(mgx_fish.gun_level>=5){mgx_fish_notice("Ray is fully upgraded");return;}
        if(mgx_fish.coins<cost){char msg[100];snprintf(msg,sizeof msg,"Ray upgrade costs $%d - need $%d more",cost,cost-mgx_fish.coins);mgx_fish_notice(msg);return;}
        mgx_fish.coins-=cost;mgx_fish.gun_level++;mgx_fish_notice("Ray upgraded - stronger, faster shots");return;}
    if(k!=SDLK_RETURN)return;
    int target=-1;float best=1e12f;
    for(int i=0;i<MGX_FISH_MONSTERS;i++)if(mgx_fish.monster[i].active){float d=mgx_len2(mgx_fish.x-mgx_fish.monster[i].x,mgx_fish.y-mgx_fish.monster[i].y);if(d<best){best=d;target=i;}}
    if(target>=0){
        Uint32 now=SDL_GetTicks();int cooldown=mgx_clampi(440-mgx_fish.gun_level*55,150,440);
        if(!mgx_fish.last_shot||now-mgx_fish.last_shot>=(Uint32)cooldown){
            MgxFishMonster *m=&mgx_fish.monster[target];float hit_x=m->x,hit_y=m->y;
            float xx=m->x-mgx_fish.x,yy=m->y-mgx_fish.y,len=sqrtf(xx*xx+yy*yy);
            if(len<.5f){xx=m->x<WIN_W/2?1.0f:-1.0f;yy=((target+mgx_fish.chapter)&1)?.25f:-.25f;len=sqrtf(xx*xx+yy*yy);}
            xx/=len;yy/=len;float impulse=55.0f+mgx_fish.gun_level*13.0f;
            m->recoil_x=xx*impulse;m->recoil_y=yy*impulse;m->x=mgx_clampf(m->x+xx*8,34,WIN_W-34);m->y=mgx_clampf(m->y+yy*8,158,WIN_H-72);
            m->hit_flash=.20f;mgx_fish.ray_from_x=mgx_fish.x;mgx_fish.ray_from_y=mgx_fish.y;
            mgx_fish.ray_x=hit_x;mgx_fish.ray_y=hit_y;mgx_fish.ray_time=.18f;mgx_fish.last_shot=now;
            for(int p=0;p<5;p++){float spread=(float)(p-2)*13.0f;mgx_fish_particle(hit_x,hit_y,xx*(50+p*8)-yy*spread,yy*(50+p*8)+xx*spread,target&1);}
            m->hp-=mgx_fish.gun_level;if(m->hp<=0)mgx_fish_monster_defeated(target);
        }return;
    }
    if(mgx_fish.coins<10){mgx_fish_notice("Food costs $10");return;}
    int room=0;for(int i=0;i<MGX_FISH_DROPS;i++)room+=!mgx_fish.drops[i].active;
    if(!room){mgx_fish_notice("Clear falling items before dropping more food");return;}
    mgx_fish.coins-=10;
    /* Extra bites still cost money; recently fed fish ignore them. */
    mgx_fish_drop_add(mgx_fish.x,mgx_fish.y,0,MGX_RES_FOOD,0);
}
static int mgx_fish_choose_target(MgxFishMonster *m){
    int target=-1;float best=1e9f;
    for(int i=0;i<MGX_FISH_MAX;i++)if(mgx_fish.fish[i].active&&mgx_fish.fish[i].type!=MGX_PET_CRAB){
        MgxFishPet *f=&mgx_fish.fish[i];float score=sqrtf(mgx_len2(m->x-f->x,m->y-f->y));
        score-=mgx_fish_resource_value(f->type)*4.0f;score-=(f->max_hp-f->hp)*34.0f;
        if(f->type==MGX_PET_ANGEL)score-=95;
        if(f->type==MGX_PET_SEAHORSE)score-=42;
        if(score<best){best=score;target=i;}
    }
    if(target<0)for(int i=0;i<MGX_FISH_MAX;i++)if(mgx_fish.fish[i].active){target=i;break;}
    return target;
}
static void mgx_fish_kill_pet(int i){
    MgxFishPet *f=&mgx_fish.fish[i];for(int p=0;p<5;p++)mgx_fish_particle(f->x,f->y,(p-2)*18.0f,-20.0f-p*5.0f,2);
    if((mgx_fish.resource_serial++&1)==0)mgx_fish_drop_add(f->x,f->y,0,MGX_RES_WASTE,0);
    f->active=0;
}
static void mgx_fish_pet_step(int i,float dt,int waste){
    MgxFishPet *f=&mgx_fish.fish[i];f->hurt_flash=fmaxf(0,f->hurt_flash-dt);f->flee_time=fmaxf(0,f->flee_time-dt);f->eat_cooldown=fmaxf(0,f->eat_cooldown-dt);
    float drain=(f->type==MGX_PET_SHARK?.56f:f->type==MGX_PET_CLAM?.48f:.88f)*(1.0f+waste*.055f);
    f->hunger-=dt*drain;if(f->hunger<=0){f->hp--;f->hunger=10;f->hurt_flash=.35f;if(f->hp<=0){mgx_fish_kill_pet(i);return;}}
    int food=-1;float food_d=150.0f*150.0f;
    if(f->eat_cooldown<=0)for(int d=0;d<MGX_FISH_DROPS;d++)if(mgx_fish.drops[d].active&&mgx_fish.drops[d].kind==MGX_RES_FOOD){float dd=mgx_len2(f->x-mgx_fish.drops[d].x,f->y-mgx_fish.drops[d].y);if(dd<food_d){food_d=dd;food=d;}}
    if(food>=0&&f->type!=MGX_PET_CLAM&&f->type!=MGX_PET_CRAB){float len=sqrtf(food_d),xx=mgx_fish.drops[food].x-f->x,yy=mgx_fish.drops[food].y-f->y;if(len>1){f->x+=xx/len*30*dt;f->y+=yy/len*30*dt;}if(len<15){mgx_fish.drops[food].active=0;mgx_fish_feed_pet(f);}}
    else if(food>=0&&food_d<20*20){mgx_fish.drops[food].active=0;mgx_fish_feed_pet(f);}
    if(f->type==MGX_PET_CLAM){f->y=WIN_H-42;f->x=mgx_clampf(f->x,45,WIN_W-45);}
    else if(f->type==MGX_PET_CRAB){
        f->y=WIN_H-40;int pick=-1;float pd=1e9f;for(int d=0;d<MGX_FISH_DROPS;d++)if(mgx_fish.drops[d].active&&mgx_fish.drops[d].kind!=MGX_RES_FOOD){float dd=fabsf(f->x-mgx_fish.drops[d].x);if(dd<pd){pd=dd;pick=d;}}
        if(pick>=0){float dir=mgx_fish.drops[pick].x>f->x?1.0f:-1.0f;f->x+=dir*42*dt;f->dir=(int)dir;if(fabsf(f->x-mgx_fish.drops[pick].x)<17){mgx_fish_collect_drop(pick);mgx_fish_check_goal(f->x,f->y);}}
    }else{
        float speed=(15+f->type*4+(i%3)*2)*(f->hp<f->max_hp?.66f:1.0f);
        if(f->type==MGX_PET_SEAHORSE){float plant=39+(i%7)*96;f->x+=(plant-f->x)*dt*.24f;speed*=.42f;}
        int danger=-1;float dd=105.0f*105.0f;for(int m=0;m<MGX_FISH_MONSTERS;m++)if(mgx_fish.monster[m].active){float d=mgx_len2(f->x-mgx_fish.monster[m].x,f->y-mgx_fish.monster[m].y);if(d<dd){dd=d;danger=m;}}
        if(danger>=0&&(f->flee_time>0||f->hp<f->max_hp)){float len=sqrtf(dd),xx=f->x-mgx_fish.monster[danger].x,yy=f->y-mgx_fish.monster[danger].y;if(len>1){f->x+=xx/len*38*dt;f->y+=yy/len*30*dt;}}
        f->x+=f->dir*speed*dt;f->y+=sinf(mgx_fish.clock*1.4f+f->bob)*5.5f*dt;
        if(f->x<52||f->x>WIN_W-52)f->dir=-f->dir;
        f->x=mgx_clampf(f->x,45,WIN_W-45);f->y=mgx_clampf(f->y,55,WIN_H-48);
    }
    f->coin_at-=dt;if(f->coin_at<=0&&f->type!=MGX_PET_CRAB){
        int kind=mgx_fish_resource_kind(f->type);f->coin_at=mgx_fish_resource_interval(f->type);
        if(kind>=0)mgx_fish_drop_add(f->x,f->y,mgx_fish_resource_value(f->type)+f->growth,kind,0);
        if((mgx_fish.resource_serial%7)==0)mgx_fish_drop_add(f->x,f->y,0,MGX_RES_WASTE,0);
    }
    if(f->type==MGX_PET_SHARK&&f->hunger<18&&f->eat_cooldown<=0){
        int prey=-1;float pd=1e9f;for(int j=0;j<MGX_FISH_MAX;j++)if(mgx_fish.fish[j].active&&j!=i&&mgx_fish.fish[j].type<=MGX_PET_TETRA){float d=mgx_len2(f->x-mgx_fish.fish[j].x,f->y-mgx_fish.fish[j].y);if(d<pd){pd=d;prey=j;}}
        if(prey>=0){float len=sqrtf(pd),xx=mgx_fish.fish[prey].x-f->x,yy=mgx_fish.fish[prey].y-f->y;if(len>1){f->x+=xx/len*31*dt;f->y+=yy/len*31*dt;}if(len<17){mgx_fish_kill_pet(prey);mgx_fish_feed_pet(f);}}
    }
}
static void mgx_fish_step(int dx,int dy){
    Uint32 now=SDL_GetTicks();float dt=fminf((now-mgx_fish.last)/1000.0f,.05f);mgx_fish.last=now;
    if(!mgx_fish.collection&&!mgx_fish.between){mgx_fish.x=mgx_clampf(mgx_fish.x+dx*235*dt,30,WIN_W-30);mgx_fish.y=mgx_clampf(mgx_fish.y+dy*235*dt,34,WIN_H-22);}
    mgx_fish.notice_time=fmaxf(0,mgx_fish.notice_time-dt);
    mgx_fish.ray_time=fmaxf(0,mgx_fish.ray_time-dt);
    for(int p=0;p<MGX_FISH_PARTICLES;p++)if(mgx_fish.particle[p].active){MgxFishParticle *q=&mgx_fish.particle[p];q->life-=dt;q->x+=q->vx*dt;q->y+=q->vy*dt;q->vx*=.91f;q->vy=q->vy*.91f+18*dt;if(q->life<=0)q->active=0;}
    if(!mgx_fish.started||mgx_fish.over)return;
    if(mgx_fish.hatch_pending){mgx_fish.hatch_time=fmaxf(0,mgx_fish.hatch_time-dt);if(mgx_fish.hatch_time==0){mgx_fish.hatch_pending=0;mgx_fish_chapter_advance(WIN_W/2,WIN_H/2);}return;}
    if(mgx_fish.between||mgx_fish.collection)return;
    if(mgx_fish.intro>0){mgx_fish.intro=fmaxf(0,mgx_fish.intro-dt);if(mgx_fish.intro==0)mgx_fish.between=0;return;}
    mgx_fish.clock+=dt;mgx_fish.level_time+=dt;int waste=mgx_fish_waste_count();
    mgx_fish.helper_clock+=dt;
    if(mgx_fish.helpers && mgx_fish.helper_clock>=2.5f){mgx_fish.helper_clock=0;
        for(int d=0;d<MGX_FISH_DROPS;d++)if(mgx_fish.drops[d].active&&mgx_fish_is_treasure(mgx_fish.drops[d].kind)){mgx_fish_collect_drop(d);break;}
        if(mgx_fish.helpers>=3)for(int m=0;m<MGX_FISH_MONSTERS;m++)if(mgx_fish.monster[m].active){mgx_fish.monster[m].hit_flash=.2f;if(--mgx_fish.monster[m].hp<=0)mgx_fish_monster_defeated(m);break;}
    }
    if(mgx_fish.helpers>=4)for(int f=0;f<MGX_FISH_MAX;f++)if(mgx_fish.fish[f].active&&mgx_fish.fish[f].growth<3)mgx_fish.fish[f].flee_time=1;
    if(mgx_fish.helpers>=5)for(int f=0;f<MGX_FISH_MAX;f++)if(mgx_fish.fish[f].active)mgx_fish.fish[f].hunger+=dt*.35f;
    for(int i=0;i<MGX_FISH_MAX;i++)if(mgx_fish.fish[i].active)mgx_fish_pet_step(i,dt,waste);
    float floor_y=WIN_H-32;
    for(int i=0;i<MGX_FISH_DROPS;i++)if(mgx_fish.drops[i].active){
        MgxFishDrop *d=&mgx_fish.drops[i];d->y=fminf(floor_y,d->y+d->vy*dt*(mgx_fish.helpers>=2 && mgx_fish_is_treasure(d->kind)?.45f:1.0f));
        if(d->kind!=MGX_RES_FOOD&&mgx_len2(mgx_fish.x-d->x,mgx_fish.y-d->y)<22*22){mgx_fish_collect_drop(i);continue;}
        if(d->kind==MGX_RES_WASTE){d->life-=dt;if(d->life<=0)d->active=0;continue;}
        if(d->y>=floor_y-.01f){if(d->bottom_age<0)d->bottom_age=0;else d->bottom_age+=dt;
            if(d->bottom_age>=2.0f){if(d->kind==MGX_RES_FOOD)mgx_fish_drop_add(d->x,d->y,0,MGX_RES_WASTE,0);d->active=0;}}
    }
    mgx_fish.waste_tick+=dt;if(waste>=5&&mgx_fish.waste_tick>=8.0f){mgx_fish.waste_tick=0;for(int i=0;i<MGX_FISH_MAX;i++)if(mgx_fish.fish[i].active&&mgx_fish.fish[i].type!=MGX_PET_CRAB){MgxFishPet *f=&mgx_fish.fish[i];f->hp--;f->hurt_flash=.35f;if(f->hp<=0)mgx_fish_kill_pet(i);}}
    int max_monsters=mgx_fish.chapter>=7?3:mgx_fish.chapter>=4?2:1;
    if(mgx_fish.clock>=mgx_fish.next_spawn&&mgx_fish_monsters_alive()<max_monsters){mgx_fish_spawn_monster();mgx_fish.next_spawn=mgx_fish.clock+90;mgx_fish_notice("Monster incoming! A fires your ray");}
    for(int mi=0;mi<MGX_FISH_MONSTERS;mi++)if(mgx_fish.monster[mi].active){
        MgxFishMonster *m=&mgx_fish.monster[mi];m->bite_cool=fmaxf(0,m->bite_cool-dt);m->retarget-=dt;m->hit_flash=fmaxf(0,m->hit_flash-dt);
        m->x=mgx_clampf(m->x+m->recoil_x*dt,32,WIN_W-32);m->y=mgx_clampf(m->y+m->recoil_y*dt,156,WIN_H-76);m->recoil_x*=powf(.035f,dt);m->recoil_y*=powf(.035f,dt);
        if(m->target<0||m->target>=MGX_FISH_MAX||!mgx_fish.fish[m->target].active||m->retarget<=0){m->target=mgx_fish_choose_target(m);m->retarget=.55f;}
        if(m->target>=0){MgxFishPet *f=&mgx_fish.fish[m->target];float xx=f->x-m->x,yy=f->y-m->y,len=sqrtf(xx*xx+yy*yy);
            if(len>1){float speed=(23+mgx_fish.chapter*1.7f+m->type*2.5f)*(m->boss?.78f:1.0f);m->x+=xx/len*speed*dt;m->y+=yy/len*speed*dt;}
            if(len<(m->boss?31:23)&&m->bite_cool<=0){int damage=1+(mgx_fish.chapter>=7)+(m->boss?1:0);f->hp-=damage;f->hurt_flash=.45f;f->flee_time=2.2f;m->bite_cool=m->boss?.48f:.82f;m->x-=xx/(len>1?len:1)*12;if(f->hp<=0)mgx_fish_kill_pet(m->target);m->retarget=0;}}
    }
    mgx_fish_check_goal(WIN_W/2,WIN_H/2);if(!mgx_fish_alive())mgx_fish.over=1;
}
static void mgx_fish_triangle(SDL_Renderer *ren,int x,int y,int dir,int size,SDL_Color c){SDL_SetRenderDrawColor(ren,c.r,c.g,c.b,255);for(int n=0;n<size;n++)SDL_RenderDrawLine(ren,x,y-size/2+n,x-dir*(size-n),y);}
static void mgx_fish_draw_pet(SDL_Renderer *ren,const MgxFishPet *f,int index){
    int x=(int)f->x,y=(int)f->y,dir=f->dir,r=12+f->growth/2+(f->type==MGX_PET_SHARK?5:f->type==MGX_PET_ANGEL?3:0);
    static const SDL_Color colors[MGX_FISH_TYPES]={{200,211,220,255},{242,195,59,255},{224,70,103,255},{81,134,158,255},{229,205,162,255},{60,208,139,255},{237,104,67,255}};
    SDL_Color c=colors[f->type];
    if(f->hp<f->max_hp){int health=mgx_clampi(f->hp*100/f->max_hp,0,100),gray=(c.r+c.g+c.b)/3;c.r=(Uint8)((c.r*health+gray*(100-health))/100);c.g=(Uint8)((c.g*health+gray*(100-health))/100);c.b=(Uint8)((c.b*health+gray*(100-health))/100);}
    if(f->hurt_flash>0&&((int)(f->hurt_flash*30)&1))c=(SDL_Color){255,244,244,255};
    if(f->type==MGX_PET_CLAM){mgx_circle(ren,x,y,12,c,255);SDL_SetRenderDrawColor(ren,119,82,109,255);for(int n=-8;n<=8;n+=4)SDL_RenderDrawLine(ren,x,y,x+n,y-10);mgx_circle(ren,x,y-3,4,(SDL_Color){252,241,211,255},255);}
    else if(f->type==MGX_PET_SEAHORSE){mgx_circle(ren,x,y-7,6,c,255);fill_rounded(ren,(SDL_Rect){x-4,y-3,8,17},4,c.r,c.g,c.b,255);SDL_SetRenderDrawColor(ren,c.r,c.g,c.b,255);SDL_RenderDrawLine(ren,x,y+12,x+dir*8,y+17);SDL_RenderDrawLine(ren,x+dir*8,y+17,x+dir*3,y+20);}
    else if(f->type==MGX_PET_CRAB){fill_rounded(ren,(SDL_Rect){x-10,y-6,20,12},6,c.r,c.g,c.b,255);SDL_SetRenderDrawColor(ren,c.r,c.g,c.b,255);for(int n=-1;n<=1;n+=2){SDL_RenderDrawLine(ren,x+n*7,y+3,x+n*14,y+9);SDL_RenderDrawLine(ren,x+n*8,y-2,x+n*15,y-8);mgx_circle(ren,x+n*16,y-9,4,c,255);}}
    else{mgx_fish_triangle(ren,x-dir*r,y,-dir,r,c);fill_rounded(ren,(SDL_Rect){x-r,y-r/2,r*2,r},r/2,c.r,c.g,c.b,255);mgx_circle(ren,x+dir*r/2,y-r/5,2,(SDL_Color){245,251,248,255},255);mgx_circle(ren,x+dir*r/2,y-r/5,1,(SDL_Color){12,29,35,255},255);
        if(f->type==MGX_PET_TETRA){SDL_SetRenderDrawColor(ren,255,239,142,230);SDL_RenderDrawLine(ren,x-r/2,y-r/2,x-r/2,y+r/2);SDL_RenderDrawLine(ren,x+r/4,y-r/2,x+r/4,y+r/2);}
        if(f->type==MGX_PET_ANGEL){mgx_fish_triangle(ren,x,y-r/2,1,r/2,(SDL_Color){255,171,190,255});mgx_fish_triangle(ren,x,y+r/2,-1,r/2,(SDL_Color){255,171,190,255});}
        if(f->type==MGX_PET_SHARK){mgx_fish_triangle(ren,x,y-r/2,dir,r/2,(SDL_Color){58,98,121,255});SDL_SetRenderDrawColor(ren,241,242,220,255);for(int t=-2;t<=2;t+=2)SDL_RenderDrawLine(ren,x+dir*(r-4)+t,y+2,x+dir*(r-1)+t,y+5);}}
    if(f->hp<f->max_hp){int w=24*f->hp/f->max_hp;SDL_SetRenderDrawColor(ren,24,28,36,220);SDL_RenderFillRect(ren,&(SDL_Rect){x-12,y-r-8,24,3});SDL_SetRenderDrawColor(ren,255,111,105,255);SDL_RenderFillRect(ren,&(SDL_Rect){x-12,y-r-8,w,3});}
    if(f->growth>=5){SDL_SetRenderDrawColor(ren,255,229,111,220);SDL_RenderDrawLine(ren,x-3,y-r-3,x+3,y-r-3);SDL_RenderDrawPoint(ren,x,y-r-6);}(void)index;
}
static void mgx_fish_draw_resource(SDL_Renderer *ren,const MgxFishDrop *d){
    float fade=(d->bottom_age>=0)?mgx_clampf(1.0f-d->bottom_age/2.0f,0,1):1.0f;
    int pulse=d->bottom_age>=0?(int)(2+sinf(d->bottom_age*18)*2):0;int r=mgx_clampi((int)((7+pulse)*(.55f+.45f*fade)),2,11);Uint8 a=(Uint8)(255*fade);
    static const SDL_Color colors[]={{205,220,231,255},{250,199,48,255},{232,62,93,255},{89,231,247,255},{248,235,207,255},{57,222,137,255},{246,123,214,255}};
    int x=(int)d->x,y=(int)d->y;if(d->kind==MGX_RES_FOOD){mgx_circle(ren,x,y,4,(SDL_Color){243,177,96,255},255);SDL_SetRenderDrawColor(ren,255,225,157,255);SDL_RenderDrawPoint(ren,x-1,y-1);return;}
    if(d->kind==MGX_RES_WASTE){fill_rounded(ren,(SDL_Rect){x-5,y-3,10,6},3,91,104,74,230);SDL_SetRenderDrawColor(ren,143,154,93,220);SDL_RenderDrawLine(ren,x-3,y-5,x+3,y-5);return;}
    SDL_Color c=colors[mgx_clampi(d->kind,0,MGX_RES_RARE)];c.a=a;SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
    if(d->bottom_age>=0){SDL_SetRenderDrawColor(ren,255,94,107,(Uint8)(80+100*fade));SDL_RenderDrawRect(ren,&(SDL_Rect){x-r-3,y-r-3,(r+3)*2,(r+3)*2});}
    if(d->kind==MGX_RES_PEARL){mgx_circle(ren,x,y,r+3,(SDL_Color){128,92,119,a/2},a/2);mgx_circle(ren,x,y,r,c,a);}
    else if(d->kind==MGX_RES_SILVER||d->kind==MGX_RES_GOLD){mgx_circle(ren,x,y,r,c,a);SDL_SetRenderDrawColor(ren,255,255,240,a);SDL_RenderDrawLine(ren,x-2,y,x+2,y);}
    else{SDL_SetRenderDrawColor(ren,c.r,c.g,c.b,a);for(int yy=-r;yy<=r;yy++){int hw=r-abs(yy);SDL_RenderDrawLine(ren,x-hw,y+yy,x+hw,y+yy);}}
}
static void mgx_fish_text_fit(SDL_Renderer *ren,const char *text,SDL_Color color,int x,int y,int width,int height){
    SDL_Texture *t=render_text(ren,font_fixed?font_fixed:font_label,text,color);if(!t)return;
    int w,h;SDL_QueryTexture(t,NULL,NULL,&w,&h);float scale=fminf((float)width/fmaxf(w,1),(float)height/fmaxf(h,1));
    SDL_RenderCopy(ren,t,NULL,&(SDL_Rect){x,y,(int)(w*scale),(int)(h*scale)});
}
static void mgx_fish_collection_render(SDL_Renderer *ren){
    Theme *th=mg_theme();mgx_panel(ren,(SDL_Rect){24,65,WIN_W-48,WIN_H-113},(SDL_Color){8,28,43,255},th->accent2,255);
    mgx_fish_text_fit(ren,"YOUR ANIMAL COLLECTION",(SDL_Color){238,248,247,255},42,82,WIN_W-84,26);
    int row=0;for(int type=0;type<MGX_FISH_TYPES;type++)if(mgx_fish_unlocked(type)){
        int y=130+row++*30;int selected=type==mgx_fish.shop_type;char text[80];snprintf(text,sizeof text,"%s%s  $%d",selected?"> ":"  ",mgx_fish_type_name[type],mgx_fish_cost[type]);
        if(selected)mgx_panel(ren,(SDL_Rect){36,y-3,WIN_W/2-40,29},(SDL_Color){30,72,88,255},th->accent2,255);
        mgx_fish_text_fit(ren,text,selected?(SDL_Color){255,230,145,255}:(SDL_Color){215,234,239,255},42,y,WIN_W/2-56,22);
    }
    MgxFishPet p={0};p.type=mgx_fish.shop_type;p.x=WIN_W*3/4;p.y=185;p.dir=1;p.growth=22;p.hp=p.max_hp=1;mgx_fish_draw_pet(ren,&p,0);
    mgx_fish_text_fit(ren,mgx_fish_ability[p.type],(SDL_Color){211,242,244,255},WIN_W/2+5,245,WIN_W/2-48,21);
    mgx_fish_text_fit(ren,"Permanent helpers travel with you",(SDL_Color){157,196,203,255},WIN_W/2+5,280,WIN_W/2-48,19);
    mgx_fish_text_fit(ren,"Hatch eggs to discover more animals",(SDL_Color){157,196,203,255},42,WIN_H-91,WIN_W-84,21);
    mgx_fish_text_fit(ren,"Up/Down Select   X Buy   A/B/Y Return",(SDL_Color){238,248,247,255},42,WIN_H-77,WIN_W-84,21);
}
static void mgx_fish_hatch_render(SDL_Renderer *ren){
    Theme *th=mg_theme();mgx_panel(ren,(SDL_Rect){65,100,WIN_W-130,290},(SDL_Color){8,28,43,255},th->accent2,255);
    int reveal=mgx_fish.hatch_time<=1.4f;int cx=WIN_W/2,cy=220;
    mgx_fish_text_fit(ren,reveal?"A NEW COMPANION!":"YOUR EGG IS HATCHING...",(SDL_Color){255,233,171,255},95,120,WIN_W-190,28);
    if(!reveal){
        cx+=(int)(sinf((4-mgx_fish.hatch_time)*28)*(4-mgx_fish.hatch_time)*2);
        SDL_SetRenderDrawColor(ren,230,229,192,255);for(int y=-52;y<=52;y++){int hw=(int)(37*sqrtf(fmaxf(0,1-(y*y)/(52.0f*52))));SDL_RenderDrawLine(ren,cx-hw,cy+y,cx+hw,cy+y);}
        if(mgx_fish.hatch_time<3.2f){SDL_SetRenderDrawColor(ren,38,73,85,255);SDL_RenderDrawLine(ren,cx,cy-45,cx-9,cy-15);SDL_RenderDrawLine(ren,cx-9,cy-15,cx+8,cy);SDL_RenderDrawLine(ren,cx+8,cy,cx-4,cy+28);}
        mgx_fish_text_fit(ren,"Three pieces - one permanent friend",(SDL_Color){211,242,244,255},95,325,WIN_W-190,23);
    }else{
        for(int i=0;i<12;i++){float a=i*6.283185f/12+(4-mgx_fish.hatch_time)*.4f;mgx_circle(ren,cx+(int)(cosf(a)*89),cy+(int)(sinf(a)*62),3,(SDL_Color){255,225,139,255},220);}
        MgxFishPet p={0};p.type=mgx_fish.hatch_type;p.x=cx;p.y=cy;p.dir=1;p.growth=46;p.hp=p.max_hp=1;mgx_fish_draw_pet(ren,&p,0);
        mgx_fish_text_fit(ren,mgx_fish_type_name[p.type],(SDL_Color){255,233,171,255},95,291,WIN_W-190,26);
        mgx_fish_text_fit(ren,mgx_fish_ability[p.type],(SDL_Color){211,242,244,255},95,326,WIN_W-190,22);
        if(mgx_fish.between==2)mgx_fish_text_fit(ren,"Saved to your collection - A Next level",(SDL_Color){255,255,245,255},95,359,WIN_W-190,22);
    }
}
static void mgx_fish_render(SDL_Renderer *ren){
    Theme *th=mg_theme();int ci=mgx_clampi((mgx_fish.chapter-1)/4,0,10);const char *chapter=mgx_fish_chapter_name[ci];
    SDL_Rect tank={8,24,WIN_W-16,WIN_H-32};mgx_panel(ren,tank,(SDL_Color){7,39,66,255},th->accent2,255);
    for(int band=0;band<7;band++){SDL_Color water={(Uint8)(8+ci*6),(Uint8)(46+band*5+ci*3),(Uint8)(70+band*6+ci*8),255};SDL_SetRenderDrawColor(ren,water.r,water.g,water.b,255);SDL_RenderFillRect(ren,&(SDL_Rect){11,28+band*(tank.h-6)/7,tank.w-6,(tank.h-6)/7+1});}
    SDL_SetRenderDrawColor(ren,77,171,188,35);for(int q=0;q<7;q++)for(int k=0;k<3;k++)SDL_RenderDrawLine(ren,55+q*91+k*5,30,21+q*91+k*11,WIN_H-24);
    SDL_SetRenderDrawColor(ren,184,154,96,255);SDL_RenderFillRect(ren,&(SDL_Rect){25,WIN_H-38,WIN_W-50,26});
    for(int i=0;i<8;i++){int bx=45+i*79,by=WIN_H-40-(i%3)*8;mgx_circle(ren,bx,by,8+(i&1)*3,(SDL_Color){58,95,85,255},255);}
    for(int i=0;i<7;i++){int px=39+i*96;SDL_SetRenderDrawColor(ren,39,130+(i&1)*24,84,255);for(int s=0;s<3;s++)SDL_RenderDrawLine(ren,px,WIN_H-39,px-10+s*10,WIN_H-77-(i*7+s*11)%34);}
    for(int i=0;i<16;i++)mgx_circle(ren,42+(i*47)%mgx_clampi(WIN_W-80,1,9999),50+(i*71+(int)(mgx_fish.clock*13))%(WIN_H-100),2+(i%3==0),(SDL_Color){135,224,229,255},80);
    for(int i=0;i<MGX_FISH_MAX;i++)if(mgx_fish.fish[i].active)mgx_fish_draw_pet(ren,&mgx_fish.fish[i],i);
    for(int i=0;i<MGX_FISH_DROPS;i++)if(mgx_fish.drops[i].active)mgx_fish_draw_resource(ren,&mgx_fish.drops[i]);
    for(int i=0;i<MGX_FISH_MONSTERS;i++)if(mgx_fish.monster[i].active){
        MgxFishMonster *m=&mgx_fish.monster[i];int x=(int)m->x,y=(int)m->y,sz=m->boss?28:19;SDL_Color mc=m->hit_flash>0?(SDL_Color){255,235,245,255}:(i&1?(SDL_Color){184,75,157,255}:(SDL_Color){112,75,188,255});
        for(int t=-2;t<=2;t++){SDL_SetRenderDrawColor(ren,mc.r,mc.g,mc.b,220);SDL_RenderDrawLine(ren,x+t*(m->boss?8:6),y+sz/2,x+t*10+(t&1?6:-6),y+sz+8);}
        mgx_circle(ren,x,y,sz,mc,255);mgx_circle(ren,x-7,y-4,5,(SDL_Color){241,242,229,255},255);mgx_circle(ren,x+7,y-4,5,(SDL_Color){241,242,229,255},255);mgx_circle(ren,x-7,y-4,2,(SDL_Color){25,13,42,255},255);mgx_circle(ren,x+7,y-4,2,(SDL_Color){25,13,42,255},255);
        int hpw=42*m->hp/mgx_clampi(m->max_hp,1,99);SDL_SetRenderDrawColor(ren,23,22,33,220);SDL_RenderFillRect(ren,&(SDL_Rect){x-21,y-sz-11,42,4});SDL_SetRenderDrawColor(ren,255,101,133,255);SDL_RenderFillRect(ren,&(SDL_Rect){x-21,y-sz-11,hpw,4});
    }
    for(int p=0;p<MGX_FISH_PARTICLES;p++)if(mgx_fish.particle[p].active){MgxFishParticle *q=&mgx_fish.particle[p];SDL_Color c=q->tint?(SDL_Color){255,126,205,255}:(SDL_Color){104,237,250,255};mgx_circle(ren,(int)q->x,(int)q->y,2,c,(Uint8)(255*mgx_clampf(q->life/.32f,0,1)));}
    int monsters=mgx_fish_monsters_alive();if(monsters&&mgx_fish.ray_time>0){SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);SDL_SetRenderDrawColor(ren,99,242,255,105);for(int n=-3;n<=3;n++)SDL_RenderDrawLine(ren,(int)mgx_fish.ray_from_x,(int)mgx_fish.ray_from_y+n,(int)mgx_fish.ray_x,(int)mgx_fish.ray_y+n);SDL_SetRenderDrawColor(ren,242,255,255,255);SDL_RenderDrawLine(ren,(int)mgx_fish.ray_from_x,(int)mgx_fish.ray_from_y,(int)mgx_fish.ray_x,(int)mgx_fish.ray_y);}
    int x=(int)mgx_fish.x,y=(int)mgx_fish.y;SDL_Color cursor=monsters?(SDL_Color){111,242,249,255}:(SDL_Color){245,248,240,255};if(monsters){fill_rounded(ren,(SDL_Rect){x-11,y+9,22,9},4,th->accent2.r,th->accent2.g,th->accent2.b,255);SDL_SetRenderDrawColor(ren,226,247,249,255);SDL_RenderFillRect(ren,&(SDL_Rect){x+7,y+11,14,4});}SDL_SetRenderDrawColor(ren,cursor.r,cursor.g,cursor.b,235);SDL_RenderDrawLine(ren,x-9,y,x-3,y);SDL_RenderDrawLine(ren,x+3,y,x+9,y);SDL_RenderDrawLine(ren,x,y-9,x,y-3);SDL_RenderDrawLine(ren,x,y+3,x,y+9);
    SDL_SetRenderDrawColor(ren,8,28,43,255);SDL_RenderFillRect(ren,&(SDL_Rect){0,0,WIN_W,24});
    char hud[180];snprintf(hud,sizeof hud,"%s %d-%d  $%d  Egg %d/3",chapter,ci+1,(mgx_fish.chapter-1)%4+1,mgx_fish.coins,mgx_fish.egg_pieces);
    mgx_fish_text_fit(ren,hud,(SDL_Color){237,242,233,255},10,3,WIN_W-220,18);
    char shop[160];if(mgx_fish.gun_level<5)snprintf(shop,sizeof shop,"Y Animals  X %s $%d  R1 Egg $%d  L1 Ray $%d",mgx_fish_type_name[mgx_fish.shop_type],mgx_fish_cost[mgx_fish.shop_type],mgx_fish_egg_cost(),65+mgx_fish.gun_level*45);
    else snprintf(shop,sizeof shop,"Y Animals  X %s $%d  R1 Egg $%d  Ray MAX",mgx_fish_type_name[mgx_fish.shop_type],mgx_fish_cost[mgx_fish.shop_type],mgx_fish_egg_cost());
    SDL_SetRenderDrawColor(ren,5,21,33,230);SDL_RenderFillRect(ren,&(SDL_Rect){10,WIN_H-29,WIN_W-20,23});
    mgx_fish_text_fit(ren,shop,(SDL_Color){230,245,248,255},12,WIN_H-28,WIN_W-24,20);
    for(int h=0;h<mgx_fish.helpers && h<6;h++){
        MgxFishPet p={0};p.type=mgx_fish_hatch_order[h];p.x=32+h*45;p.y=51+sinf(mgx_fish.clock+h)*5;p.dir=1;p.hp=p.max_hp=1;mgx_fish_draw_pet(ren,&p,h);
    }
    if(!mgx_fish.started)mgx_overlay_message(ren,"CRAZY FISH","A Begin   Y Animal collection - buy 3 egg pieces with R1 to hatch a helper");
    else if(mgx_fish.intro>0){char title[90];snprintf(title,sizeof title,"%s - LEVEL %d",chapter,(mgx_fish.chapter-1)%4+1);
        mgx_overlay_message(ren,title,"A Feed $10 / attack. Fish eat every 10s. Monsters every 90s. L1 upgrades ray.");}
    if(mgx_fish.over)mgx_overlay_message(ren,"THE AQUARIUM IS EMPTY","A Rebuild aquarium    B Exit");
    if(mgx_fish.collection)mgx_fish_collection_render(ren);
    if(mgx_fish.between)mgx_fish_hatch_render(ren);
    if(mgx_fish.notice_time>0&&!mgx_fish.between){SDL_SetRenderDrawColor(ren,5,21,33,245);SDL_RenderFillRect(ren,&(SDL_Rect){20,WIN_H-122,WIN_W-40,30});mgx_fish_text_fit(ren,mgx_fish.notice,(SDL_Color){255,231,163,255},30,WIN_H-118,WIN_W-60,22);}
}

#endif /* SNAP_FE_MINI_GAMES_EXTRA_H */
