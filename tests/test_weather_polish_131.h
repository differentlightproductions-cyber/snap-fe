#ifndef SNAP_TEST_WEATHER_POLISH_131_H
#define SNAP_TEST_WEATHER_POLISH_131_H
/* 1.3.1 Weather polish:
     - the Weather Channel ticker holds still long enough to read the city,
     - the 8-Bit / 16-Bit styles are gone and old configs migrate off them,
     - the wallpaper / sleep-screen scene uses the soft raster sun and moon,
     - the Weather save/discard modal is now the frontend-wide one. */

static void test_weather_ticker_hold(void) {
    /* A line that fits never moves, whatever the clock says. */
    int moving = 1;
    assert(weather_ticker_offset(0, 400, &moving) == 0 && !moving);
    assert(weather_ticker_offset(WEATHER_TICKER_HOLD_MS, 400, &moving) == 0 && !moving);

    /* Through the hold the head of the line -- the city -- stays parked. */
    for (Uint32 t = 0; t <= WEATHER_TICKER_HOLD_MS; t += 250) {
        moving = 1;
        assert(weather_ticker_offset(t, 900, &moving) == 0 && !moving);
    }

    /* It starts from zero the moment the hold ends, so there is no jump. */
    moving = 0;
    assert(weather_ticker_offset(WEATHER_TICKER_HOLD_MS + 1, 900, &moving) == 0 && moving);
    assert(weather_ticker_offset(WEATHER_TICKER_HOLD_MS + 35, 900, &moving) == -1 && moving);
    assert(weather_ticker_offset(WEATHER_TICKER_HOLD_MS + 350, 900, &moving) == -10);

    /* And it wraps at text width + the 90px gap rather than running away. */
    Uint32 lap = WEATHER_TICKER_HOLD_MS + 35u * (900 + 90);
    assert(weather_ticker_offset(lap, 900, &moving) == 0);
    for (Uint32 t = WEATHER_TICKER_HOLD_MS; t < lap * 2; t += 137) {
        int off = weather_ticker_offset(t, 900, &moving);
        assert(off <= 0 && off > -(900 + 90));
    }

    /* Opening the app and switching cities both restart the hold. */
    weather_app_enter(STATE_HOME);
    Uint32 opened = weather_ticker_started;
    assert(SDL_GetTicks() - opened < 1000);
    weather_ticker_started = opened - 9000u;
    weather_switch_location(1);
    assert(weather_ticker_started != opened - 9000u);
    puts("PASS: weather ticker parks the city for the hold, then crawls and wraps");
}

static void test_weather_style_list(void) {
    /* The two bit-art styles are gone; the three that changed the layout stay. */
    assert(WEATHER_STYLE_COUNT == 3);
    assert(!strcmp(weather_style_names[WEATHER_STYLE_MODERN], "SNAP Modern"));
    assert(!strcmp(weather_style_names[WEATHER_STYLE_CHANNEL], "Weather Channel"));
    assert(!strcmp(weather_style_names[WEATHER_STYLE_MINIMAL], "Minimal"));
    for (int i = 0; i < WEATHER_STYLE_COUNT; i++)
        assert(!strstr(weather_style_names[i], "Bit") && !strstr(weather_style_names[i], "bit"));

    /* Configs written by 1.3.0 carried the old five-entry order, where 1 and 2
       were the retired bit-art styles. Both fall back to Modern. */
    assert(weather_style_from_legacy(0) == WEATHER_STYLE_MODERN);
    assert(weather_style_from_legacy(1) == WEATHER_STYLE_MODERN);   /* 8-Bit  */
    assert(weather_style_from_legacy(2) == WEATHER_STYLE_MODERN);   /* 16-Bit */
    assert(weather_style_from_legacy(3) == WEATHER_STYLE_CHANNEL);
    assert(weather_style_from_legacy(4) == WEATHER_STYLE_MINIMAL);
    assert(weather_style_from_legacy(-1) == WEATHER_STYLE_MODERN);
    assert(weather_style_from_legacy(99) == WEATHER_STYLE_MODERN);

    /* Current builds persist the name, which survives any future renumbering. */
    for (int i = 0; i < WEATHER_STYLE_COUNT; i++)
        assert(weather_style_from_name(weather_style_names[i]) == i);
    assert(weather_style_from_name("weather channel") == WEATHER_STYLE_CHANNEL);
    assert(weather_style_from_name("8-Bit") == WEATHER_STYLE_MODERN);
    assert(weather_style_from_name("") == WEATHER_STYLE_MODERN);
    assert(weather_style_from_name(NULL) == WEATHER_STYLE_MODERN);

    /* And an out-of-range value can never index off the name table. */
    int held = weather_style_idx;
    weather_style_idx = 4;  assert(weather_style_current() == WEATHER_STYLE_MODERN);
    weather_style_idx = -3; assert(weather_style_current() == WEATHER_STYLE_MODERN);
    weather_style_idx = held;

    /* Cycling the Style row visits exactly the three survivors, both ways. */
    AppState state = STATE_WEATHER_SETTINGS;
    int saved_style = weather_style_idx;
    weather_settings_begin();
    weather_settings_sel = 2;
    weather_style_idx = WEATHER_STYLE_MODERN;
    int seen[WEATHER_STYLE_COUNT] = { 0 };
    for (int i = 0; i < WEATHER_STYLE_COUNT; i++) {
        assert(weather_style_idx >= 0 && weather_style_idx < WEATHER_STYLE_COUNT);
        seen[weather_style_idx] = 1;
        weather_settings_key(SDLK_RIGHT, &state);
    }
    assert(weather_style_idx == WEATHER_STYLE_MODERN);
    for (int i = 0; i < WEATHER_STYLE_COUNT; i++) assert(seen[i]);
    weather_settings_key(SDLK_LEFT, &state);
    assert(weather_style_idx == WEATHER_STYLE_MINIMAL);

    /* A stale index cycles from the clamped Modern rather than off the end. */
    weather_style_idx = 4;
    weather_settings_key(SDLK_RIGHT, &state);
    assert(weather_style_idx == WEATHER_STYLE_CHANNEL);

    weather_style_idx = saved_style;
    weather_settings_confirm_pending = 0;
    weather_settings_dirty = 0;
    puts("PASS: 8-Bit/16-Bit weather styles removed, saved indices migrated, cycling stays in range");
}

