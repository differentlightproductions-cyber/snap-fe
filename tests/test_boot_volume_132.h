#ifndef SNAP_TEST_BOOT_VOLUME_132_H
#define SNAP_TEST_BOOT_VOLUME_132_H
/* The boot chime plays at the player's saved volume. custom.sh brings audio up
   at the saved level (not a fixed 90%), and Snap FE sets it and waits before
   the chime, rather than through the coalesced path the main loop sends from. */

static void test_boot_volume_132(void) {
    static char text[131072];
    FILE *f = fopen("knulli/custom.sh", "r");
    if (!f) f = fopen("../knulli/custom.sh", "r");
    assert(f);
    size_t n = fread(text, 1, sizeof text - 1, f);
    fclose(f);
    text[n] = 0;
    assert(strstr(text, "sys_volume_pct=") && strstr(text, "setSystemVolume \"$vol\""));
    assert(!strstr(text, "setSystemVolume 90 "));

    sys_volume_pending = 55;
    sys_volume_send_now(40);
    assert(sys_volume_pending < 0 && sys_volume_last_sent == 40);
    puts("PASS: the boot chime plays at the saved volume -- custom.sh starts audio at it and Snap FE sets it before the chime");
}
#endif
