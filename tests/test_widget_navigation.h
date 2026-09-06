/* Drive the same translated gamepad keys used by Home. Directional events
   fall through from the location handler into the real spatial grid. */
static void widget_test_pad_press(AppState *state, int kind, int app_view,
                                  SDL_Keycode key, int *rows, int row_count) {
    SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);
    pad_push_key(key);
    SDL_Event event; int downs = 0, ups = 0;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_KEYDOWN) {
            int handled = !app_view && home_info_widget_cycle_key(event.key.keysym.sym);
            if (!handled && !home_place_widget_key(kind, app_view, event.key.keysym.sym, state) && app_view)
                home_selected = hgrid_nav_layout(rows, row_count, home_selected, event.key.keysym.sym);
            downs++;
        } else if (event.type == SDL_KEYUP) ups++;
    }
    assert(downs == 1 && ups == 1);
}

static void widget_test_shoulder_focus(void) {
    int old_view = home_view_idx, old_top = home_widget_idx, old_bottom = home_widget2_idx;
    int old_focus = home_recent_focus_slot, old_selected = home_selected, old_recent = home_recent_widget_sel;
    int old_activity_count = activity_record_count;
    ActivityRecord saved_activity[2]; memcpy(saved_activity, activity_records, sizeof saved_activity);
    AppState state = STATE_HOME;
    home_view_idx = HOME_VIEW_MINIMAL; home_selected = 7; // retained left-navigation selection
    SDL_Keycode r2 = pad_map_joybutton(pad_idx(PADK_R2, 14));
    SDL_Keycode l2 = pad_map_joybutton(pad_idx(PADK_L2, 13));
    SDL_Keycode r1 = pad_map_joybutton(pad_idx(PADK_R1, 8));
    home_widget_idx = HOME_WIDGET_WEATHER; home_widget2_idx = HOME_WIDGET_CLOCK; home_recent_focus_slot = 1;
    widget_test_pad_press(&state, HOME_WIDGET_WEATHER, 0, r2, NULL, 0);
    assert(home_widget_idx == HOME_WIDGET_WEATHER && home_widget2_idx == HOME_WIDGET_DATE);
    assert(home_recent_focus_slot == 1 && home_selected == 7 && state == STATE_HOME);
    widget_test_pad_press(&state, HOME_WIDGET_WEATHER, 0, l2, NULL, 0);
    assert(home_widget2_idx == HOME_WIDGET_CLOCK && home_recent_focus_slot == 1 && home_selected == 7);
    home_widget_idx = HOME_WIDGET_CLOCK; home_widget2_idx = HOME_WIDGET_WEATHER; home_recent_focus_slot = 2;
    widget_test_pad_press(&state, HOME_WIDGET_WEATHER, 0, r1, NULL, 0);
    assert(home_widget_idx == HOME_WIDGET_DATE && home_widget2_idx == HOME_WIDGET_WEATHER && home_recent_focus_slot == 2);
    for (int slot = 1; slot <= 2; slot++) {
        home_widget_idx = slot == 1 ? HOME_WIDGET_CLOCK : HOME_WIDGET_STATS;
        home_widget2_idx = slot == 2 ? HOME_WIDGET_CLOCK : HOME_WIDGET_STATS;
        home_recent_focus_slot = slot;
        widget_test_pad_press(&state, HOME_WIDGET_CLOCK, 0, slot == 1 ? r1 : r2, NULL, 0);
        assert(home_recent_focus_slot == slot); // Date remains interactive.
        widget_test_pad_press(&state, HOME_WIDGET_DATE, 0, slot == 1 ? r1 : r2, NULL, 0);
        assert(home_recent_focus_slot == 0 && home_selected == 7); // Now Playing does not accept focus.
    }
    home_widget_idx = HOME_WIDGET_CLOCK; home_widget2_idx = HOME_WIDGET_STATS; home_recent_focus_slot = 0;
    widget_test_pad_press(&state, HOME_WIDGET_CLOCK, 0, r1, NULL, 0);
    assert(!home_recent_focus_slot && home_selected == 7); // Cycling never steals focus from the menu.
    memset(activity_records, 0, sizeof saved_activity); activity_record_count = 2;
    activity_records[0].last_played = 100; activity_records[1].last_played = 200;
    home_widget2_idx = HOME_WIDGET_RADIO; home_recent_focus_slot = 2; home_recent_widget_sel = 95;
    widget_test_pad_press(&state, HOME_WIDGET_RADIO, 0, r2, NULL, 0);
    assert(home_widget2_idx == HOME_WIDGET_RECENT && home_recent_focus_slot == 2 && home_recent_widget_sel == 0);
    activity_record_count = 0; home_widget2_idx = HOME_WIDGET_RADIO; home_recent_focus_slot = 2;
    widget_test_pad_press(&state, HOME_WIDGET_RADIO, 0, r2, NULL, 0);
    assert(home_widget2_idx == HOME_WIDGET_RECENT && !home_recent_focus_slot);
    assert(!home_info_widget_cycle_key(SDLK_s) && !home_info_widget_cycle_key(SDLK_f));
    home_view_idx = HOME_VIEW_APPS;
    assert(!home_info_widget_cycle_key(r2)); // App Focused keeps its own zone/page controls.
    memcpy(activity_records, saved_activity, sizeof saved_activity); activity_record_count = old_activity_count;
    home_view_idx = old_view; home_widget_idx = old_top; home_widget2_idx = old_bottom;
    home_recent_focus_slot = old_focus; home_selected = old_selected; home_recent_widget_sel = old_recent;
}

