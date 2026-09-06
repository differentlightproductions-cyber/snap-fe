static void test_sound_options(SDL_Renderer *ren) {
    int old_radio_pid=radio_pid,old_music_pid=music_pid,old_paused=music_paused;
    int old_radio_over=radio_over_games,old_music_over=music_over_games;
    int old_radio_audio=radio_game_audio,old_music_audio=music_game_audio;
    int old_radio_open=snd_grp_radio_open,old_music_open=snd_grp_music_open;
    int old_os_open=snd_grp_osui_open,old_boot_open=snd_grp_boot_open;
    for(int flags=0;flags<64;flags++) {
        radio_pid=(flags&1)?100:0; music_pid=(flags&2)?101:0;
        radio_over_games=!!(flags&4); music_over_games=!!(flags&8);
        radio_game_audio=!!(flags&16); music_game_audio=!!(flags&32); music_paused=0;
        int expected=((flags&1)&&(flags&4)&&!(flags&16))||((flags&2)&&(flags&8)&&!(flags&32));
        assert(game_background_audio_mutes()==expected);
    }
    radio_pid=0;music_pid=101;music_over_games=1;music_game_audio=0;music_paused=1;
    assert(!game_background_audio_mutes()); // a paused track does not silence a game
    music_paused=0;assert(game_background_audio_mutes());

    // Exercise the actual mini-game mixer without opening an audio device.
    assert(!audio_dev);
    Sint16 samples[16];for(int i=0;i<16;i++)samples[i]=1000;
    Sint16 out[16];Sint16 *old_buf=mg_music_buf;int old_len=mg_music_len,old_pos=mg_music_pos;
    int old_master=master_volume_pct,old_os=os_audio_enabled,old_running=game_running;
    ActiveSound old_sounds[MAX_ACTIVE_SOUNDS];memcpy(old_sounds,active_sounds,sizeof old_sounds);
    memset(active_sounds,0,sizeof active_sounds);mg_music_buf=samples;mg_music_len=16;mg_music_pos=0;
    master_volume_pct=100;os_audio_enabled=1;game_running=0;
    minigame_background_audio_tick(1);audio_callback(NULL,(Uint8*)out,sizeof out);
    for(int i=0;i<16;i++)assert(out[i]==0);
    music_game_audio=1;minigame_background_audio_tick(1);audio_callback(NULL,(Uint8*)out,sizeof out);
    for(int i=0;i<16;i++)assert(out[i]==450);
    radio_pid=100;radio_over_games=1;radio_game_audio=0;
    minigame_background_audio_tick(1);audio_callback(NULL,(Uint8*)out,sizeof out);
    for(int i=0;i<16;i++)assert(out[i]==0);
    radio_pid=0;minigame_background_audio_tick(1);audio_callback(NULL,(Uint8*)out,sizeof out);
    for(int i=0;i<16;i++)assert(out[i]==450); // stream stopped: game audio returns immediately
    music_game_audio=0;minigame_background_audio_tick(0);audio_callback(NULL,(Uint8*)out,sizeof out);
    for(int i=0;i<16;i++)assert(out[i]==450); // leaving the game restores ordinary UI audio
    mg_music_buf=old_buf;mg_music_len=old_len;mg_music_pos=old_pos;
    master_volume_pct=old_master;os_audio_enabled=old_os;game_running=old_running;
    memcpy(active_sounds,old_sounds,sizeof old_sounds);radio_pid=music_pid=0;

    snd_grp_osui_open=snd_grp_boot_open=snd_grp_radio_open=snd_grp_music_open=1;
    int rows[MAX_SOUND_ROWS],extra[MAX_SOUND_ROWS];
    for(int mode=0;mode<4;mode++) {
        radio_over_games=!!(mode&1);music_over_games=!!(mode&2);
        int n=build_sound_rows(rows,extra),radio_rows=0,music_rows=0;
        assert(n<=MAX_SOUND_ROWS);
        for(int i=0;i<n;i++){radio_rows+=rows[i]==ROW_SND_RADIO_GAME_AUDIO;music_rows+=rows[i]==ROW_SND_MUSIC_GAME_AUDIO;}
        assert(radio_rows==radio_over_games&&music_rows==music_over_games);
    }
    // Save/reload both independent choices in the harness's temporary data root.
    radio_game_audio=0;music_game_audio=1;save_settings();
    radio_game_audio=1;music_game_audio=0;load_settings();
    assert(radio_game_audio==0&&music_game_audio==1);
    radio_game_audio=1;music_game_audio=0;save_settings();
    radio_game_audio=0;music_game_audio=1;load_settings();
    assert(radio_game_audio==1&&music_game_audio==0);
    restore_current_settings_tab(TAB_SOUND);
    assert(radio_game_audio==1&&music_game_audio==0&&!radio_over_games&&!music_over_games);
    radio_pid=old_radio_pid;music_pid=old_music_pid;music_paused=old_paused;
    radio_over_games=old_radio_over;music_over_games=old_music_over;
    radio_game_audio=old_radio_audio;music_game_audio=old_music_audio;
    snd_grp_radio_open=old_radio_open;snd_grp_music_open=old_music_open;
    snd_grp_osui_open=old_os_open;snd_grp_boot_open=old_boot_open;

    int old_genre=mgx_genre_sel,old_game=mg_menu_sel,ids[MG_COUNT];
    mgx_genre_sel=0;int n=mgx_genre_list(0,ids);assert(n>4);
    mg_menu_sel=ids[0];mgx_render_menu(ren);capture(ren,"minigames-scrollbar-top");
    for(int i=1;i<n;i++)mgx_menu_key(SDLK_DOWN);
    assert(mg_menu_sel==ids[n-1]);mgx_render_menu(ren);capture(ren,"minigames-scrollbar-bottom");
    mgx_genre_sel=3;mgx_genre_list(3,ids);mg_menu_sel=ids[0];
    mgx_render_menu(ren);capture(ren,"minigames-no-scrollbar");
    mgx_genre_sel=old_genre;mg_menu_sel=old_game;
    puts("PASS: independent Radio/SD Music game-audio policies, persistence/reset, real mini-game mixer transitions and pause/stop handling; scrolling genre menu captures");
}
