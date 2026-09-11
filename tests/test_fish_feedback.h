static void fish_feedback_fresh(void){mgx_fish_reset();mgx_fish.helpers=0;mgx_fish.stage=1;mgx_fish.started=1;mgx_fish.intro=0;mgx_fish.picking=0;mgx_fish.shop_open=0;}
/* Buying moved behind one shop menu on R1, so the test buys the way a player
   does: open it, walk to the row, press A, close it. */
static void fish_shop_buy(int row){
    mgx_fish_key(SDLK_e);assert(mgx_fish.shop_open);
    while(mgx_fish.shop_row!=row)mgx_fish_key(SDLK_DOWN);
    mgx_fish_key(SDLK_RETURN);
    if(mgx_fish.shop_open)mgx_fish_key(SDLK_ESCAPE);
}
static void test_fish_feedback(SDL_Renderer *ren){
    fish_feedback_fresh();mgx_fish.coins=100;mgx_fish.fish[1].active=0;MgxFishPet *pet=&mgx_fish.fish[0];pet->x=300;pet->y=240;
    int pellet=mgx_fish_food_cost();
    mgx_fish.x=pet->x;mgx_fish.y=pet->y;mgx_fish_key(SDLK_RETURN);assert(mgx_fish.coins==100-pellet);
    /* A fish that has just eaten skips a second pellet for a moment, and no longer. */
    mgx_fish_pet_step(0,.01f,0);assert(pet->eat_cooldown>0&&pet->eat_cooldown<=1.2f&&pet->growth==1);
    mgx_fish.x=pet->x;mgx_fish.y=pet->y;mgx_fish_key(SDLK_RETURN);assert(mgx_fish.coins==100-pellet*2);
    int food=fish_find_drop(MGX_RES_FOOD);assert(food>=0);mgx_fish_pet_step(0,.02f,0);assert(pet->growth==1&&mgx_fish.drops[food].active);
    pet->eat_cooldown=.01f;mgx_fish.drops[food].x=pet->x;mgx_fish.drops[food].y=pet->y;mgx_fish_pet_step(0,.02f,0);assert(pet->growth==2&&!mgx_fish.drops[food].active);
    mgx_fish.coins=pellet-1;mgx_fish_key(SDLK_RETURN);assert(mgx_fish.coins==pellet-1&&strstr(mgx_fish.notice,"costs"));

    /* Better food costs more per pellet and holds a fish longer. */
    fish_feedback_fresh();
    assert(mgx_fish_food_cost()==5);
    mgx_fish.coins=mgx_food_upgrade_cost[1];fish_shop_buy(MGX_SHOP_FOOD);
    assert(mgx_fish.food_tier==1&&mgx_fish_food_cost()==8&&mgx_fish.coins==0);
    mgx_fish.coins=mgx_food_upgrade_cost[2];fish_shop_buy(MGX_SHOP_FOOD);
    assert(mgx_fish.food_tier==2&&mgx_fish_food_cost()==12);
    fish_shop_buy(MGX_SHOP_FOOD);assert(strstr(mgx_fish.notice,"best"));
    {   /* Same fish, better food -> a longer window before it starves. */
        MgxFishPet a={0},b={0};a.type=b.type=MGX_PET_GUPPY;
        a.hunger_span=mgx_food_hold[0];b.hunger_span=mgx_food_hold[2];
        assert(mgx_fish_hunger_span(&b)>mgx_fish_hunger_span(&a));
    }

    /* An affordable purchase through the shop succeeds immediately. */
    fish_feedback_fresh();mgx_fish.coins=mgx_fish_egg_cost();fish_shop_buy(MGX_SHOP_EGG);
    assert(mgx_fish.egg_pieces==1&&mgx_fish.coins==0);
    fish_shop_buy(MGX_SHOP_EGG);assert(mgx_fish.egg_pieces==1&&strstr(mgx_fish.notice,"need"));
    assert(!mgx_fish_unlocked(MGX_PET_SHARK));for(int i=0;i<20;i++){mgx_fish_select_pet(1);assert(mgx_fish.shop_type==MGX_PET_GUPPY);}
    mgx_fish_key(SDLK_f);assert(mgx_fish.collection);mgx_fish_render(ren);capture(ren,"fish-collection-starter");mgx_fish_key(SDLK_ESCAPE);assert(!mgx_fish.collection);
    mgx_fish.coins=10000;fish_shop_buy(MGX_SHOP_EGG);fish_shop_buy(MGX_SHOP_EGG);assert(mgx_fish.hatch_pending&&!mgx_fish_unlocked(MGX_PET_TETRA));
    int cash=mgx_fish.coins;fish_shop_buy(MGX_SHOP_EGG);assert(mgx_fish.coins==cash);mgx_fish.hatch_time=2.4f;mgx_fish_render(ren);capture(ren,"fish-egg-cracking");
    mgx_fish.hatch_time=.01f;mgx_fish.last=SDL_GetTicks()-20;mgx_fish_step(0,0);assert(mgx_fish.helpers==1&&mgx_fish_unlocked(MGX_PET_TETRA)&&mgx_fish.between==2);
    mgx_fish_render(ren);capture(ren,"fish-unlocked");mgx_fish_key(SDLK_RETURN);
    /* One helper discovered is not a choice, so the picker is skipped and the
       level starts straight away. */
    assert(!mgx_fish.between&&!mgx_fish.picking&&mgx_fish.intro==5);
    mgx_fish_reset();assert(mgx_fish.helpers==1&&mgx_fish_unlocked(MGX_PET_TETRA));mgx_fish.started=1;mgx_fish.intro=0;mgx_fish.helpers=6;
    mgx_fish.shop_type=MGX_PET_SHARK;mgx_fish.coins=mgx_fish_cost[MGX_PET_SHARK];int count=mgx_fish_alive();mgx_fish_key(SDLK_s);assert(mgx_fish_alive()==count+1&&mgx_fish.coins==0);
    mgx_fish_key(SDLK_s);assert(strstr(mgx_fish.notice,"need"));
    mgx_fish.collection=1;mgx_fish.shells=30;mgx_fish.rares=(1<<MGX_RARE_KOI)|(1<<MGX_RARE_GHOST);
    mgx_fish_render(ren);capture(ren,"fish-collection-complete");mgx_fish.collection=0;mgx_fish.rares=0;mgx_fish.shells=0;
    mgx_fish.coins=110;mgx_fish.gun_level=1;fish_shop_buy(MGX_SHOP_RAY);assert(mgx_fish.gun_level==2&&mgx_fish.coins==0);
    fish_shop_buy(MGX_SHOP_RAY);assert(strstr(mgx_fish.notice,"$155"));
    mgx_fish_spawn_monster();int hp=mgx_fish.monster[0].hp;mgx_fish.x=30;mgx_fish.y=40;mgx_fish.last_shot=0;mgx_fish.coins=100;mgx_fish_key(SDLK_RETURN);
    assert(mgx_fish.monster[0].hp==hp-2&&mgx_fish.coins==100&&fish_find_drop(MGX_RES_FOOD)<0);
    mgx_fish_render(ren);draw_fish_status(ren,60);capture(ren,"fish-compact-play");
    TTF_Font *saved=font_fixed;font_fixed=TTF_OpenFont("assets/fonts/DejaVuSans.ttf",14);assert(font_fixed);text_cache_clear();
    int old_fps=show_fps,old_wifi=g_wifi_bars;show_fps=1;g_wifi_bars=4;
    mgx_fish_render(ren);draw_fish_status(ren,60);capture(ren,"fish-compact-sans-status");
    show_fps=old_fps;g_wifi_bars=old_wifi;text_cache_clear();TTF_CloseFont(font_fixed);font_fixed=saved;
    /* The first invasion still lands ninety active seconds in; menus pause the
       simulation. */
    fish_feedback_fresh();mgx_fish.clock=89;mgx_fish.last=SDL_GetTicks()-20;mgx_fish_step(0,0);assert(!mgx_fish_monsters_alive());
    mgx_fish.clock=90;mgx_fish.last=SDL_GetTicks()-20;mgx_fish_step(0,0);assert(mgx_fish_monsters_alive()==1);
    /* After that the gap closes as the campaign goes on, instead of every
       chapter being as quiet as the first. Chapter one is the longest wait. */
    float gap_ch1=mgx_fish.next_spawn-90.0f;
    assert(gap_ch1>45.0f&&gap_ch1<=100.0f);
    memset(mgx_fish.monster,0,sizeof mgx_fish.monster);
    mgx_fish.clock=mgx_fish.next_spawn-1;mgx_fish.last=SDL_GetTicks()-20;mgx_fish_step(0,0);
    assert(!mgx_fish_monsters_alive());
    mgx_fish.clock=mgx_fish.next_spawn+.1f;mgx_fish.last=SDL_GetTicks()-20;mgx_fish_step(0,0);
    assert(mgx_fish_monsters_alive()==1);
    {   /* A late chapter waits noticeably less, and never drops below the floor. */
        fish_feedback_fresh();mgx_fish.stage=9;
        mgx_fish.clock=90;mgx_fish.last=SDL_GetTicks()-20;mgx_fish_step(0,0);
        float gap_ch9=mgx_fish.next_spawn-90.0f;
        assert(gap_ch9<gap_ch1&&gap_ch9>=45.0f);
        fish_feedback_fresh();mgx_fish.stage=40;   /* far past the table */
        mgx_fish.clock=90;mgx_fish.last=SDL_GetTicks()-20;mgx_fish_step(0,0);
        assert(mgx_fish.next_spawn-90.0f>=45.0f);
    }
    fish_feedback_fresh();mgx_fish.clock=200;
    float clock_before=mgx_fish.clock;mgx_fish.collection=1;mgx_fish.last=SDL_GetTicks()-20;mgx_fish_step(0,0);assert(mgx_fish.clock==clock_before);
    mgx_fish.collection=0;
    puts("PASS: staged hunger and food tiers, one R1 shop, hidden locked animals, hatch reveal/persistence, shark purchase, attack-only A and invasions that close in with the chapter");
}
