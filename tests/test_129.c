/* Isolated tests exercise production functions, without starting the frontend,
   scanning personal ROMs, touching hardware, or changing user settings. */
#define main snapfe_application_main
#define SNAPFE_TESTING 1
#include "../main.c"
#undef main
#include <assert.h>

static void capture(SDL_Renderer *ren, const char *name) {
    char path[256]; snprintf(path,sizeof path,"tests/output/%s.png",name);
    SDL_Surface *s=SDL_CreateRGBSurfaceWithFormat(0,WIN_W,WIN_H,32,SDL_PIXELFORMAT_ARGB8888);
    assert(s && SDL_RenderReadPixels(ren,NULL,s->format->format,s->pixels,s->pitch)==0);
    assert(IMG_SavePNG(s,path)==0); SDL_FreeSurface(s);
    draw_theme_background(ren,&themes[theme_idx],theme_idx);
}

static void test_puzzle_art_filter(void) {
    SDL_Surface *s=SDL_CreateRGBSurfaceWithFormat(0,96,64,32,SDL_PIXELFORMAT_RGBA32);
    assert(s);
    SDL_FillRect(s,NULL,SDL_MapRGBA(s->format,0,0,0,0));
    assert(!mgx_swap_surface_has_picture(s));
    SDL_FillRect(s,NULL,SDL_MapRGBA(s->format,255,255,255,255));
    assert(!mgx_swap_surface_has_picture(s));
    SDL_FillRect(s,NULL,SDL_MapRGBA(s->format,35,92,151,255));
    SDL_FillRect(s,&(SDL_Rect){12,9,70,42},SDL_MapRGBA(s->format,242,171,51,255));
    assert(mgx_swap_surface_has_picture(s));
    SDL_FreeSurface(s);
}

static int crossing_find_zone(int wanted) {
    for(int section=2;section<240;section++)
        if(mgx_road_zone(section*6)==wanted)return section*6;
    return -1;
}

static void crossing_capture_zone(SDL_Renderer *ren,int zone,const char *name) {
    int row=crossing_find_zone(zone);assert(row>=0);
    mgx_road_reset();mgx_road.scroll_base=row;mgx_road.farthest=row;
    mgx_road.distance=row;mgx_road.started=1;mgx_road.clock=2.35f;
    draw_theme_background(ren,&themes[theme_idx],theme_idx);
    mgx_road_render(ren);capture(ren,name);
}

static void test_crossing_world(SDL_Renderer *ren) {
    unsigned zones=0;
    mgx_road_reset();
    for(int row=0;row<1200;row++) {
        int zone=mgx_road_zone(row),kind=mgx_road_kind(row);zones|=1u<<zone;
        if(row%6==0)assert(kind==MGX_LAND);
        if(kind==MGX_FOREST||kind==MGX_BUILD||kind==MGX_BRIDGE) {
            int passable=0;
            for(int x=30;x<=WIN_W-30;x+=2)if(!mgx_road_fixed_blocked(row,(float)x)){passable=1;break;}
            assert(passable);
        }
        if(mgx_road_has_coin(row)) {
            int x=mgx_road_coin_x(row);
            assert(x>=30&&x<=WIN_W-30&&!mgx_road_fixed_blocked(row,(float)x));
        }
        if(kind==MGX_RIVER) {
            for(int sample=0;sample<24;sample++) {
                int visible=0;mgx_road.clock=sample*.31f;
                for(int i=0;i<MGX_ROAD_LOG_COUNT;i++) {
                    float x=mgx_road_log_x(row,i);
                    visible+=x<WIN_W-20&&x+MGX_ROAD_LOG_WIDTH>20;
                }
                assert(visible>=3);
            }
        }
    }
    assert(zones==((1u<<(MGX_ZONE_BRIDGE+1))-1u));
    for(int row=0;row<12;row++)
        assert(mgx_road_zone(row)==MGX_ZONE_ROADS||mgx_road_zone(row)==MGX_ZONE_WOODS);
    mgx_road.farthest=0;float early=mgx_road_speed(13);
    mgx_road.farthest=220;assert(mgx_road_speed(13)>early);
    crossing_capture_zone(ren,MGX_ZONE_RIVER,"crossing-river");
    crossing_capture_zone(ren,MGX_ZONE_STATION,"crossing-railroad");
    crossing_capture_zone(ren,MGX_ZONE_CONSTRUCTION,"crossing-construction");
    crossing_capture_zone(ren,MGX_ZONE_NIGHT,"crossing-night");
    crossing_capture_zone(ren,MGX_ZONE_CITY,"crossing-city");
}

static int fish_find_drop(int kind) {
    for(int i=0;i<MGX_FISH_DROPS;i++)
        if(mgx_fish.drops[i].active&&mgx_fish.drops[i].kind==kind)return i;
    return -1;
}

