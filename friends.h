#ifndef SNAP_FE_FRIENDS_H
#define SNAP_FE_FRIENDS_H
/* UI-only friendship/presence channel. Gameplay keeps its existing transport.
 * A stable random ID, rather than a display name or IP, owns each saved friend.
 * Incoming invitations require an explicit local button press. */
#define SF_PORT 38431
#define SF_MAX 16
#define SF_MAGIC 0x53464632u
enum {SF_BEACON=1,SF_CONNECT,SF_INVITE,SF_FRIEND,SF_HELLO,SF_ACCEPT,SF_DENY};
typedef struct {uint32_t magic,token;uint8_t type,version;uint16_t game;
    char id[33],to[33],name[40],system[16],title[128],message[40];} SfPacket;
typedef struct {char id[33],name[40];struct sockaddr_in addr;Uint32 seen;int saved,met;} SfPeer;
static struct {
    int fd,initialized,n,sel,mini,choice,message,prompt,outgoing,start,host;
    char id[33],connected[33],status[128];SfPeer peers[SF_MAX];
    SfPacket incoming,request,response;struct sockaddr_in incoming_addr,request_addr,response_addr;
    Uint32 beacon,tx,deadline,response_until;uint32_t serial;
} sf={.fd=-1,.start=-1};
static const char *sf_messages[]={"","Trade?","Battle?","Race?","Hello!"};
static int sf_offer=-1,sf_solo=0,sf_bypass=0;
static char sf_solo_path[768],sf_solo_title[128],sf_solo_system[32];
static int sf_supported(int game){return game==MG_PONG||game==MG_TTT||game==MG_TANK;}
static int sf_find(const char *id){for(int i=0;i<sf.n;i++)if(!strcmp(sf.peers[i].id,id))return i;return -1;}
static int sf_online(int i){return i>=0&&i<sf.n&&sf.peers[i].seen&&SDL_GetTicks()-sf.peers[i].seen<6500;}
static void sf_path(char *out,size_t n,const char *name){snprintf(out,n,"%s/%s",sn_data_root(),name);}
static void sf_save(void){
    char path[768],tmp[780];sf_path(path,sizeof path,"friends.cfg");snprintf(tmp,sizeof tmp,"%s.tmp",path);
    FILE *f=fopen(tmp,"w");if(!f)return;
    for(int i=0;i<sf.n;i++)if(sf.peers[i].saved&&sf.peers[i].met)fprintf(f,"%s\t%s\n",sf.peers[i].id,sf.peers[i].name);
    int ok=!ferror(f);if(fclose(f)!=0)ok=0;if(ok)rename(tmp,path);
}
static int sf_valid_id(const char *s){if(strlen(s)!=32)return 0;for(int i=0;i<32;i++)if(!isxdigit((unsigned char)s[i]))return 0;return 1;}
static void sf_clean(char *s,size_t n){s[n-1]=0;for(size_t i=0;s[i];i++)if((unsigned char)s[i]<32||s[i]==127)s[i]=' ';}
static void sf_init(void){
    if(sf.initialized)return;sf.initialized=1;char path[768];sf_path(path,sizeof path,"player-id.txt");
    FILE *f=fopen(path,"r");if(f){if(!fgets(sf.id,sizeof sf.id,f))sf.id[0]=0;fclose(f);}
    if(!sf_valid_id(sf.id)){
        unsigned char bytes[16];FILE *r=fopen("/dev/urandom","rb");int ok=r&&fread(bytes,1,sizeof bytes,r)==sizeof bytes;if(r)fclose(r);if(!ok)return;
        for(int i=0;i<16;i++)snprintf(sf.id+i*2,3,"%02x",bytes[i]);
        f=fopen(path,"w");if(!f)return;fprintf(f,"%s\n",sf.id);fclose(f);
    }
    sf_path(path,sizeof path,"friends.cfg");f=fopen(path,"r");if(f){char line[180];while(sf.n<SF_MAX&&fgets(line,sizeof line,f)){
        char *tab=strchr(line,'\t');if(!tab)continue;*tab++=0;tab[strcspn(tab,"\r\n")]=0;
        if(!sf_valid_id(line)||!strcmp(line,sf.id)||sf_find(line)>=0)continue;
        SfPeer *p=&sf.peers[sf.n++];snprintf(p->id,sizeof p->id,"%s",line);snprintf(p->name,sizeof p->name,"%.39s",tab);p->saved=p->met=1;
    }fclose(f);}
    sf.fd=mgx_net_socket(SF_PORT);sf.serial=(uint32_t)time(NULL)^(uint32_t)getpid();
}
static void sf_packet(SfPacket *p,int type,const char *to){memset(p,0,sizeof *p);p->magic=htonl(SF_MAGIC);p->version=1;p->type=type;p->token=htonl(++sf.serial);snprintf(p->id,sizeof p->id,"%s",sf.id);snprintf(p->to,sizeof p->to,"%s",to?to:"");snprintf(p->name,sizeof p->name,"%.39s",player_name[0]?player_name:link_my_name);}
static void sf_send(const SfPacket *p,const struct sockaddr_in *addr){if(sf.fd>=0)sendto(sf.fd,p,sizeof *p,0,(const struct sockaddr*)addr,sizeof *addr);}
static void sf_request(int peer,int type,int game){
    if(!sf_online(peer)||sf.outgoing){snprintf(sf.status,sizeof sf.status,"Friend offline or an invitation is already pending");return;}
    sf_packet(&sf.request,type,sf.peers[peer].id);sf.request.game=htons(game);
    if(game>=1000 && game-1000<link_game_count){struct LinkGame *g=&link_games[game-1000];snprintf(sf.request.system,sizeof sf.request.system,"%s",g->sys);snprintf(sf.request.title,sizeof sf.request.title,"%s",g->name);}
    snprintf(sf.request.message,sizeof sf.request.message,"%s",sf_messages[sf.message]);
    sf.request_addr=sf.peers[peer].addr;sf.outgoing=1;sf.deadline=SDL_GetTicks()+30000;sf.tx=0;
    snprintf(sf.status,sizeof sf.status,"Waiting for %.39s to confirm...",sf.peers[peer].name);
}
static int sf_offer_regular(const char *path,const char *title,const char *system){
    if(sf_bypass){sf_bypass=0;return 0;}
    if(!sf_online(sf_find(sf.connected)))return 0;
    /* Only GBA/GBC/GB can be played together. Anything else launches straight
       away instead of walking every ROM folder on the card first. */
    {int lp=platform_of_path(path);if(lp<0||lp>2)return 0;}
    link_scan_games();
    for(int i=0;i<link_game_count;i++)if(!strcmp(link_games[i].path,path)){
        sf_offer=1000+i;snprintf(sf_solo_path,sizeof sf_solo_path,"%s",path);snprintf(sf_solo_title,sizeof sf_solo_title,"%s",title);snprintf(sf_solo_system,sizeof sf_solo_system,"%s",system);return 1;
    }return 0;
}
static void sf_offer_answer(int yes){if(sf_offer<0)return;if(yes)sf_request(sf_find(sf.connected),SF_INVITE,sf_offer);else sf_solo=1;sf_offer=-1;}
static int sf_resolve_game(const SfPacket *p){
    int g=ntohs(p->game);if(sf_supported(g))return g;
    if(g>=1000){link_scan_games();for(int i=0;i<link_game_count;i++)if(!strcmp(link_games[i].sys,p->system)&&!strcmp(link_games[i].name,p->title))return 1000+i;}
    return -1;
}
static void sf_accept_effect(const SfPacket *p,int host){
    int i=sf_find(p->id);if(i<0)return;
    if(p->type==SF_CONNECT){sf.peers[i].met=1;snprintf(sf.connected,sizeof sf.connected,"%s",p->id);snprintf(sf.status,sizeof sf.status,"Connected - X Add Friend, L1 Say Hello");}
    else if(p->type==SF_FRIEND && sf.peers[i].met){sf.peers[i].saved=1;sf_save();snprintf(sf.status,sizeof sf.status,"Friend added");}
    else if(p->type==SF_INVITE){sf.start=sf_resolve_game(p);sf.host=host;if(sf.start<0)snprintf(sf.status,sizeof sf.status,"Compatible game is not installed");}
}
static void sf_answer(int yes){
    SfPacket *p=&sf.incoming;int i=sf_find(p->id);
    if(i<0){sf.prompt=0;return;}
    if(p->type==SF_INVITE && sf_resolve_game(p)<0)yes=0;
    sf_packet(&sf.response,yes?SF_ACCEPT:SF_DENY,p->id);sf.response.token=p->token;
    sf.response_addr=sf.incoming_addr;sf.response_until=SDL_GetTicks()+30000;sf_send(&sf.response,&sf.response_addr);
    if(yes)sf_accept_effect(p,0);sf.prompt=0;
}
static void sf_tick(void){
    sf_init();if(sf.fd<0)return;Uint32 now=SDL_GetTicks();
    if(now-sf.beacon>=1500){sf.beacon=now;SfPacket p;sf_packet(&p,SF_BEACON,NULL);struct sockaddr_in a={0};a.sin_family=AF_INET;a.sin_port=htons(SF_PORT);a.sin_addr.s_addr=htonl(INADDR_BROADCAST);sf_send(&p,&a);}
    for(int budget=0;budget<24;budget++){
        SfPacket p;struct sockaddr_in a;socklen_t len=sizeof a;ssize_t n=recvfrom(sf.fd,&p,sizeof p,0,(struct sockaddr*)&a,&len);if(n<0)break;
        if(n!=sizeof p||ntohl(p.magic)!=SF_MAGIC||p.version!=1)continue;
        p.id[32]=p.to[32]=0;sf_clean(p.name,sizeof p.name);sf_clean(p.system,sizeof p.system);sf_clean(p.title,sizeof p.title);sf_clean(p.message,sizeof p.message);
        if(!sf_valid_id(p.id)||!strcmp(p.id,sf.id))continue;
        int i=sf_find(p.id);if(i<0){if(sf.n==SF_MAX)continue;i=sf.n++;snprintf(sf.peers[i].id,sizeof sf.peers[i].id,"%s",p.id);}
        SfPeer *peer=&sf.peers[i];peer->addr=a;peer->seen=now;snprintf(peer->name,sizeof peer->name,"%s",p.name);
        if(p.type==SF_BEACON||strcmp(p.to,sf.id))continue;
        if((p.type==SF_ACCEPT||p.type==SF_DENY)&&sf.outgoing&&p.token==sf.request.token&&!strcmp(p.id,sf.request.to)&&a.sin_addr.s_addr==sf.request_addr.sin_addr.s_addr){
            sf.outgoing=0;
            if(p.type==SF_ACCEPT){SfPacket accepted=sf.request;snprintf(accepted.id,sizeof accepted.id,"%s",p.id);sf_accept_effect(&accepted,1);}
            else snprintf(sf.status,sizeof sf.status,"Invitation declined");continue;
        }
        if(p.type<SF_CONNECT||p.type>SF_HELLO)continue;
        if(sf.response_until&&!SDL_TICKS_PASSED(now,sf.response_until)&&p.token==sf.response.token&&!strcmp(p.id,sf.response.to)){sf_send(&sf.response,&a);continue;}
        if(p.type!=SF_CONNECT && strcmp(p.id,sf.connected))continue;
        if(p.type==SF_HELLO){snprintf(sf.status,sizeof sf.status,"%.39s says hello!",p.name);continue;}
        if(p.type==SF_FRIEND&&!peer->met)continue;
        if(!sf.prompt){sf.incoming=p;sf.incoming_addr=a;sf.prompt=1;}
    }
    if(sf.outgoing){if(SDL_TICKS_PASSED(now,sf.deadline)){sf.outgoing=0;snprintf(sf.status,sizeof sf.status,"Invitation timed out");}
        else if(!sf.tx||now-sf.tx>=450){sf.tx=now;sf_send(&sf.request,&sf.request_addr);}}
    int connected=sf_find(sf.connected);if(sf.connected[0]&&!sf_online(connected)){sf.connected[0]=0;snprintf(sf.status,sizeof sf.status,"Friend disconnected");}
    /* A successful gameplay connection also establishes a recent player. */
    if(mgx_net.connected)for(int i=0;i<sf.n;i++)if(sf_online(i)&&sf.peers[i].addr.sin_addr.s_addr==mgx_net.peer.sin_addr.s_addr){sf.peers[i].met=1;snprintf(sf.connected,sizeof sf.connected,"%s",sf.peers[i].id);}
    if(link_conn>=0&&link_said_hello)for(int i=0;i<sf.n;i++)if(sf_online(i)&&!strcmp(inet_ntoa(sf.peers[i].addr.sin_addr),link_peer_ip)){sf.peers[i].met=1;snprintf(sf.connected,sizeof sf.connected,"%s",sf.peers[i].id);}
}
static void sf_launch(AppState *state){
    if(sf.start<0)return;int game=sf.start;sf.start=-1;int peer=sf_find(sf.connected);if(!sf_online(peer))return;
    if(game>=1000){struct LinkGame *g=&link_games[game-1000];snprintf(link_my_game,sizeof link_my_game,"%s",g->name);snprintf(link_my_sys,sizeof link_my_sys,"%s",g->sys);snprintf(link_my_path,sizeof link_my_path,"%s",g->path);link_my_mode=g->mode;link_open();if(sf.host)link_host();else link_join(inet_ntoa(sf.peers[peer].addr.sin_addr));*state=STATE_LINK;return;}
    mg_cur=game;int mode=sf.host?1:2;
    if(game==MG_PONG){mgx_pong_reset();mgx_pong_choice=mode;mgx_pong_key(SDLK_RETURN);}
    if(game==MG_TTT){mgx_c4_reset();mgx_c4_choice=mode;mgx_c4_key(SDLK_RETURN);}
    if(game==MG_TANK){mgx_tank_reset();mgx_tank.choice=mode;mgx_tank_key(SDLK_RETURN);mgx_tank_key(SDLK_RETURN);}
    if(!sf.host && mgx_net.fd>=0){mgx_net.peer=sf.peers[peer].addr;mgx_net.peer.sin_port=0; /* Discovery obtains the host's fresh gameplay session ID. */}
    *state=STATE_MINIGAME;mgx_exit_confirm=0;
}
static void sf_key(SDL_Keycode k,AppState *state){
    if(k==SDLK_ESCAPE){*state=STATE_LINK;sf.mini=0;return;}
    if(k==SDLK_LEFT)sf.message=(sf.message+4)%5;if(k==SDLK_RIGHT)sf.message=(sf.message+1)%5;
    if(sf.mini){static const int games[]={MG_PONG,MG_TTT,MG_TANK};sf.choice=mgx_clampi(sf.choice,0,2);
        if(k==SDLK_f){sf.mini=0;return;}
        if(k==SDLK_DOWN)sf.choice=(sf.choice+1)%3;if(k==SDLK_UP)sf.choice=(sf.choice+2)%3;
        if(k==SDLK_RETURN){int p=sf_find(sf.connected);if(sf_online(p))sf_request(p,SF_INVITE,games[sf.choice]);else{sf.start=games[sf.choice];mg_cur=sf.start;sf.start=-1;if(mg_cur==MG_PONG)mgx_pong_reset();if(mg_cur==MG_TTT)mgx_c4_reset();if(mg_cur==MG_TANK)mgx_tank_reset();*state=STATE_MINIGAME;}}return;}
    if(k==SDLK_e){sf.mini=1;return;}if(!sf.n)return;
    if(k==SDLK_UP)sf.sel=(sf.sel+sf.n-1)%sf.n;if(k==SDLK_DOWN)sf.sel=(sf.sel+1)%sf.n;SfPeer *p=&sf.peers[sf.sel];
    if(k==SDLK_RETURN)sf_request(sf.sel,SF_CONNECT,0);
    if(k==SDLK_s&&p->met&&!strcmp(p->id,sf.connected))sf_request(sf.sel,SF_FRIEND,0);
    if(k==SDLK_f&&p->saved){p->saved=0;sf_save();}
    if(k==SDLK_q&&!strcmp(p->id,sf.connected)){SfPacket hello;sf_packet(&hello,SF_HELLO,p->id);sf_send(&hello,&p->addr);}
}
/* This page owns navigation immediately after the shared wake/modal controls.
 * Keep Link shortcuts and the resulting page in one route so a pad press cannot
 * fall through to the system picker or another screen's directional controls. */
