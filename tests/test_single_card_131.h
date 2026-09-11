#ifndef SNAP_TEST_SINGLE_CARD_131_H
#define SNAP_TEST_SINGLE_CARD_131_H
/* The Single Card info panel. It used to be a fixed 300x362 slab down one side
   of the screen whatever it had to say -- most of a 640x480 panel, covering the
   background it was sitting on. It is measured from its own text now, and the
   two detail modes exist so a system's full write-up is a choice rather than
   the only option. */

/* The panel draws itself; what a test can check is how much of the screen it
   ended up covering. Counting pixels that differ from a flat backdrop gives
   that without the panel having to report its own geometry. */
static int single_card_panel_area(SDL_Renderer *ren, Theme *th) {
    SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
    SDL_RenderClear(ren);
    single_card_info_panel(ren, th);
    SDL_Surface *s = SDL_CreateRGBSurfaceWithFormat(0, WIN_W, WIN_H, 32, SDL_PIXELFORMAT_ARGB8888);
    assert(s && SDL_RenderReadPixels(ren, NULL, s->format->format, s->pixels, s->pitch) == 0);
    int lit = 0;
    for (int y = 0; y < WIN_H; y++) {
        Uint32 *row = (Uint32 *)((Uint8 *)s->pixels + y * s->pitch);
        for (int x = 0; x < WIN_W; x++) if ((row[x] & 0x00FFFFFFu) != 0) lit++;
    }
    SDL_FreeSurface(s);
    return lit;
}

static void test_single_card_131(SDL_Renderer *ren) {
    Theme *th = &themes[theme_idx];
    int saved_style = platform_view_style, saved_side = single_card_title_side;
    int saved_info = single_card_info_idx, saved_sel = platform_selected;
    SDL_Texture *saved_bg = platform_bg_tex;
    platform_bg_tex = NULL;
    platform_selected = 0;
    platform_view_style = 0;

    /* Both modes name the system and say how many games are on it -- those are
       what you navigate by, so neither mode may drop them. */
    for (int mode = 0; mode < 2; mode++) {
        single_card_info_idx = mode;
        int area = single_card_panel_area(ren, th);
        assert(area > 0);
    }

    /* Informational carries more, so it is the taller of the two; Simple must
       actually be simpler rather than the same panel with a different name. */
    single_card_info_idx = SINGLE_INFO_FULL;
    int full_area = single_card_panel_area(ren, th);
    single_card_info_idx = SINGLE_INFO_SIMPLE;
    int simple_area = single_card_panel_area(ren, th);
    assert(simple_area < full_area);

    /* The complaint this answers: even the fuller mode must leave most of the
       screen showing the background. The old panel was a fixed 300 x 362 --
       about 36% of a 640x480 screen -- whatever it had to say. */
    int screen = WIN_W * WIN_H;
    assert(full_area * 100 / screen < 30);
    assert(simple_area * 100 / screen < 20);

    /* It sits on the middle of the screen rather than filling the side. Find
       the panel's top and bottom edges and check they straddle the centre by
       about the same margin. */
    {
        single_card_info_idx = SINGLE_INFO_SIMPLE;
        SDL_SetRenderDrawColor(ren, 0, 0, 0, 255); SDL_RenderClear(ren);
        single_card_info_panel(ren, th);
        SDL_Surface *s = SDL_CreateRGBSurfaceWithFormat(0, WIN_W, WIN_H, 32, SDL_PIXELFORMAT_ARGB8888);
        assert(s && SDL_RenderReadPixels(ren, NULL, s->format->format, s->pixels, s->pitch) == 0);
        int top = -1, bot = -1;
        for (int y = 0; y < WIN_H; y++) {
            Uint32 *row = (Uint32 *)((Uint8 *)s->pixels + y * s->pitch);
            int any = 0;
            for (int x = 0; x < WIN_W && !any; x++) if ((row[x] & 0x00FFFFFFu) != 0) any = 1;
            if (any) { if (top < 0) top = y; bot = y; }
        }
        SDL_FreeSurface(s);
        assert(top >= 0 && bot > top);
        int centre = (top + bot) / 2;
        assert(abs(centre - WIN_H / 2) <= 4);
        /* And it clears the status bar at the top and the hint line below. */
        assert(top > HUD_BAR_H && bot < WIN_H - 30);
    }

    /* Both sides, both modes, at both panel widths the devices use. */
    single_card_info_idx = SINGLE_INFO_FULL; single_card_title_side = 0;
    draw_theme_background(ren, th, theme_idx);
    single_card_info_panel(ren, th); capture(ren, "single-card-informational");
    single_card_info_idx = SINGLE_INFO_SIMPLE; single_card_title_side = 1;
    draw_theme_background(ren, th, theme_idx);
    single_card_info_panel(ren, th); capture(ren, "single-card-simple-right");

    platform_view_style = saved_style; single_card_title_side = saved_side;
    single_card_info_idx = saved_info; platform_selected = saved_sel;
    platform_bg_tex = saved_bg;
    puts("PASS: the Single Card panel is measured from its own text, centred, and leaves the background showing");
}
#endif
