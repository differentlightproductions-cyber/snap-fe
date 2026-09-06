static void test_battery_prompt(void) {
    char path[896], missing[896];
    snprintf(path, sizeof path, "%s/test-battery-prompt.cfg", sn_data_root());
    snprintf(missing, sizeof missing, "%s/no-prompt-directory/state.cfg", sn_data_root());
    unlink(path);
    BatteryPromptState s;
    battery_prompt_read_state(&s, path);
    assert(s.loaded && !s.answered && !s.notices && !s.level);
    battery_prompt_sample(&s, 21, 0, 0, 0); assert(!s.level && !s.dirty);
    battery_prompt_sample(&s, 20, 0, 0, 1); assert(s.level == 20 && s.percent == 20);
    assert(!battery_prompt_answer(&s, 0) && s.answered == 1 && !s.level && s.dirty);
    assert(battery_prompt_write_state(&s, path) && !s.dirty);
    battery_prompt_read_state(&s, path); // same discharge, frontend restarted
    for (int p = 20; p > 5; p--) {
        battery_prompt_sample(&s, p, 0, 0, (Uint32)(100-p));
        assert(!s.level && !s.dirty);
    }
    battery_prompt_sample(&s, 5, 0, 0, 100); assert(s.level == 5);
    assert(!battery_prompt_answer(&s, 0) && s.answered == 2);
    assert(battery_prompt_write_state(&s, path));
    battery_prompt_read_state(&s, path);
    for (int p = 5; p >= 0; p--) {
        battery_prompt_sample(&s, p, 0, 0, 200u + (Uint32)p);
        assert(!s.level && !s.dirty);
    }
    // Low charging and a brief plug above 20 do not reset declined stages.
    battery_prompt_sample(&s, 19, 1, 0, 1000);
    battery_prompt_sample(&s, 19, 1, 0, 40000); assert(s.answered == 2 && !s.dirty);
    battery_prompt_sample(&s, 21, 1, 0, 50000);
    battery_prompt_sample(&s, 21, 1, 0, 79999); assert(s.answered == 2);
    battery_prompt_sample(&s, 20, 0, 0, 80000); assert(s.answered == 2 && !s.level);
    battery_prompt_sample(&s, 21, 1, 0, 90000);
    battery_prompt_sample(&s, 21, 1, 0, 119999); assert(s.answered == 2);
    battery_prompt_sample(&s, 21, 1, 0, 120000); assert(!s.answered && s.dirty);
    battery_prompt_sample(&s, 20, 0, 0, 120001); assert(s.level == 20);
    // Accepting also consumes the stage; disabling Power Save cannot nag again.
    assert(battery_prompt_answer(&s, 1) && s.answered == 1);
    battery_prompt_sample(&s, 19, 0, 0, 120002); assert(!s.level);
    battery_prompt_sample(&s, 5, 0, 0, 120003); assert(s.level == 5);
    assert(battery_prompt_answer(&s, 1) && s.answered == 2);
    assert(!battery_prompt_answer(&s, 1)); // no queued choice cannot enable
    assert(battery_prompt_write_state(&s, path));
    battery_prompt_read_state(&s, path);
    // Recharged while powered off, then unplugged before boot.
    battery_prompt_sample(&s, 80, 0, 0, 200000);
    battery_prompt_sample(&s, 80, 0, 0, 229999); assert(s.answered == 2);
    battery_prompt_sample(&s, 80, 0, 0, 230000); assert(!s.answered && s.dirty);
    battery_prompt_sample(&s, 20, 0, 0, 230001); assert(s.level == 20);
    battery_prompt_sample(&s, 20, 0, 1, 230002); assert(!s.level); // explicit Auto Power Save won
    battery_prompt_sample(&s, 5, 0, 1, 230003); assert(!s.level);
    battery_prompt_sample(&s, 5, 1, 0, 230004); assert(!s.level);
    battery_prompt_sample(&s, -1, 0, 0, 230005); assert(!s.level);
    battery_prompt_sample(&s, 101, 0, 0, 230006); assert(!s.level);
    memset(&s, 0, sizeof s);
    battery_prompt_sample(&s, 4, 0, 0, 0); assert(s.level == 5 && s.percent == 4);
    battery_prompt_answer(&s, 0); assert(s.answered == 2);
    s.notices = 3;
    battery_prompt_sample(&s, 25, 0, 0, 0xfffffff0u);
    battery_prompt_sample(&s, 25, 0, 0, 0xfffffff0u + 30000u);
    assert(!s.answered && !s.notices);
    memset(&s, 0, sizeof s);
    assert(battery_prompt_sample_due(&s, 0xfffffff0u));
    assert(!battery_prompt_sample_due(&s, 0xfffffff0u + 749u));
    assert(battery_prompt_sample_due(&s, 0xfffffff0u + 750u));
    s.dirty = 1; assert(!battery_prompt_write_state(&s, missing) && s.dirty);
    assert(battery_prompt_write_state(&s, path) && !s.dirty);
    FILE *f = fopen(path, "w"); assert(f); fputs("1 999 0\n", f); fclose(f);
    battery_prompt_read_state(&s, path); assert(!s.answered && !s.notices);
    unlink(path);

    BatteryPromptState saved = battery_prompt_state;
    battery_prompt_read_state(&battery_prompt_state, path);
    battery_prompt_state.allow_present = 0;
    battery_prompt_sample(&battery_prompt_state, 18, 0, 0, 100);
    assert(battery_prompt_pending() && battery_prompt_percent() == 18 && battery_prompt_game_notice_due());
    battery_prompt_game_notice_marked();
    assert(battery_prompt_pending() && !battery_prompt_game_notice_due() && !battery_prompt_state.answered);
    battery_prompt_read_state(&battery_prompt_state, battery_prompt_path());
    battery_prompt_sample(&battery_prompt_state, 17, 0, 0, 200);
    assert(battery_prompt_pending() && !battery_prompt_game_notice_due());
    battery_prompt_sample(&battery_prompt_state, 5, 0, 0, 300);
    assert(battery_prompt_pending() && battery_prompt_game_notice_due() && battery_prompt_percent() == 5);
    battery_prompt_game_notice_marked();
    battery_prompt_read_state(&battery_prompt_state, battery_prompt_path());
    battery_prompt_sample(&battery_prompt_state, 4, 0, 0, 400);
    assert(battery_prompt_pending() && !battery_prompt_game_notice_due());
    battery_prompt_respond(0); assert(!battery_prompt_pending() && battery_prompt_state.answered == 2);
    unlink(battery_prompt_path()); battery_prompt_state = saved;

    BatteryPromptGuard g = {0};
    battery_prompt_guard_tick(&g, 1, 1, 1000);
    assert(battery_prompt_guard_choice(&g, SDLK_RETURN, 0) == -1);
    battery_prompt_guard_tick(&g, 1, 0, 1449);
    assert(!g.armed);
    battery_prompt_guard_tick(&g, 1, 1, 1450); assert(!g.armed);
    battery_prompt_guard_tick(&g, 1, 1, 2000); assert(!g.armed);
    battery_prompt_guard_tick(&g, 1, 0, 2001); assert(g.armed);
    assert(battery_prompt_guard_choice(&g, SDLK_RETURN, 1) == -1);
    assert(battery_prompt_guard_choice(&g, SDLK_x, 0) == -1);
    assert(battery_prompt_guard_choice(&g, SDLK_RETURN, 0) == 1);
    assert(battery_prompt_guard_choice(&g, SDLK_ESCAPE, 0) == 0);
    battery_prompt_guard_tick(&g, 0, 0, 2002); assert(!g.visible && !g.armed);
    battery_prompt_guard_tick(&g, 1, 0, 0xfffffff0u);
    battery_prompt_guard_tick(&g, 1, 0, 0xfffffff0u + 450u); assert(g.armed);
    puts("PASS: persistent 20%/5% choices and game notices; first-low boot; auto-save priority; recharge recovery; guarded confirmation");
}

