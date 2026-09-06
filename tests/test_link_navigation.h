/* Exercise the real gamepad-to-key queue and the Link/Friends event owner. */
static void link_test_pad_press(AppState *state,SDL_Keycode key){
    SDL_FlushEvents(SDL_FIRSTEVENT,SDL_LASTEVENT);
    pad_push_key(key);
    SDL_Event event;int downs=0,ups=0;
    while(SDL_PollEvent(&event)){
        if(event.type==SDL_KEYDOWN){assert(sf_route_key(event.key.keysym.sym,state));downs++;}
        else if(event.type==SDL_KEYUP)ups++;
    }
    assert(downs==1&&ups==1);
}
static void link_test_font_screens(SDL_Renderer *ren){
    TTF_Font *saved_small=font_small,*saved_label=font_label,*saved_fixed=font_fixed;
    static const struct {int family,size;const char *name;} presets[]={
        {5,1,"link-minigames-sans-medium"},{5,3,"link-minigames-sans-xlarge"},
        {6,1,"link-minigames-terminal-medium"},{8,1,"link-minigames-retro-medium"}
    };
    sf.mini=1;sf.choice=1;
    for(int i=0;i<(int)(sizeof presets/sizeof presets[0]);i++){
        int family=presets[i].family;float size=font_size_scale[presets[i].size];char path[256];
        snprintf(path,sizeof path,"assets/fonts/%s",font_choice_files[family]);
        font_small=TTF_OpenFont(path,(int)(font_choice_small_size[family]*size));
        font_label=TTF_OpenFont(path,(int)(font_choice_label_size[family]*size));
        font_fixed=TTF_OpenFont(path,14);assert(font_small&&font_label&&font_fixed);
        sf_render(ren);capture(ren,presets[i].name);
        text_cache_clear();TTF_CloseFont(font_small);TTF_CloseFont(font_label);TTF_CloseFont(font_fixed);
    }
    font_small=saved_small;font_label=saved_label;font_fixed=saved_fixed;
}
static void test_link_navigation(SDL_Renderer *ren){
    AppState state=STATE_LINK;
    sf.prompt=sf.outgoing=sf.mini=sf.choice=sf.message=0;sf.connected[0]=0;
    int old_system=link_system_sel,old_game=link_game_sel;
    link_test_pad_press(&state,pad_map_joybutton(pad_idx(PADK_R1,8)));
    assert(state==STATE_FRIENDS&&sf.mini&&sf.choice==0);
    link_test_pad_press(&state,SDLK_DOWN);assert(sf.choice==1);
    sf_render(ren);capture(ren,"link-minigames-selected-connect4");
    link_test_pad_press(&state,SDLK_DOWN);assert(sf.choice==2);
    link_test_pad_press(&state,SDLK_DOWN);assert(sf.choice==0);
    link_test_pad_press(&state,SDLK_UP);assert(sf.choice==2);
    sf_render(ren);capture(ren,"link-minigames-selected-tank");
    link_test_pad_press(&state,SDLK_RIGHT);assert(sf.choice==2&&sf.message==1);
    link_test_pad_press(&state,SDLK_LEFT);assert(sf.choice==2&&sf.message==0);
    assert(link_system_sel==old_system&&link_game_sel==old_game);
    link_test_pad_press(&state,pad_map_joybutton(pad_idx(PADK_Y,5)));
    assert(state==STATE_FRIENDS&&!sf.mini);
    link_test_pad_press(&state,pad_map_joybutton(pad_idx(PADK_R1,8)));
    assert(state==STATE_FRIENDS&&sf.mini&&sf.choice==2);
    link_test_pad_press(&state,SDLK_UP);assert(sf.choice==1);
    link_test_pad_press(&state,SDLK_RETURN);assert(state==STATE_MINIGAME&&mg_cur==MG_TTT);
    /* Other screens must keep ownership of their keys. */
    assert(!sf_route_key(SDLK_DOWN,&state));
    state=STATE_LINK;sf.choice=99;
    link_test_pad_press(&state,SDLK_e);assert(sf.choice==2);
    link_test_pad_press(&state,SDLK_ESCAPE);assert(state==STATE_LINK&&!sf.mini);
    sf_header(ren,"LINK PLAY");capture(ren,"link-header-shortcuts");
    link_test_font_screens(ren);
    sf.mini=sf.choice=sf.message=0;
    puts("PASS: Link gamepad queue, Up/Down selection and wrap, scoped messages, Friends shortcut, selected-game launch and B return");
}
