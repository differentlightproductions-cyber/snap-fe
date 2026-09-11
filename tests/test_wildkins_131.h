#ifndef SNAP_TEST_WILDKINS_131_H
#define SNAP_TEST_WILDKINS_131_H
/* Wildkins is its own app: a separate full-screen program on the Apps page (not
   pinned to Home by default), not one of the mini games SNAP draws inside its window. What
   has to hold: SNAP runs the game's own runner and never with --preview (that
   flag pauses snapos_ui, which would be SNAP freezing itself), it only runs a
   runner from inside the game's folder, and the app is offered honestly. */

static void test_wildkins_131(void) {
    char out[512];

    /* The launcher exactly as the Wildkins installer writes it. */
    const char *installed =
        "#!/bin/sh\n# Wildkins standalone launcher\n"
        "exec python3 /userdata/roms/ports/wildkins/versions/0123456789abcdef0123/run-game.py --preview\n";
    assert(wildkins_parse_launcher(installed, out, sizeof out));
    assert(!strcmp(out, "/userdata/roms/ports/wildkins/versions/0123456789abcdef0123/run-game.py"));
    assert(!strstr(out, "preview"));   /* the flag that pauses the frontend is left behind */

    /* shlex.quote() wraps a path in single quotes when it needs them. */
    assert(wildkins_parse_launcher(
        "exec python3 '/userdata/roms/ports/wildkins/versions/a b/run-game.py' --preview\n", out, sizeof out));
    assert(!strcmp(out, "/userdata/roms/ports/wildkins/versions/a b/run-game.py"));

    /* Anything but the game's own runner, in the game's own folder, is refused --
       including the retired Mosslight install. */
    assert(!wildkins_parse_launcher("exec python3 /tmp/run-game.py --preview\n", out, sizeof out));
    assert(!wildkins_parse_launcher("exec python3 /userdata/roms/ports/mosslight/versions/x/run-game.py\n", out, sizeof out));
    assert(!wildkins_parse_launcher("exec python3 /userdata/roms/ports/wildkins/../../../bin/run-game.py\n", out, sizeof out));
    assert(!wildkins_parse_launcher("exec python3 /userdata/roms/ports/wildkins/versions/x/other.py\n", out, sizeof out));
    assert(!wildkins_parse_launcher("exec python3 '/userdata/roms/ports/wildkins/versions/x/run-game.py\n", out, sizeof out));
    assert(!wildkins_parse_launcher("#!/bin/sh\necho hello\n", out, sizeof out));
    assert(!wildkins_parse_launcher(NULL, out, sizeof out));
    char tiny[8];
    assert(!wildkins_parse_launcher(installed, tiny, sizeof tiny));   /* never overruns the caller */

    /* Not a mini game any more: no entry, no genre tab of its own. */
    for (int id = 0; id < MG_COUNT; id++) {
        assert(strcmp(mg_games[id].name, "Wildkins") != 0);
        assert(strcmp(mg_games[id].name, "Mosslight") != 0);
        int g = mgx_game_genre(id);
        assert(g >= 0 && g < MGX_GENRE_COUNT);
    }
    assert(MGX_GENRE_COUNT == 4);

    /* An app of its own, wired both ways, reachable from Home and the Apps page. */
    assert(!strcmp(home_app_names[APP_WILDKINS], "Wildkins"));
    assert(home_app_row_type(APP_WILDKINS) == ROW_H_QUICK_WILDKINS);
    assert(home_app_id_for_row(ROW_H_QUICK_WILDKINS) == APP_WILDKINS);
    assert(!strcmp(home_app_slug(APP_WILDKINS), "wildkins"));
    int key = home_tile_key(ROW_H_QUICK_WILDKINS);
    assert(key >= 0 && key < HOME_ORDER_COUNT);
    /* Offered on the Apps page, but not pinned to Home unless the player pins it. */
    assert(!(APP_PINNED_DEFAULT & (1 << APP_WILDKINS)));

    /* The PC build has nothing to launch: it says so, and SNAP keeps its window. */
    int was_running = game_running;
    wildkins_notice_until = 0;
    wildkins_launch(NULL, NULL, NULL);
    assert(game_running == was_running);
    assert(wildkins_notice_until > SDL_GetTicks() && wildkins_notice[0]);
    assert(!wildkins_installed());
    wildkins_notice_until = 0;

    puts("PASS: Wildkins is its own full-screen app on the Apps page (not pinned to Home by default), never a mini game, and never launched with the flag that pauses SNAP FE");
}
#endif