static void test_battery_prompt_render(SDL_Renderer *ren) {
    BatteryPromptState saved = battery_prompt_state;
    BatteryPromptGuard saved_guard = battery_prompt_guard;
    int *flags[] = {&game_running, &lid_closed, &is_sleeping, &screen_saver_active,
        &sf.prompt, &calendar_reminder_popup, &wifi_prompt_active, &settings_confirm_pending,
        &settings_autops_confirm, &settings_night_theme_confirm, &aspect_preview_active,
        &flashlight_pending, &app_widget_picker, &app_widget_moving, &home_grid_reorder,
        &g_theme_editor.active};
    int old[sizeof flags / sizeof flags[0]], old_offer = sf_offer;
    for (int i = 0; i < (int)(sizeof flags / sizeof flags[0]); i++) { old[i] = *flags[i]; *flags[i] = 0; }
    sf_offer = -1;
    assert(battery_prompt_frontend_allowed(STATE_HOME));
    assert(battery_prompt_frontend_allowed(STATE_SETTINGS));
    assert(battery_prompt_frontend_allowed(STATE_MINIGAMES));
    assert(!battery_prompt_frontend_allowed(STATE_MINIGAME));
    assert(!battery_prompt_frontend_allowed(STATE_SETUP));
    assert(!battery_prompt_frontend_allowed(STATE_KEYBOARD));
    for (int i = 0; i < (int)(sizeof flags / sizeof flags[0]); i++) {
        *flags[i] = 1; assert(!battery_prompt_frontend_allowed(STATE_HOME)); *flags[i] = 0;
    }
    memset(&battery_prompt_state, 0, sizeof battery_prompt_state);
    battery_prompt_state.allow_present = 1;
    battery_prompt_sample(&battery_prompt_state, 18, 0, 0, 0);
    battery_prompt_guard_tick(&battery_prompt_guard, 0, 0, 0);
    battery_prompt_guard_tick(&battery_prompt_guard, 1, 0, 1000);
    assert(battery_prompt_frontend_key(STATE_HOME, SDLK_RETURN, 0));
    assert(battery_prompt_pending() && !battery_prompt_state.answered);
    battery_prompt_guard_tick(&battery_prompt_guard, 1, 0, 1450);
    assert(!battery_prompt_frontend_key(STATE_MINIGAME, SDLK_ESCAPE, 0));
    assert(battery_prompt_pending());
    draw_theme_background(ren, &themes[theme_idx], theme_idx);
    battery_prompt_render(ren, STATE_HOME); assert(battery_prompt_panel);
    capture(ren, "battery-save-choice-18");
    assert(battery_prompt_frontend_key(STATE_HOME, SDLK_ESCAPE, 0));
    assert(!battery_prompt_pending() && !battery_prompt_guard.visible && battery_prompt_state.answered == 1);
    battery_prompt_render(ren, STATE_HOME); assert(!battery_prompt_panel);
    battery_prompt_sample(&battery_prompt_state, 5, 0, 0, 2000);
    battery_prompt_render(ren, STATE_MINIGAME);
    assert(battery_prompt_panel && battery_prompt_pending() && !battery_prompt_game_notice_due());
    capture(ren, "battery-minigame-notice-5");
    // Explicit texture release is idempotent and permits recreation afterward.
    battery_prompt_render_clear(); battery_prompt_render_clear(); assert(!battery_prompt_panel);
    battery_prompt_state.notices = 0;
    battery_prompt_render(ren, STATE_MINIGAME); assert(battery_prompt_panel);
    battery_prompt_render_clear();
    battery_prompt_state = saved; battery_prompt_guard = saved_guard;
    for (int i = 0; i < (int)(sizeof flags / sizeof flags[0]); i++) *flags[i] = old[i];
    sf_offer = old_offer; unlink(battery_prompt_path());
}
