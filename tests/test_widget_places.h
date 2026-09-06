static void test_widget_places(SDL_Renderer *ren){
    FILE *reply=tmpfile();assert(reply);fputs("success\nPhoenix\n",reply);rewind(reply);char city[64];widget_local_read_reply(reply,1,city);assert(!strcmp(city,"Phoenix"));fclose(reply);
    reply=tmpfile();assert(reply);fputs("fail\nrate limited\n",reply);rewind(reply);widget_local_read_reply(reply,1,city);assert(!city[0]);fclose(reply);
    widget_places_init();memset(widget_places,0,sizeof widget_places);widget_place_count[0]=widget_place_count[1]=1;widget_place_sel[0]=widget_place_sel[1]=0;
    strcpy(widget_places[0][0].name,"Local");strcpy(widget_places[1][0].name,"Local");strcpy(widget_local_city,"Phoenix");
    assert(!strcmp(widget_place_label(0,0),"Local (Phoenix)"));widget_places_save();
    widget_places_open(0);AppState state=STATE_WIDGET_PLACES;widget_places_key(SDLK_RETURN,&state);assert(state==STATE_KEYBOARD&&kb_purpose==KB_PURPOSE_WIDGET_PLACE);
    strcpy(kb_buffer,"Tokoyo");kb_commit(ren,font_label,&platform_selected);
    assert(widget_place_searching&&widget_place_result_count>0&&widget_place_count[0]==1);
    assert(!strcmp(wloc_items[widget_place_results[0]].name,"Tokyo"));state=STATE_WIDGET_PLACES;
    widget_places_render(ren);capture(ren,"widget-location-typo-results");
    widget_places_key(SDLK_RETURN,&state);assert(!widget_place_searching&&widget_place_count[0]==2&&!strcmp(widget_place_name(0),"Tokyo"));
    struct tm tokyo=widget_place_time(0,0);assert(tokyo.tm_hour==9);
    widget_place_search("Phonix");widget_places_key(SDLK_RETURN,&state);assert(widget_place_count[0]==3&&!strcmp(widget_place_name(0),"Phoenix"));
    widget_places_key(SDLK_RETURN,&state);assert(state==STATE_WIDGET_PLACES&&strstr(widget_place_status,"Three"));
    widget_places_render(ren);capture(ren,"widget-three-time-locations");
    widget_places_loaded=0;widget_places_init();assert(widget_place_count[0]==3&&widget_place_sel[0]==2&&!strcmp(widget_places[0][1].name,"Tokyo")&&!strcmp(widget_places[0][2].zone,"America/Phoenix"));
    widget_place_group=0;widget_place_select(0,2);widget_place_remove();assert(widget_place_count[0]==2&&!strcmp(widget_places[0][1].name,"Tokyo"));
    widget_places_open(1);widget_place_search("Seaattle");assert(widget_place_count[1]==1);widget_places_key(SDLK_RETURN,&state);
    assert(widget_place_count[1]==2&&!strcmp(widget_place_name(1),"Seattle")&&strstr(weather_loc,"Seattle"));
    strcpy(g_weather_str,"55F Clear");g_weather_at=123;weather_unit=0;widget_place_cycle(1,-1);widget_place_cycle(1,1);assert(!strcmp(g_weather_str,"55F Clear")&&g_weather_at==123);
    widget_place_cycle(1,-1);weather_unit=1;widget_place_cycle(1,1);assert(!g_weather_str[0]&&g_weather_at==0);
    widget_place_search("zzzzzzqqqq");assert(widget_place_result_count==0&&widget_place_searching);widget_places_render(ren);capture(ren,"widget-location-no-match");
    widget_places_key(SDLK_ESCAPE,&state);assert(!widget_place_searching&&state==STATE_WIDGET_PLACES);widget_places_key(SDLK_ESCAPE,&state);assert(state==STATE_HOME);
    puts("PASS: keyboard search requires confirmation; typo choices; Local city; selected location immediately updates widgets; three-place limit; persistence/removal; weather units; empty results and Back");
}