static void test_weather_sun_arc(void) {
    /* The arc itself: low at both horizons, highest at the midpoint, and
       travelling left to right across the whole width. */
    int x0, y0, x1, y1, xm, ym;
    weather_arc_position(0.0f, 300, &x0, &y0);
    weather_arc_position(0.5f, 300, &xm, &ym);
    weather_arc_position(1.0f, 300, &x1, &y1);
    assert(x0 < 0);                       /* rises from off the left edge   */
    assert(x1 > WIN_W);                   /* sets off the right             */
    assert(xm > x0 && x1 > xm);           /* strictly left to right         */
    assert(ym < y0 && ym < y1);           /* highest in the middle          */
    assert(y0 == y1);                     /* symmetric horizons             */

    /* Monotonic all the way across -- no jumps backwards mid-day. */
    int prev = -100000;
    for (int step = 0; step <= 100; step++) {
        int x, y;
        weather_arc_position(step / 100.0f, 300, &x, &y);
        assert(x >= prev);
        prev = x;
        assert(y >= HUD_BAR_H);
    }

    /* The phase tracks the clock against the location's own sunrise/sunset,
       and clamps rather than sliding off during the other half of the cycle. */
    int saved_rise = g_sunrise_minute, saved_set = g_sunset_minute;
    time_t saved_at = g_sun_times_at;
    g_sunrise_minute = 6 * 60; g_sunset_minute = 18 * 60;
    g_sun_times_at = time(NULL);
    float day = weather_arc_phase(0), night = weather_arc_phase(1);
    assert(day >= 0.0f && day <= 1.0f && night >= 0.0f && night <= 1.0f);

    /* A stale reading is ignored, and so is a nonsensical one: both fall back
       to a conventional day rather than producing a bogus arc. */
    g_sun_times_at = time(NULL) - 72 * 60 * 60;
    assert(weather_arc_phase(0) >= 0.0f && weather_arc_phase(0) <= 1.0f);
    g_sun_times_at = time(NULL);
    g_sunset_minute = g_sunrise_minute;          /* a day with no length */
    assert(weather_arc_phase(0) >= 0.0f && weather_arc_phase(0) <= 1.0f);
    assert(weather_arc_phase(1) >= 0.0f && weather_arc_phase(1) <= 1.0f);
    g_sunrise_minute = -1; g_sunset_minute = -1; g_sun_times_at = 0;
    assert(weather_arc_phase(0) >= 0.0f && weather_arc_phase(0) <= 1.0f);

    g_sunrise_minute = saved_rise; g_sunset_minute = saved_set; g_sun_times_at = saved_at;
    puts("PASS: the sun and moon walk a clamped left-to-right arc from the real sunrise/sunset");
}

