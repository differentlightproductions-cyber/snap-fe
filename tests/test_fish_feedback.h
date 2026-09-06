static void fish_feedback_fresh(void){mgx_fish_reset();mgx_fish.helpers=0;mgx_fish.chapter=1;mgx_fish.started=1;mgx_fish.intro=0;}
static void test_fish_feedback(SDL_Renderer *ren){
    fish_feedback_fresh();mgx_fish.coins=100;mgx_fish.fish[1].active=0;MgxFishPet *pet=&mgx_fish.fish[0];pet->x=300;pet->y=240;
    mgx_fish.x=pet->x;mgx_fish.y=pet->y;mgx_fish_key(SDLK_RETURN);assert(mgx_fish.coins==90);
    mgx_fish_pet_step(0,.01f,0);assert(pet->eat_cooldown==10&&pet->growth==1);
    mgx_fish.x=pet->x;mgx_fish.y=pet->y;mgx_fish_key(SDLK_RETURN);assert(mgx_fish.coins==80);
    int food=fish_find_drop(MGX_RES_FOOD);assert(food>=0);mgx_fish_pet_step(0,.02f,0);assert(pet->growth==1&&mgx_fish.drops[food].active);
    pet->eat_cooldown=.01f;mgx_fish.drops[food].x=pet->x;mgx_fish.drops[food].y=pet->y;mgx_fish_pet_step(0,.02f,0);assert(pet->growth==2&&!mgx_fish.drops[food].active);
    mgx_fish.coins=9;mgx_fish_key(SDLK_RETURN);assert(mgx_fish.coins==9&&strstr(mgx_fish.notice,"$10"));
    /* No time gate: an affordable R1 purchase succeeds immediately. */
    fish_feedback_fresh();mgx_fish.coins=mgx_fish_egg_cost();mgx_fish_key(SDLK_e);assert(mgx_fish.egg_pieces==1&&mgx_fish.coins==0);
    mgx_fish_key(SDLK_e);assert(mgx_fish.egg_pieces==1&&strstr(mgx_fish.notice,"need"));
    assert(!mgx_fish_unlocked(MGX_PET_SHARK));for(int i=0;i<20;i++){mgx_fish_select_pet(1);assert(mgx_fish.shop_type==MGX_PET_GUPPY);}
    mgx_fish_key(SDLK_f);assert(mgx_fish.collection);mgx_fish_render(ren);capture(ren,"fish-collection-starter");mgx_fish_key(SDLK_ESCAPE);assert(!mgx_fish.collection);
    mgx_fish.coins=10000;mgx_fish_key(SDLK_e);mgx_fish_key(SDLK_e);assert(mgx_fish.hatch_pending&&!mgx_fish_unlocked(MGX_PET_TETRA));
    int cash=mgx_fish.coins;mgx_fish_key(SDLK_e);assert(mgx_fish.coins==cash);mgx_fish.hatch_time=2.4f;mgx_fish_render(ren);capture(ren,"fish-egg-cracking");
    mgx_fish.hatch_time=.01f;mgx_fish.last=SDL_GetTicks()-20;mgx_fish_step(0,0);assert(mgx_fish.helpers==1&&mgx_fish_unlocked(MGX_PET_TETRA)&&mgx_fish.between==2);
    mgx_fish_render(ren);capture(ren,"fish-unlocked");mgx_fish_key(SDLK_RETURN);assert(!mgx_fish.between&&mgx_fish.intro==5);
    mgx_fish_reset();assert(mgx_fish.helpers==1&&mgx_fish_unlocked(MGX_PET_TETRA));mgx_fish.started=1;mgx_fish.intro=0;mgx_fish.helpers=6;
    mgx_fish.shop_type=MGX_PET_SHARK;mgx_fish.coins=mgx_fish_cost[MGX_PET_SHARK];int count=mgx_fish_alive();mgx_fish_key(SDLK_s);assert(mgx_fish_alive()==count+1&&mgx_fish.coins==0);
    mgx_fish_key(SDLK_s);assert(strstr(mgx_fish.notice,"need"));
    mgx_fish.collection=1;mgx_fish_render(ren);capture(ren,"fish-collection-complete");mgx_fish.collection=0;
    mgx_fish.coins=110;mgx_fish.gun_level=1;mgx_fish_key(SDLK_q);assert(mgx_fish.gun_level==2&&mgx_fish.coins==0);
    mgx_fish_key(SDLK_q);assert(strstr(mgx_fish.notice,"$155"));
    mgx_fish_spawn_monster();int hp=mgx_fish.monster[0].hp;mgx_fish.x=30;mgx_fish.y=40;mgx_fish.last_shot=0;mgx_fish.coins=100;mgx_fish_key(SDLK_RETURN);
    assert(mgx_fish.monster[0].hp==hp-2&&mgx_fish.coins==100&&fish_find_drop(MGX_RES_FOOD)<0);
    mgx_fish_render(ren);draw_fish_status(ren,60);capture(ren,"fish-compact-play");
    TTF_Font *saved=font_fixed;font_fixed=TTF_OpenFont("assets/fonts/DejaVuSans.ttf",14);assert(font_fixed);text_cache_clear();
    int old_fps=show_fps,old_wifi=g_wifi_bars;show_fps=1;g_wifi_bars=4;
    mgx_fish_render(ren);draw_fish_status(ren,60);capture(ren,"fish-compact-sans-status");
    show_fps=old_fps;g_wifi_bars=old_wifi;text_cache_clear();TTF_CloseFont(font_fixed);font_fixed=saved;
    /* Ninety active seconds between invasions; menus pause the simulation. */
    fish_feedback_fresh();mgx_fish.clock=89;mgx_fish.last=SDL_GetTicks()-20;mgx_fish_step(0,0);assert(!mgx_fish_monsters_alive());
    mgx_fish.clock=90;mgx_fish.last=SDL_GetTicks()-20;mgx_fish_step(0,0);assert(mgx_fish_monsters_alive()==1&&mgx_fish.next_spawn>=180);
    memset(mgx_fish.monster,0,sizeof mgx_fish.monster);mgx_fish.clock=179;mgx_fish.last=SDL_GetTicks()-20;mgx_fish_step(0,0);assert(!mgx_fish_monsters_alive());
    mgx_fish.clock=180.1f;mgx_fish.last=SDL_GetTicks()-20;mgx_fish_step(0,0);assert(mgx_fish_monsters_alive()==1);
    float clock_before=mgx_fish.clock;mgx_fish.collection=1;mgx_fish.last=SDL_GetTicks()-20;mgx_fish_step(0,0);assert(mgx_fish.clock==clock_before);
    puts("PASS: $10 food, ten-second appetite, R1 purchases, hidden locked animals, hatch reveal/persistence, shark purchase, L1 upgrade, attack-only A and ninety-second invasions");
}
