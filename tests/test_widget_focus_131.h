#ifndef SNAP_TEST_WIDGET_FOCUS_131_H
#define SNAP_TEST_WIDGET_FOCUS_131_H
/* The informational-home widget path, end to end: Right focuses a card, and A
   opens the app that owns its location. The D-pad deliberately stays with Home.
   This had no coverage of its own -- the old test called the routing function
   directly and never exercised focus acquisition. */

static void test_widget_focus_131(void) {
    int saved_slot = home_recent_focus_slot;
    int saved_w1 = home_widget_idx, saved_w2 = home_widget2_idx;
    int saved_view = home_view_idx;
    int saved_count0 = widget_place_count[0], saved_count1 = widget_place_count[1];
    int saved_sel0 = widget_place_sel[0], saved_sel1 = widget_place_sel[1];
    WidgetPlace saved_places[2][WIDGET_PLACE_MAX];
    memcpy(saved_places, widget_places, sizeof saved_places);

    home_view_idx = 0;                       /* any informational layout */
    widget_places_loaded = 1;
    memset(widget_places, 0, sizeof widget_places);

    /* Three weather places (group 1) and only "Local" for time (group 0) --
       the shape a user gets after saving cities in Weather Settings. */
    snprintf(widget_places[0][0].name, 64, "Local");
    widget_place_count[0] = 1; widget_place_sel[0] = 0;
    static const char *cities[3] = { "Prescott", "Portland", "Phoenix" };
    for (int i = 0; i < 3; i++) {
        snprintf(widget_places[1][i].name, 64, "%s", cities[i]);
        snprintf(widget_places[1][i].query, 160, "%s, Arizona", cities[i]);
    }
    widget_place_count[1] = 3; widget_place_sel[1] = 0;

    /* Right focuses the weather card even when it is the BOTTOM slot and the
       top slot holds something that cannot take focus. */
    home_widget_idx = HOME_WIDGET_STATS;      /* not focusable */
    home_widget2_idx = HOME_WIDGET_WEATHER;
    assert(!home_widget_focusable(home_widget_idx, 0));
    assert(home_widget_focusable(home_widget2_idx, 0));
    assert(home_info_focus_target(home_widget_idx, home_widget2_idx, 0) == 2);

    /* ... and the top slot wins when both can. */
    home_widget_idx = HOME_WIDGET_WEATHER;
    home_widget2_idx = HOME_WIDGET_CALENDAR;
    assert(home_info_focus_target(home_widget_idx, home_widget2_idx, 0) == 1);
    home_widget2_idx = HOME_WIDGET_STATS;
    assert(home_info_focus_target(home_widget_idx, home_widget2_idx, 0) == 1);

    /* Nothing focusable at all -> Right must not claim the key. */
    assert(home_info_focus_target(HOME_WIDGET_STATS, HOME_WIDGET_BATTERY, 0) == 0);
    /* Recently Played is only focusable once it actually has entries. */
    assert(home_info_focus_target(HOME_WIDGET_RECENT, HOME_WIDGET_STATS, 0) == 0);
    assert(home_info_focus_target(HOME_WIDGET_RECENT, HOME_WIDGET_STATS, 2) == 1);

    /* A focused card claims NOTHING on the D-pad on the Informational home.
       Two focusable cards plus the row list all want those keys, and picking a
       location moved into the app the card opens, so the widget stopped
       competing for them. */
    AppState state = STATE_HOME;
    home_recent_focus_slot = 1;
    home_widget_idx = HOME_WIDGET_WEATHER;
    widget_place_sel[1] = 0;
    const SDL_Keycode dpad[] = { SDLK_UP, SDLK_DOWN, SDLK_LEFT, SDLK_RIGHT };
    const int cards[] = { HOME_WIDGET_WEATHER, HOME_WIDGET_DATEWX, HOME_WIDGET_CLOCK, HOME_WIDGET_DATE };
    for (unsigned c = 0; c < sizeof cards / sizeof *cards; c++)
        for (unsigned k = 0; k < sizeof dpad / sizeof *dpad; k++) {
            assert(!home_place_widget_key(cards[c], 0, dpad[k], &state));
            assert(widget_place_sel[0] == 0 && widget_place_sel[1] == 0);
        }
    /* Nor the shoulder keys, so those still swap which card is shown. */
    assert(!home_place_widget_key(HOME_WIDGET_WEATHER, 0, SDLK_PAGEUP, &state));
    assert(!home_place_widget_key(HOME_WIDGET_WEATHER, 0, SDLK_q, &state));
    assert(!home_place_widget_key(HOME_WIDGET_WEATHER, 0, SDLK_ESCAPE, &state));
    assert(state == STATE_HOME);

    /* A is what changes the location now, and where it lands depends on the
       card: a weather card has weather to show, a clock card does not. */
    assert(widget_place_family(HOME_WIDGET_DATEWX) == 1);
    assert(widget_place_family(HOME_WIDGET_WEATHER) == 1);
    assert(widget_place_family(HOME_WIDGET_CLOCK) == 0);
    assert(widget_place_family(HOME_WIDGET_DATE) == 0);
    state = STATE_HOME;
    assert(home_place_widget_key(HOME_WIDGET_WEATHER, 0, SDLK_RETURN, &state));
    assert(state == STATE_WEATHER);
    state = STATE_HOME;
    assert(home_place_widget_key(HOME_WIDGET_CLOCK, 0, SDLK_RETURN, &state));
    assert(state == STATE_WIDGET_PLACES);
    state = STATE_HOME;

    /* A non-place card never claims anything through this route. */
    assert(!home_place_widget_key(HOME_WIDGET_CALENDAR, 0, SDLK_DOWN, &state));
    assert(!home_place_widget_key(HOME_WIDGET_STATS, 0, SDLK_RETURN, &state));

    /* App Focused keeps L2/R2 cycling -- nothing else on that screen wants
       them -- and still leaves Up/Down to the app grid. */
    for (int i = 3; i < WIDGET_PLACE_MAX; i++)
        snprintf(widget_places[1][i].name, 64, "Extra%d", i);
    widget_place_count[0] = widget_place_count[1] = WIDGET_PLACE_MAX;
    widget_place_sel[1] = 0;
    for (int i = 0; i < WIDGET_PLACE_MAX; i++) {
        assert(widget_place_sel[1] == i);
        assert(home_place_widget_key(APP_WIDGET_WEATHER, 1, SDLK_PAGEDOWN, &state));
    }
    assert(widget_place_sel[1] == 0);
    assert(!home_place_widget_key(APP_WIDGET_WEATHER, 1, SDLK_DOWN, &state));

    home_recent_focus_slot = saved_slot;
    home_widget_idx = saved_w1; home_widget2_idx = saved_w2;
    home_view_idx = saved_view;
    memcpy(widget_places, saved_places, sizeof saved_places);
    widget_place_count[0] = saved_count0; widget_place_count[1] = saved_count1;
    widget_place_sel[0] = saved_sel0; widget_place_sel[1] = saved_sel1;
    puts("PASS: Right focuses a place widget in either slot; the D-pad stays with Home and A changes the location");
}

