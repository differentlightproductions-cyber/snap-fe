#ifndef SNAP_TEST_FISH_PAUSE_131_H
#define SNAP_TEST_FISH_PAUSE_131_H
/* START used to fall straight through to the frontend's Settings from inside
   the tank -- interrupting a live level with a menu belonging to something
   else, and leaving by a route that never offered to save. START is the game's
   own menu now: it starts the game, then pauses it. */

static void test_fish_pause_131(SDL_Renderer *ren) {
    /* --- Before the first level, START is a start button. --- */
    mgx_fish_reset();
    assert(!mgx_fish.started);
    mgx_fish_key(SDLK_F1);
    assert(mgx_fish.started && !mgx_fish.paused);

    /* --- In play it pauses, and the pause really holds the simulation. --- */
    fish_feedback_fresh();
    mgx_fish.clock = 12.0f;
    mgx_fish_key(SDLK_F1);
    assert(mgx_fish.paused && mgx_fish.pause_row == MGX_PAUSE_RESUME);
    float clock_held = mgx_fish.clock;
    for (int i = 0; i < 20; i++) { mgx_fish.last = SDL_GetTicks() - 50; mgx_fish_step(0, 0); }
    assert(mgx_fish.clock == clock_held);
    mgx_fish_render(ren); capture(ren, "fish-pause-menu");

    /* START and B both put you back in the tank. */
    mgx_fish_key(SDLK_F1); assert(!mgx_fish.paused);
    mgx_fish_key(SDLK_F1); assert(mgx_fish.paused);
    mgx_fish_key(SDLK_ESCAPE); assert(!mgx_fish.paused);

    /* --- B in play opens the menu rather than leaving, so exiting always
       passes an offer to save. --- */
    assert(!mgx_fish.want_exit);
    mgx_fish_key(SDLK_ESCAPE);
    assert(mgx_fish.paused && !mgx_fish.want_exit);

    /* One press never means two things: an overlay closes first. */
    mgx_fish_key(SDLK_ESCAPE); assert(!mgx_fish.paused);
    mgx_fish_key(SDLK_e); assert(mgx_fish.shop_open);
    mgx_fish_key(SDLK_F1); assert(!mgx_fish.shop_open && !mgx_fish.paused);
    mgx_fish_key(SDLK_f); assert(mgx_fish.collection);
    mgx_fish_key(SDLK_F1); assert(!mgx_fish.collection && !mgx_fish.paused);

    /* --- Save writes the campaign without leaving. --- */
    fish_feedback_fresh();
    mgx_fish.stage = 9; mgx_fish.shells = 12; mgx_fish.rares = 1 << MGX_RARE_KOI;
    mgx_fish_key(SDLK_F1);
    while (mgx_fish.pause_row != MGX_PAUSE_SAVE) mgx_fish_key(SDLK_DOWN);
    mgx_fish_key(SDLK_RETURN);
    assert(mgx_fish.paused && !mgx_fish.want_exit);
    assert(strstr(mgx_fish.notice, "saved"));
    {   /* and it really reached the disk */
        int stage = mgx_fish.stage, shells = mgx_fish.shells, rares = mgx_fish.rares;
        mgx_fish_reset();
        assert(mgx_fish.stage == stage && mgx_fish.shells == shells && mgx_fish.rares == rares);
    }

    /* --- Restart replays the level and keeps everything you earned. --- */
    fish_feedback_fresh();
    mgx_fish.stage = 9; mgx_fish.shells = 12; mgx_fish.rares = 1 << MGX_RARE_KOI;
    mgx_fish.food_tier = 2; mgx_fish.gun_level = 3; mgx_fish.clock = 88.0f;
    mgx_fish_key(SDLK_F1);
    while (mgx_fish.pause_row != MGX_PAUSE_RESTART) mgx_fish_key(SDLK_DOWN);
    mgx_fish_key(SDLK_RETURN);
    assert(!mgx_fish.paused && !mgx_fish.want_exit);
    assert(mgx_fish.stage == 9 && mgx_fish.shells == 12);
    assert(mgx_fish.rares == (1 << MGX_RARE_KOI));
    assert(mgx_fish.food_tier == 2 && mgx_fish.gun_level == 3);
    assert(mgx_fish.clock == 0.0f && mgx_fish.started);

    /* --- Reset is the only destructive row, so it asks twice. --- */
    fish_feedback_fresh();
    mgx_fish.stage = 9; mgx_fish.shells = 12; mgx_fish.rares = 1 << MGX_RARE_KOI;
    mgx_fish_save_helpers();
    mgx_fish_key(SDLK_F1);
    while (mgx_fish.pause_row != MGX_PAUSE_RESET) mgx_fish_key(SDLK_DOWN);
    mgx_fish_key(SDLK_RETURN);
    assert(mgx_fish.pause_confirm && mgx_fish.stage == 9);   /* nothing lost yet */
    mgx_fish_render(ren); capture(ren, "fish-pause-reset-confirm");
    /* Moving off the row takes the confirmation back. */
    mgx_fish_key(SDLK_UP); assert(!mgx_fish.pause_confirm);
    mgx_fish_key(SDLK_DOWN); mgx_fish_key(SDLK_RETURN); assert(mgx_fish.pause_confirm);
    mgx_fish_key(SDLK_RETURN);
    assert(mgx_fish.stage == 1 && mgx_fish.shells == 0 && mgx_fish.rares == 0);
    assert(!mgx_fish.paused && mgx_fish.started);

    /* --- The two exits differ in exactly one way: whether they save first. --- */
    fish_feedback_fresh();
    mgx_fish.stage = 5; mgx_fish.shells = 44; mgx_fish_save_helpers();
    mgx_fish.shells = 99;                        /* unsaved since */
    mgx_fish_key(SDLK_F1);
    while (mgx_fish.pause_row != MGX_PAUSE_EXIT) mgx_fish_key(SDLK_DOWN);
    mgx_fish_key(SDLK_RETURN);
    assert(mgx_fish.want_exit && !mgx_fish.paused);
    mgx_fish.want_exit = 0;
    mgx_fish_reset();
    assert(mgx_fish.shells == 44);               /* the 99 was never written */

    fish_feedback_fresh();
    mgx_fish.stage = 5; mgx_fish.shells = 99;
    mgx_fish_key(SDLK_F1);
    while (mgx_fish.pause_row != MGX_PAUSE_EXIT_SAVE) mgx_fish_key(SDLK_DOWN);
    mgx_fish_key(SDLK_RETURN);
    assert(mgx_fish.want_exit && !mgx_fish.paused);
    mgx_fish.want_exit = 0;
    mgx_fish_reset();
    assert(mgx_fish.shells == 99);

    /* --- An empty aquarium can still reach the menu, or nothing could be done
       about it but start over. --- */
    fish_feedback_fresh();
    mgx_fish.over = 1;
    mgx_fish_key(SDLK_F1);
    assert(mgx_fish.paused);

    mgx_fish.paused = 0; mgx_fish.over = 0;
    mgx_fish.shells = 0; mgx_fish.rares = 0; mgx_fish.stage = 1; mgx_fish_save_helpers();
    puts("PASS: START is Crazy Fish's own pause menu -- it starts the game, holds it, saves, restarts, resets behind a confirm, and owns the way out");
}
#endif
