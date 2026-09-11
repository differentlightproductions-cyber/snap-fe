#ifndef SNAP_TEST_SETTINGS_PAGES_131_H
#define SNAP_TEST_SETTINGS_PAGES_131_H
/* Settings used to expand its groups in place. Every group is its own page
   now, built by differencing the tab builder rather than by hand -- so the
   thing worth testing is not any one page's contents but that the mechanism
   holds for all of them: every group is reachable, opens onto something,
   gives its rows back, and puts the cursor where it found it. */

static int sp_rows_rt[MAX_SETTINGS_ROWS], sp_rows_rx[MAX_SETTINGS_ROWS];

static int sp_build(int tab) { return settings_build_rows(tab, sp_rows_rt, sp_rows_rx); }

static int sp_find(int n, int rt, int rx, int indexed) {
    for (int i = 0; i < n; i++)
        if (sp_rows_rt[i] == rt && (!indexed || sp_rows_rx[i] == rx)) return i;
    return -1;
}
static int sp_has(int n, int rt) { return sp_find(n, rt, 0, 0) >= 0; }

/* Walk to a group the way a person would: look for it on the tab, and if it is
   not there, step into each group on the tab and look again. Nothing in SNAP
   FE is deeper than that, and the assertion below is what says so. */
static int sp_open_group(int tab, const SettingsGroup *g, int extra, int *sel, int *scroll) {
    int n = sp_build(tab);
    if (sp_find(n, g->row, extra, g->indexed) >= 0)
        return settings_group_enter(tab, g->row, extra, sel, scroll);
    for (int i = 0; i < SETTINGS_GROUP_COUNT; i++) {
        if (settings_groups[i].tab != tab) continue;
        if (&settings_groups[i] == g) continue;
        n = sp_build(tab);
        if (sp_find(n, settings_groups[i].row, 0, 0) < 0) continue;
        if (!settings_group_enter(tab, settings_groups[i].row, 0, sel, scroll)) continue;
        n = sp_build(tab);
        if (sp_find(n, g->row, extra, g->indexed) >= 0 &&
            settings_group_enter(tab, g->row, extra, sel, scroll))
            return 1;
        settings_group_leave(tab, sel, scroll);
    }
    return 0;
}