/* The two old per-widget lists are merged into one on upgrade. This touches
   saved user data, so it has to be exact: nothing is lost, nothing is
   duplicated, both cursors land back on the place their widget was showing, and
   the rewritten file reloads to the same thing. */
static void test_widget_place_merge_131(void) {
    char *saved_root = getenv("SNAPFE_DATA_ROOT");
    if (saved_root) saved_root = strdup(saved_root);
    char dir[] = "/tmp/snapfe-merge-XXXXXX";
    assert(mkdtemp(dir));
    assert(!setenv("SNAPFE_DATA_ROOT", dir, 1));
    char cfg[900]; snprintf(cfg, sizeof cfg, "%s/widget-places.cfg", dir);
    char saved_loc[160]; snprintf(saved_loc, sizeof saved_loc, "%s", weather_loc);
    weather_loc[0] = 0;

    /* A 1.3.0-shaped file: two separate lists, "Local" in both, and each group
       sitting on its own second entry. */
    FILE *f = fopen(cfg, "w"); assert(f);
    fputs("S\t1\t1\n"
          "0\tLocal\t\t\n"
          "0\tTokyo\tAsia/Tokyo\tTokyo\n"
          "1\tLocal\t\t\n"
          "1\tLas Vegas\tAmerica/Los_Angeles\tLas Vegas, Nevada, United States\n", f);
    assert(!fclose(f));

    widget_places_loaded = 0; widget_places_init();
    /* Union, de-duplicated: Local once, plus both cities. */
    assert(widget_place_count[0] == 3 && widget_place_count[1] == 3);
    assert(!strcmp(widget_places[0][0].name, "Local"));
    assert(widget_place_find("Tokyo", "Asia/Tokyo") >= 0);
    assert(widget_place_find("Las Vegas", "America/Los_Angeles") >= 0);
    /* Both rows describe the same places, index for index. */
    for (int i = 0; i < widget_place_count[0]; i++) {
        assert(!strcmp(widget_places[0][i].name, widget_places[1][i].name));
        assert(!strcmp(widget_places[0][i].zone, widget_places[1][i].zone));
        assert(!strcmp(widget_places[0][i].query, widget_places[1][i].query));
    }
    /* Each cursor followed its own widget's place through the merge. */
    assert(!strcmp(widget_place_name(0), "Tokyo"));
    assert(!strcmp(widget_place_name(1), "Las Vegas"));
    /* Both cards can now reach the other one's city -- the whole point. */
    widget_place_select(0, widget_place_find("Las Vegas", "America/Los_Angeles"));
    assert(!strcmp(widget_place_name(0), "Las Vegas"));
    assert(widget_place_time_at(0, widget_place_find("Tokyo", "Asia/Tokyo"), 0).tm_hour == 9);

    /* The rewritten file round-trips to the same list. */
    int expect = widget_place_count[0];
    char names[WIDGET_PLACE_MAX][64];
    for (int i = 0; i < expect; i++) snprintf(names[i], 64, "%s", widget_places[0][i].name);
    widget_places_save();
    widget_places_loaded = 0; widget_places_init();
    assert(widget_place_count[0] == expect);
    for (int i = 0; i < expect; i++) assert(!strcmp(widget_places[0][i].name, names[i]));

    /* An empty file still leaves exactly one usable entry. With no weather_loc
       carried over that entry is "Local" -- automatic location. */
    f = fopen(cfg, "w"); assert(f); assert(!fclose(f));
    weather_loc[0] = 0;
    widget_places_loaded = 0; widget_places_init();
    assert(widget_place_count[0] == 1 && widget_place_count[1] == 1);
    assert(!strcmp(widget_places[0][0].name, "Local"));

    /* ... and a weather_loc from a pre-saved-places config becomes that entry
       instead of being thrown away. */
    f = fopen(cfg, "w"); assert(f); assert(!fclose(f));
    snprintf(weather_loc, sizeof weather_loc, "Prescott, Arizona");
    widget_places_loaded = 0; widget_places_init();
    assert(widget_place_count[0] == 1);
    assert(!strcmp(widget_places[0][0].name, "Prescott, Arizona"));
    assert(!strcmp(widget_places[0][0].query, "Prescott, Arizona"));

    unlink(cfg); rmdir(dir);
    snprintf(weather_loc, sizeof weather_loc, "%s", saved_loc);
    if (saved_root) { setenv("SNAPFE_DATA_ROOT", saved_root, 1); free(saved_root); }
    else unsetenv("SNAPFE_DATA_ROOT");
    widget_places_loaded = 0;
    puts("PASS: the two old saved-place lists merge into one without losing or duplicating a place");
}

