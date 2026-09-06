#ifndef SNAPFE_BATTERY_LED_H
#define SNAPFE_BATTERY_LED_H
/* Knulli normally gives EmulationStation ownership of this warning light.
   SNAP replaces ES, so the same raw percentage used by our status bar must
   drive the low-power LED. Charging does not change the warning threshold. */
#define BATTERY_LOW_PERCENT 20
#define BATTERY_LED_POLL_MS 750u
static int battery_is_low(int percent) {
    return percent >= 0 && percent <= BATTERY_LOW_PERCENT;
}

typedef struct { int applied, sampled; Uint32 last; } BatteryLedState;
static int battery_led_sample_due(BatteryLedState *state, Uint32 now, int force) {
    if (!force && state->sampled && now - state->last < BATTERY_LED_POLL_MS) return 0;
    state->last = now; state->sampled = 1; return 1;
}

static int battery_led_apply_sample(BatteryLedState *state, const char *path, int percent, int force) {
    if (percent < 0 || percent > 100) return 0;
    int low = battery_is_low(percent);
    if (!force && state->applied == low) return 0;
    /* These H700 attributes accept writes but return no readable value.
       Open the existing node only: an unsupported device must not create a
       pretend control file. Leave work_led and system.power.led untouched. */
    int fd = open(path, O_WRONLY | O_CLOEXEC);
    if (fd < 0) return -1;
    const char *value = low ? "1\n" : "0\n";
    ssize_t written;
    do { written = write(fd, value, 2); } while (written < 0 && errno == EINTR);
    int closed = close(fd);
    if (written != 2 || closed != 0) return -1;
    state->applied = low;
    return 1;
}

static void battery_led_tick(int force) {
#if defined(SNAPOS_TARGET_KNULLI) && !defined(SNAPFE_TESTING)
    static BatteryLedState led = {.applied = -1};
    if (!battery_led_sample_due(&led, SDL_GetTicks(), force)) return;
    int percent, charging; read_battery(&percent, &charging);
    (void)charging;
    battery_led_apply_sample(&led, "/sys/class/power_supply/axp2202-battery/lowpwr_led", percent, force);
#else
    /* Native and ARM regression tests use temporary files through the sample
       function above; no simulated percentage may reach a physical LED. */
    (void)force;
#endif
}
#endif