static void test_settings_pages_131(SDL_Renderer *ren) {
    (void)ren;
    int saved_tab = current_tab, saved_sel = settings_selected, saved_scroll = settings_scroll_offset;

    /* --- every group opens onto something, and closes back up ------------ */
    for (int i = 0; i < SETTINGS_GROUP_COUNT; i++) {
        const SettingsGroup *g = &settings_groups[i];
        settings_close_all_groups();
        int extra = 0;
        int sel = 3, scroll = 1;                  /* a cursor worth restoring */
        current_tab = g->tab;

        int opened = sp_open_group(g->tab, g, extra, &sel, &scroll);
        /* Some groups only have rows under certain values -- Library View only
           when the layout is Grid. Those must refuse to open rather than push
           an empty page; anything else is a group that cannot be reached. */
        if (!opened) {
            assert(settings_group_body(g->tab, g, extra) <= 0);
            assert(sgp_depth == 0);
            continue;
        }
        assert(sgp_depth >= 1 && sgp_depth <= 2);   /* never deeper than two */

        int n = sp_build(g->tab);
        assert(n > 0);                              /* a page is never blank */
        assert(g->title && g->title[0]);            /* and always says where you are */
        const char *heading = settings_page_heading();
        assert(heading && strcmp(heading, g->title) == 0);

        /* The page shows the group's body and not the header you pressed. */
        assert(sp_find(n, g->row, extra, g->indexed) < 0);
        assert(n == settings_group_body(g->tab, g, extra));

        while (sgp_depth > 0) settings_group_leave(g->tab, &sel, &scroll);
        assert(settings_page_heading() == NULL);
        assert(sel == 3 && scroll == 1);            /* left you where you were */
    }

    /* --- Restore to Default is the last row of every tab ------------------ */
    settings_close_all_groups();
    for (int tab = 0; tab < TAB_COUNT; tab++) {
        int n = sp_build(tab);
        assert(n > 0);
        assert(settings_row_is_restore(tab, sp_rows_rt[n - 1]));
        /* and nothing else on the tab claims to be one */
        for (int i = 0; i < n - 1; i++) assert(!settings_row_is_restore(tab, sp_rows_rt[i]));
    }

    /* --- the Device tab is headed by the handheld, which is not a row ----- */
    {
        current_tab = TAB_DEVICE;
        int n = sp_build(TAB_DEVICE);
        assert(sp_rows_rt[0] == ROW_DEV_HEADER);
        assert(!settings_row_selectable(TAB_DEVICE, ROW_DEV_HEADER));
        /* Arriving at the tab, and stepping off either end of the list, all
           have to move past it rather than sit on it. */
        int sel = 0;
        settings_skip_unselectable(TAB_DEVICE, sp_rows_rt, n, &sel, 1);
        assert(sel == 1);
        sel = (1 - 1 + n) % n;                       /* Up from the first row */
        settings_skip_unselectable(TAB_DEVICE, sp_rows_rt, n, &sel, -1);
        assert(sel == n - 1);                        /* wraps to the bottom */
        for (int i = 1; i < n; i++) assert(settings_row_selectable(TAB_DEVICE, sp_rows_rt[i]));
    }

    /* --- what moved actually moved --------------------------------------- */
    {
        settings_close_all_groups();
        /* Syncthing is an online service; it left the Device tab entirely. */
        int dev = sp_build(TAB_DEVICE);
        assert(!sp_has(dev, ROW_DEV_SYNCTHING) && !sp_has(dev, ROW_DEV_SYNCTHING_INFO));
        assert(!sp_has(dev, ROW_ONL_SYNC));
        int onl = sp_build(TAB_ACCOUNT);
        assert(sp_has(onl, ROW_ONL_GRP_SYNC));

        /* Fast Forward and Auto-Save sit together, off the Game tab's face. */
        int game = sp_build(TAB_GAME);
        assert(sp_has(game, ROW_G_GRP_PLAY));
        assert(!sp_has(game, ROW_G_FASTFORWARD) && !sp_has(game, ROW_G_AUTOSAVE));
        int sel = 0, scroll = 0;
        current_tab = TAB_GAME;
        assert(settings_group_enter(TAB_GAME, ROW_G_GRP_PLAY, 0, &sel, &scroll));
        int play = sp_build(TAB_GAME);
        assert(sp_has(play, ROW_G_FASTFORWARD) && sp_has(play, ROW_G_AUTOSAVE));
        settings_group_leave(TAB_GAME, &sel, &scroll);

        /* The device profile is still changeable, one page in under System. */
        current_tab = TAB_DEVICE;
        sel = 0; scroll = 0;
        assert(settings_group_enter(TAB_DEVICE, ROW_DEV_GRP_SYSTEM, 0, &sel, &scroll));
        int sys = sp_build(TAB_DEVICE);
        assert(sp_has(sys, ROW_DEV_DEVICE));
        assert(sp_has(sys, ROW_DEV_EXIT_ES) && sp_has(sys, ROW_DEV_RESET));
        settings_group_leave(TAB_DEVICE, &sel, &scroll);
    }

    /* --- Single Card Detail belongs to Single Card ------------------------ */
    {
        settings_close_all_groups();
        int keep = platform_view_style;
        current_tab = TAB_DISPLAY;
        for (int style = 0; style < VIEW_STYLE_COUNT; style++) {
            platform_view_style = style;
            int sel = 0, scroll = 0;
            sgp_depth = 0;
            assert(settings_group_enter(TAB_DISPLAY, ROW_DISP_GRP_VIEW, 0, &sel, &scroll));
            int n = sp_build(TAB_DISPLAY);
            /* It was being emitted for every layout, because the two rows it
               was added beside were never braced under the Single Card test. */
            assert(sp_has(n, ROW_DISP_SINGLE_CARD_INFO) == (style == 0));
            settings_group_leave(TAB_DISPLAY, &sel, &scroll);
        }
        platform_view_style = keep;
    }

    /* --- Presentation leads with the three layout choices ---------------- */
    {
        settings_close_all_groups();
        current_tab = TAB_DISPLAY;
        int sel = 0, scroll = 0;
        assert(settings_group_enter(TAB_DISPLAY, ROW_DISP_GRP_VIEW, 0, &sel, &scroll));
        int n = sp_build(TAB_DISPLAY);
        assert(n >= 4);
        assert(sp_rows_rt[0] == ROW_DISP_SYS_VIEW && sp_rows_rt[1] == ROW_DISP_LIB_VIEW &&
               sp_rows_rt[2] == ROW_DISP_ART_VALUE);
        for (int i = 0; i < 3; i++) assert(settings_row_cycles(TAB_DISPLAY, sp_rows_rt[i]));
        settings_group_leave(TAB_DISPLAY, &sel, &scroll);
        /* The parent row only opens the page; it no longer changes the view. */
        assert(!settings_row_cycles(TAB_DISPLAY, ROW_DISP_GRP_VIEW));
        int root = sp_build(TAB_DISPLAY);
        assert(!sp_has(root, ROW_DISP_GRP_LIBVIEW) && !sp_has(root, ROW_DISP_ART_HEADER));
    }

    /* --- Home carries no second copy of the Apps page's pin list ---------- */
    {
        settings_close_all_groups();
        int sel = 0, scroll = 0;
        assert(settings_group_enter(TAB_DISPLAY, ROW_DISP_GRP_HOME, 0, &sel, &scroll));
        int n = sp_build(TAB_DISPLAY);
        assert(!sp_has(n, ROW_DISP_GRP_APPS) && !sp_has(n, ROW_DISP_APP_ITEM));
        /* The first row is the layout, and it shows it can be changed. */
        assert(sp_rows_rt[0] == ROW_DISP_HOME_VIEW && settings_row_cycles(TAB_DISPLAY, ROW_DISP_HOME_VIEW));
        settings_group_leave(TAB_DISPLAY, &sel, &scroll);
    }

    /* --- Online is four pages, and none of them goes past two deep ------- */
    {
        settings_close_all_groups();
        current_tab = TAB_ACCOUNT;
        int n = sp_build(TAB_ACCOUNT);
        assert(n == 5);
        assert(sp_rows_rt[0] == ROW_SCRAPE_HEADER && sp_rows_rt[1] == ROW_RA_HEADER &&
               sp_rows_rt[2] == ROW_WALLHAVEN_HEADER && sp_rows_rt[3] == ROW_ONL_GRP_SYNC &&
               sp_rows_rt[4] == ROW_ACCT_RESTORE);
        int sel = 0, scroll = 0;
        assert(settings_group_enter(TAB_ACCOUNT, ROW_SCRAPE_HEADER, 0, &sel, &scroll));
        assert(settings_group_enter(TAB_ACCOUNT, ROW_SCRAPE_SYSTEM_HEADER, 0, &sel, &scroll));
        n = sp_build(TAB_ACCOUNT);
        int makers = 0, systems = 0;
        for (int i = 0; i < n; i++) {
            /* Consoles sit under maker headings the cursor steps over, not
               behind a third level of pages. */
            assert(settings_group_find(TAB_ACCOUNT, sp_rows_rt[i]) == NULL);
            if (sp_rows_rt[i] == ROW_SCRAPE_MAKER) { makers++; assert(!settings_row_selectable(TAB_ACCOUNT, ROW_SCRAPE_MAKER)); }
            if (sp_rows_rt[i] == ROW_SCRAPE_SYSTEM_ITEM) systems++;
        }
        assert(makers > 0 && systems == PLATFORM_COUNT);
        assert(sp_rows_rt[0] == ROW_SCRAPE_MAKER);
        sel = 0;
        settings_skip_unselectable(TAB_ACCOUNT, sp_rows_rt, n, &sel, 1);
        assert(sp_rows_rt[sel] == ROW_SCRAPE_SYSTEM_ITEM);   /* lands on a console */
        while (sgp_depth > 0) settings_group_leave(TAB_ACCOUNT, &sel, &scroll);
    }

    /* --- Restore to Default does nothing until it is confirmed ------------ */
    {
        int keep_style = hud_chrome_style, keep_color = hud_chrome_color_idx, keep_font = hud_font_color_idx;
        hud_chrome_style = 2;
        restore_confirm_open(RC_DISP_GROUP, ROW_DISP_RST_HUD);
        assert(hud_chrome_style == 2);                /* asking is not doing */
        restore_confirm_kind = RC_NONE;               /* B */
        assert(hud_chrome_style == 2);
        restore_confirm_open(RC_DISP_GROUP, ROW_DISP_RST_HUD);
        restore_confirm_accept();                     /* A */
        assert(hud_chrome_style == 0 && restore_confirm_kind == RC_NONE);
        hud_chrome_style = keep_style; hud_chrome_color_idx = keep_color; hud_font_color_idx = keep_font;
    }

    /* --- A edits a theme, and the hint says so only there ----------------- */
    assert(!disp_row_is_value(ROW_DISP_THEME) && !disp_row_is_value(ROW_DISP_CUSTOM_THEME));
    assert(settings_row_hint(TAB_DISPLAY, ROW_DISP_THEME) && strstr(settings_row_hint(TAB_DISPLAY, ROW_DISP_THEME), "A  "));
    assert(settings_row_hint(TAB_DISPLAY, ROW_DISP_CUSTOM_THEME));
    assert(!settings_row_hint(TAB_DISPLAY, ROW_DISP_FONT_SIZE));
    /* ...and both theme rows still show that Left/Right steps through them. */
    assert(settings_row_cycles(TAB_DISPLAY, ROW_DISP_THEME) && settings_row_cycles(TAB_DISPLAY, ROW_DISP_CUSTOM_THEME));

    settings_close_all_groups();
    current_tab = saved_tab; settings_selected = saved_sel; settings_scroll_offset = saved_scroll;
    puts("PASS: every settings group opens as its own page, never blank, never deeper than two, and hands the cursor back");
}
#endif