static void widget_test_focus_screens(SDL_Renderer *ren) {
    TTF_Font *saved_big = font_big, *saved_small = font_small, *saved_label = font_label;
    TTF_Font *saved_bold = font_small_bold, *saved_label_bold = font_label_bold, *saved_fixed = font_fixed;
    int old_theme = theme_idx, old_focus = home_recent_focus_slot;
    static const struct { int family, size, theme; const char *name; } presets[] = {
        {5, 1, 0, "widget-focus-sans-medium"}, {5, 3, 0, "widget-focus-sans-xlarge"},
        {6, 1, 2, "widget-focus-terminal-dark"}, {8, 1, 0, "widget-focus-retro-medium"}
    };
    for (int i = 0; i < (int)(sizeof presets / sizeof presets[0]); i++) {
        int family = presets[i].family; float scale = font_size_scale[presets[i].size]; char path[256];
        snprintf(path, sizeof path, "assets/fonts/%s", font_choice_files[family]);
        font_big = TTF_OpenFont(path, (int)(font_choice_big_size[family] * scale));
        font_small = TTF_OpenFont(path, (int)(font_choice_small_size[family] * scale));
        font_label = TTF_OpenFont(path, (int)(font_choice_label_size[family] * scale));
        font_small_bold = font_small; font_label_bold = font_label;
        font_fixed = TTF_OpenFont(path, 14);
        assert(font_big && font_small && font_label && font_fixed);
        theme_idx = presets[i].theme; g_ui_text = themes[theme_idx].text; g_ui_dim = themes[theme_idx].dim;
        g_weather_kicked = SDL_GetTicks() ? SDL_GetTicks() : 1;
        snprintf(g_weather_str, sizeof g_weather_str, "72F Sunny"); g_weather_at = time(NULL);
        draw_theme_background(ren, &themes[theme_idx], theme_idx);
        home_recent_focus_slot = 1;
        draw_home_widget(ren, HOME_WIDGET_CLOCK, 340, 66, 255, 278, 0);
        draw_home_widget(ren, HOME_WIDGET_WEATHER, 340, 267, 460, 278, 1);
        capture(ren, presets[i].name);
        home_recent_focus_slot = 2;
        draw_home_widget(ren, HOME_WIDGET_CLOCK, 340, 66, 255, 278, 0);
        draw_home_widget(ren, HOME_WIDGET_DATEWX, 340, 267, 460, 278, 1);
        char name[100]; snprintf(name, sizeof name, "%s-combined", presets[i].name); capture(ren, name);
        draw_app_grid_widget(ren, &themes[theme_idx], (SDL_Rect){24, 72, 280, 132}, 1);
        snprintf(name, sizeof name, "%s-app", presets[i].name); capture(ren, name);
        text_cache_clear(); TTF_CloseFont(font_big); TTF_CloseFont(font_small); TTF_CloseFont(font_label); TTF_CloseFont(font_fixed);
    }
    font_big = saved_big; font_small = saved_small; font_label = saved_label;
    font_small_bold = saved_bold; font_label_bold = saved_label_bold; font_fixed = saved_fixed;
    theme_idx = old_theme; g_ui_text = themes[theme_idx].text; g_ui_dim = themes[theme_idx].dim;
    home_recent_focus_slot = old_focus;
}

