static void test_battery_game_notice(SDL_Renderer *ren) {
    battery_game_notice_hide();
    assert(!battery_game_notice_update(100,1,0,20)); // an already announced threshold stays quiet
    assert(battery_game_notice_update(100,1,1,19)==2);
    assert(battery_game_notice_update(101,1,0,19)==1); // cached frame, no redraw
    assert(battery_game_notice_update(500,1,0,18)==2); // actual percentage updates without resetting timer
    assert(battery_game_notice_update(6099,1,0,18)==1);
    assert(!battery_game_notice_update(6100,1,0,18)); // exactly six seconds
    assert(!battery_game_notice_update(7000,1,0,18));
    assert(battery_game_notice_update(8000,1,1,5)==2); // the next threshold can announce once
    assert(!battery_game_notice_update(8001,0,0,5)); // charging, accepted, or cancelled recommendation
    assert(!battery_game_notice_update(8002,1,1,-1));
    assert(!battery_game_notice_update(8003,1,1,101));
    Uint32 start=UINT32_MAX-2000u;
    assert(battery_game_notice_update(start,1,1,20)==2);
    assert(battery_game_notice_update(start+5999u,1,0,20)==1);
    assert(!battery_game_notice_update(start+6000u,1,0,20));
    assert(battery_game_notice_update(10000,1,1,5)==2);
    battery_game_notice_hide(); // sleep/exit never leaves an overlay visible
    assert(!battery_game_notice_update(10001,1,0,5));

    // Render the same pixels used by the hardware OSD entirely in memory.
    const int values[]={19,5};const char *shots[]={"battery-game-notice-19","battery-game-notice-5"};
    for(int i=0;i<2;i++) {
        SDL_Surface *panel=battery_game_notice_surface(values[i]);assert(panel);
        assert(panel->w==BATTERY_GAME_NOTICE_W&&panel->h==BATTERY_GAME_NOTICE_H);
        SDL_Texture *texture=SDL_CreateTextureFromSurface(ren,panel);assert(texture);
        SDL_FreeSurface(panel);
        SDL_RenderCopy(ren,texture,NULL,&(SDL_Rect){(WIN_W-BATTERY_GAME_NOTICE_W)/2,
            WIN_H-BATTERY_GAME_NOTICE_H-24,BATTERY_GAME_NOTICE_W,BATTERY_GAME_NOTICE_H});
        SDL_DestroyTexture(texture);capture(ren,shots[i]);
    }
    puts("PASS: passive in-game battery banner, six-second deadline/tick wrap, change-only rendering, sleep/exit cleanup, actual-percent text; no controller input interception");
}