static int sf_route_key(SDL_Keycode k,AppState *state){
    if(*state==STATE_FRIENDS){sf_key(k,state);return 1;}
    if(*state==STATE_LINK&&(k==SDLK_e||k==SDLK_f)){
        sf.mini=k==SDLK_e;sf.choice=mgx_clampi(sf.choice,0,2);
        *state=STATE_FRIENDS;return 1;
    }
    return 0;
}
static int sf_header(SDL_Renderer *ren,const char *title){
    Theme *th=mg_theme();int right=WIN_W-30;
    SDL_Texture *shortcuts=render_text(ren,font_label,"R1 Mini Games    Y Friends",g_ui_text);
    int sw=0,sh=0;if(shortcuts)SDL_QueryTexture(shortcuts,NULL,NULL,&sw,&sh);
    float scale=sh>0?28.0f/sh:1.0f;
    int sx=right;
    if(sw*scale>334){
        /* Wide pixel faces stay legible by stacking, instead of shrinking. */
        const char *labels[]={"R1 Mini Games","Y Friends"};SDL_Texture *lines[2];
        int widths[2]={0},heights[2]={0},widest=0;
        for(int i=0;i<2;i++){
            lines[i]=render_text(ren,font_label,labels[i],g_ui_text);int w=0,h=0;
            if(lines[i])SDL_QueryTexture(lines[i],NULL,NULL,&w,&h);
            float ls=h>0?24.0f/h:1.0f;if(w*ls>334)ls=334.0f/w;
            widths[i]=(int)(w*ls);heights[i]=(int)(h*ls);if(widths[i]>widest)widest=widths[i];
        }
        sx=right-widest;mgx_panel(ren,(SDL_Rect){sx-10,52,widest+20,54},th->bg,th->dim,240);
        for(int i=0;i<2;i++)if(lines[i])SDL_RenderCopy(ren,lines[i],NULL,&(SDL_Rect){right-widths[i],54+i*25,widths[i],heights[i]});
    }else if(shortcuts){
        int dw=(int)(sw*scale),dh=(int)(sh*scale);sx=right-dw;
        mgx_panel(ren,(SDL_Rect){sx-10,62,dw+20,36},th->bg,th->dim,240);
        SDL_RenderCopy(ren,shortcuts,NULL,&(SDL_Rect){sx,80-dh/2,dw,dh});
    }
    SDL_Texture *name=render_text(ren,font_small,title,th->accent2);
    int nw=0,nh=0;if(name)SDL_QueryTexture(name,NULL,NULL,&nw,&nh);
    scale=nh>0?30.0f/nh:1.0f;
    int available=sx-40-24;if(nw*scale>available&&nw>0)scale=(float)available/nw;
    if(name)SDL_RenderCopy(ren,name,NULL,&(SDL_Rect){40,80-(int)(nh*scale)/2,(int)(nw*scale),(int)(nh*scale)});
    return 100;
}
static void sf_render(SDL_Renderer *ren){
    Theme *th=mg_theme();if(sf.mini)sf_header(ren,"MINI GAMES");else mg_chrome(ren,"FRIENDS");
    if(sf.mini){static const char *names[]={"PONG","CONNECT 4","TANK DUEL"};for(int i=0;i<3;i++){
        int selected=i==sf.choice;SDL_Rect r={30,112+i*65,WIN_W-60,52};
        mgx_panel(ren,r,selected?th->select_bg:th->bg,selected?th->accent2:th->dim,selected?255:190);
        if(selected){SDL_SetRenderDrawColor(ren,th->accent2.r,th->accent2.g,th->accent2.b,255);SDL_RenderFillRect(ren,&(SDL_Rect){r.x+2,r.y+2,6,r.h-4});}
        mgx_text(ren,font_label,names[i],selected?g_ui_text:g_ui_dim,55,r.y+15);
        if(selected)mgx_text(ren,font_label,"<",th->accent2,WIN_W-65,r.y+15);
    }}
    else{int first=sf.sel>4?sf.sel-4:0;for(int i=first;i<sf.n&&i<first+5;i++){SfPeer *p=&sf.peers[i];char label[120];snprintf(label,sizeof label,"%s  %s%s",p->name,p->saved?(sf_online(i)?"ONLINE":"OFFLINE"):"NEARBY",!strcmp(p->id,sf.connected)?" - CONNECTED":"");SDL_Rect r={26,106+(i-first)*48,WIN_W-52,42};mgx_panel(ren,r,th->bg,i==sf.sel?th->accent2:th->dim,220);mgx_text(ren,font_label,label,g_ui_text,36,r.y+9);}
        if(!sf.n)mgx_text_center(ren,font_label,"Looking for nearby SNAP FE devices...",g_ui_dim,WIN_W/2,160);
    }
    char msg[100];snprintf(msg,sizeof msg,"Message: %s  (Left/Right)",sf_messages[sf.message][0]?sf_messages[sf.message]:"None");mgx_text(ren,font_fixed?font_fixed:font_label,msg,g_ui_dim,30,354);
    mgx_text(ren,font_fixed?font_fixed:font_label,sf.status,th->accent2,30,382);
    mgx_text(ren,font_label,sf.mini?"Up/Down Choose   A Play / Invite   B Back":"A Connect  X Add  Y Remove  L1 Hello  B Back",g_ui_dim,30,WIN_H-34);
}
static void sf_prompt_render(SDL_Renderer *ren){if(sf_offer>=0){char sub[120];snprintf(sub,sizeof sub,"%s  A Invite / B Play solo   Left/Right Message",sf_messages[sf.message]);mgx_overlay_message(ren,"INVITE CONNECTED PLAYER?",sub);return;}if(!sf.prompt)return;char title[100],sub[170];SfPacket *p=&sf.incoming;
    snprintf(title,sizeof title,"%.39s %s",p->name,p->type==SF_CONNECT?"wants to connect":p->type==SF_FRIEND?"sent a friend request":"invites you");
    int g=ntohs(p->game);const char *game=sf_supported(g)?mg_games[g].name:p->title;
    snprintf(sub,sizeof sub,"%.40s %.39s  A CONFIRM / B DENY",p->type==SF_INVITE?game:"",p->message);mgx_overlay_message(ren,title,sub);
}
#endif