static void test_widget_navigation(SDL_Renderer *ren) {
    int old_view = home_view_idx, old_kind = app_widget_kind, old_slot = app_widget_slot, old_selected = home_selected;
    int old_focus = home_recent_focus_slot, old_theme = theme_idx;
    int saved_order[HOME_ORDER_COUNT]; memcpy(saved_order, home_tile_order, sizeof saved_order);
    for (int i = 0; i < HOME_ORDER_COUNT; i++) home_tile_order[i] = i;
    int rows[] = {ROW_H_QUICK_CONSOLES, ROW_H_QUICK_LIBRARY, ROW_H_QUICK_FAVORITES,
        ROW_H_QUICK_RETROARCH, ROW_H_QUICK_LINK, ROW_H_QUICK_RADIO, ROW_H_QUICK_MUSIC,
        ROW_H_QUICK_SETTINGS, ROW_H_QUICK_FLASHLIGHT, ROW_H_QUICK_MINIGAMES,
        ROW_H_QUICK_ACHIEVEMENTS, ROW_H_QUICK_CALCULATOR, ROW_H_APP_WIDGET};
    int count = sizeof rows / sizeof rows[0], widget_row = count - 1;
    AppState state = STATE_HOME;
    home_view_idx = HOME_VIEW_APPS;
    widget_places_init();
    for (int group = 0; group < 2; group++) {
        widget_place_count[group] = 3; widget_place_sel[group] = 0;
        strcpy(widget_places[group][1].name, "Tokyo"); strcpy(widget_places[group][1].zone, "Asia/Tokyo");
        strcpy(widget_places[group][2].name, "London"); strcpy(widget_places[group][2].zone, "Europe/London");
        app_widget_kind = group ? APP_WIDGET_WEATHER : APP_WIDGET_CLOCK;
        /* Every valid two-cell position can leave in each physically available
           direction. Neither Up nor Down is allowed to change the city. */
        for (int slot = 0; slot <= count - 1; slot++) {
            app_widget_slot = hgrid_widget_slot_normalize(slot);
            HGridItem items[MAX_HOME_ROWS]; int total = 0;
            int n = hgrid_layout(rows, count, items, &total), widget_cell = -1;
            for (int i = 0; i < n; i++) if (items[i].row_index == widget_row) widget_cell = items[i].slot;
            assert(widget_cell >= 0);
            SDL_Keycode dirs[] = {SDLK_UP, SDLK_DOWN, SDLK_LEFT, SDLK_RIGHT};
            for (int k = 0; k < 4; k++) {
                home_selected = widget_row;
                int expected = hgrid_nav_layout(rows, count, home_selected, dirs[k]);
                widget_test_pad_press(&state, app_widget_kind, 1, dirs[k], rows, count);
                assert(home_selected == expected && widget_place_sel[group] == 0 && state == STATE_HOME);
                if (k < 2) {
                    int neighbor = widget_cell + (k == 0 ? -HOME_GRID_COLS : HOME_GRID_COLS);
                    if (neighbor >= 0 && hgrid_layout_row_at(items, n, neighbor) >= 0) assert(home_selected != widget_row);
                }
            }
        }
        home_selected = widget_row;
        widget_test_pad_press(&state, app_widget_kind, 1, pad_map_joybutton(pad_idx(PADK_R2,14)), rows, count);
        assert(home_selected == widget_row && widget_place_sel[group] == 1 && state == STATE_HOME);
        widget_test_pad_press(&state, app_widget_kind, 1, pad_map_joybutton(pad_idx(PADK_L2,13)), rows, count);
        assert(widget_place_sel[group] == 0);
        widget_test_pad_press(&state, app_widget_kind, 1, SDLK_RETURN, rows, count);
        assert(state == STATE_WIDGET_PLACES && widget_place_group == group);
        state = STATE_HOME;
        assert(!home_place_widget_key(app_widget_kind, 1, SDLK_s, &state));
        widget_test_pad_press(&state, app_widget_kind, 1, SDLK_s, rows, count);
        assert(state == STATE_HOME); // X remains available to the in-place widget picker.
    }
    int info_kinds[] = {HOME_WIDGET_CLOCK, HOME_WIDGET_DATE, HOME_WIDGET_WEATHER, HOME_WIDGET_DATEWX};
    for (int i = 0; i < 4; i++) {
        int kind = info_kinds[i], group = widget_place_family(kind);
        widget_place_sel[group] = 0;
        widget_test_pad_press(&state, kind, 0, SDLK_DOWN, rows, count); assert(widget_place_sel[group] == 1);
        widget_test_pad_press(&state, kind, 0, SDLK_UP, rows, count); assert(widget_place_sel[group] == 0);
        assert(!home_place_widget_key(kind, 0, SDLK_LEFT, &state));
        assert(!home_place_widget_key(kind, 0, SDLK_PAGEUP, &state));
        assert(!home_place_widget_key(kind, 0, SDLK_s, &state));
        assert(!home_place_widget_key(kind, 0, SDLK_f, &state));
        widget_test_pad_press(&state, kind, 0, SDLK_RETURN, rows, count);
        assert(state == STATE_WIDGET_PLACES && widget_place_group == group); state = STATE_HOME;
    }
    widget_test_shoulder_focus();
    /* Selecting an informational card must change a substantial, bounded
       area, not merely a tiny text hint that is easy to miss. */
    SDL_Surface *before = SDL_CreateRGBSurfaceWithFormat(0, WIN_W, WIN_H, 32, SDL_PIXELFORMAT_ARGB8888);
    SDL_Surface *after = SDL_CreateRGBSurfaceWithFormat(0, WIN_W, WIN_H, 32, SDL_PIXELFORMAT_ARGB8888);
    assert(before && after);
    theme_idx = 0; g_ui_text = themes[0].text; g_ui_dim = themes[0].dim;
    home_recent_focus_slot = 0; draw_theme_background(ren, &themes[theme_idx], theme_idx);
    draw_home_widget(ren, HOME_WIDGET_CLOCK, 340, 66, 255, 278, 0);
    assert(SDL_RenderReadPixels(ren, NULL, before->format->format, before->pixels, before->pitch) == 0);
    home_recent_focus_slot = 1; draw_theme_background(ren, &themes[theme_idx], theme_idx);
    draw_home_widget(ren, HOME_WIDGET_CLOCK, 340, 66, 255, 278, 0);
    assert(SDL_RenderReadPixels(ren, NULL, after->format->format, after->pixels, after->pitch) == 0);
    int changed = 0;
    for (int y = 0; y < WIN_H; y++) for (int x = 0; x < WIN_W; x++) {
        Uint32 a = *(Uint32*)((Uint8*)before->pixels + y * before->pitch + x * 4);
        Uint32 b = *(Uint32*)((Uint8*)after->pixels + y * after->pitch + x * 4);
        if (a != b) { changed++; assert(x >= 332 && x < 626 && y >= 60 && y < 260); }
    }
    assert(changed > 2000); SDL_FreeSurface(before); SDL_FreeSurface(after);
    app_widget_kind = APP_WIDGET_CLOCK; home_grid_reorder = app_widget_moving = 0;
    widget_test_focus_screens(ren);
    memcpy(home_tile_order, saved_order, sizeof saved_order);
    home_view_idx = old_view; app_widget_kind = old_kind; app_widget_slot = old_slot; home_selected = old_selected;
    home_recent_focus_slot = old_focus; theme_idx = old_theme; g_ui_text = themes[old_theme].text; g_ui_dim = themes[old_theme].dim;
    puts("PASS: widget gamepad navigation at every two-cell position, A-only locations, App L2/R2 zones, shoulder cycling preserves the correct focus, informational focus screens");
}