static void test_fish_campaign(void) {
    /* Feeding is physical: food appears at the cursor, is consumed by a
       nearby fish, heals it, and grows it. */
    mgx_fish_reset();mgx_fish_key(SDLK_RETURN);mgx_fish.intro=0;int cash=mgx_fish.coins;
    mgx_fish.x=mgx_fish.fish[0].x;mgx_fish.y=mgx_fish.fish[0].y;
    mgx_fish.fish[0].hunger=1;mgx_fish.fish[0].hp=1;
    int old_growth=mgx_fish.fish[0].growth;mgx_fish_key(SDLK_RETURN);mgx_fish.intro=0;
    int food=fish_find_drop(MGX_RES_FOOD);assert(food>=0&&mgx_fish.coins==cash-10);
    mgx_fish.drops[food].x=mgx_fish.fish[0].x;mgx_fish.drops[food].y=mgx_fish.fish[0].y;
    mgx_fish.last=SDL_GetTicks()-20;mgx_fish_step(0,0);
    assert(!mgx_fish.drops[food].active&&mgx_fish.fish[0].growth==old_growth+1&&
           mgx_fish.fish[0].hunger>1&&mgx_fish.fish[0].hp==2);

    /* A resource gets exactly its two-second rescue window only after it
       reaches the floor. */
    mgx_fish_drop_add(250,WIN_H-32,7,MGX_RES_GOLD,0);
    int drop=fish_find_drop(MGX_RES_GOLD);assert(drop>=0);
    mgx_fish.drops[drop].bottom_age=1.90f;mgx_fish.last=SDL_GetTicks()-40;
    mgx_fish_step(0,0);assert(mgx_fish.drops[drop].active&&mgx_fish.drops[drop].bottom_age<2.0f);
    mgx_fish.drops[drop].bottom_age=1.99f;mgx_fish.last=SDL_GetTicks()-40;
    mgx_fish_step(0,0);assert(!mgx_fish.drops[drop].active);

    /* The first invasion occurs after ninety active seconds, even on level one. */
    mgx_fish_reset();mgx_fish_key(SDLK_RETURN);mgx_fish.intro=0;mgx_fish.clock=89;
    mgx_fish.last=SDL_GetTicks()-20;mgx_fish_step(0,0);assert(!mgx_fish_monsters_alive());
    mgx_fish.clock=90;mgx_fish.last=SDL_GetTicks()-20;mgx_fish_step(0,0);
    assert(mgx_fish_monsters_alive()==1);

    /* Contact damages before it kills, making health meaningful. */
    mgx_fish_reset();mgx_fish_key(SDLK_RETURN);mgx_fish.intro=0;mgx_fish_spawn_monster();
    int hp=mgx_fish.fish[0].hp,before=mgx_fish_alive();
    mgx_fish.monster[0].target=0;mgx_fish.monster[0].retarget=10;
    mgx_fish.monster[0].x=mgx_fish.fish[0].x;mgx_fish.monster[0].y=mgx_fish.fish[0].y;
    mgx_fish.monster[0].bite_cool=0;mgx_fish.last=SDL_GetTicks()-16;mgx_fish_step(0,0);
    assert(mgx_fish_alive()==before&&mgx_fish.fish[0].hp==hp-1);
    while(mgx_fish.fish[0].active) {
        mgx_fish.monster[0].target=0;mgx_fish.monster[0].retarget=10;
        mgx_fish.monster[0].x=mgx_fish.fish[0].x;mgx_fish.monster[0].y=mgx_fish.fish[0].y;
        mgx_fish.monster[0].bite_cool=0;mgx_fish.last=SDL_GetTicks()-16;mgx_fish_step(0,0);
    }
    assert(mgx_fish_alive()==before-1);

    /* Shots always repel away from the cursor's impact direction. */
    static const float direction[4][2]={{1,0},{-1,0},{0,1},{0,-1}};
    for(int d=0;d<4;d++) {
        mgx_fish_reset();mgx_fish_key(SDLK_RETURN);mgx_fish.intro=0;mgx_fish_spawn_monster();
        MgxFishMonster *m=&mgx_fish.monster[0];m->x=320;m->y=250;m->hp=m->max_hp=100;
        mgx_fish.x=m->x-direction[d][0]*30;mgx_fish.y=m->y-direction[d][1]*30;
        mgx_fish.last_shot=0;mgx_fish_key(SDLK_RETURN);mgx_fish.intro=0;
        assert(m->active&&m->recoil_x*direction[d][0]+m->recoil_y*direction[d][1]>40);
        assert((m->x-320)*direction[d][0]+(m->y-250)*direction[d][1]>5);
    }

    /* Target selection values both production roles and visible injury. */
    mgx_fish_reset();memset(mgx_fish.fish,0,sizeof mgx_fish.fish);
    assert(mgx_fish_add_type(MGX_PET_GUPPY)&&mgx_fish_add_type(MGX_PET_ANGEL));
    MgxFishMonster probe={0};probe.x=300;probe.y=250;
    mgx_fish.fish[0].x=290;mgx_fish.fish[0].y=250;
    mgx_fish.fish[1].x=400;mgx_fish.fish[1].y=250;
    assert(mgx_fish_choose_target(&probe)==1);
    mgx_fish.fish[0].type=MGX_PET_TETRA;mgx_fish.fish[0].max_hp=4;
    mgx_fish.fish[0].hp=1;mgx_fish.fish[0].x=230;
    assert(mgx_fish_choose_target(&probe)==0);

    unsigned resource_kinds=0;
    for(int type=0;type<MGX_PET_CRAB;type++) {
        int kind=mgx_fish_resource_kind(type);assert(kind>=0&&kind<MGX_RES_FOOD);
        assert(!(resource_kinds&(1u<<kind)));resource_kinds|=1u<<kind;
    }
    assert(mgx_fish_resource_kind(MGX_PET_CRAB)<0&&
           mgx_fish_resource_value(MGX_PET_SHARK)>mgx_fish_resource_value(MGX_PET_ANGEL));

    mgx_fish_reset();mgx_fish_key(SDLK_RETURN);mgx_fish.intro=0;mgx_fish.chapter=7;mgx_fish.helpers=6;
    mgx_fish.coins=1000;mgx_fish.shop_type=MGX_PET_SHARK;int fish_count=mgx_fish_alive();
    mgx_fish_key(SDLK_s);assert(mgx_fish_alive()==fish_count+1);
    int sharks=0;for(int i=0;i<MGX_FISH_MAX;i++)
        sharks+=mgx_fish.fish[i].active&&mgx_fish.fish[i].type==MGX_PET_SHARK;
    assert(sharks==1);
}