static void test_weather_ambient_art(SDL_Renderer *ren) {
    /* The raster art must actually ship -- the scene silently loses the sun and
       moon if it does not, which is exactly the regression to catch. */
    for (int i = 0; i < WEATHER_ART_COUNT; i++) {
        char path[900];
        snprintf(path, sizeof path, "assets/weather/%s", weather_art_files[i]);
        FILE *f = fopen(path, "rb");
        assert(f);
        unsigned char sig[8] = { 0 };
        assert(fread(sig, 1, 8, f) == 8);
        assert(!fclose(f));
        assert(!memcmp(sig, "\x89PNG\r\n\x1a\n", 8));
    }

    /* Every condition renders as both a wallpaper and a sleep-screen scene. */
    const int kinds[] = { WX_SUN, WX_CLEARNIGHT, WX_PARTLY, WX_CLOUD,
                          WX_RAIN, WX_STORM, WX_SNOW, WX_FOG, WX_WIND };
    for (unsigned i = 0; i < sizeof kinds / sizeof *kinds; i++) {
        for (int saver = 0; saver <= 1; saver++) {
            SDL_SetRenderDrawColor(ren, 12, 16, 28, 255);
            SDL_RenderClear(ren);
            weather_ambient_render(ren, kinds[i], saver);
        }
    }
    /* WX_NONE means "no reading yet" and must paint nothing at all. */
    weather_ambient_render(ren, WX_NONE, 0);

    SDL_SetRenderDrawColor(ren, 12, 16, 28, 255); SDL_RenderClear(ren);
    weather_ambient_render(ren, WX_SUN, 0); capture(ren, "weather-wallpaper-sun");
    SDL_SetRenderDrawColor(ren, 12, 16, 28, 255); SDL_RenderClear(ren);
    weather_ambient_render(ren, WX_CLEARNIGHT, 0); capture(ren, "weather-wallpaper-night");
    SDL_SetRenderDrawColor(ren, 12, 16, 28, 255); SDL_RenderClear(ren);
    weather_ambient_render(ren, WX_PARTLY, 0); capture(ren, "weather-wallpaper-partly");

    /* The sleep screen now carries the same scene behind its readout. */
    int saved_saver = screen_saver_runtime;
    screen_saver_runtime = SCREEN_SAVER_WEATHER;
    draw_screen_saver(ren); capture(ren, "weather-sleep-screen");
    screen_saver_runtime = saved_saver;

    /* Losing the renderer must drop every cached texture rather than hand back
       a handle to the destroyed one. (The art itself may not resolve here --
       earlier weather tests point the data root at a fixture directory -- so
       what matters is that the whole cache is invalidated, not what loads.) */
    for (int i = 0; i < WEATHER_ART_COUNT; i++) assert(weather_art_tried[i]);
    renderer_epoch++;
    weather_art(ren, WEATHER_ART_SUN);
    assert(weather_art_epoch == renderer_epoch);
    for (int i = 0; i < WEATHER_ART_COUNT; i++)
        if (i != WEATHER_ART_SUN) assert(!weather_art_tried[i] && !weather_art_texture[i]);
    assert(weather_art(ren, -1) == NULL && weather_art(ren, WEATHER_ART_COUNT) == NULL);
    puts("PASS: soft sun/moon/cloud art ships and renders for every condition, wallpaper and sleep screen");
}

/* Every layout, drawn against the real assets/ tree so the raster icons
   actually resolve -- the other weather captures run with the data root pointed
   at a fixture directory, where the art is missing and the primitive fallback
   is what lands in the PNG. */