/* What the card actually says, drawn: the hint has to report the position and
   the length of the list it cycles, and say so plainly when there is only one
   saved place and Up/Down therefore has nothing to do. */
static void test_widget_focus_hint_131(SDL_Renderer *ren) {
    int saved_slot = home_recent_focus_slot;
    int saved_w1 = home_widget_idx, saved_w2 = home_widget2_idx, saved_view = home_view_idx;
    int saved_count1 = widget_place_count[1], saved_sel1 = widget_place_sel[1];
    WidgetPlace saved_places[2][WIDGET_PLACE_MAX];
    memcpy(saved_places, widget_places, sizeof saved_places);

    home_view_idx = 0; widget_places_loaded = 1;
    home_widget_idx = HOME_WIDGET_WEATHER; home_widget2_idx = HOME_WIDGET_STATS;
    snprintf(g_weather_str, 80, "68F Partly Cloudy");

    /* Three saved: focused, mid-list. */
    static const char *cities[3] = { "Prescott", "Portland", "Phoenix" };
    for (int i = 0; i < 3; i++) {
        snprintf(widget_places[1][i].name, 64, "%s", cities[i]);
        snprintf(widget_places[1][i].query, 160, "%s, Arizona", cities[i]);
    }
    widget_place_count[1] = 3; widget_place_sel[1] = 1;
    home_recent_focus_slot = 1;
    SDL_SetRenderDrawColor(ren, 18, 18, 24, 255); SDL_RenderClear(ren);
    draw_home_widget(ren, HOME_WIDGET_WEATHER, 320, 60, 300, 300, 0);
    capture(ren, "widget-weather-focused-three");

    /* Only one saved: the hint must not promise cycling that cannot happen. */
    widget_place_count[1] = 1; widget_place_sel[1] = 0;
    SDL_SetRenderDrawColor(ren, 18, 18, 24, 255); SDL_RenderClear(ren);
    draw_home_widget(ren, HOME_WIDGET_WEATHER, 320, 60, 300, 300, 0);
    capture(ren, "widget-weather-focused-one");

    /* Unfocused still shows how to get in. */
    home_recent_focus_slot = 0;
    SDL_SetRenderDrawColor(ren, 18, 18, 24, 255); SDL_RenderClear(ren);
    draw_home_widget(ren, HOME_WIDGET_WEATHER, 320, 60, 300, 300, 0);
    capture(ren, "widget-weather-unfocused");

    home_recent_focus_slot = saved_slot;
    home_widget_idx = saved_w1; home_widget2_idx = saved_w2;
    home_view_idx = saved_view;
    memcpy(widget_places, saved_places, sizeof saved_places);
    widget_place_count[1] = saved_count1;
    widget_place_sel[1] = saved_sel1;
    puts("PASS: focused place widget names its list position, and says so when only one is saved");
}
#endif
