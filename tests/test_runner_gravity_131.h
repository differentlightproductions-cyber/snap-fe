#ifndef SNAP_TEST_RUNNER_GRAVITY_131_H
#define SNAP_TEST_RUNNER_GRAVITY_131_H
/* Pulse Runner's gravity flip was a free toggle you never had to use. Later
   stages now contain chasms wider than a jump can cover, and the ceiling is
   unbroken, so crossing them means flipping up and running across the roof. */

static void test_runner_gravity_131(void) {
    int saved_level = mgx_run.level, saved_inv = mgx_run.inverted;

    /* Stages one to five stay jumpable end to end -- the flip is not even
       offered there, so nothing may be impossible. */
    for (int level = 0; level < 5; level++) {
        const MgxRunLevel *l = &mgx_run_levels[level];
        for (int i = 0; i < l->n; i++)
            assert(!mgx_run_chasm(&l->o[i], level));
    }

    /* Every stage that DOES offer the flip has at least one gap that cannot be
       jumped, so the mechanic is required rather than decorative. */
    for (int level = 5; level < 10; level++) {
        const MgxRunLevel *l = &mgx_run_levels[level];
        int forced = 0;
        for (int i = 0; i < l->n; i++) if (mgx_run_chasm(&l->o[i], level)) forced++;
        assert(forced >= 1);
    }

    /* The span a jump covers is derived from the real physics, not guessed. */
    for (int level = 0; level < 10; level++) {
        float span = mgx_run_jump_span(level);
        assert(span > 100.0f && span < 400.0f);
    }

    /* Upright, a chasm is a hole you fall through. Inverted, the ceiling is
       solid over the same span -- that is what makes the crossing possible. */
    for (int level = 5; level < 10; level++) {
        const MgxRunLevel *l = &mgx_run_levels[level];
        mgx_run.level = level;
        for (int i = 0; i < l->n; i++) {
            if (!mgx_run_chasm(&l->o[i], level)) continue;
            float mid = l->o[i].x + l->o[i].w / 2.0f;
            mgx_run.inverted = 0; assert(mgx_run_in_gap(mid));
            mgx_run.inverted = 1; assert(!mgx_run_in_gap(mid));
        }
    }

    /* Objects must still fit inside the stage they belong to, or the wrapped
       copies would overlap the originals and stack hazards on top of hazards. */
    for (int level = 0; level < 10; level++) {
        const MgxRunLevel *l = &mgx_run_levels[level];
        for (int i = 0; i < l->n; i++) {
            int w = l->o[i].type == 0 ? 30 : l->o[i].w;
            assert(l->o[i].x >= 0 && l->o[i].x + w <= l->length);
        }
    }

    mgx_run.level = saved_level; mgx_run.inverted = saved_inv;
    puts("PASS: Pulse Runner's later stages need the gravity flip, and the ceiling is solid where the floor is not");
}
#endif
