static void test_systems_dropdown_capture(SDL_Renderer *ren, const int *rows, int first, int end,
                                         const char *name) {
    draw_theme_background(ren, &themes[theme_idx], theme_idx);
    int y = 92;
    for (int i = first; i < end; ++i) {
        char label[100] = ""; int indent = 0;
        assert(format_systems_view_row(rows[i], label, sizeof label, &indent));
        if (i == first) {
            SDL_Color c = themes[theme_idx].select_bg;
            SDL_SetRenderDrawColor(ren, c.r, c.g, c.b, 255);
            SDL_RenderFillRect(ren, &(SDL_Rect){40, y - 4, WIN_W - 80, 42});
        }
        SDL_Texture *text = render_text_fit(ren, font_label, label,
            i == first ? themes[theme_idx].accent2 : g_ui_text, WIN_W - 120 - indent * 20);
        assert(text);
        int w, h; SDL_QueryTexture(text, NULL, NULL, &w, &h);
        SDL_RenderCopy(ren, text, NULL, &(SDL_Rect){60 + indent * 20, y, w, h});
        y += 48;
    }
    capture(ren, name);
}

static void test_systems_dropdown(SDL_Renderer *ren) {
    int old_layout = platform_view_style, old_open = disp_grp_view_open;
    int old_favorites = favorites_view_idx;
    int rows[MAX_DISPLAY_ROWS], extra[MAX_DISPLAY_ROWS];
    int game[MAX_GAME_ROWS], game_extra[MAX_GAME_ROWS];
    int old_home = home_view_idx, old_home_open = disp_grp_home_open;
    int old_widgets_open = disp_grp_widgets_open, old_stats_open = disp_grp_stats_open;
    int old_widget1 = home_widget_idx, old_widget2 = home_widget2_idx;
    int old_stat_groups[STAT_GRP_COUNT]; memcpy(old_stat_groups, stat_grp_open, sizeof old_stat_groups);
    const int removed_widget_rows[] = { ROW_DISP_GRP_WIDGETS, ROW_DISP_HOME_WIDGET,
        ROW_DISP_HOME_WIDGET2, ROW_DISP_APP_WIDGET, ROW_DISP_WEATHER_UNIT,
        ROW_DISP_GRP_STATS, ROW_DISP_STAT_GRP, ROW_DISP_STAT_ITEM };
    for (int home = 0; home < HOME_VIEW_COUNT; ++home)
        for (int flags = 0; flags < 8; ++flags)
            for (int widget1 = 0; widget1 < HOME_WIDGET_COUNT; ++widget1)
                for (int widget2 = 0; widget2 < HOME_WIDGET_COUNT; ++widget2) {
                    home_view_idx = home;
                    disp_grp_home_open = !!(flags & 1);
                    disp_grp_widgets_open = !!(flags & 2);
                    disp_grp_stats_open = !!(flags & 4);
                    for (int g = 0; g < STAT_GRP_COUNT; ++g) stat_grp_open[g] = 1;
                    home_widget_idx = widget1; home_widget2_idx = widget2;
                    int n = build_display_rows(rows, extra);
                    assert(n <= MAX_DISPLAY_ROWS);
                    for (int i = 0; i < n; ++i)
                        for (size_t r = 0; r < sizeof removed_widget_rows / sizeof removed_widget_rows[0]; ++r)
                            assert(rows[i] != removed_widget_rows[r]);
                    // Hiding settings does not reset either in-place widget selection.
                    assert(home_widget_idx == widget1 && home_widget2_idx == widget2);
                }
    home_view_idx = old_home; disp_grp_home_open = old_home_open;
    disp_grp_widgets_open = old_widgets_open; disp_grp_stats_open = old_stats_open;
    home_widget_idx = old_widget1; home_widget2_idx = old_widget2;
    memcpy(stat_grp_open, old_stat_groups, sizeof old_stat_groups);
    for (int layout = 0; layout < VIEW_STYLE_COUNT; ++layout) {
        platform_view_style = layout;
        for (int open = 0; open <= 1; ++open) {
            disp_grp_view_open = open;
            int n = build_display_rows(rows, extra), first = -1, end = -1, headers = 0;
            assert(n <= MAX_DISPLAY_ROWS);
            for (int i = 0; i < n; ++i) {
                if (rows[i] == ROW_DISP_GRP_VIEW) { ++headers; first = i; }
                if (rows[i] == ROW_DISP_GRP_LIBVIEW) end = i;
            }
            assert(headers == 1 && first >= 0 && end > first);
            assert(!disp_row_is_value(rows[first])); // A opens; Left/Right retain layout selection.
            if (!open) assert(end == first + 1);
            else {
                assert(rows[first + 1] == ROW_DISP_SHOW_EMPTY);
                assert(rows[first + 2] == ROW_DISP_FAVORITES_VIEW);
                assert(rows[end - 1] == ROW_DISP_RST_VIEW);
            }
            char previous[100] = "";
            for (int i = first; i < end; ++i) {
                // Seed with the header to reproduce the old stale stack-text failure.
                char label[100] = "> Systems View: List"; int indent = -1;
                assert(format_systems_view_row(rows[i], label, sizeof label, &indent));
                assert(label[0] && strcmp(label, previous));
                if (i == first) assert(strstr(label, "Systems View:") && indent == 0);
                else assert(!strstr(label, "Systems View") && indent == 1);
                snprintf(previous, sizeof previous, "%s", label);
            }
            // Presentation has one owner, Display; Game cannot add a second group.
            int gn = build_game_rows(game, game_extra);
            assert(gn <= MAX_GAME_ROWS);
            for (int i = 0; i < gn; ++i)
                assert(game[i] != ROW_G_VIEW_HEADER && game[i] != ROW_G_SHOW_EMPTY &&
                       game[i] != ROW_G_FAVORITES_VIEW && game[i] != ROW_G_VIEW_RESTORE);
            if (layout == 2 && open)
                test_systems_dropdown_capture(ren, rows, first, end, "systems-dropdown-grid");
            if (layout == 3)
                test_systems_dropdown_capture(ren, rows, first, end,
                    open ? "systems-dropdown-list" : "systems-dropdown-collapsed");
        }
    }
    for (int view = 0; view < FAVORITES_VIEW_COUNT; ++view) {
        favorites_view_idx = view;
        char label[100] = "Systems View"; int indent = -1;
        assert(format_systems_view_row(ROW_DISP_FAVORITES_VIEW, label, sizeof label, &indent));
        assert(!strncmp(label, "Favorites View: ", 16) && strstr(label, favorite_view_names[view]));
    }
    platform_view_style = old_layout; disp_grp_view_open = old_open; favorites_view_idx = old_favorites;
    puts("PASS: one Systems View header, Show Systems Without Games first, distinct rendered Favorites row, all five layouts, no duplicate group in Game settings; both home layouts omit all widget settings rows while retaining widget selections");
}
