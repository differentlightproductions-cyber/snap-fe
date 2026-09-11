#ifndef SNAP_TEST_FISH_CAMPAIGN_131_H
#define SNAP_TEST_FISH_CAMPAIGN_131_H
/* Crazy Fish rebuild, part two: the campaign is chapters of four to six levels
   rather than a level counter that only goes up, a bonus round lands every
   second level, and shells are a currency that outlives the run. */

/* Clear the current level the way the game does -- three egg pieces, then the
   hatch timer runs out. */
static void fish_campaign_clear_level(void) {
    mgx_fish.coins = 100000;
    mgx_fish_buy_egg(); mgx_fish_buy_egg(); mgx_fish_buy_egg();
    assert(mgx_fish.hatch_pending);
    mgx_fish.hatch_time = 0.01f;
    mgx_fish.last = SDL_GetTicks() - 20; mgx_fish_step(0, 0);
}
static void fish_campaign_run(float secs) {
    for (float t = 0; t < secs; t += .05f) { mgx_fish.last = SDL_GetTicks() - 50; mgx_fish_step(0, 0); }
}

static void test_fish_campaign_131(SDL_Renderer *ren) {
    /* --- Chapters are 4 to 6 levels, and the stage/chapter/level maths agree
       with each other at every boundary. --- */
    for (int c = 1; c <= MGX_FISH_CHAPTERS; c++) {
        int n = mgx_fish_chapter_levels(c);
        assert(n >= 4 && n <= 6);
    }
    assert(mgx_fish_chapter_of(1) == 1 && mgx_fish_level_of(1) == 1);
    int stage = 1;
    for (int c = 1; c <= MGX_FISH_CHAPTERS + 2; c++) {
        int n = mgx_fish_chapter_levels(c);
        for (int l = 1; l <= n; l++, stage++) {
            assert(mgx_fish_chapter_of(stage) == c);
            assert(mgx_fish_level_of(stage) == l);
            assert(mgx_fish_is_chapter_end(stage) == (l == n));
        }
        assert(mgx_fish_chapter_first_stage(c + 1) == stage);
    }
    /* Past the named chapters the reef keeps going rather than running off the
       end of the table. */
    assert(mgx_fish_chapter_levels(MGX_FISH_CHAPTERS + 5) == MGX_FISH_ENDLESS_LEVELS);
    assert(!strcmp(mgx_fish_chapter_title(MGX_FISH_CHAPTERS + 5), "ENDLESS REEF"));
    assert(strcmp(mgx_fish_chapter_title(1), "ENDLESS REEF") != 0);

    /* --- A bonus round is never more than two levels away. --- */
    for (int s = 1; s <= 40; s++) {
        int gap = 0;
        for (int t = s; t <= s + 2 && !mgx_fish_bonus_after(t); t++) gap++;
        assert(gap <= 2);
    }
    /* Every chapter ends on one, so a chapter never rolls over without one. */
    for (int c = 1; c <= MGX_FISH_CHAPTERS; c++) {
        int last = mgx_fish_chapter_first_stage(c) + mgx_fish_chapter_levels(c) - 1;
        assert(mgx_fish_bonus_after(last));
    }

    /* --- Clearing level 2 queues a bonus round, which runs BEFORE level 3. --- */
    fish_feedback_fresh();
    mgx_fish.stage = 2; mgx_fish.helpers = 0;
    assert(!mgx_fish.bonus && !mgx_fish.bonus_pending);
    fish_campaign_clear_level();
    assert(mgx_fish.stage == 3 && mgx_fish.bonus_pending && mgx_fish.between == 2);
    mgx_fish_key(SDLK_RETURN);                    /* dismiss the hatch reveal */
    assert(mgx_fish.bonus && !mgx_fish.bonus_pending && !mgx_fish.picking);

    /* Nothing hunts during it, and treasure falls on its own. */
    int monsters_before = mgx_fish_monsters_alive();
    assert(monsters_before == 0);
    mgx_fish.clock = mgx_fish.next_spawn + 50;    /* the invasion clock is due */
    fish_campaign_run(3.0f);
    assert(mgx_fish_monsters_alive() == 0);       /* and still nothing spawned */
    int falling = 0;
    for (int i = 0; i < MGX_FISH_DROPS; i++)
        if (mgx_fish.drops[i].active && mgx_fish.drops[i].kind != MGX_RES_WASTE) falling++;
    assert(falling > 0);

    /* Hatching cannot cut the round short. */
    int stage_now = mgx_fish.stage;
    mgx_fish.coins = 100000; mgx_fish_buy_egg();
    assert(mgx_fish.egg_pieces == 0 && mgx_fish.stage == stage_now);
    assert(strstr(mgx_fish.notice, "bonus round"));

    /* --- Shells: caught during the round, kept afterwards. --- */
    int shells_before = mgx_fish.shells;
    mgx_fish_drop_add(mgx_fish.x, mgx_fish.y, 12, MGX_RES_SHELL, 0);
    fish_campaign_run(0.2f);
    assert(mgx_fish.shells == shells_before + 1);
    /* A clam's pearl is treasure, not currency -- one name, one meaning. */
    int shells_held = mgx_fish.shells, coins_held = mgx_fish.coins;
    mgx_fish_drop_add(mgx_fish.x, mgx_fish.y, 9, MGX_RES_PEARL, 0);
    fish_campaign_run(0.2f);
    assert(mgx_fish.shells == shells_held && mgx_fish.coins > coins_held);

    mgx_fish_render(ren); capture(ren, "fish-bonus-round");

    /* The round ends on its own and hands over to the next level. */
    mgx_fish.bonus_time = 0.02f;
    fish_campaign_run(0.2f);
    assert(!mgx_fish.bonus);
    assert(mgx_fish.picking || mgx_fish.intro > 0);   /* level 3 is starting */

    /* --- Finishing a chapter pays a shell purse that grows with the depth. --- */
    fish_feedback_fresh();
    mgx_fish.stage = mgx_fish_chapter_levels(1);      /* last level of chapter 1 */
    assert(mgx_fish_is_chapter_end(mgx_fish.stage));
    int purse_before = mgx_fish.shells;
    fish_campaign_clear_level();
    assert(mgx_fish.shells > purse_before);
    assert(mgx_fish_chapter_of(mgx_fish.stage) == 2 && mgx_fish_level_of(mgx_fish.stage) == 1);
    int purse_ch1 = mgx_fish.shells - purse_before;
    fish_feedback_fresh();
    mgx_fish.stage = mgx_fish_chapter_first_stage(4) + mgx_fish_chapter_levels(4) - 1;
    int deep_before = mgx_fish.shells;
    fish_campaign_clear_level();
    assert(mgx_fish.shells - deep_before > purse_ch1);

    /* --- Shells survive the run; coins do not. --- */
    mgx_fish.shells = 27;
    mgx_fish_save_helpers();
    mgx_fish_reset();
    assert(mgx_fish.shells == 27 && mgx_fish.coins == 70);
    mgx_fish.shells = 0; mgx_fish_save_helpers();

    /* --- Where you had got to is saved as a number of its own. It used to be
       derived from the hatch count, which is a different thing that happens to
       have moved in step, and which stopped at the hatch cap. --- */
    {
        char path[768];
        snprintf(path, sizeof path, "%s/fish-helpers.cfg", sn_data_root());
        mgx_fish.stage = 23; mgx_fish.helpers = 4; mgx_fish.shells = 5; mgx_fish.rares = 0;
        mgx_fish_save_helpers();
        mgx_fish_reset();
        assert(mgx_fish.stage == 23 && mgx_fish.helpers == 4);
        /* Deep enough that the old derivation could not have reached it. */
        assert(mgx_fish_chapter_of(23) > 4);

        /* A file written before that line existed keeps its place rather than
           being reset to level one. */
        FILE *old = fopen(path, "w");
        assert(old); fprintf(old, "6\n"); fclose(old);
        mgx_fish_reset();
        assert(mgx_fish.helpers == 6 && mgx_fish.stage == 7);
        assert(mgx_fish.shells == 0 && mgx_fish.rares == 0);

        remove(path);
        mgx_fish_reset();
        assert(mgx_fish.stage == 1 && mgx_fish.helpers == 0);
    }

    /* --- Chapters end on a boss from the third on, and nothing in the endless
       chapters outruns the ray. --- */
    {
        fish_feedback_fresh();
        mgx_fish.stage = mgx_fish_chapter_first_stage(1);      /* chapter 1, level 1 */
        mgx_fish_spawn_monster();
        assert(mgx_fish_monsters_alive() == 1 && !mgx_fish.monster[0].boss);

        fish_feedback_fresh();
        mgx_fish.stage = mgx_fish_chapter_first_stage(4) + mgx_fish_chapter_levels(4) - 1;
        mgx_fish_spawn_monster();
        assert(mgx_fish.monster[0].boss);
        int hp_ch4 = mgx_fish.monster[0].max_hp;

        /* Mid-chapter is never a boss, however deep. */
        fish_feedback_fresh();
        mgx_fish.stage = mgx_fish_chapter_first_stage(6);
        mgx_fish_spawn_monster();
        assert(!mgx_fish.monster[0].boss);
        int hp_mid = mgx_fish.monster[0].max_hp;

        /* Deeper chapters bring bigger bosses, and an ordinary monster far out
           in the endless reef is no tougher than one at the end of the table --
           the curve is capped, not unbounded. */
        fish_feedback_fresh();
        mgx_fish.stage = mgx_fish_chapter_first_stage(9) + mgx_fish_chapter_levels(9) - 1;
        mgx_fish_spawn_monster();
        assert(mgx_fish.monster[0].boss && mgx_fish.monster[0].max_hp > hp_ch4);

        fish_feedback_fresh();
        mgx_fish.stage = mgx_fish_chapter_first_stage(60);   /* deep, mid-chapter */
        assert(!mgx_fish_is_chapter_end(mgx_fish.stage));
        mgx_fish_spawn_monster();
        assert(!mgx_fish.monster[0].boss && mgx_fish.monster[0].max_hp == hp_mid);
        /* Even a boss out there is a fight, not a wall. */
        fish_feedback_fresh();
        mgx_fish.stage = mgx_fish_chapter_first_stage(60) + MGX_FISH_ENDLESS_LEVELS - 1;
        mgx_fish_spawn_monster();
        assert(mgx_fish.monster[0].boss && mgx_fish.monster[0].max_hp < 120);
    }

    puts("PASS: the reef is chapters of 4-6 levels, a bonus round lands at least every second level, and shells outlive the run");
}
#endif
