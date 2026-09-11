static void test_continued_129(SDL_Renderer *ren){
    /* Repeated objective checks cannot advance; all three paid fragments can. */
    mgx_fish_reset();mgx_fish.started=1;mgx_fish.intro=0;mgx_fish.level_time=100;mgx_fish.coins=10000;
    mgx_fish.level_resources=999;mgx_fish.stage_kills=999;
    for(int i=0;i<100;i++)mgx_fish_check_goal(100,100);assert(mgx_fish.stage==1);
    mgx_fish_buy_egg();mgx_fish_buy_egg();assert(mgx_fish.stage==1&&mgx_fish.egg_pieces==2);
    mgx_fish_buy_egg();assert(mgx_fish.stage==1&&mgx_fish.helpers==0&&mgx_fish.hatch_pending);
    mgx_fish.hatch_time=.01f;mgx_fish.last=SDL_GetTicks()-20;mgx_fish_step(0,0);
    assert(mgx_fish.stage==2&&mgx_fish.helpers==1&&mgx_fish.between==2);
    int cash=mgx_fish.coins;mgx_fish_key(SDLK_e);assert(mgx_fish.coins==cash);
    mgx_fish_reset();assert(mgx_fish.helpers==1&&mgx_fish.stage==2);
    mgx_fish.started=1;mgx_fish.intro=0;mgx_fish.x=120;mgx_fish.y=80;mgx_fish_key(SDLK_RETURN);
    int food=fish_find_drop(MGX_RES_FOOD);assert(food>=0&&mgx_fish.drops[food].x==120&&mgx_fish.drops[food].y==80);
    mgx_fish_render(ren);capture(ren,"continued-fish");
    for(int i=0;i<10;i++){mgx_run.level=i;mgx_runner_retry();assert(mgx_run_length()==mgx_run_levels[i].length*3);
        mgx_run.started=1;mgx_run.scroll=mgx_run_length()-121;assert(mgx_run_progress()==99);
        mgx_run.last=SDL_GetTicks()-16;mgx_runner_step();assert(mgx_run.won&&mgx_run_progress()==100);}
    mgx_run.level=5;mgx_runner_retry();mgx_run.started=1;float physical=mgx_run.y;
    mgx_runner_key(SDLK_s);assert(mgx_run.inverted&&mgx_run_screen_y((int)mgx_run.y,24)==(int)physical);
    mgx_runner_render(ren);capture(ren,"continued-runner-gravity");
    mgx_pong_reset();mgx_pong_render(ren);capture(ren,"continued-pong-lobby");
    mgx_pong_lobby=0;mgx_pong_mode=1;mgx_net.connected=1;mgx_net.fd=-1;mgx_pong_ready=1;mgx_pong_remote_ready=0;
    mgx_pong_step(0);assert(!pg.started&&!mgx_pong_launch);
    mgx_pong_remote_ready=1;mgx_pong_step(0);assert(!pg.started&&mgx_pong_launch);
    mgx_pong_render(ren);capture(ren,"continued-pong-countdown");
    mgx_pong_launch=SDL_GetTicks()-1;mgx_pong_step(0);assert(pg.started);
    mgx_net_close();mgx_c4_reset();mgx_c4_lobby=0;mgx_c4_mode=1;
    for(int col=0;col<3;col++){mgx_c4_drop(col,1);mgx_c4_drop(col,2);}mgx_c4_drop(3,1);assert(tt.over&&tt.winner==1);
    int board[42];memcpy(board,tt.cell,sizeof board);mgx_c4_drop(5,2);assert(!memcmp(board,tt.cell,sizeof board));
    mgx_c4_render(ren);capture(ren,"continued-connect4");
    mgx_tank_reset();assert(mgx_tank.series_target==2);mgx_tank.lobby=0;mgx_tank.over=1;mgx_tank.hp[1]=0;
    mgx_tank_step(0,0);assert(mgx_tank.wins[0]==1&&mgx_tank.next_round);mgx_tank_key(SDLK_RETURN);assert(mgx_tank.over);
    mgx_tank.next_round=SDL_GetTicks()-1;mgx_tank_step(0,0);assert(!mgx_tank.over&&mgx_tank.wins[0]==1);
    mgx_tank.over=1;mgx_tank.hp[1]=0;mgx_tank_step(0,0);assert(mgx_tank.wins[0]==2&&!mgx_tank.next_round);
    int rows[MAX_DISPLAY_ROWS],extra[MAX_DISPLAY_ROWS];home_view_idx=HOME_VIEW_APPS;disp_grp_home_open=disp_grp_widgets_open=disp_grp_view_open=1;
    int count=build_display_rows(rows,extra),systems=0;for(int i=0;i<count;i++){systems+=rows[i]==ROW_DISP_GRP_VIEW;assert(rows[i]!=ROW_DISP_APP_WIDGET&&rows[i]!=ROW_DISP_GRP_WIDGETS);}assert(systems==1);
    int acct[MAX_ACCOUNT_ROWS],ax[MAX_ACCOUNT_ROWS];scrape_systems_open=1;for(int i=0;i<7;i++)scrape_maker_open[i]=1;
    count=build_account_rows(acct,ax);assert(count<=MAX_ACCOUNT_ROWS);
    /* Full-size lists retain unique textures and matching cache ownership. */
    free_games(ren);game_count=MAX_GAMES;for(int i=0;i<game_count;i++){memset(&games[i],0,sizeof games[i]);snprintf(games[i].title,128,"Game %04d",game_count-i);snprintf(games[i].path,768,"/test/%d",i);strcpy(games[i].platform_dir,"gb");}
    Uint32 began=SDL_GetTicks();sort_games(0);assert(!strcmp(games[0].title,"Game 0001"));assert(SDL_GetTicks()-began<3000);
    for(int i=1;i<game_count;i++)assert(strcmp(games[i-1].title,games[i].title)<0);free_games(ren);
    /* Saved places: the list fills to WIDGET_PLACE_MAX (Local occupies the
       first slot) and then refuses further additions rather than overrunning. */
    widget_places_loaded=0;widget_places_init();widget_place_group=0;
    assert(widget_place_add("Miami"));assert(widget_place_add("Tokyo"));
    const char *fillers[]={"London","Paris","Berlin","Madrid"};
    for(unsigned f=0;f<sizeof fillers/sizeof *fillers;f++){
        int room=widget_place_count[0]<WIDGET_PLACE_MAX;
        assert(widget_place_add(fillers[f])==room);
    }
    assert(widget_place_count[0]==WIDGET_PLACE_MAX);
    /* A brand-new city is refused once the list is full. (Re-adding one that is
       already saved still succeeds -- it just selects it.) */
    assert(!widget_place_add("Lisbon"));
    assert(widget_place_add("Tokyo") && widget_place_count[0]==WIDGET_PLACE_MAX);
    /* Cycling walks every saved place and wraps, whatever the limit is. */
    widget_place_select(0,0);
    for(int i=0;i<WIDGET_PLACE_MAX;i++){
        assert(widget_place_sel[0]==i);
        widget_place_cycle(0,1);
    }
    assert(widget_place_sel[0]==0);
    widget_place_cycle(0,-1);assert(widget_place_sel[0]==WIDGET_PLACE_MAX-1);
    widget_place_select(0,2);assert(!strcmp(widget_place_name(0),"Tokyo"));
    struct tm tokyo=widget_place_time(0,0);assert(tokyo.tm_hour==9);
    int before_remove=widget_place_count[0];
    widget_place_remove();assert(widget_place_count[0]==before_remove-1);
    widget_places_loaded=0;widget_places_init();assert(widget_place_count[0]==before_remove-1);
    puts("PASS: egg-only progression + permanent helpers; 100% runner finish + gravity; dual Pong readiness; nearby Connect 4; best-of-three; settings rows; 4000-game sort; five saved widget places");
}