static void test_sleep_screens(SDL_Renderer *ren) {
    static const int modes[] = {
        SCREEN_SAVER_BOUNCE, SCREEN_SAVER_STARFIELD,
        SCREEN_SAVER_AQUARIUM, SCREEN_SAVER_SYSTEM_DREAM
    };
    static const char *shots[] = {
        "sleep-dvd-bounce", "sleep-starfield", "sleep-aquarium", "sleep-system-dream"
    };
    assert(SCREEN_SAVER_COUNT==6);
    assert(!strcmp(screen_saver_names[SCREEN_SAVER_OFF],"Off"));
    assert(!strcmp(screen_saver_names[SCREEN_SAVER_RANDOM],"Random"));
    srand(0x129);
    for(int i=0;i<4;i++) {
        screen_saver_idx=modes[i];screen_saver_begin();
        assert(screen_saver_active&&screen_saver_runtime==modes[i]);
        Uint32 now=SDL_GetTicks();
        /* Advance purely visual phases so every static capture is representative. */
        if(modes[i]==SCREEN_SAVER_STARFIELD) {
            screen_saver_started=now-4300u;
            saver_star_effect=1;saver_star_effect_started=now-650u;saver_star_effect_until=now+850u;
        } else if(modes[i]==SCREEN_SAVER_AQUARIUM) {
            saver_aqua_event=1;saver_aqua_event_started=now-900u;saver_aqua_event_until=now+1700u;
        } else if(modes[i]==SCREEN_SAVER_SYSTEM_DREAM) {
            screen_saver_started=now-6200u;
        }
        draw_screen_saver(ren);capture(ren,shots[i]);
        screen_saver_wake();
        assert(!screen_saver_active&&screen_saver_started==0&&screen_saver_last_frame==0);
        assert(!saver_star_effect&&!saver_aqua_event&&!saver_star_logo_until);
    }

    /* Random is resolved only at activation and must reach all four live
       scenes over repeated sleeps; Off and Random can never become runtime. */
    unsigned seen=0;
    screen_saver_idx=SCREEN_SAVER_RANDOM;srand(0x129);
    for(int i=0;i<96;i++) {
        screen_saver_begin();
        assert(screen_saver_runtime>=SCREEN_SAVER_BOUNCE&&
               screen_saver_runtime<=SCREEN_SAVER_SYSTEM_DREAM);
        seen|=1u<<screen_saver_runtime;
        screen_saver_wake();
    }
    assert(seen==((1u<<SCREEN_SAVER_BOUNCE)|(1u<<SCREEN_SAVER_STARFIELD)|
                  (1u<<SCREEN_SAVER_AQUARIUM)|(1u<<SCREEN_SAVER_SYSTEM_DREAM)));
    screen_saver_idx=SCREEN_SAVER_BOUNCE;
}
/* Runs the actual gameplay/transport code on two devices without opening a
   display, reading user settings or stopping the running frontend. */
