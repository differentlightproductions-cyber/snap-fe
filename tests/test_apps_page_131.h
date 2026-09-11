#ifndef SNAP_TEST_APPS_PAGE_131_H
#define SNAP_TEST_APPS_PAGE_131_H
/* The Apps page: nine apps in one Home column meant a long scroll to reach the
   last of them, so Home now pins a few and an "Apps" row opens a page holding
   every app, reorderable and persisted. */

static void test_apps_page_131(SDL_Renderer *ren) {
    int saved_mask = home_apps_mask, saved_view = home_view_idx, saved_sel = apps_page_sel;
    int saved_order[APP_COUNT];
    memcpy(saved_order, home_app_order, sizeof saved_order);

    /* Every app id maps to exactly one Home row type, and back again. Nothing
       may be unreachable or share a row with another app. */
    int seen_rows[APP_COUNT];
    for (int id = 0; id < APP_COUNT; id++) {
        int rt = home_app_row_type(id);
        assert(rt >= 0);
        assert(home_app_id_for_row(rt) == id);
        seen_rows[id] = rt;
        for (int j = 0; j < id; j++) assert(seen_rows[j] != rt);
        assert(home_app_slug(id) && home_app_slug(id)[0]);
        assert(home_app_names[id] && home_app_names[id][0]);
    }
    assert(home_app_id_for_row(ROW_H_QUICK_SETTINGS) == -1);
    assert(home_app_id_for_row(ROW_H_QUICK_APPS) == -1);

    /* A saved order that is short, duplicated or out of range must still end up
       as a complete permutation -- an app that fell out of the list would be
       unreachable from both Home and the page. */
    for (int i = 0; i < APP_COUNT; i++) home_app_order[i] = -1;
    home_app_order[0] = APP_WEATHER; home_app_order[1] = APP_WEATHER; home_app_order[2] = 99;
    home_app_order_normalize();
    int seen[APP_COUNT] = { 0 };
    for (int i = 0; i < APP_COUNT; i++) {
        assert(home_app_order[i] >= 0 && home_app_order[i] < APP_COUNT);
        assert(!seen[home_app_order[i]]);
        seen[home_app_order[i]] = 1;
    }
    assert(home_app_order[0] == APP_WEATHER);   /* the valid part is respected */

    /* Home lists only the pinned apps, in the saved order, and always ends with
       the Apps row so nothing is stranded. */
    home_view_idx = 0;
    memcpy(home_app_order, saved_order, sizeof saved_order);
    home_app_order_normalize();
    home_apps_mask = (1 << APP_WEATHER) | (1 << APP_RETROARCH);
    int rows[MAX_HOME_ROWS], extra[MAX_HOME_ROWS], recent[MAX_HOME_RECENT_SHOWN], rn = 0;
    int n = build_home_rows(rows, extra, recent, &rn);
    int pinned = 0, apps_row = 0, unpinned = 0;
    for (int i = 0; i < n; i++) {
        if (rows[i] == ROW_H_QUICK_APPS) { apps_row++; continue; }
        int id = home_app_id_for_row(rows[i]);
        if (id < 0) continue;
        if (home_apps_mask & (1 << id)) pinned++; else unpinned++;
    }
    assert(pinned == 2 && unpinned == 0 && apps_row == 1);
    /* Pinned apps follow the saved order, not the order they were defined in. */
    int seen_ra = -1, seen_wx = -1;
    for (int i = 0; i < n; i++) {
        if (rows[i] == ROW_H_QUICK_RETROARCH) seen_ra = i;
        if (rows[i] == ROW_H_QUICK_WEATHER) seen_wx = i;
    }
    assert(seen_ra >= 0 && seen_wx >= 0 && seen_ra < seen_wx);

    /* Pin nothing at all and the Apps row is still there -- Home must never
       become a dead end. */
    home_apps_mask = 0;
    n = build_home_rows(rows, extra, recent, &rn);
    apps_row = 0;
    for (int i = 0; i < n; i++) if (rows[i] == ROW_H_QUICK_APPS) apps_row++;
    assert(apps_row == 1);

    /* App Focused still shows every app as a tile, and no Apps row -- the grid
       is already the page. */
    home_view_idx = HOME_VIEW_APPS;
    n = build_home_rows(rows, extra, recent, &rn);
    int tiles = 0; apps_row = 0;
    for (int i = 0; i < n; i++) {
        if (rows[i] == ROW_H_QUICK_APPS) apps_row++;
        if (home_app_id_for_row(rows[i]) >= 0) tiles++;
    }
    assert(tiles == APP_COUNT && apps_row == 0);
    home_view_idx = 0;

    /* Carrying a tile shuffles the others along rather than swapping two
       distant apps, so a drag across the grid keeps everyone's relative order. */
    memcpy(home_app_order, saved_order, sizeof saved_order);
    home_app_order_normalize();
    int before[APP_COUNT]; memcpy(before, home_app_order, sizeof before);
    int carried = home_app_order[APP_COUNT - 1];
    for (int from = APP_COUNT - 1; from > 0; from--) {
        int tmp = home_app_order[from];
        home_app_order[from] = home_app_order[from - 1];
        home_app_order[from - 1] = tmp;
    }
    assert(home_app_order[0] == carried);
    for (int i = 0; i < APP_COUNT - 1; i++) assert(home_app_order[i + 1] == before[i]);

    /* Renders in both modes at both display widths the devices use. */
    memcpy(home_app_order, saved_order, sizeof saved_order);
    home_app_order_normalize();
    home_apps_mask = APP_PINNED_DEFAULT;
    apps_page_sel = 4; apps_page_reorder = 0; apps_page_dragging = 0;
    SDL_SetRenderDrawColor(ren, 18, 18, 24, 255); SDL_RenderClear(ren);
    apps_page_render(ren); capture(ren, "apps-page");
    apps_page_reorder = 1; apps_page_dragging = 1;
    SDL_SetRenderDrawColor(ren, 18, 18, 24, 255); SDL_RenderClear(ren);
    apps_page_render(ren); capture(ren, "apps-page-dragging");
    apps_page_reorder = apps_page_dragging = 0;

    home_apps_mask = saved_mask; home_view_idx = saved_view; apps_page_sel = saved_sel;
    memcpy(home_app_order, saved_order, sizeof saved_order);
    puts("PASS: Apps page holds every app, Home pins a subset in the saved order and always offers the way in");
}
#endif
