static int battery_led_test_value(const char *path) {
    FILE *f = fopen(path, "r"); assert(f); int value = -1;
    assert(fscanf(f, "%d", &value) == 1); fclose(f); return value;
}
static void test_battery_led(void) {
    char led_path[768], missing_path[768], power_path[768];
    snprintf(led_path, sizeof led_path, "%s/test-lowpwr-led", sn_data_root());
    snprintf(missing_path, sizeof missing_path, "%s/test-no-led-node", sn_data_root());
    snprintf(power_path, sizeof power_path, "%s/test-work-led", sn_data_root());
    FILE *f = fopen(led_path, "w"); assert(f); fclose(f); // write-only sysfs reads return empty
    f = fopen(power_path, "w"); assert(f); fputs("1\n", f); fclose(f);
    BatteryLedState led = {.applied = -1};
    const int percentages[] = {100, 21, 20, 19, 0, 21, 20, 30};
    const int expected[] = {0, 0, 1, 1, 1, 0, 1, 0};
    for (int i = 0; i < (int)(sizeof percentages / sizeof percentages[0]); i++) {
        int result = battery_led_apply_sample(&led, led_path, percentages[i], 0);
        assert(result == (i == 0 || expected[i] != expected[i - 1] ? 1 : 0));
        assert(battery_led_test_value(led_path) == expected[i]);
        assert(battery_is_low(percentages[i]) == expected[i]);
        assert(battery_led_test_value(power_path) == 1);
    }
    assert(!battery_is_low(-1) && !battery_is_low(101));
    assert(battery_led_apply_sample(&led, led_path, 20, 0) == 1);
    assert(!battery_led_apply_sample(&led, led_path, -1, 1));
    assert(!battery_led_apply_sample(&led, led_path, 101, 1));
    assert(battery_led_test_value(led_path) == 1);
    /* Normal polling does not keep overwriting notification/critical flashes;
       waking explicitly restores red even if another power action cleared it. */
    f = fopen(led_path, "w"); assert(f); fputs("0\n", f); fclose(f);
    assert(!battery_led_apply_sample(&led, led_path, 20, 0));
    assert(battery_led_apply_sample(&led, led_path, 20, 1) == 1);
    assert(battery_led_test_value(led_path) == 1);
    BatteryLedState missing = {.applied = -1};
    assert(battery_led_apply_sample(&missing, missing_path, 20, 0) == -1);
    assert(access(missing_path, F_OK) != 0 && missing.applied == -1);
    f = fopen(missing_path, "w"); assert(f); fclose(f);
    assert(battery_led_apply_sample(&missing, missing_path, 20, 0) == 1);

    BatteryLedState poll = {.applied = -1};
    assert(battery_led_sample_due(&poll, 1000u, 0));
    assert(!battery_led_sample_due(&poll, 1001u, 0));
    assert(!battery_led_sample_due(&poll, 1749u, 0));
    assert(battery_led_sample_due(&poll, 1750u, 0));
    assert(battery_led_sample_due(&poll, 1751u, 1));
    poll.last = 0xffffff00u;
    assert(!battery_led_sample_due(&poll, poll.last + 749u, 0));
    assert(battery_led_sample_due(&poll, poll.last + 750u, 0));
    /* The production tick is intentionally inert under SNAPFE_TESTING, even
       in an ARM binary run on a handheld. Only the temporary files above move. */
    battery_led_tick(0); battery_led_tick(1);
    assert(battery_led_test_value(power_path) == 1);
    unlink(led_path); unlink(missing_path); unlink(power_path);
    puts("PASS: physical LED/statusbar share inclusive20% threshold, transition-only writes, forced wake restoration, invalid/missing sensor safety and bounded polling");
}
