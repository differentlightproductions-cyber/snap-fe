#ifndef SNAP_TEST_FISH_REEF_131_H
#define SNAP_TEST_FISH_REEF_131_H
/* Crazy Fish rebuild, part three: the rare reef is what shells are for, it is
   kept forever, and the fish you buy turn up where you can see them -- in the
   tank, on the sleep screen, and behind the interface. */

static void test_fish_reef_131(SDL_Renderer *ren) {
    /* --- The reef is priced in ascending order, so "the next one" is always
       the cheapest one you do not own. --- */
    for (int r = 1; r < MGX_RARE_COUNT; r++)
        assert(mgx_rare_fish[r].shells > mgx_rare_fish[r - 1].shells);
    assert(mgx_rare_next(0) == 0);
    assert(mgx_rare_next(1 << 0) == 1);
    assert(mgx_rare_next((1 << MGX_RARE_COUNT) - 1) == -1);
    assert(mgx_rare_owned_count(0) == 0);
    assert(mgx_rare_owned_count((1 << MGX_RARE_COUNT) - 1) == MGX_RARE_COUNT);

    /* --- The prism fish is the RGB one: its colour moves, everything else's
       is fixed. --- */
    {
        SDL_Color a = mgx_rare_color(MGX_RARE_PRISM, 0);
        SDL_Color b = mgx_rare_color(MGX_RARE_PRISM, 2000);
        SDL_Color c = mgx_rare_color(MGX_RARE_PRISM, 4000);
        assert(a.r != b.r || a.g != b.g || a.b != b.b);
        assert(b.r != c.r || b.g != c.g || b.b != c.b);
        /* It stays a real colour all the way round rather than washing out. */
        for (Uint32 t = 0; t < 6000; t += 250) {
            SDL_Color k = mgx_rare_color(MGX_RARE_PRISM, t);
            int hi = k.r > k.g ? (k.r > k.b ? k.r : k.b) : (k.g > k.b ? k.g : k.b);
            int lo = k.r < k.g ? (k.r < k.b ? k.r : k.b) : (k.g < k.b ? k.g : k.b);
            assert(hi >= 200 && hi - lo >= 100 && k.a == 255);
        }
        SDL_Color koi_a = mgx_rare_color(MGX_RARE_KOI, 0), koi_b = mgx_rare_color(MGX_RARE_KOI, 3100);
        assert(koi_a.r == koi_b.r && koi_a.g == koi_b.g && koi_a.b == koi_b.b);
    }

    /* --- Buying: shells only, never coins, and once each. --- */
    fish_feedback_fresh();
    mgx_fish.rares = 0; mgx_fish.shells = 0; mgx_fish.coins = 999999;
    fish_shop_buy(MGX_SHOP_RARE);
    assert(mgx_fish.rares == 0 && mgx_fish.coins == 999999);   /* coins buy nothing here */
    assert(strstr(mgx_fish.notice, "shells"));

    mgx_fish.shells = mgx_rare_fish[MGX_RARE_KOI].shells;
    fish_shop_buy(MGX_SHOP_RARE);
    assert(mgx_rare_owned(mgx_fish.rares, MGX_RARE_KOI) && mgx_fish.shells == 0);
    /* The shop moves on to the next one rather than re-selling the same fish. */
    assert(mgx_rare_next(mgx_fish.rares) == MGX_RARE_LANTERN);

    mgx_fish.shells = 100000;
    for (int r = 1; r < MGX_RARE_COUNT; r++) fish_shop_buy(MGX_SHOP_RARE);
    assert(mgx_rare_owned_count(mgx_fish.rares) == MGX_RARE_COUNT);
    int spent = mgx_fish.shells;
    fish_shop_buy(MGX_SHOP_RARE);
    assert(mgx_fish.shells == spent && strstr(mgx_fish.notice, "complete"));

    mgx_fish.shop_open = 1; mgx_fish.shop_row = MGX_SHOP_RARE;
    mgx_fish_render(ren); capture(ren, "fish-shop-rare-reef");
    mgx_fish.shop_open = 0;
    mgx_fish_render(ren); capture(ren, "fish-tank-with-rares");

    /* --- The reef survives a wipe, the way shells do and coins do not. --- */
    int owned = mgx_fish.rares;
    mgx_fish_save_helpers();
    mgx_fish_reset();
    assert(mgx_fish.rares == owned);

    /* --- The sleep screen and the wallpaper read the same saved reef. --- */
    assert(saver_tank_rares() == owned);
    {
        int saved_saver = screen_saver_idx, saved_wall = aquarium_wallpaper_enabled;
        screen_saver_idx = SCREEN_SAVER_AQUARIUM;
        screen_saver_begin();
        int rare_in_tank = 0;
        for (int i = 0; i < SAVER_FISH_COUNT; i++) if (saver_fish[i].rare) rare_in_tank++;
        assert(rare_in_tank == MGX_RARE_COUNT);
        draw_screen_saver(ren); capture(ren, "sleep-aquarium-your-reef");

        /* The wallpaper is off unless asked for, never over a game, and stocks
           the same fish when it is on. */
        aquarium_wallpaper_enabled = 0;
        assert(!tank_wallpaper_allowed(STATE_HOME));
        aquarium_wallpaper_enabled = 1;
        assert(tank_wallpaper_allowed(STATE_HOME));
        assert(!tank_wallpaper_allowed(STATE_MENU));
        assert(!tank_wallpaper_allowed(STATE_MINIGAME));
        assert(!tank_wallpaper_allowed(STATE_BOOT));
        tank_wallpaper_render(ren, STATE_HOME);
        int rare_on_wall = 0;
        for (int i = 0; i < TANK_WALL_COUNT; i++) if (tank_wall_fish[i].rare) rare_on_wall++;
        assert(rare_on_wall > 0);
        capture(ren, "wallpaper-aquarium");

        /* Leaving for a page it is not allowed on drops the school, so it comes
           back stocked from whatever the reef is by then. */
        tank_wallpaper_render(ren, STATE_MINIGAME);
        assert(!tank_wall_ready);

        aquarium_wallpaper_enabled = saved_wall;
        screen_saver_idx = saved_saver; screen_saver_begin();
    }

    /* Leave no reef behind for the tests that follow. */
    mgx_fish.rares = 0; mgx_fish.shells = 0; mgx_fish_save_helpers();
    puts("PASS: the rare reef is bought with shells, kept forever, and shows up in the tank, the sleep screen and the wallpaper");
}
#endif
