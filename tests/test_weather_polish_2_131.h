#ifndef SNAP_TEST_WEATHER_POLISH_2_131_H
#define SNAP_TEST_WEATHER_POLISH_2_131_H
/* Second round of weather polish: the sleep screen has to stay legible whatever
   theme is set, Sleep Screen Weather and Settings > Screen & Power are one
   choice rather than two that fight, and a hand-typed place name gets corrected
   to the spelling the geocoder actually resolved. */

static void test_weather_saver_legibility(void) {
    int saved_theme = theme_idx, saved_style = weather_style_idx;
    /* Whatever the theme, the readout picks ink from the BACKDROP, so a dark
       theme never leaves dim grey text on a dark sky. */
    for (int t = 0; t < theme_count; t++) {
        theme_idx = t;
        weather_style_idx = WEATHER_STYLE_MODERN;
        int light = weather_saver_backdrop_is_light(&themes[t]);
        SDL_Color bg = themes[t].bg;
        int lum = (bg.r * 30 + bg.g * 59 + bg.b * 11) / 100;
        assert(light == (lum > 128));
    }
    /* The Weather Channel backdrop is its own gradient, always dark, so the
       theme must not talk it into dark-on-dark. */
    for (int t = 0; t < theme_count; t++) {
        theme_idx = t;
        weather_style_idx = WEATHER_STYLE_CHANNEL;
        assert(!weather_saver_backdrop_is_light(&themes[t]));
    }
    theme_idx = saved_theme; weather_style_idx = saved_style;
    puts("PASS: the weather sleep screen picks its ink from the backdrop, not the theme");
}

static void test_sleep_screen_one_choice(void) {
    int saved_idx = screen_saver_idx, saved_prev = screen_saver_prev_idx;
    int saved_on = weather_screensaver_enabled, saved_always = screen_saver_always_on;
    AppState state = STATE_WEATHER_SETTINGS;

    /* Turning it on from Weather Settings sets the real sleep screen and
       remembers what was there before. */
    screen_saver_idx = SCREEN_SAVER_AQUARIUM;
    weather_settings_begin();
    assert(!weather_screensaver_enabled);          /* read back from the setting */
    weather_screensaver_enabled = 1;
    weather_settings_commit(&state);
    assert(screen_saver_idx == SCREEN_SAVER_WEATHER);
    assert(screen_saver_prev_idx == SCREEN_SAVER_AQUARIUM);

    /* Coming back in it reads ON, because it IS the sleep screen now. */
    weather_settings_begin();
    assert(weather_screensaver_enabled);
    /* Turning it off restores the previous choice rather than a default. */
    weather_screensaver_enabled = 0;
    weather_settings_commit(&state);
    assert(screen_saver_idx == SCREEN_SAVER_AQUARIUM);

    /* And the other direction: choosing a different sleep screen in Screen &
       Power shows up in Weather Settings as OFF. */
    screen_saver_idx = SCREEN_SAVER_STARFIELD;
    weather_settings_begin();
    assert(!weather_screensaver_enabled);
    /* ... while selecting Weather there shows up as ON. */
    screen_saver_idx = SCREEN_SAVER_WEATHER;
    weather_settings_begin();
    assert(weather_screensaver_enabled);

    /* Never restores itself into Weather, which would make the toggle a no-op. */
    screen_saver_prev_idx = SCREEN_SAVER_WEATHER;
    weather_screensaver_enabled = 0;
    weather_settings_commit(&state);
    assert(screen_saver_idx != SCREEN_SAVER_WEATHER);

    /* Keep Sleep Screen On is honoured only on wall power: the timeout still
       applies on battery however the setting is left. */
    screen_saver_always_on = 1;
    screen_saver_active = 1;
    screen_saver_started = 0;
    if (!power_is_plugged_in()) assert(screen_saver_should_sleep(screen_saver_duration_ms() + 1));
    screen_saver_always_on = 0;
    assert(screen_saver_should_sleep(screen_saver_duration_ms() + 1));
    screen_saver_active = 0;

    screen_saver_idx = saved_idx; screen_saver_prev_idx = saved_prev;
    weather_screensaver_enabled = saved_on; screen_saver_always_on = saved_always;
    weather_settings_confirm_pending = 0; weather_settings_dirty = 0;
    puts("PASS: Sleep Screen Weather and Screen & Power are one setting, and always-on is A/C only");
}

