#ifndef SNAPFE_BATTERY_PROMPT_H
#define SNAPFE_BATTERY_PROMPT_H

/* Answers survive a frontend restart. A brief charge below 20% does not
   create a new discharge cycle and bring back a dismissed recommendation. */
#define BATTERY_PROMPT_POLL_MS 750u
#define BATTERY_PROMPT_RECHARGE_MS 30000u
#define BATTERY_PROMPT_ARM_MS 450u
typedef struct {
    int answered, notices, level, percent, loaded, dirty, sampled;
    int recharging, allow_present;
    Uint32 last_sample, recharge_since;
} BatteryPromptState;
static BatteryPromptState battery_prompt_state;

static const char *battery_prompt_path(void) {
    static char path[768];
    snprintf(path, sizeof path, "%s/battery-prompt.cfg", sn_data_root());
    return path;
}

static void battery_prompt_read_state(BatteryPromptState *s, const char *path) {
    memset(s, 0, sizeof *s); s->percent = -1; s->loaded = 1;
    FILE *f = fopen(path, "r");
    if (!f) return;
    int version, answered, notices; char extra;
    if (fscanf(f, " %d %d %d %c", &version, &answered, &notices, &extra) == 3 &&
        version == 1 && answered >= 0 && answered <= 2 && notices >= 0 && notices <= 3) {
        s->answered = answered; s->notices = notices;
    }
    fclose(f);
}

static int battery_prompt_write_state(BatteryPromptState *s, const char *path) {
    if (!s->dirty) return 1;
    char temp[896]; snprintf(temp, sizeof temp, "%s.tmp.%ld", path, (long)getpid());
    FILE *f = fopen(temp, "w");
    if (!f) return 0;
    int ok = fprintf(f, "1 %d %d\n", s->answered, s->notices) > 0;
    if (fflush(f) != 0 || fsync(fileno(f)) != 0) ok = 0;
    if (fclose(f) != 0) ok = 0;
    if (ok && rename(temp, path) == 0) { s->dirty = 0; return 1; }
    unlink(temp); return 0;
}

static int battery_prompt_sample_due(BatteryPromptState *s, Uint32 now) {
    if (s->sampled && now - s->last_sample < BATTERY_PROMPT_POLL_MS) return 0;
    s->sampled = 1; s->last_sample = now; return 1;
}

static void battery_prompt_sample(BatteryPromptState *s, int percent, int charging,
                                  int saving, Uint32 now) {
    s->percent = percent; s->level = 0;
    if (percent < 0 || percent > 100) { s->recharging = 0; return; }
    /* A substantial recovery also catches charging while SNAP was powered
       off. The five-point margin avoids resetting on a 20/21% gauge bounce. */
    if ((charging && percent > 20) || percent >= 25) {
        if (!s->recharging) { s->recharging = 1; s->recharge_since = now; }
        if (now - s->recharge_since >= BATTERY_PROMPT_RECHARGE_MS &&
            (s->answered || s->notices)) {
            s->answered = s->notices = 0; s->dirty = 1;
        }
    } else s->recharging = 0;
    if (charging || saving) return;
    if (percent <= 5 && s->answered < 2) s->level = 5;
    else if (percent <= 20 && s->answered == 0) s->level = 20;
}

static void battery_prompt_tick(int allow_present) {
    BatteryPromptState *s = &battery_prompt_state;
    if (!s->loaded) battery_prompt_read_state(s, battery_prompt_path());
    s->allow_present = allow_present;
    Uint32 now = SDL_GetTicks();
    if (!battery_prompt_sample_due(s, now)) return;
    int percent, charging; read_battery(&percent, &charging);
    battery_prompt_sample(s, percent, charging, power_save_mode, now);
    battery_prompt_write_state(s, battery_prompt_path());
}

static int battery_prompt_pending(void) { return battery_prompt_state.level != 0; }
static int battery_prompt_percent(void) { return battery_prompt_state.percent; }
static int battery_prompt_game_notice_due(void) {
    int bit = battery_prompt_state.level == 5 ? 2 : 1;
    return battery_prompt_pending() && !(battery_prompt_state.notices & bit);
}
static void battery_prompt_game_notice_marked(void) {
    if (!battery_prompt_pending()) return;
    battery_prompt_state.notices |= battery_prompt_state.level == 5 ? 2 : 1;
    battery_prompt_state.dirty = 1;
    battery_prompt_write_state(&battery_prompt_state, battery_prompt_path());
}

/* Returns the explicit enable decision; tests exercise this policy function
   without changing governors, backlights or any other hardware state. */
static int battery_prompt_answer(BatteryPromptState *s, int accept) {
    if (!s->level) return 0;
    s->answered = s->level == 5 ? 2 : 1;
    s->dirty = 1; s->level = 0;
    return accept != 0;
}
static void battery_prompt_respond(int accept) {
    int enable = battery_prompt_answer(&battery_prompt_state, accept);
    battery_prompt_write_state(&battery_prompt_state, battery_prompt_path());
    if (enable) set_power_save(1, 0);
}

/* Requiring release after a short arm interval prevents the press that
   brought up the prompt (or a held/repeating A) from accepting it. */
typedef struct { int visible, armed; Uint32 since; } BatteryPromptGuard;
static void battery_prompt_guard_tick(BatteryPromptGuard *g, int visible,
                                      int held, Uint32 now) {
    if (!visible) { memset(g, 0, sizeof *g); return; }
    if (!g->visible) { g->visible = 1; g->armed = 0; g->since = now; }
    if (!g->armed && now - g->since >= BATTERY_PROMPT_ARM_MS && !held) g->armed = 1;
}
static int battery_prompt_guard_choice(const BatteryPromptGuard *g,
                                       SDL_Keycode key, int repeat) {
    if (!g->visible || !g->armed || repeat) return -1;
    return key == SDLK_RETURN ? 1 : key == SDLK_ESCAPE ? 0 : -1;
}
#endif
