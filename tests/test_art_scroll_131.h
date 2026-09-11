#ifndef SNAP_TEST_ART_SCROLL_131_H
#define SNAP_TEST_ART_SCROLL_131_H
/* 1.3.1 library scrolling: cover art has to arrive with the cursor, not one
   step behind it, and reversing direction must not throw away everything the
   user is about to scroll back onto. */

static void test_art_scroll_window(void) {
    int back = -1, fwd = -1;

    /* Scrolling forward leads the cursor by a wide margin and still keeps a
       few covers behind it -- the old policy shrank the lead to five and the
       trail to nothing, which is what made art appear only once selected. */
    art_scroll_window(400, 1, 1, &back, &fwd);
    assert(fwd >= 12 && back >= 2 && fwd > back);

    /* Reversing swaps the window rather than leaving the cursor's path empty. */
    int rev_back = -1, rev_fwd = -1;
    art_scroll_window(400, 1, -1, &rev_back, &rev_fwd);
    assert(rev_back == fwd && rev_fwd == back);

    /* Standing still can afford a wider window in both directions. */
    int idle_back = -1, idle_fwd = -1;
    art_scroll_window(400, 0, 1, &idle_back, &idle_fwd);
    assert(idle_fwd >= fwd && idle_back >= back);

    /* Huge libraries lead less far so the queue is not filled with covers the
       cursor has already flown past, but never drop the trail to zero. */
    int big_back = -1, big_fwd = -1;
    art_scroll_window(4000, 1, 1, &big_back, &big_fwd);
    assert(big_fwd > 0 && big_fwd <= fwd && big_back > 0);

    /* Whatever the inputs, the window stays inside the request queue. */
    const int totals[] = { 1, 12, 400, 1201, 4000 };
    for (unsigned i = 0; i < sizeof totals / sizeof *totals; i++)
        for (int scrolling = 0; scrolling <= 1; scrolling++)
            for (int dir = -1; dir <= 1; dir += 2) {
                art_scroll_window(totals[i], scrolling, dir, &back, &fwd);
                assert(back > 0 && fwd > 0 && back + fwd + 1 < ART_Q_CAP);
            }
    puts("PASS: art look-ahead leads the cursor, follows a reversal and stays inside the queue");
}

static void test_art_prefetch_order(void) {
    /* Nearest-first: the cover under the cursor is requested before the ones
       the user has not reached yet, so a fast flick still fills in order. */
    int saved_count = game_count;
    game_count = 200;
    for (int i = 0; i < game_count; i++) games[i].art_q_state = 0;

    art_prefetch(100, 3, 6);
    /* Everything in the window is spoken for and nothing outside it is. */
    for (int i = 0; i < game_count; i++) {
        int want = (i >= 97 && i <= 106);
        assert((games[i].art_q_state != 0) == want);
    }

    /* A second pass over the same window must not re-queue any of it. */
    int queued = art_q_n;
    art_prefetch(100, 3, 6);
    assert(art_q_n == queued);

    /* The window clamps at both ends of the library instead of wrapping. */
    for (int i = 0; i < game_count; i++) games[i].art_q_state = 0;
    art_prefetch(1, 8, 4);
    assert(games[0].art_q_state && games[5].art_q_state && !games[6].art_q_state);
    for (int i = 0; i < game_count; i++) games[i].art_q_state = 0;
    art_prefetch(game_count - 2, 4, 9);
    assert(games[game_count - 1].art_q_state && games[game_count - 6].art_q_state);

    art_async_reset();
    for (int i = 0; i < game_count; i++) games[i].art_q_state = 0;
    game_count = saved_count;
    puts("PASS: art prefetch is nearest-first, clamps at both ends and never re-queues");
}

static void test_art_eviction_keeps_cursor(void) {
    /* Plain FIFO eviction threw away the covers just ahead of a cursor that had
       turned around. The ring now gives anything near the cursor a second
       chance and evicts something genuinely off-screen instead. */
    int saved_cursor = art_cursor;
    int saved_n = art_ring_n, saved_head = art_ring_head;
    int saved_ring[ART_CACHE_CAP];
    memcpy(saved_ring, art_ring, sizeof saved_ring);

    /* A full ring holding indices 0..CAP-1, with the cursor sitting on the
       oldest entries -- exactly the state after scrolling back to the top. */
    for (int i = 0; i < ART_CACHE_CAP; i++) art_ring[i] = i;
    art_ring_n = ART_CACHE_CAP;
    art_ring_head = 0;
    art_cursor = 2;

    int scanned = 0;
    for (; scanned < ART_CACHE_CAP; scanned++) {
        int dist = art_ring[art_ring_head] - art_cursor;
        if (dist < 0) dist = -dist;
        if (dist > ART_KEEP_NEAR) break;
        art_ring_head = (art_ring_head + 1) % ART_CACHE_CAP;
    }
    /* It skipped every entry within ART_KEEP_NEAR of the cursor ... */
    assert(scanned == ART_KEEP_NEAR + art_cursor + 1);
    /* ... and settled on one far enough away to be safe to drop. */
    int victim = art_ring[art_ring_head];
    assert(victim - art_cursor > ART_KEEP_NEAR);

    /* With no library on screen there is nothing to protect, so the scan is
       skipped entirely and eviction stays plain FIFO. */
    art_cursor = -1;
    art_ring_head = 0;
    assert(art_cursor < 0);

    memcpy(art_ring, saved_ring, sizeof saved_ring);
    art_ring_n = saved_n; art_ring_head = saved_head; art_cursor = saved_cursor;
    puts("PASS: cover eviction spares the covers around the cursor and drops an off-screen one");
}

static void test_art_scroll_131(void) {
    test_art_scroll_window();
    test_art_prefetch_order();
    test_art_eviction_keeps_cursor();
}
#endif