static int test_nearby(int role) {
    assert(SDL_Init(SDL_INIT_TIMER)==0);
    for(int game=0;game<3;game++) {
        if(!game){mgx_pong_reset();mgx_pong_choice=role;mgx_pong_key(SDLK_RETURN);}
        else if(game==2){mgx_c4_reset();mgx_c4_choice=role;mgx_c4_key(SDLK_RETURN);}
        else {
            mgx_tank_reset();mgx_tank.choice=role;
            mgx_tank_key(SDLK_RETURN); /* mode -> map chooser */
            mgx_tank_key(SDLK_RETURN); /* map chooser -> match */
        }
        Uint32 started=SDL_GetTicks(),connected_at=0;int packets=0;uint32_t prev=0;
        while(SDL_GetTicks()-started<14000) {
            int direction=((SDL_GetTicks()-started)/700)%2?1:-1;
            if(!game){if(mgx_net.connected)mgx_pong_key(SDLK_RETURN);mgx_pong_step(direction);}else if(game==1)mgx_tank_step(0,direction);else{mgx_c4_step();if(mgx_net.connected&&!tt.over){tt.cur=mgx_c4_ply()%7;mgx_c4_key(SDLK_RETURN);}}
            if(mgx_net.rx_seq!=prev){packets++;prev=mgx_net.rx_seq;}
            if(mgx_net.connected&&!connected_at)connected_at=SDL_GetTicks();
            if(connected_at&&SDL_GetTicks()-connected_at>5000)break;
            SDL_Delay(16);
        }
        printf("%s %s: connected=%d packets=%d\n",game==2?"Connect4":game?"Tank":"Pong",role==1?"host":"join",mgx_net.connected,packets);
        fflush(stdout);assert(mgx_net.connected&&packets>=40);
        if(!game)assert(pg.started&&isfinite(pg.bx)&&pg.you>=pg.top&&pg.cpu<=pg.bot);
        else if(game==2)assert(mgx_c4_ply()>1);
        else assert(mgx_tank.hp[0]>0&&mgx_tank.hp[1]>0&&isfinite(mgx_tank.y[1]));
        mgx_net_close();SDL_Delay(500);
    }
    puts("PASS: automatic LAN discovery, handshake, bidirectional input/state, Pong, Tank and Connect 4 simulation");
    SDL_Quit();return 0;
}
#include "test_continued_129.h"
#include "test_friends_129.h"
#include "test_assets_129.h"
#include "test_fish_feedback.h"
#include "test_pong_latency.h"
#include "test_link_navigation.h"
#include "test_widget_navigation.h"
#include "test_sleep_duration.h"
#include "test_battery_led.h"
#include "test_battery_game_notice.h"
#include "test_battery_prompt.h"
#include "test_location_catalog.h"
#include "test_widget_places.h"
#include "test_weather_refresh.h"
#include "test_systems_dropdown.h"
#include "test_sound_options.h"
#include "test_surprise_art_odds.h"
int main(int argc,char **argv) {
#ifdef SNAPOS_TARGET_KNULLI
    if(argc==2&&!strcmp(argv[1],"--widget-smoke")){
        char temporary[]="/tmp/snapfe-widget-check-XXXXXX";assert(mkdtemp(temporary));assert(!setenv("SNAPFE_DATA_ROOT",temporary,1));
        assert(SDL_Init(SDL_INIT_TIMER)==0);widget_places_init();widget_place_group=0;widget_place_search("Tokoyo");
        assert(widget_place_result_count>0&&!strcmp(wloc_items[widget_place_results[0]].name,"Tokyo"));
        AppState state=STATE_WIDGET_PLACES;widget_places_key(SDLK_RETURN,&state);assert(!strcmp(widget_place_name(0),"Tokyo"));
        assert(widget_place_time(0,0).tm_hour==9);widget_places_loaded=0;widget_places_init();assert(!strcmp(widget_place_name(0),"Tokyo"));
        widget_place_group=1;widget_place_search("Phonix");assert(widget_place_result_count>0);widget_places_key(SDLK_RETURN,&state);assert(strstr(weather_loc,"Phoenix"));
        int rows[MAX_DISPLAY_ROWS],extra[MAX_DISPLAY_ROWS];disp_grp_view_open=1;int n=build_display_rows(rows,extra),headers=0;
        for(int i=0;i<n;i++)if(rows[i]==ROW_DISP_GRP_VIEW){headers++;assert(i+1<n&&rows[i+1]==ROW_DISP_SHOW_EMPTY);}assert(headers==1);
        printf("PASS: %d installed city choices, typo selection, saved Tokyo clock, Phoenix weather, single Systems View group\n",wloc_count);
        for(int i=0;i<SCREEN_SAVER_DURATION_COUNT;i++){
            screen_saver_duration_idx=i;save_settings();screen_saver_duration_idx=-1;load_settings();
            assert(screen_saver_duration_idx==i);screen_saver_active=1;screen_saver_started=1000u;
            assert(!screen_saver_should_sleep(1000u+screen_saver_duration_ms()-1u));
            assert(screen_saver_should_sleep(1000u+screen_saver_duration_ms()));screen_saver_wake();
            assert(!screen_saver_should_sleep(1000000u));
        }
        puts("PASS: all six sleep-screen preferences persist and use the correct sleep deadline");
        test_battery_led();
        test_battery_prompt();
        IMG_Init(IMG_INIT_PNG|IMG_INIT_JPG);test_surprise_art_odds();IMG_Quit();
        test_weather_refresh();
        widget_local_fetch(NULL);printf("Local city lookup: %s\n",widget_local_result[0]?"available":"offline fallback available");
        SDL_Quit();return 0;
    }
    if(argc==2&&!strcmp(argv[1],"--weather-live")){
        char temporary[]="/tmp/snapfe-weather-live-XXXXXX";assert(mkdtemp(temporary));assert(!setenv("SNAPFE_DATA_ROOT",temporary,1));
        char places[256];snprintf(places,sizeof places,"%s/widget-places.cfg",temporary);
        FILE *f=fopen(places,"w");assert(f);
        fputs("S\t0\t2\n0\tLocal\t\t\n1\tLocal\t\t\n1\tPyongyang\tAsia/Pyongyang\tPyongyang, Korea (North)\n1\tLas Vegas\tAmerica/Los_Angeles\tLas Vegas, Nevada, United State\n",f);fclose(f);
        assert(SDL_Init(SDL_INIT_TIMER)==0);widget_places_init();
        assert(widget_place_count[1]==3&&widget_place_sel[1]==2);
        assert(!strcmp(weather_loc,"Las Vegas, Nevada, United States"));
        weather_test_fetch_enabled=1;
        for(int index=2;index>=1;index--){
            widget_place_select(1,index);weather_kick(1);assert(weather_thread);
            Uint32 start=SDL_GetTicks();
            while(weather_thread&&SDL_GetTicks()-start<11000u){SDL_Delay(20);weather_read();}
            assert(!weather_thread&&!g_weather_failed&&g_weather_str[0]);
            printf("PASS: live %s weather from saved slot %d: %s\n",widget_place_name(1),index+1,g_weather_str);
        }
        widget_place_select(1,2);weather_read();assert(g_weather_str[0]&&!g_weather_failed);
        widget_places_loaded=0;widget_places_init();assert(!strcmp(widget_places[1][2].query,"Las Vegas, Nevada, United States"));
        puts("PASS: existing malformed third-slot query repaired and persisted; city-specific cache survives switching");
        weather_stop();SDL_Quit();return 0;
    }
    if(argc==2&&!strcmp(argv[1],"--asset-smoke")) {
        assert(SDL_Init(SDL_INIT_TIMER)==0);IMG_Init(IMG_INIT_PNG|IMG_INIT_JPG);
        for(int pack=0;pack<HOME_ICON_PACK_COUNT;pack++) {
            char path[700];snprintf(path,sizeof path,"%s/assets/icons/home/%s/game-default.svg",
                                    sn_data_root(),home_icon_pack_dirs[pack]);
            SDL_Surface *icon=IMG_Load(path);assert(icon&&icon->w==24&&icon->h==24);
            SDL_FreeSurface(icon);
        }
        puts("PASS: both installed icon packs decode their 24x24 SVG no-art fallback");
        char list_icon[800];snprintf(list_icon,sizeof list_icon,"%s/assets/icons/list/n64/n64 icon.png",sn_data_root());
        SDL_Surface *n64=IMG_Load(list_icon);assert(n64&&n64->w==40&&n64->h==40);SDL_FreeSurface(n64);
        puts("PASS: installed Nintendo 64 list icon decodes correctly");
        SDL_Quit();return 0;
    }
    if(argc==2&&!strcmp(argv[1],"--osd-smoke")) {
        assert(SDL_Init(SDL_INIT_TIMER)==0);
        for(int frame=0;frame<12;frame++) {
            int previous=osd_bar.front,slot=osd_layer_begin_draw(&osd_bar);
            if(slot<0){ingame_osd_close();return 2;}
            assert(slot!=previous);
            memset(osd_bar.px,0,osd_bar.map_len);
            osd_fill(&osd_bar,2,2,316,60,0xEE101018);
            osd_fill(&osd_bar,14,26,24+frame*23,12,0xFFAA80DD);
            osd_layer_end_draw(&osd_bar);
            osd_layer_commit(&osd_bar,slot,1,(osd_scr_w-320)/2,osd_scr_h-76,320,64);
            if(!osd_bar.up||osd_bar.front!=slot){ingame_osd_close();return 3;}
            int previous_battery=osd_battery.front,battery_slot=osd_layer_begin_draw(&osd_battery);
            if(battery_slot<0){ingame_osd_close();return 4;}
            assert(battery_slot!=previous_battery);
            memset(osd_battery.px,0,osd_battery.map_len);
            osd_fill(&osd_battery,2,2,BATTERY_GAME_NOTICE_W-4,BATTERY_GAME_NOTICE_H-4,0xEE101018);
            // Deliberate abstract test pattern, never a fake low-battery warning.
            osd_fill(&osd_battery,16,24,40+frame*28,24,0xFF65C7DC);
            osd_fill(&osd_battery,16,66,360-frame*20,16,0xFFAA80DD);
            osd_layer_end_draw(&osd_battery);
            osd_layer_commit(&osd_battery,battery_slot,1,(osd_scr_w-BATTERY_GAME_NOTICE_W)/2,
                20,BATTERY_GAME_NOTICE_W,BATTERY_GAME_NOTICE_H);
            if(!osd_battery.up||osd_battery.front!=battery_slot){ingame_osd_close();return 5;}
            SDL_Delay(100);
        }
        ingame_osd_close();assert(!osd_bar.up&&!osd_battery.up);
        puts("PASS: hardware volume/battery overlay layers allocate, alternate 12 frames, commit and hide; abstract test patterns, brightness/volume untouched");
        SDL_Quit();return 0;
    }
#endif
    if(argc==2&&!strcmp(argv[1],"--net-host"))return test_nearby(1);
    if(argc==2&&!strcmp(argv[1],"--net-join"))return test_nearby(2);
    char temp[]="/tmp/snapfe-129-tests-XXXXXX"; assert(mkdtemp(temp));
    assert(setenv("SNAPFE_DATA_ROOT",temp,1)==0);
    char config[640];snprintf(config,sizeof config,"%s/config",temp);assert(mkdir(config,0700)==0);
    assert(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER)==0); assert(TTF_Init()==0);
    IMG_Init(IMG_INIT_PNG|IMG_INIT_JPG);
    SDL_Window *w=SDL_CreateWindow("SNAP FE test",0,0,WIN_W,WIN_H,SDL_WINDOW_HIDDEN);assert(w);
    SDL_Renderer *ren=SDL_CreateRenderer(w,-1,SDL_RENDERER_SOFTWARE);assert(ren);
    SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
    font_big=TTF_OpenFont("assets/fonts/VT323-Regular.ttf",64);
    font_small=TTF_OpenFont("assets/fonts/VT323-Regular.ttf",30);
    font_label=TTF_OpenFont("assets/fonts/VT323-Regular.ttf",22);
    font_small_bold=TTF_OpenFont("assets/fonts/VT323-Regular.ttf",30);
    font_label_bold=TTF_OpenFont("assets/fonts/VT323-Regular.ttf",22);
    font_fixed=TTF_OpenFont("assets/fonts/VT323-Regular.ttf",22);
    assert(font_big&&font_small&&font_label&&font_small_bold&&font_label_bold&&font_fixed);
    mkdir("tests/output",0700);
    theme_idx=0; g_ui_text=themes[0].text;g_ui_dim=themes[0].dim;
    if(argc==2&&!strcmp(argv[1],"--sleep-screens")) {
        test_sleep_screens(ren);text_cache_clear();
        SDL_DestroyRenderer(ren);SDL_DestroyWindow(w);
        puts("PASS: every fixed sleep screen renders; Random resolves only to all four live modes; wake state resets");
        return 0;
    }
    Theme originals[THEME_COUNT];memcpy(originals,themes,sizeof originals);
    theme_count=THEME_COUNT;
    theme_editor_init_with_capacity(config,themes,THEME_COUNT,THEME_CAPACITY,&theme_count);
    assert(theme_count==THEME_COUNT);
    theme_editor_open(0);theme_editor_handle_key(SDLK_RIGHT);assert(g_theme_editor.dirty);
    theme_editor_handle_key(SDLK_ESCAPE);assert(g_theme_editor.close_prompt);
    theme_editor_handle_key(SDLK_ESCAPE);assert(theme_editor_active());
    theme_editor_handle_key(SDLK_RETURN);theme_editor_handle_key(SDLK_f);
    assert(!theme_editor_active()&&theme_editor_color_equal(originals[0].bg,themes[0].bg));
    theme_editor_open(0);theme_editor_handle_key(SDLK_RIGHT);
    SDL_Color saved=themes[0].bg;
    theme_editor_handle_key(SDLK_RETURN);
    assert(theme_editor_handle_key(SDLK_RETURN)&THEME_EDITOR_RESULT_NAME);
    int custom=theme_editor_commit_name("My Night Theme");
    assert(custom==THEME_COUNT&&theme_count==THEME_COUNT+1&&!theme_editor_active());
    assert(theme_editor_color_equal(originals[0].bg,themes[0].bg));
    assert(theme_editor_color_equal(saved,themes[custom].bg));
    assert(!strcmp(themes[custom].name,"My Night Theme"));

    memset(themes+THEME_COUNT,0,sizeof(Theme)*THEME_CUSTOM_CAPACITY);
    memcpy(themes,originals,sizeof originals);theme_count=THEME_COUNT;
    theme_editor_init_with_capacity(config,themes,THEME_COUNT,THEME_CAPACITY,&theme_count);
    assert(theme_count==THEME_COUNT+1);
    custom=THEME_COUNT;
    assert(theme_editor_color_equal(saved,themes[custom].bg));
    theme_editor_open(custom);
    assert(theme_editor_handle_key(SDLK_f)&THEME_EDITOR_RESULT_NAME);
    assert(theme_editor_commit_name("My Renamed Theme")==custom);
    assert(!strcmp(themes[custom].name,"My Renamed Theme"));
    theme_editor_open(custom);theme_editor_handle_key(SDLK_RIGHT);
    SDL_Color updated=themes[custom].bg;
    theme_editor_handle_key(SDLK_RETURN);theme_editor_handle_key(SDLK_RETURN);
    assert(!theme_editor_active());
    memcpy(themes,originals,sizeof originals);theme_count=THEME_COUNT;
    theme_editor_init_with_capacity(config,themes,THEME_COUNT,THEME_CAPACITY,&theme_count);
    assert(theme_count==THEME_COUNT+1&&!strcmp(themes[custom].name,"My Renamed Theme"));
    assert(theme_editor_color_equal(updated,themes[custom].bg));
    theme_editor_open(custom);theme_editor_handle_key(SDLK_SLASH);
    assert(theme_editor_color_equal(originals[0].bg,themes[custom].bg));
    theme_editor_handle_key(SDLK_ESCAPE);theme_editor_handle_key(SDLK_f);
    assert(theme_editor_color_equal(updated,themes[custom].bg));

    /* Built-ins never expose deletion. A custom deletion is confirmed, can
       be cancelled safely, falls back before deleting the active theme, and
       compacts later custom entries without leaving stale name pointers. */
    assert(!theme_editor_request_delete(0));
    assert(theme_count==THEME_COUNT+1&&!theme_editor_active());
    theme_editor_open(1);theme_editor_handle_key(SDLK_RIGHT);
    SDL_Color survivor_color=themes[1].bg;
    theme_editor_handle_key(SDLK_RETURN);
    assert(theme_editor_handle_key(SDLK_RETURN)&THEME_EDITOR_RESULT_NAME);
    int survivor=theme_editor_commit_name("Compaction Survivor");
    assert(survivor==THEME_COUNT+1&&theme_count==THEME_COUNT+2);
    assert(theme_editor_color_equal(originals[1].bg,themes[1].bg));

    int active_custom=custom;
    assert(theme_editor_request_delete(custom)&&g_theme_editor.delete_prompt);
    int delete_result=theme_editor_handle_key_for_active(SDLK_ESCAPE,&active_custom);
    assert((delete_result&THEME_EDITOR_RESULT_CLOSED)&&
           !(delete_result&THEME_EDITOR_RESULT_DELETED));
    assert(active_custom==custom&&theme_count==THEME_COUNT+2);
    assert(!strcmp(themes[custom].name,"My Renamed Theme"));

    assert(theme_editor_request_delete(custom)&&g_theme_editor.delete_prompt);
    delete_result=theme_editor_handle_key_for_active(SDLK_RETURN,&active_custom);
    assert((delete_result&THEME_EDITOR_RESULT_DELETED)&&
           (delete_result&THEME_EDITOR_RESULT_CLOSED));
    assert(active_custom==0&&theme_editor_last_deleted_index()==custom);
    assert(theme_count==THEME_COUNT+1);
    assert(!strcmp(themes[custom].name,"Compaction Survivor"));
    assert(themes[custom].name==g_theme_editor.names[custom]);
    assert(g_theme_editor.defaults[custom].name==g_theme_editor.names[custom]);
    assert(theme_editor_color_equal(survivor_color,themes[custom].bg));
    assert(themes[THEME_COUNT+1].name==NULL&&
           !g_theme_editor.names[THEME_COUNT+1][0]);

    memset(themes+THEME_COUNT,0,sizeof(Theme)*THEME_CUSTOM_CAPACITY);
    memcpy(themes,originals,sizeof originals);theme_count=THEME_COUNT;
    theme_editor_init_with_capacity(config,themes,THEME_COUNT,THEME_CAPACITY,&theme_count);
    custom=THEME_COUNT;
    assert(theme_count==THEME_COUNT+1);
    assert(!strcmp(themes[custom].name,"Compaction Survivor"));
    assert(themes[custom].name==g_theme_editor.names[custom]);
    assert(theme_editor_color_equal(survivor_color,themes[custom].bg));
    assert(!strcmp(greeting_period(390,360,1100),"Good morning"));
    assert(!strcmp(greeting_period(800,360,1100),"Good afternoon"));
    assert(!strcmp(greeting_period(1120,360,1100),"Good evening"));
    assert(greeting_new_boot());greeting_mark_shown();greeting_boot_pending=-1;assert(!greeting_new_boot());
    int quote=boot_quote_choose();assert(quote>=0&&quote<BOOT_QUOTE_COUNT&&quote==boot_quote_choose());
    unsigned quote_seen=1u<<quote;
    for(int i=1;i<BOOT_QUOTE_COUNT;i++) {
        char path[700];snprintf(path,sizeof path,"%s/boot-quotes.dat",config);
        FILE *f=fopen(path,"r");assert(f);int count,cursor,current,deck[BOOT_QUOTE_COUNT];char boot[64];
        assert(fscanf(f,"%d %d %d %63s",&count,&cursor,&current,boot)==4);
        for(int j=0;j<BOOT_QUOTE_COUNT;j++)assert(fscanf(f,"%d",&deck[j])==1);
        fclose(f);f=fopen(path,"w");assert(f);
        fprintf(f,"%d %d %d simulated-previous-boot\n",count,cursor,current);
        for(int j=0;j<BOOT_QUOTE_COUNT;j++)fprintf(f,"%d ",deck[j]);fclose(f);
        int next=boot_quote_choose();assert(!(quote_seen&(1u<<next)));quote_seen|=1u<<next;
    }
    assert(quote_seen==((1u<<BOOT_QUOTE_COUNT)-1));
    mgx_swap_pick(ren);assert(mgx_swap.choosing&&!mgx_swap.art&&!mgx_swap.loading);
    mgx_swap_key(ren,SDLK_RIGHT);assert(mgx_swap.difficulty==1&&!mgx_swap.art);
    mgx_swap_key(ren,SDLK_RETURN);assert(!mgx_swap.choosing&&mgx_swap.loading&&!mgx_swap.art);
    mgx_swap.loading=0;mgx_swap.load_failed=0;
    mgx_swap_key(ren,SDLK_f);
    assert(mgx_swap.difficulty==1&&!mgx_swap.choosing&&mgx_swap.loading);
    mgx_swap.loading=0;mgx_swap.load_failed=0;
    test_puzzle_art_filter();
    for(int d=0;d<3;d++)for(int iter=0;iter<100;iter++) {
        mgx_swap.difficulty=d;mgx_swap_shuffle();assert(mgx_swap.n==(d==0?4:d==1?9:16));
        assert(mgx_swap.cols==mgx_swap.rows&&mgx_swap.n==mgx_swap.cols*mgx_swap.rows);
        int seen=0,mismatch=0;
        for(int i=0;i<mgx_swap.n;i++){assert(mgx_swap.tile[i]>=0&&mgx_swap.tile[i]<mgx_swap.n);seen|=1<<mgx_swap.tile[i];mismatch|=mgx_swap.tile[i]!=i;}
        assert(seen==(1<<mgx_swap.n)-1&&mismatch);
    }
    for(int pack=0;pack<HOME_ICON_PACK_COUNT;pack++) {
        char icon_path[256];snprintf(icon_path,sizeof icon_path,
            "assets/icons/home/%s/game-default.svg",home_icon_pack_dirs[pack]);
        FILE *icon_file=fopen(icon_path,"rb");assert(icon_file);fclose(icon_file);
        SDL_Surface *icon=IMG_Load(icon_path);assert(icon&&icon->w==24&&icon->h==24);
        SDL_FreeSurface(icon);
    }
    test_fish_campaign();
    mgx_react_reset();mgx_react.phase=2;mgx_react.target=2;mgx_react.shown=SDL_GetTicks();
    mgx_react_key(SDLK_ESCAPE);assert(mgx_react.lives==3&&mgx_react.phase==3);
    for(int level=0;level<BLK_LEVEL_COUNT;level++){blk_level=level;block_retry();assert(block_pose_ok(br.x,br.y,0));}
    for(int level=1;level<=BRK_LEVELS;level++){bo.level=level;brk_fill_level();int bricks=0;for(int y=0;y<BRK_ROWS;y++)for(int x=0;x<BRK_COLS;x++)bricks+=bo.brick[y][x]!=0;assert(bricks>0);}
    draw_theme_background(ren,&themes[theme_idx],theme_idx);
    mgx_menu_enter();mgx_render_menu(ren);capture(ren,"minigames");
    mgx_runner_reset();mgx_runner_render(ren);capture(ren,"runner");
    mgx_road_reset();mgx_road_render(ren);capture(ren,"crossing");
    test_crossing_world(ren);
    mgx_swap_pick(ren);mgx_swap_render(ren);capture(ren,"puzzle-difficulty");
    { SDL_Surface *art=SDL_CreateRGBSurfaceWithFormat(0,360,240,32,SDL_PIXELFORMAT_ARGB8888);assert(art);
      SDL_FillRect(art,NULL,SDL_MapRGB(art->format,34,102,154));
      SDL_FillRect(art,&(SDL_Rect){35,30,290,180},SDL_MapRGB(art->format,238,178,64));
      mgx_swap.art=SDL_CreateTextureFromSurface(ren,art);SDL_FreeSurface(art);assert(mgx_swap.art);
      mgx_swap.aw=360;mgx_swap.ah=240;mgx_swap.choosing=mgx_swap.loading=mgx_swap.load_failed=0;
      mgx_swap.difficulty=2;mgx_swap_grid();mgx_swap.moves=17;mgx_swap.won=1;
      mgx_swap_render(ren);capture(ren,"puzzle-solved");mgx_swap_close(); }
    mgx_react_reset();mgx_react_render(ren);capture(ren,"reaction");
    mgx_tank_reset();mgx_tank_key(SDLK_RETURN);mgx_tank.map_choice=3;mgx_tank_render(ren);capture(ren,"tank-map-select");
    for(int map=0;map<MGX_TANK_MAP_COUNT;map++){mgx_tank.map=map;mgx_tank_round();assert(mgx_tank_clear(mgx_tank.x[0],mgx_tank.y[0]));assert(mgx_tank_clear(mgx_tank.x[1],mgx_tank.y[1]));}
    mgx_fish_reset();mgx_fish_key(SDLK_RETURN);mgx_fish.intro=0;mgx_fish_spawn_monster();mgx_fish.x=mgx_fish.monster[0].x;mgx_fish.y=mgx_fish.monster[0].y;mgx_fish_key(SDLK_RETURN);mgx_fish.intro=0;mgx_fish_render(ren);capture(ren,"aquarium-combat");
    theme_editor_open(custom);theme_editor_draw(ren,font_small,font_label,WIN_W,WIN_H);capture(ren,"theme-editor-custom");
    test_sleep_screens(ren);
    draw_theme_background(ren,&themes[theme_idx],theme_idx);draw_screen_power_page(ren,&themes[theme_idx]);capture(ren,"screen-power");
    draw_boot_sequence(ren,7000,"Preparing artwork",.75f);capture(ren,"boot");
    g_boot_quote_seed=12;draw_boot_sequence(ren,8500,"Ready",1);capture(ren,"boot-long-quote");
    syshelp_open(0);syshelp_draw(ren,&themes[theme_idx],0);capture(ren,"computer-controls");
    test_continued_129(ren);
    test_friends_129(ren);
    test_assets_129();
    test_fish_feedback(ren);
    test_pong_latency();
    test_link_navigation(ren);
    test_widget_navigation(ren);
    test_sleep_duration(ren);
    test_battery_led();
    test_battery_game_notice(ren);
    test_battery_prompt();
    test_battery_prompt_render(ren);
    test_location_catalog();
    test_widget_places(ren);
    test_weather_refresh();
    test_systems_dropdown(ren);
    test_sound_options(ren);
    test_surprise_art_odds();
    mgx_swap_close();mgx_net_close();text_cache_clear();
    SDL_DestroyRenderer(ren);SDL_DestroyWindow(w);
    puts("PASS: custom theme delete/compaction; boot greeting/quote; blank-safe square puzzles + retained difficulty; fair 12-zone endless crossing; pack-aware SVG no-art; aquarium resource/feeding/health/targeting/knockback/shark; B-button reaction; five tank maps; all sleep screens; UI render smoke tests");
    return 0;
}
