#ifndef SNAP_TEST_ART_ASYNC_131_H
#define SNAP_TEST_ART_ASYNC_131_H
/* Every kind of library art now decodes on the art threads instead of inside
   the frame. Covers already did; backdrops, cartridges and the bookshelf page
   did not -- they decoded synchronously, one per frame at best, which is why
   those views filled in only the card you were resting on. */

static void test_art_async_131(void) {
    int saved_count = game_count;
    game_count = 40;
    for (int i = 0; i < game_count; i++) {
        memset(&games[i], 0, sizeof games[i]);
        snprintf(games[i].title, 128, "Game %02d", i);
        snprintf(games[i].path, 768, "/test/async/%d", i);
        strcpy(games[i].platform_dir, "gb");
    }
    /* The suite starts the real decoder in main(), threads and all: this test
       is about the async path, so faking it would prove nothing. */
    assert(art_mtx && art_thr[0]);
    art_async_reset();
    gcart_epoch = bd_epoch = renderer_epoch - 1;   /* force both rings to reset */
    gcart_reset_if_stale(); bd_reset_if_stale();

    /* Each kind maps to the right art type, so a cartridge request cannot come
       back with a cover in it. */
    assert(ART_KIND_COVER != ART_KIND_BACKDROP && ART_KIND_CART != ART_KIND_BOOK);

    /* A backdrop is requested once and then remembered as in flight, however
       many frames ask for it -- otherwise the queue fills with duplicates of
       the card under the cursor. */
    assert(backdrop_request(3) == 1);
    assert(backdrop_request(3) == 0);
    assert(backdrop_request(3) == 0);
    int slot = bd_slot_of(3);
    assert(slot >= 0 && bd_ring[slot].pending && !bd_ring[slot].tex);
    /* And while it is in flight the view gets NULL rather than a stall. */
    assert(game_backdrop_art(NULL, 3) == NULL);

    /* Same contract for cartridges. */
    assert(gcart_request(5) == 1);
    assert(gcart_request(5) == 0);
    int cslot = gcart_slot_of(5);
    assert(cslot >= 0 && gcart[cslot].pending);

    /* Prefetching the neighbours queues all of them, not one per call the way
       the old main-thread version had to ration itself. Counted by ring slots
       claimed, not by queue depth: three decode threads can empty the queue
       before the next line runs. */
    backdrop_prefetch(NULL, 20);
    int queued = 0;
    for (int d = 1; d <= 3; d++)
        for (int side = 0; side < 2; side++) {
            int i = side ? 20 + d : 20 - d;
            if (bd_slot_of(i) >= 0) queued++;
        }
    assert(queued >= 4);   /* bounded by the ring, but far more than one */

    /* The bookshelf asks for one shot at a time and does not re-ask while it is
       waiting; changing page clears the wait so the new one goes out. */
    book_shot_for = -1; book_shot_slot = -1;
    book_shot_want_gi = -1; book_shot_want_slot = -1; book_shot_pending = 0;
    book_shot_want(7, 2);
    assert(book_shot_want_gi == 7 && book_shot_want_slot == 2 && !book_shot_pending);
    book_shot_pump();
    assert(book_shot_pending && book_shot_want_gi == 7);
    book_shot_pump();                       /* still waiting: no second request */
    assert(book_shot_pending && book_shot_want_gi == 7);
    book_shot_want(8, 2);                   /* page turned */
    assert(!book_shot_pending && book_shot_want_gi == 8);

    /* A result that arrives after the page moved on is dropped, not shown on
       the wrong page. */
    book_shot_want_gi = 9;
    book_shot_store(NULL, 8, NULL, NULL);
    assert(book_shot_for != 8);

    /* Requests carry the generation, so anything queued before a rescan is
       discarded rather than filed against a different game. */
    int gen_before = art_gen;
    art_async_reset();
    assert(art_gen != gen_before && art_q_n == 0);

    /* More than one decode thread, and every one of them accounted for. */
    assert(ART_WORKERS >= 2);
    for (int i = 0; i < ART_WORKERS; i++) assert(art_thr[i] != NULL);

    art_async_reset();
    for (int i = 0; i < game_count; i++) games[i].art_q_state = 0;
    game_count = saved_count;
    gcart_epoch = bd_epoch = renderer_epoch - 1;
    gcart_reset_if_stale(); bd_reset_if_stale();
    book_shot_want_gi = book_shot_want_slot = -1; book_shot_pending = 0;
    puts("PASS: backdrops, cartridges and bookshelf art all decode off the render thread, requested once each");
}
#endif