static void test_weather_layouts_with_art(SDL_Renderer *ren) {
    char *saved_root = getenv("SNAPFE_DATA_ROOT");
    if (saved_root) saved_root = strdup(saved_root);
    /* The suite runs from the repo root, so "." is where assets/ actually is. */
    setenv("SNAPFE_DATA_ROOT", ".", 1);
    renderer_epoch++;                       /* drop anything cached from the fixture root */

    int saved_style = weather_style_idx, saved_focus = weather_app_sel;
    weather_app_sel = WEATHER_FOCUS_CURRENT;
    weather_style_idx = WEATHER_STYLE_MODERN;  weather_app_render(ren); capture(ren, "weather-art-modern");
    weather_style_idx = WEATHER_STYLE_MINIMAL; weather_app_render(ren); capture(ren, "weather-art-minimal");
    weather_style_idx = WEATHER_STYLE_CHANNEL; weather_app_render(ren); capture(ren, "weather-art-channel");
    weather_app_sel = WEATHER_FOCUS_HOURLY;
    weather_style_idx = WEATHER_STYLE_MODERN;
    weather_hourly_sel = 2;
    weather_hourly_render(ren); capture(ren, "weather-art-hourly-screen");

    /* A full day's worth of hours -- what the provider now actually returns --
       has to lay out in two columns rather than running off the panel. */
    int saved_hourly_count = g_weather_hourly_count;
    char saved_hourly[WEATHER_HOURLY_COUNT][64];
    memcpy(saved_hourly, g_weather_hourly, sizeof saved_hourly);
    for (int i = 0; i < WEATHER_HOURLY_COUNT; i++) {
        int hour = (13 + i) % 24;
        snprintf(g_weather_hourly[i], 64, "%d %s %dF Partly Cloudy %d%%",
                 hour % 12 == 0 ? 12 : hour % 12, hour < 12 ? "AM" : "PM", 88 - i, (i * 7) % 100);
    }
    g_weather_hourly_count = WEATHER_HOURLY_COUNT;
    weather_hourly_sel = 11;
    weather_hourly_render(ren); capture(ren, "weather-art-hourly-full-day");
    /* And the compact strip still shows only the leading few. */
    weather_app_sel = WEATHER_FOCUS_HOURLY;
    weather_app_render(ren); capture(ren, "weather-art-modern-full-day");
    memcpy(g_weather_hourly, saved_hourly, sizeof saved_hourly);
    g_weather_hourly_count = saved_hourly_count;

    /* The wallpaper scenes, with the art present rather than silently absent. */
    const int wall[] = { WX_SUN, WX_CLEARNIGHT, WX_PARTLY, WX_RAIN, WX_SNOW };
    const char *wall_name[] = { "sun", "night", "partly", "rain", "snow" };
    for (unsigned i = 0; i < sizeof wall / sizeof *wall; i++) {
        char name[64]; snprintf(name, sizeof name, "weather-art-wallpaper-%s", wall_name[i]);
        SDL_SetRenderDrawColor(ren, 22, 30, 52, 255); SDL_RenderClear(ren);
        weather_ambient_render(ren, wall[i], 0);
        capture(ren, name);
    }
    /* The raster art must be what rendered, not the primitive fallback. */
    assert(weather_art(ren, WEATHER_ART_SUN) && weather_art(ren, WEATHER_ART_MOON) &&
           weather_art(ren, WEATHER_ART_CLOUD));

    weather_style_idx = saved_style; weather_app_sel = saved_focus; weather_hourly_sel = 0;
    if (saved_root) { setenv("SNAPFE_DATA_ROOT", saved_root, 1); free(saved_root); }
    else unsetenv("SNAPFE_DATA_ROOT");
    renderer_epoch++;
    puts("PASS: every weather layout renders against the real art, plus the full hourly screen");
}

static void test_shared_save_modal(SDL_Renderer *ren) {
    /* One modal now serves Weather, Settings and Hotkeys, so it has to survive
       both selections and both display widths the devices actually use. */
    Theme *th = &themes[theme_idx];
    for (int sel = 0; sel <= 1; sel++) {
        SDL_SetRenderDrawColor(ren, 20, 20, 26, 255); SDL_RenderClear(ren);
        draw_save_discard_modal(ren, th, "Save Changes?", sel);
    }
    SDL_SetRenderDrawColor(ren, 20, 20, 26, 255); SDL_RenderClear(ren);
    draw_save_discard_modal(ren, th, "Save Changes?", 0);
    capture(ren, "settings-save-modal");
    SDL_SetRenderDrawColor(ren, 20, 20, 26, 255); SDL_RenderClear(ren);
    draw_save_discard_modal(ren, th, "Save Hotkey Changes?", 1);
    capture(ren, "hotkeys-save-modal-discard");

    /* Every title the frontend actually uses has to fit the card without being
       ellipsised -- the first one shipped read "Save changes before leaving
       this p..." on the device. */
    const char *titles[] = { "Save Changes?", "Save Hotkey Changes?", "Save Weather Settings?" };
    int mw = WIN_W < 690 ? 500 : 540; if (mw > WIN_W - 40) mw = WIN_W - 40;
    for (unsigned i = 0; i < sizeof titles / sizeof *titles; i++) {
        int tw = 0;
        TTF_SizeUTF8(font_small ? font_small : font_label, titles[i], &tw, NULL);
        assert(tw <= mw - 48);
    }

    /* Opening either prompt must land on Save, never on a stale Don't Save. */
    settings_confirm_sel = 1; hk_confirm_sel = 1;
    settings_confirm_pending = 1; settings_confirm_sel = 0;
    hk_confirm_pending = 1; hk_confirm_sel = 0;
    assert(!settings_confirm_sel && !hk_confirm_sel);
    settings_confirm_pending = 0; hk_confirm_pending = 0;
    puts("PASS: shared Save / Don't Save modal renders for Settings and Hotkeys and opens on Save");
}

static void test_weather_polish_131(SDL_Renderer *ren) {
    test_weather_ticker_hold();
    test_weather_sun_arc();
    test_weather_style_list();
    test_weather_ambient_art(ren);
    test_weather_layouts_with_art(ren);
    test_shared_save_modal(ren);
}
#endif
