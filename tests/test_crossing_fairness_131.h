#ifndef SNAP_TEST_CROSSING_FAIRNESS_131_H
#define SNAP_TEST_CROSSING_FAIRNESS_131_H
/* Pocket Crossing: nothing may move because the PLAYER moved. Hazards used to
   be positioned at a speed derived from how far the runner had got, so stepping
   over a difficulty threshold re-placed every car, log and train on the board
   at once -- which is how you die in a lane you had just checked and found
   empty. Row geometry is now a pure function of the row and the clock. */

static void test_crossing_fairness_131(void) {
    mgx_road_choosing = 0; mgx_road_reset();
    mgx_road.clock = 6.25f;

    /* --- The same row at the same moment, before and after the player has
       travelled a long way: identical. --- */
    for (int row = 1; row <= 60; row++) {
        mgx_road.farthest = 0;
        int count_near = mgx_road_vehicle_count(row);
        float speed_near = mgx_road_speed(row);
        float car_near[8], log_near[MGX_ROAD_LOG_COUNT];
        for (int i = 0; i < count_near && i < 8; i++) car_near[i] = mgx_road_object(row, i);
        for (int i = 0; i < MGX_ROAD_LOG_COUNT; i++) log_near[i] = mgx_road_log_x(row, i);
        int tw_near = 0; float train_near = mgx_road_train_x(row, &tw_near);
        int warn_near = mgx_road_train_warning(row);

        /* Everything the player could possibly have achieved. */
        for (int far = 1; far <= 400; far += 37) {
            mgx_road.farthest = far;
            assert(mgx_road_vehicle_count(row) == count_near);
            assert(mgx_road_speed(row) == speed_near);
            for (int i = 0; i < count_near && i < 8; i++)
                assert(mgx_road_object(row, i) == car_near[i]);
            for (int i = 0; i < MGX_ROAD_LOG_COUNT; i++)
                assert(mgx_road_log_x(row, i) == log_near[i]);
            int tw = 0;
            assert(mgx_road_train_x(row, &tw) == train_near && tw == tw_near);
            assert(mgx_road_train_warning(row) == warn_near);
        }
    }

    /* --- Difficulty still escalates; it is just the row that carries it. --- */
    mgx_road.farthest = 0;
    assert(mgx_road_row_difficulty(0, 20, 9) == 0);
    assert(mgx_road_row_difficulty(200, 20, 9) == 9);        /* clamped */
    assert(mgx_road_row_difficulty(60, 20, 9) > mgx_road_row_difficulty(10, 20, 9));
    assert(mgx_road_row_difficulty(-5, 20, 9) == 0);         /* never negative */
    {   /* A deep row really is faster and busier than a shallow one. */
        int busy = 0, calm = 0;
        float fast = 0, slow = 0;
        for (int r = 120; r < 130; r++) { busy += mgx_road_vehicle_count(r); fast += mgx_road_speed(r); }
        for (int r = 1; r < 11; r++)    { calm += mgx_road_vehicle_count(r);  slow += mgx_road_speed(r); }
        assert(busy > calm && fast > slow);
    }

    /* --- The crossing hazard's column belongs to the section it is in, and
       sections are four to eight rows long now rather than always six. Two rows
       of one section put the hazard in the same place; the phase differs per
       row, so the comparison has to be made when both are actually active. --- */
    {
        int checked = 0;
        for (int row = 1; row < 200 && checked < 4; row++) {
            int len = 0, local = 0;
            mgx_road_section_at(row, &local, &len);
            if (local != 0 || len < 3) continue;
            int centre[2] = {-1, -1};
            for (int which = 0; which < 2; which++) {
                int r = row + 1 + which * (len - 2);
                /* Walk the clock until this row's hazard is showing, then take
                   the middle of the band it kills in. */
                for (int tick = 0; tick < 900 && centre[which] < 0; tick++) {
                    mgx_road.clock = (float)tick * 0.05f;
                    int lo = -1, hi = -1;
                    for (int x = 0; x < WIN_W; x++)
                        if (mgx_road_intersection_hit(r, (float)x)) { if (lo < 0) lo = x; hi = x; }
                    if (lo >= 0) centre[which] = (lo + hi) / 2;
                }
            }
            if (centre[0] < 0 || centre[1] < 0) continue;
            assert(centre[0] == centre[1]);      /* one section, one column */
            checked++;
        }
        assert(checked > 0);
    }

    /* --- And the death itself is honest: if the step kills you in traffic,
       something is actually overlapping you. --- */
    mgx_road_reset(); mgx_road.started = 1; mgx_road.clock = 3.5f;
    int deaths = 0, checks = 0;
    for (int trial = 0; trial < 400; trial++) {
        mgx_road.clock += 0.05f;
        mgx_road.scroll_base = trial % 50;
        mgx_road.row = MGX_ROAD_ROWS / 2;
        int world_row = mgx_road_world_at(mgx_road.row);
        int kind = mgx_road_kind(world_row);
        if (kind != MGX_TRAFFIC && kind != MGX_NIGHT && kind != MGX_TUNNEL) continue;
        mgx_road.x = (float)(60 + (trial * 37) % (WIN_W - 120));
        mgx_road.dead = 0;
        mgx_road.last = SDL_GetTicks();
        mgx_road_step();
        checks++;
        if (!mgx_road.dead) continue;
        deaths++;
        int overlapped = 0, count = mgx_road_vehicle_count(world_row);
        for (int i = 0; i < count; i++) {
            float x = mgx_road_object(world_row, i);
            int w = mgx_road_car_width(world_row, i);
            if (mgx_road.x + 10 > x && mgx_road.x - 10 < x + w) { overlapped = 1; break; }
        }
        assert(overlapped);          /* never killed by a car that is not there */
    }
    assert(checks > 20 && deaths > 0);   /* the trial actually exercised traffic */

    mgx_road_reset();
    puts("PASS: Pocket Crossing hazards depend on the row, not the runner's progress, so nothing teleports onto a player who moved");
}
#endif