static void test_place_name_canonicalised(void) {
    int saved_loaded = widget_places_loaded;
    int saved_count[2] = { widget_place_count[0], widget_place_count[1] };
    int saved_sel[2] = { widget_place_sel[0], widget_place_sel[1] };
    WidgetPlace saved[2][WIDGET_PLACE_MAX];
    memcpy(saved, widget_places, sizeof saved);
    char saved_loc[160]; snprintf(saved_loc, sizeof saved_loc, "%s", weather_loc);

    widget_places_loaded = 1;
    memset(widget_places, 0, sizeof widget_places);
    widget_place_count[0] = widget_place_count[1] = 1;
    widget_place_sel[0] = widget_place_sel[1] = 0;
    /* Typed by hand before there was a picker: no comma anywhere. */
    for (int g = 0; g < 2; g++) {
        snprintf(widget_places[g][0].name, 64, "Prescott Arizona");
        snprintf(widget_places[g][0].query, 160, "Prescott Arizona");
    }
    assert(widget_place_adopt_canonical("Prescott, Arizona") == 1);
    assert(!strcmp(widget_places[0][0].name, "Prescott, Arizona"));
    assert(!strcmp(widget_places[1][0].query, "Prescott, Arizona"));
    assert(!strcmp(weather_loc, "Prescott, Arizona"));
    /* Already correct: nothing to do, and no pointless re-save. */
    assert(widget_place_adopt_canonical("Prescott, Arizona") == 0);

    /* It must never rename a place into a DIFFERENT one. */
    for (int g = 0; g < 2; g++) {
        snprintf(widget_places[g][0].name, 64, "Portland");
        snprintf(widget_places[g][0].query, 160, "Portland");
    }
    assert(widget_place_adopt_canonical("Phoenix, Arizona") == 0);
    assert(!strcmp(widget_places[0][0].name, "Portland"));
    /* Nor offer a form that is no better. */
    assert(widget_place_adopt_canonical("Portland") == 0);
    /* Nor touch automatic location. */
    for (int g = 0; g < 2; g++) snprintf(widget_places[g][0].name, 64, "Local");
    assert(widget_place_adopt_canonical("Springfield, Illinois") == 0);
    assert(!strcmp(widget_places[0][0].name, "Local"));

    memcpy(widget_places, saved, sizeof saved);
    widget_place_count[0] = saved_count[0]; widget_place_count[1] = saved_count[1];
    widget_place_sel[0] = saved_sel[0]; widget_place_sel[1] = saved_sel[1];
    widget_places_loaded = saved_loaded;
    snprintf(weather_loc, sizeof weather_loc, "%s", saved_loc);
    puts("PASS: a hand-typed place name is corrected to the geocoder's spelling, and only when it is the same place");
}

static void test_weather_polish_2_131(SDL_Renderer *ren) {
    /* Both new icons have to resolve, or the row falls back to a text badge --
       which is what the Weather row was doing, because no weather.svg existed. */
    const char *slugs[] = { "weather", "apps" };
    for (unsigned i = 0; i < sizeof slugs / sizeof *slugs; i++) {
        char path[600];
        snprintf(path, sizeof path, "assets/icons/home/simple/%s.svg", slugs[i]);
        FILE *f = fopen(path, "rb");
        assert(f);
        char head[6] = { 0 };
        assert(fread(head, 1, 5, f) == 5);
        assert(!fclose(f));
        assert(!strncmp(head, "<svg", 4));
    }
    (void)ren;
    test_weather_saver_legibility();
    test_sleep_screen_one_choice();
    test_place_name_canonicalised();
}
#endif
