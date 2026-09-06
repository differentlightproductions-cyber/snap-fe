static void test_sleep_duration(SDL_Renderer *ren) {
    int old_duration = screen_saver_duration_idx, old_saver = screen_saver_idx;
    int old_timeout = auto_sleep_idx, old_sel = screenpower_sel, old_scroll = screenpower_scroll;
    int old_dirty = settings_dirty;
    Uint32 old_input = last_input_time;
    save_settings();
    FILE *original = fopen(settings_path(), "rb"); assert(original);
    assert(!fseek(original, 0, SEEK_END)); long original_size = ftell(original); assert(original_size >= 0);
    rewind(original); char *original_text = malloc((size_t)original_size + 1); assert(original_text);
    assert(fread(original_text, 1, (size_t)original_size, original) == (size_t)original_size); fclose(original);

    static const Uint32 expected[] = {15000u, 30000u, 60000u, 120000u, 300000u, 600000u};
    assert(SCREEN_SAVER_DURATION_COUNT == (int)(sizeof expected / sizeof expected[0]));
    for (int i = 0; i < SCREEN_SAVER_DURATION_COUNT; i++) {
        screen_saver_duration_idx = i;
        screen_saver_idx = SCREEN_SAVER_AQUARIUM;
        screen_saver_begin();
        screen_saver_started = 4000u;
        assert(screen_saver_duration_ms() == expected[i]);
        assert(!screen_saver_should_sleep(4000u + expected[i] - 1u));
        assert(screen_saver_should_sleep(4000u + expected[i]));
        assert(screen_saver_should_sleep(4000u + expected[i] + 1u));
        /* SDL's 32-bit tick counter wraps after long uptime. */
        screen_saver_started = 0xfffffff0u;
        assert(!screen_saver_should_sleep(screen_saver_started + expected[i] - 1u));
        assert(screen_saver_should_sleep(screen_saver_started + expected[i]));
        last_input_time = 123456u;
        screen_saver_wake();
        assert(!screen_saver_should_sleep(999999u) && last_input_time == 123456u);
        assert(!screen_saver_active && screen_saver_started == 0 && screen_saver_last_frame == 0);
        save_settings(); screen_saver_duration_idx = -1; load_settings();
        assert(screen_saver_duration_idx == i && screen_saver_duration_ms() == expected[i]);
    }
    const char *invalid[] = {"-1", "6", "9999", "text", "2junk", "", "   "};
    for (int i = 0; i < (int)(sizeof invalid / sizeof invalid[0]); i++) {
        FILE *f = fopen(settings_path(), "w"); assert(f);
        fprintf(f, "screen_saver_duration_idx=%s\n", invalid[i]); fclose(f);
        screen_saver_duration_idx = 5; load_settings();
        assert(screen_saver_duration_idx == SCREEN_SAVER_DURATION_DEFAULT && screen_saver_duration_ms() == 60000u);
    }
    FILE *legacy = fopen(settings_path(), "w"); assert(legacy);
    fprintf(legacy, "screen_saver_idx=%d\n", SCREEN_SAVER_STARFIELD); fclose(legacy);
    screen_saver_duration_idx = 5; load_settings();
    assert(screen_saver_idx == SCREEN_SAVER_STARFIELD && screen_saver_duration_ms() == 60000u);
    screen_saver_duration_idx = 5; restore_display_group(ROW_DISP_RST_SCREEN);
    assert(screen_saver_duration_idx == SCREEN_SAVER_DURATION_DEFAULT);

    screen_saver_duration_idx = 4; screen_saver_idx = SCREEN_SAVER_AQUARIUM; auto_sleep_idx = 1;
    screenpower_sel = SP_ROW_SAVER_DURATION; screenpower_scroll = 0;
    draw_theme_background(ren, &themes[theme_idx], theme_idx);
    draw_screen_power_page(ren, &themes[theme_idx]); capture(ren, "screen-power-sleep-duration");

    original = fopen(settings_path(), "wb"); assert(original);
    assert(fwrite(original_text, 1, (size_t)original_size, original) == (size_t)original_size); fclose(original); free(original_text);
    load_settings();
    screen_saver_duration_idx = old_duration; screen_saver_idx = old_saver; auto_sleep_idx = old_timeout;
    screenpower_sel = old_sel; screenpower_scroll = old_scroll; settings_dirty = old_dirty; last_input_time = old_input;
    puts("PASS: six sleep-screen durations, exact deadlines and tick wrap, wake cancellation, saved preference, invalid/legacy defaults and screen reset");
}
