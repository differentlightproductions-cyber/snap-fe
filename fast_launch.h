#ifndef SNAPFE_FAST_LAUNCH_H
#define SNAPFE_FAST_LAUNCH_H
/* Fast Game Launch.
 *
 * Knulli starts every game through its Python launcher (configgen), which
 * rebuilds RetroArch's config, controller profile and hotkeys before RetroArch
 * even exists: about three seconds on the H700, every launch, even when
 * nothing changed. SNAP FE's own share of a launch is about 0.1 s.
 *
 * The first launch of a system still goes through Knulli. Once RetroArch is
 * running, SNAP records exactly what Knulli started: RetroArch's command line,
 * environment and working directory, the helpers Knulli runs beside it (evmapy,
 * its controller mapper), and a copy of the config files they read. The next
 * game on the same system and core puts those configs back, starts the helpers
 * and RetroArch with the new ROM, and stops the helpers when the game ends:
 * the same session Knulli would have produced.
 *
 * Only when the result is provably the same. Everything Knulli builds a
 * session from (knulli.conf, EmulationStation's controller and settings files,
 * the core, the attached controllers and input devices) is fingerprinted, and
 * any change sends the launch back through Knulli, which records a fresh copy.
 * Never recorded: per-game settings or key maps, configs that name the game,
 * sessions with a helper SNAP doesn't know how to replay, anything that isn't
 * RetroArch. A fast launch that fails in its first seconds is forgotten and
 * retried through Knulli at once. On by default; Settings > Device >
 * Performance > Fast Game Launch turns it off, which clears every recording.
 * Every decision is logged to /tmp/snapfe-launch-perf.log. */

static char fl_cache_dir[512] = "/userdata/system/snapos/fastlaunch";
static char fl_knulli_conf[512] = "/userdata/system/knulli.conf";
static char fl_es_input[512] = "/userdata/system/configs/emulationstation/es_input.cfg";
static char fl_es_settings[512] = "/userdata/system/configs/emulationstation/es_settings.cfg";
static int fl_require_retroarch = 1;   /* tests record an ordinary process */
static char fl_evmapy_dir[512] = "/var/run/evmapy";   /* evmapy reads every key map in here */
static char fl_scratch_dir[512] = "/tmp/snapfe-fast";  /* this launch's settings files, for the log */
/* The in-game RetroArch preferences SNAP carries in knulli.conf (save slot,
   volume, ...): their values don't call for a new recording. */
#ifdef SNAPOS_TARGET_KNULLI
static int (*fl_user_setting)(const char *key) = ra_user_setting_allowed;
#else
static int (*fl_user_setting)(const char *key) = NULL;   /* tests supply their own */
#endif

#define FL_MAX_ARGS 64
#define FL_MAX_ENV 128
#define FL_MAX_FILES 16
#define FL_MAX_HELPERS 4
#define FL_LINE_MAX 4000
#define FL_FILE_CAP (4 * 1024 * 1024)

typedef struct {
    char exe[512], cwd[512];
    int argc, envc;
    char *argv[FL_MAX_ARGS + 1];
    char *envp[FL_MAX_ENV + 1];
} FlProc;

typedef struct {
    char core[64], rom[640];
    char inputs[20], inputs2[20];       /* settings files before Knulli's launcher ran, and as it left them */
    char sig[1536];                     /* controllers and input devices, kept for the log */
    FlProc game;                        /* RetroArch */
    int helperc;
    FlProc helper[FL_MAX_HELPERS];      /* started first, stopped when the game ends */
    int filec;
    char file[FL_MAX_FILES][512];
} FastLaunch;

static void fl_log(const char *fmt, ...) {
#ifdef SNAPOS_TARGET_KNULLI
    FILE *f = fopen("/tmp/snapfe-launch-perf.log", "a");
    if (!f) return;
    va_list ap;
    va_start(ap, fmt);
    fprintf(f, "%u fast-launch: ", SDL_GetTicks());
    vfprintf(f, fmt, ap);
    fputc('\n', f);
    va_end(ap);
    fclose(f);
#else
    (void)fmt;
#endif
}

static void fl_proc_free(FlProc *p) {
    for (int i = 0; i < p->argc; i++) free(p->argv[i]);
    for (int i = 0; i < p->envc; i++) free(p->envp[i]);
    p->argc = p->envc = 0;
    p->argv[0] = NULL;
    p->envp[0] = NULL;
}
static void fastlaunch_free(FastLaunch *fl) {
    fl_proc_free(&fl->game);
    for (int i = 0; i < fl->helperc; i++) fl_proc_free(&fl->helper[i]);
    fl->helperc = fl->filec = 0;
}

static int fl_push(char **list, int *count, int max, const char *s) {
    if (*count >= max || strlen(s) >= FL_LINE_MAX || strchr(s, '\n')) return 0;
    char *copy = strdup(s);
    if (!copy) return 0;
    list[(*count)++] = copy;
    list[*count] = NULL;
    return 1;
}

/* One recording per system and core: "gba-default", "snes-snes9x". */
static void fl_key(const char *sys, const char *core, char *out, size_t cap) {
    const char *parts[2] = { sys && sys[0] ? sys : "unknown", core && core[0] ? core : "default" };
    size_t n = 0;
    for (int p = 0; p < 2; p++) {
        if (p && n + 1 < cap) out[n++] = '-';
        for (const char *c = parts[p]; *c && n + 1 < cap; c++)
            out[n++] = (isalnum((unsigned char)*c) || *c == '_') ? *c : '_';
    }
    out[n] = 0;
}
static void fl_path(char *out, size_t cap, const char *key, const char *suffix) {
    snprintf(out, cap, "%s/%s%s", fl_cache_dir, key, suffix);
}

static uint64_t fl_fnv(uint64_t h, const void *data, size_t n) {
    const unsigned char *p = data;
    for (size_t i = 0; i < n; i++) { h ^= p[i]; h *= 1099511628211ULL; }
    return h;
}
static uint64_t fl_fnv_str(uint64_t h, const char *s) {
    if (s) h = fl_fnv(h, s, strlen(s));
    return fl_fnv(h, "\x1f", 1);
}
static uint64_t fl_fnv_file(uint64_t h, const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return fl_fnv_str(h, "<missing>");
    char buf[8192];
    size_t n;
    while ((n = fread(buf, 1, sizeof buf, f)) > 0) h = fl_fnv(h, buf, n);
    fclose(f);
    return fl_fnv(h, "\x1e", 1);
}

/* Whole small file (or /proc entry) into buf; -1 when missing or too long. */
static int fl_read_file(const char *path, char *buf, size_t cap) {
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    size_t n = fread(buf, 1, cap - 1, f);
    int more = fgetc(f) != EOF;
    fclose(f);
    if (more) return -1;
    buf[n] = 0;
    return (int)n;
}

/* The settings files Knulli's launcher reads. Contents, not dates: SNAP
   rewrites knulli.conf with identical text around launches. */
/* "key = value" (RetroArch) or "key=value" (after knulli.conf's
   "global.retroarch."): the key, and the value without spaces or quotes. */
static int fl_kv(const char *line, char *key, size_t kcap, char *val, size_t vcap) {
    while (*line == ' ' || *line == '\t') line++;
    const char *eq = strchr(line, '=');
    if (!eq || *line == '#') return 0;
    const char *ke = eq;
    while (ke > line && (ke[-1] == ' ' || ke[-1] == '\t')) ke--;
    size_t kl = (size_t)(ke - line);
    if (!kl || kl >= kcap) return 0;
    memcpy(key, line, kl);
    key[kl] = 0;
    const char *v = eq + 1;
    while (*v == ' ' || *v == '\t') v++;
    size_t vl = strcspn(v, "\r\n");
    while (vl && (v[vl - 1] == ' ' || v[vl - 1] == '\t')) vl--;
    if (vl >= 2 && v[0] == '"' && v[vl - 1] == '"') { v++; vl -= 2; }
    if (vl >= vcap) return 0;
    memcpy(val, v, vl);
    val[vl] = 0;
    return 1;
}
static int fl_is_user_line(const char *line) {
    char k[160], v[1024];
    return fl_user_setting && !strncmp(line, "global.retroarch.", 17) &&
           fl_kv(line + 17, k, sizeof k, v, sizeof v) && fl_user_setting(k);
}
typedef struct { char *key, *val; int idx; } FlConfLine;
static int fl_conf_cmp(const void *a, const void *b) {
    const FlConfLine *x = a, *y = b;
    int c = strcmp(x->key, y->key);
    return c ? c : x->idx - y->idx;
}
/* knulli.conf as settings rather than text: the order of lines doesn't matter
   (SNAP writes RetroArch's preferences in whatever order RetroArch saved them)
   and a repeated key counts once, the last one winning as it does for Knulli.
   In-game preferences count by name only; their values are applied at launch. */
static uint64_t fl_hash_knulli(uint64_t h) {
    FILE *f = fopen(fl_knulli_conf, "r");
    if (!f) return fl_fnv_str(h, "<missing>");
    FlConfLine *lines = NULL;
    int n = 0, cap = 0;
    char line[2048];
    while (fgets(line, sizeof line, f)) {
        line[strcspn(line, "\r\n")] = 0;
        char *p = line;
        while (*p == ' ' || *p == '\t') p++;
        char *eq = strchr(p, '=');
        if (!*p || *p == '#' || !eq) continue;
        if (n == cap) {
            int grown_cap = cap ? cap * 2 : 256;
            FlConfLine *grown = realloc(lines, (size_t)grown_cap * sizeof *grown);
            if (!grown) break;
            lines = grown;
            cap = grown_cap;
        }
        int user = fl_is_user_line(p);
        *eq = 0;
        lines[n].key = strdup(p);
        lines[n].val = strdup(user ? "" : eq + 1);
        lines[n].idx = n;
        if (!lines[n].key || !lines[n].val) { free(lines[n].key); free(lines[n].val); break; }
        n++;
    }
    fclose(f);
    if (n > 1) qsort(lines, (size_t)n, sizeof *lines, fl_conf_cmp);
    for (int i = 0; i < n; i++) {
        if (i + 1 < n && !strcmp(lines[i].key, lines[i + 1].key)) continue;   /* a later one wins */
        h = fl_fnv_str(h, lines[i].key);
        h = fl_fnv_str(h, lines[i].val);
    }
    for (int i = 0; i < n; i++) { free(lines[i].key); free(lines[i].val); }
    free(lines);
    return fl_fnv(h, "\x1e", 1);
}
static uint64_t fl_inputs_hash(void) {
    uint64_t h = 1469598103934665603ULL;
    h = fl_hash_knulli(h);
    h = fl_fnv_file(h, fl_es_input);
    h = fl_fnv_file(h, fl_es_settings);
    return h;
}

/* The core file as it is now: replaced or updated means a new recording. */
static void fl_core_stamp(const char *core_so, char *out, size_t cap) {
    struct stat st;
    if (core_so && core_so[0] && stat(core_so, &st) == 0)
        snprintf(out, cap, "%lld:%lld", (long long)st.st_mtime, (long long)st.st_size);
    else
        snprintf(out, cap, "none");
}
static const char *fl_input_path(int i) { return i == 0 ? fl_knulli_conf : i == 1 ? fl_es_input : fl_es_settings; }
static const char *fl_input_name(int i) { return i == 0 ? "knulli.conf" : i == 1 ? "es_input.cfg" : "es_settings.cfg"; }

/* The attached controllers in SDL's order (Knulli binds player 1 from them)
   and every input device node, which is what evmapy's maps are written for. */
static void fl_joysig(char *out, size_t cap) {
    int n = SDL_WasInit(SDL_INIT_JOYSTICK) ? SDL_NumJoysticks() : 0;
    snprintf(out, cap, "%d", n);
    for (int j = 0; j < n; j++) {
        char guid[64] = "";
        SDL_JoystickGetGUIDString(SDL_JoystickGetDeviceGUID(j), guid, sizeof guid);
        const char *name = SDL_JoystickNameForIndex(j);
        size_t used = strlen(out);
        if (used + 2 < cap) snprintf(out + used, cap - used, "|%s:%s", guid, name ? name : "");
    }
    for (int i = 0; i < 32; i++) {
        char path[96], name[160];
        snprintf(path, sizeof path, "/sys/class/input/event%d/device/name", i);
        if (fl_read_file(path, name, sizeof name) < 0) continue;
        name[strcspn(name, "\r\n")] = 0;
        size_t used = strlen(out);
        if (used + 2 < cap) snprintf(out + used, cap - used, "|e%d:%s", i, name);
    }
}

/* What Knulli's launcher is about to read, taken as a launch starts. Never
   while the game runs: evmapy adds a virtual input device of its own then. */
static int fl_copy(const char *src, const char *dst);
static void fl_mkdirs(const char *file_path);
static char fl_now_sig[1536], fl_now_inputs[20];
static void fl_snapshot_inputs(void) {
    fl_joysig(fl_now_sig, sizeof fl_now_sig);
    snprintf(fl_now_inputs, sizeof fl_now_inputs, "%016llx", (unsigned long long)fl_inputs_hash());
    /* Copies (tmpfs) kept with a recording, so a later mismatch can say which lines changed. */
    for (int i = 0; i < 3; i++) {
        char dst[600];
        snprintf(dst, sizeof dst, "%s/now.%d", fl_scratch_dir, i);
        fl_mkdirs(dst);
        if (!fl_copy(fl_input_path(i), dst)) unlink(dst);
    }
}

/* sys["Game.ext"].key=value lines are settings Knulli applies to one game. */
static int fl_has_game_overrides(const char *sys) {
    FILE *f = fopen(fl_knulli_conf, "r");
    if (!f) return 0;
    char line[1024], prefix[80];
    snprintf(prefix, sizeof prefix, "%s[\"", sys ? sys : "");
    size_t pl = strlen(prefix);
    int found = 0;
    while (!found && fgets(line, sizeof line, f)) found = strncmp(line, prefix, pl) == 0;
    fclose(f);
    return found;
}

/* evmapy key maps for one game (Game.gba.keys) or one folder (padto.keys). */
static int fl_rom_has_keys(const char *rom) {
    char p[800];
    snprintf(p, sizeof p, "%s.keys", rom);
    if (access(p, F_OK) == 0) return 1;
    const char *slash = strrchr(rom, '/');
    if (slash) {
        snprintf(p, sizeof p, "%.*s/padto.keys", (int)(slash - rom), rom);
        if (access(p, F_OK) == 0) return 1;
    }
    return 0;
}

static const char *fl_core_so(const FlProc *p) {
    for (int i = 0; i + 1 < p->argc; i++)
        if (!strcmp(p->argv[i], "-L") || !strcmp(p->argv[i], "--libretro")) return p->argv[i + 1];
    return "";
}

/* The launcher itself and its shells: part of Knulli's setup, not the game. */
static int fl_helper_allowed(const char *comm) {
    static const char *ok[] = { "retroarch", "emulatorlaunche", "emulatorlauncher", "sh", "bash", "dash", NULL };
    for (int i = 0; ok[i]; i++) if (!strcmp(comm, ok[i])) return 1;
    return 0;
}
/* Helpers Knulli keeps running during a game that SNAP records and replays. */
static int fl_helper_replayable(const char *comm) {
    return !strcmp(comm, "evmapy");
}

typedef struct { pid_t pid; char comm[32]; } FlMember;

/* The launcher runs in its own session (SNAP starts it with setsid), so every
   live process it started shares that session id. */
static int fl_session_members(pid_t sid, FlMember *out, int max) {
    if (sid <= 0) return 0;
    DIR *d = opendir("/proc");
    if (!d) return 0;
    struct dirent *e;
    int count = 0;
    while ((e = readdir(d)) != NULL && count < max) {
        if (!isdigit((unsigned char)e->d_name[0])) continue;
        char path[64], line[512];
        snprintf(path, sizeof path, "/proc/%s/stat", e->d_name);
        FILE *f = fopen(path, "r");
        if (!f) continue;
        size_t n = fread(line, 1, sizeof line - 1, f);
        fclose(f);
        line[n] = 0;
        char *open = strchr(line, '('), *close = strrchr(line, ')');
        if (!open || !close || close < open) continue;
        char state;
        int ppid, pgrp, session;
        if (sscanf(close + 1, " %c %d %d %d", &state, &ppid, &pgrp, &session) != 4 ||
            session != (int)sid || state == 'Z') continue;
        size_t cl = (size_t)(close - open - 1);
        if (cl >= sizeof out[count].comm) cl = sizeof out[count].comm - 1;
        memcpy(out[count].comm, open + 1, cl);
        out[count].comm[cl] = 0;
        out[count].pid = (pid_t)strtol(e->d_name, NULL, 10);
        count++;
    }
    closedir(d);
    return count;
}

/* 1 when every process in the session is the launcher, RetroArch or a helper
   SNAP can replay; otherwise names the first one it can't. */
static int fl_session_clean(pid_t sid, char *why, size_t cap, char *members, size_t mcap) {
    FlMember m[64];
    int n = fl_session_members(sid, m, 64), clean = 1;
    if (why && cap) why[0] = 0;
    if (members && mcap) members[0] = 0;
    for (int i = 0; i < n; i++) {
        if (members && mcap) {
            size_t used = strlen(members);
            if (used + 2 < mcap) snprintf(members + used, mcap - used, "%s%s", used ? " " : "", m[i].comm);
        }
        if (clean && !fl_helper_allowed(m[i].comm) && !fl_helper_replayable(m[i].comm)) {
            clean = 0;
            if (why && cap) snprintf(why, cap, "Knulli also ran %s", m[i].comm);
        }
    }
    return clean;
}

static int fl_read_proc(pid_t pid, FlProc *p, char *buf, size_t bufcap, char *why, size_t cap) {
    char path[64];
    snprintf(path, sizeof path, "/proc/%d/exe", (int)pid);
    ssize_t n = readlink(path, p->exe, sizeof p->exe - 1);
    if (n <= 0) { snprintf(why, cap, "process %d had already exited", (int)pid); return 0; }
    p->exe[n] = 0;
    snprintf(path, sizeof path, "/proc/%d/cmdline", (int)pid);
    int len = fl_read_file(path, buf, bufcap);
    if (len <= 0) { snprintf(why, cap, "no command line for %d", (int)pid); return 0; }
    for (int at = 0; at < len; at += (int)strlen(buf + at) + 1)
        if (!fl_push(p->argv, &p->argc, FL_MAX_ARGS, buf + at)) { snprintf(why, cap, "unusual command line"); return 0; }
    snprintf(path, sizeof path, "/proc/%d/environ", (int)pid);
    len = fl_read_file(path, buf, bufcap);
    for (int at = 0; len > 0 && at < len; at += (int)strlen(buf + at) + 1)
        if (buf[at]) fl_push(p->envp, &p->envc, FL_MAX_ENV, buf + at);   /* over-long entries are left out */
    snprintf(path, sizeof path, "/proc/%d/cwd", (int)pid);
    n = readlink(path, p->cwd, sizeof p->cwd - 1);
    p->cwd[n > 0 ? n : 0] = 0;
    return 1;
}

static int fl_copy(const char *src, const char *dst) {
    FILE *in = fopen(src, "rb");
    if (!in) return 0;
    char tmp[600];
    snprintf(tmp, sizeof tmp, "%s.snapfe-tmp", dst);
    FILE *out = fopen(tmp, "wb");
    if (!out) { fclose(in); return 0; }
    char buf[8192];
    size_t n;
    int ok = 1;
    while ((n = fread(buf, 1, sizeof buf, in)) > 0)
        if (fwrite(buf, 1, n, out) != n) { ok = 0; break; }
    if (ferror(in)) ok = 0;
    fclose(in);
    if (fclose(out) != 0) ok = 0;
    if (!ok || rename(tmp, dst) != 0) { unlink(tmp); return 0; }
    return 1;
}

/* Create the folders above a file path. */
static void fl_mkdirs(const char *file_path) {
    char tmp[600];
    snprintf(tmp, sizeof tmp, "%s", file_path);
    for (char *p = tmp + 1; *p; p++)
        if (*p == '/') { *p = 0; mkdir(tmp, 0755); *p = '/'; }
}

static void fl_rom_stem(const char *rom, char *out, size_t cap) {
    const char *base = strrchr(rom, '/');
    snprintf(out, cap, "%s", base ? base + 1 : rom);
    char *dot = strrchr(out, '.');
    if (dot && dot != out) *dot = 0;
}

static int fl_file_mentions(const char *path, const char *needle) {
    struct stat st;
    if (stat(path, &st) != 0 || st.st_size > FL_FILE_CAP) return 0;
    char *buf = malloc((size_t)st.st_size + 1);
    if (!buf) return 0;
    int n = fl_read_file(path, buf, (size_t)st.st_size + 1);
    int found = n >= 0 && strstr(buf, needle) != NULL;
    free(buf);
    return found;
}

/* Config files a session reads that Knulli regenerates: under the user's
   system folder or the runtime folders, never ROMs, cores or system files. */
static void fl_add_file(FastLaunch *fl, const char *p) {
    static const char *roots[] = { "/userdata/system/", "/var/run/", "/run/", "/tmp/", NULL };
    int ok = 0;
    for (int i = 0; roots[i]; i++) if (!strncmp(p, roots[i], strlen(roots[i]))) ok = 1;
    struct stat st;
    if (!ok || !strcmp(p, fl_core_so(&fl->game)) ||   /* the core is fingerprinted, not copied */
        stat(p, &st) != 0 || !S_ISREG(st.st_mode) || st.st_size > FL_FILE_CAP ||
        fl->filec >= FL_MAX_FILES || strlen(p) >= sizeof fl->file[0]) return;
    for (int i = 0; i < fl->filec; i++) if (!strcmp(fl->file[i], p)) return;
    snprintf(fl->file[fl->filec++], sizeof fl->file[0], "%s", p);
}
/* Absolute paths among a process's arguments (not the program, not the ROM);
   "--appendconfig a|b" names two. */
static void fl_arg_paths(const FlProc *p, int skip, void (*each)(void *, const char *), void *ctx) {
    for (int i = 1; i < p->argc; i++) {
        if (i == skip || p->argv[i][0] != '/') continue;
        char tmp[FL_LINE_MAX], *save = NULL;
        snprintf(tmp, sizeof tmp, "%s", p->argv[i]);
        for (char *part = strtok_r(tmp, "|", &save); part; part = strtok_r(NULL, "|", &save)) each(ctx, part);
    }
}
static void fl_add_file_cb(void *ctx, const char *p) { fl_add_file((FastLaunch *)ctx, p); }
static int fl_rom_index(const FastLaunch *fl) {
    for (int i = 0; i < fl->game.argc; i++) if (!strcmp(fl->game.argv[i], fl->rom)) return i;
    return -1;
}
static void fl_collect_files(FastLaunch *fl) {
    fl_arg_paths(&fl->game, fl_rom_index(fl), fl_add_file_cb, fl);
    for (int h = 0; h < fl->helperc; h++) fl_arg_paths(&fl->helper[h], -1, fl_add_file_cb, fl);
    /* RetroArch's config names the core-options file it reads next. */
    for (int i = 0; i < fl->filec; i++) {
        FILE *f = fopen(fl->file[i], "r");
        if (!f) continue;
        char line[1024];
        while (fgets(line, sizeof line, f)) {
            char *k = line;
            while (*k == ' ' || *k == '\t') k++;
            if (strncmp(k, "core_options_path", 17)) continue;
            char *q1 = strchr(k, '"'), *q2 = q1 ? strchr(q1 + 1, '"') : NULL;
            if (q1 && q2 && q2 > q1 + 1) { *q2 = 0; fl_add_file(fl, q1 + 1); }
        }
        fclose(f);
    }
}

static void fl_write_proc(FILE *f, const char *kind, const FlProc *p) {
    fprintf(f, "proc=%s\nexe=%s\ncwd=%s\n", kind, p->exe, p->cwd);
    for (int i = 0; i < p->argc; i++) fprintf(f, "arg=%s\n", p->argv[i]);
    for (int i = 0; i < p->envc; i++) fprintf(f, "env=%s\n", p->envp[i]);
}
static int fl_write_entry(const char *key, const FastLaunch *fl) {
    char path[600], tmp[620];
    fl_path(path, sizeof path, key, ".launch");
    snprintf(tmp, sizeof tmp, "%s.tmp", path);
    fl_mkdirs(path);
    FILE *f = fopen(tmp, "w");
    if (!f) return 0;
    fprintf(f, "SNAPFAST 6\ncore=%s\nrom=%s\ninputs=%s\ninputs2=%s\nsig=%s\n",
            fl->core, fl->rom, fl->inputs, fl->inputs2, fl->sig);
    fl_write_proc(f, "main", &fl->game);
    for (int h = 0; h < fl->helperc; h++) fl_write_proc(f, "helper", &fl->helper[h]);
    for (int i = 0; i < fl->filec; i++) fprintf(f, "file=%s\n", fl->file[i]);
    if (fclose(f) != 0 || rename(tmp, path) != 0) { unlink(tmp); return 0; }
    return 1;
}

static int fl_read_entry(const char *key, FastLaunch *fl) {
    memset(fl, 0, sizeof *fl);
    char path[600];
    fl_path(path, sizeof path, key, ".launch");
    FILE *f = fopen(path, "r");
    if (!f) return 0;
    char line[FL_LINE_MAX + 16];
    FlProc *cur = NULL;
    int ok = fgets(line, sizeof line, f) && !strcmp(line, "SNAPFAST 6\n");
    while (ok && fgets(line, sizeof line, f)) {
        size_t n = strlen(line);
        if (!n || line[n - 1] != '\n') { ok = 0; break; }
        line[--n] = 0;
        if (!strncmp(line, "core=", 5)) snprintf(fl->core, sizeof fl->core, "%s", line + 5);
        else if (!strncmp(line, "inputs2=", 8)) snprintf(fl->inputs2, sizeof fl->inputs2, "%s", line + 8);
        else if (!strncmp(line, "rom=", 4)) snprintf(fl->rom, sizeof fl->rom, "%s", line + 4);
        else if (!strncmp(line, "inputs=", 7)) snprintf(fl->inputs, sizeof fl->inputs, "%s", line + 7);
        else if (!strncmp(line, "sig=", 4)) snprintf(fl->sig, sizeof fl->sig, "%s", line + 4);
        else if (!strcmp(line, "proc=main")) cur = &fl->game;
        else if (!strcmp(line, "proc=helper")) {
            if (fl->helperc < FL_MAX_HELPERS) cur = &fl->helper[fl->helperc++];
            else ok = 0;
        }
        else if (!strncmp(line, "file=", 5)) {
            if (fl->filec < FL_MAX_FILES) snprintf(fl->file[fl->filec++], sizeof fl->file[0], "%s", line + 5);
            else ok = 0;
        }
        else if (!cur) ok = 0;
        else if (!strncmp(line, "exe=", 4)) snprintf(cur->exe, sizeof cur->exe, "%s", line + 4);
        else if (!strncmp(line, "cwd=", 4)) snprintf(cur->cwd, sizeof cur->cwd, "%s", line + 4);
        else if (!strncmp(line, "arg=", 4)) ok = fl_push(cur->argv, &cur->argc, FL_MAX_ARGS, line + 4);
        else if (!strncmp(line, "env=", 4)) ok = fl_push(cur->envp, &cur->envc, FL_MAX_ENV, line + 4);
    }
    fclose(f);
    for (int h = 0; ok && h < fl->helperc; h++) ok = fl->helper[h].argc > 0 && fl->helper[h].exe[0];
    if (!ok || !fl->game.argc || !fl->game.exe[0] || !fl->rom[0] || !fl->inputs[0] || !fl->core[0]) {
        fastlaunch_free(fl);
        return 0;
    }
    return 1;
}

static void fastlaunch_forget(const char *key) {
    char path[600], suffix[16];
    fl_path(path, sizeof path, key, ".launch");
    unlink(path);
    for (int i = 0; i < FL_MAX_FILES; i++) {
        snprintf(suffix, sizeof suffix, ".f%d", i);
        fl_path(path, sizeof path, key, suffix);
        unlink(path);
    }
    for (int i = 0; i < 3; i++) {
        snprintf(suffix, sizeof suffix, ".in%d", i);
        fl_path(path, sizeof path, key, suffix);
        unlink(path);
    }
}

static void fastlaunch_clear_all(void) {
    DIR *d = opendir(fl_cache_dir);
    if (!d) return;
    struct dirent *e;
    while ((e = readdir(d)) != NULL) {
        if (e->d_name[0] == '.') continue;
        char path[800];
        snprintf(path, sizeof path, "%s/%s", fl_cache_dir, e->d_name);
        unlink(path);
    }
    closedir(d);
}

static void fl_log_proc(const char *key, const char *kind, const FlProc *p) {
    char shown[700] = "";
    for (int i = 0; i < p->argc && strlen(shown) + 2 < sizeof shown; i++)
        snprintf(shown + strlen(shown), sizeof shown - strlen(shown), "%s%s", i ? " " : "", p->argv[i]);
    fl_log("%s for %s: %s", kind, key, shown);
}

/* Record the RetroArch Knulli just started (pid), with the helpers in the
   launcher's session (sid), as the recipe for this system and core. */
static int fl_capture(pid_t pid, pid_t sid, const char *sys, const char *core, const char *rom,
                      const char *sig, const char *inputs) {
    char key[160], why[300] = "", members[400] = "", stem[256], path[600], suffix[16];
    int has_evmapy = 0;
    fl_key(sys, core, key, sizeof key);
    if (!fast_launch_enabled || pid <= 0 || !rom || !rom[0]) return 0;
    FastLaunch fl;
    memset(&fl, 0, sizeof fl);
    snprintf(fl.rom, sizeof fl.rom, "%s", rom);
    fl_rom_stem(rom, stem, sizeof stem);
    int named = strlen(stem) >= 4, ok = 0, rom_index;
    char *buf = malloc(65536);
    if (!buf) { snprintf(why, sizeof why, "out of memory"); goto done; }

    if (!fl_read_proc(pid, &fl.game, buf, 65536, why, sizeof why)) goto done;
    const char *base = strrchr(fl.game.exe, '/');
    base = base ? base + 1 : fl.game.exe;
    if (fl_require_retroarch && strcmp(base, "retroarch")) { snprintf(why, sizeof why, "not RetroArch (%s)", base); goto done; }
    fl_log_proc(key, "command", &fl.game);
    if (fl_has_game_overrides(sys)) { snprintf(why, sizeof why, "%s has per-game settings", sys); goto done; }
    if (fl_rom_has_keys(rom)) { snprintf(why, sizeof why, "this game has its own key map"); goto done; }
    rom_index = fl_rom_index(&fl);
    if (rom_index < 0) { snprintf(why, sizeof why, "ROM not on the command line"); goto done; }
    for (int i = rom_index + 1; i < fl.game.argc; i++)
        if (!strcmp(fl.game.argv[i], rom)) { snprintf(why, sizeof why, "ROM named twice"); goto done; }

    int clean = fl_session_clean(sid, why, sizeof why, members, sizeof members);
    fl_log("session for %s: %s", key, members[0] ? members : "(none)");
    if (!clean) goto done;
    FlMember m[64];
    int mc = fl_session_members(sid, m, 64);
    for (int i = 0; i < mc; i++) {
        if (m[i].pid == pid || !fl_helper_replayable(m[i].comm)) continue;
        if (fl.helperc >= FL_MAX_HELPERS) { snprintf(why, sizeof why, "too many helpers"); goto done; }
        if (!fl_read_proc(m[i].pid, &fl.helper[fl.helperc++], buf, 65536, why, sizeof why)) goto done;
        if (!strcmp(m[i].comm, "evmapy")) has_evmapy = 1;
        fl_log_proc(key, "helper", &fl.helper[fl.helperc - 1]);
    }

    for (int i = 0; named && i < fl.game.argc; i++)
        if (i != rom_index && strstr(fl.game.argv[i], stem)) { snprintf(why, sizeof why, "an argument names this game"); goto done; }
    for (int h = 0; named && h < fl.helperc; h++)
        for (int i = 0; i < fl.helper[h].argc; i++)
            if (strstr(fl.helper[h].argv[i], stem)) { snprintf(why, sizeof why, "a helper argument names this game"); goto done; }
    fl_collect_files(&fl);
    if (has_evmapy) {
        /* evmapy takes no arguments; it reads every key map Knulli wrote here,
           and Knulli deletes them when the game ends. */
        int before = fl.filec;
        DIR *d = opendir(fl_evmapy_dir);
        if (d) {
            struct dirent *e;
            while ((e = readdir(d)) != NULL) {
                if (e->d_name[0] == '.') continue;
                snprintf(path, sizeof path, "%s/%s", fl_evmapy_dir, e->d_name);
                fl_add_file(&fl, path);
            }
            closedir(d);
        }
        fl_log("evmapy key maps for %s: %d in %s", key, fl.filec - before, fl_evmapy_dir);
        if (fl.filec == before) { snprintf(why, sizeof why, "evmapy's key maps were not found"); goto done; }
    }
    for (int i = 0; named && i < fl.filec; i++)
        if (fl_file_mentions(fl.file[i], stem)) { snprintf(why, sizeof why, "%s names this game", fl.file[i]); goto done; }
    snprintf(fl.sig, sizeof fl.sig, "%s", sig ? sig : "");
    snprintf(fl.inputs, sizeof fl.inputs, "%s", inputs ? inputs : "");
    snprintf(fl.inputs2, sizeof fl.inputs2, "%016llx", (unsigned long long)fl_inputs_hash());   /* as Knulli left them */
    fl_core_stamp(fl_core_so(&fl.game), fl.core, sizeof fl.core);

    fastlaunch_forget(key);
    fl_path(path, sizeof path, key, ".launch");
    fl_mkdirs(path);
    for (int i = 0; i < fl.filec; i++) {
        snprintf(suffix, sizeof suffix, ".f%d", i);
        fl_path(path, sizeof path, key, suffix);
        if (!fl_copy(fl.file[i], path)) { snprintf(why, sizeof why, "could not copy %s", fl.file[i]); fastlaunch_forget(key); goto done; }
    }
    if (!fl_write_entry(key, &fl)) { snprintf(why, sizeof why, "could not save"); fastlaunch_forget(key); goto done; }
    for (int i = 0; i < 3; i++) {   /* the settings files this launch started from */
        char from[600];
        snprintf(from, sizeof from, "%s/now.%d", fl_scratch_dir, i);
        snprintf(suffix, sizeof suffix, ".in%d", i);
        fl_path(path, sizeof path, key, suffix);
        if (!fl_copy(from, path)) unlink(path);
    }
    ok = 1;
done:
    if (ok) fl_log("recorded %s (%d args, %d helpers, %d configs)", key, fl.game.argc, fl.helperc, fl.filec);
    else fl_log("not recorded for %s: %s", key, why);
    free(buf);
    fastlaunch_free(&fl);
    return ok;
}

/* Record a session with the inputs as they are right now. */
static int fastlaunch_capture(pid_t pid, pid_t sid, const char *sys, const char *core, const char *rom) {
    fl_snapshot_inputs();
    return fl_capture(pid, sid, sys, core, rom, fl_now_sig, fl_now_inputs);
}

/* The launch in progress, when it went through Knulli and may be recorded,
   with the inputs as they were when it started. */
static int fl_pending = 0;
static pid_t fl_pending_sid = 0;
static char fl_pending_sys[64], fl_pending_core[128], fl_pending_rom[640];
static char fl_pending_sig[1536], fl_pending_inputs[20];
static char fl_captured_key[160];   /* re-checked a few seconds in */

static void fastlaunch_arm(const char *sys, const char *core, const char *rom, pid_t launcher) {
    fl_pending = fast_launch_enabled && launcher > 0;
    fl_pending_sid = launcher;
    fl_captured_key[0] = 0;
    snprintf(fl_pending_sys, sizeof fl_pending_sys, "%s", sys ? sys : "");
    snprintf(fl_pending_core, sizeof fl_pending_core, "%s", core ? core : "");
    snprintf(fl_pending_rom, sizeof fl_pending_rom, "%s", rom ? rom : "");
    snprintf(fl_pending_sig, sizeof fl_pending_sig, "%s", fl_now_sig);        /* from fastlaunch_prepare */
    snprintf(fl_pending_inputs, sizeof fl_pending_inputs, "%s", fl_now_inputs);
}
static void fastlaunch_on_ready(pid_t retroarch) {
    if (!fl_pending) return;
    fl_pending = 0;
    if (fl_capture(retroarch, fl_pending_sid, fl_pending_sys, fl_pending_core, fl_pending_rom,
                   fl_pending_sig, fl_pending_inputs))
        fl_key(fl_pending_sys, fl_pending_core, fl_captured_key, sizeof fl_captured_key);
}
/* A helper Knulli starts a little after RetroArch also rules the recording out. */
static void fastlaunch_recheck(void) {
    if (!fl_captured_key[0]) return;
    char why[300];
    if (!fl_session_clean(fl_pending_sid, why, sizeof why, NULL, 0)) {
        fastlaunch_forget(fl_captured_key);
        fl_log("dropped %s: %s", fl_captured_key, why);
    }
    fl_captured_key[0] = 0;
}

static int fl_bypass = 0;   /* the retry after a failed fast launch goes through Knulli */

/* A settings line as it may appear in the log: passwords and keys are hidden. */
static void fl_mask(const char *line, char *out, size_t cap) {
    const char *eq = strchr(line, '=');
    size_t kl = eq ? (size_t)(eq - line) : strlen(line);
    char name[256];
    if (kl >= sizeof name) kl = sizeof name - 1;
    for (size_t i = 0; i < kl; i++) name[i] = (char)tolower((unsigned char)line[i]);
    name[kl] = 0;
    int secret = strstr(name, "pass") || strstr(name, "token") || strstr(name, "secret") ||
                 strstr(name, "psk") || strstr(name, "api") ||
                 (kl >= 4 && !strcmp(name + kl - 4, ".key"));
    if (eq && secret) snprintf(out, cap, "%.*s=<hidden>", (int)kl, line);
    else snprintf(out, cap, "%s", line);
}
static int fl_lines(char *buf, char **lines, int max) {
    int n = 0;
    char *save = NULL;
    for (char *l = strtok_r(buf, "\n", &save); l && n < max; l = strtok_r(NULL, "\n", &save)) {
        size_t len = strlen(l);
        if (len && l[len - 1] == '\r') l[len - 1] = 0;
        lines[n++] = l;
    }
    return n;
}
static int fl_has_line(char **lines, int n, const char *l) {
    for (int i = 0; i < n; i++) if (!strcmp(lines[i], l)) return 1;
    return 0;
}
/* Which settings lines differ from the ones the recording was made with. */
static void fl_log_input_changes(const char *key) {
    const uint64_t basis = 1469598103934665603ULL;
    for (int i = 0; i < 3; i++) {
        char old_path[600], suffix[16], masked[320];
        snprintf(suffix, sizeof suffix, ".in%d", i);
        fl_path(old_path, sizeof old_path, key, suffix);
        const char *now_path = fl_input_path(i);
        if (fl_fnv_file(basis, old_path) == fl_fnv_file(basis, now_path)) continue;
        struct stat so, sn;
        if (stat(old_path, &so) != 0) { fl_log("%s changed (no earlier copy to compare)", fl_input_name(i)); continue; }
        if (stat(now_path, &sn) != 0) { fl_log("%s is gone", fl_input_name(i)); continue; }
        if (so.st_size > 512 * 1024 || sn.st_size > 512 * 1024) { fl_log("%s changed", fl_input_name(i)); continue; }
        char *a = malloc((size_t)so.st_size + 1), *b = malloc((size_t)sn.st_size + 1);
        char **la = malloc(4000 * sizeof *la), **lb = malloc(4000 * sizeof *lb);
        if (a && b && la && lb && fl_read_file(old_path, a, (size_t)so.st_size + 1) >= 0 &&
            fl_read_file(now_path, b, (size_t)sn.st_size + 1) >= 0) {
            int na = fl_lines(a, la, 4000), nb = fl_lines(b, lb, 4000), shown = 0, total = 0;
            for (int k = 0; k < nb; k++)
                if (!(i == 0 && fl_is_user_line(lb[k])) && !fl_has_line(la, na, lb[k]) && total++ >= 0 && shown++ < 8) {
                    fl_mask(lb[k], masked, sizeof masked);
                    fl_log("%s now has: %s", fl_input_name(i), masked);
                }
            for (int k = 0; k < na; k++)
                if (!(i == 0 && fl_is_user_line(la[k])) && !fl_has_line(lb, nb, la[k]) && total++ >= 0 && shown++ < 8) {
                    fl_mask(la[k], masked, sizeof masked);
                    fl_log("%s no longer has: %s", fl_input_name(i), masked);
                }
            fl_log("%s: %d changed lines%s", fl_input_name(i), total, total ? "" : " (same lines, new order)");
        }
        free(a); free(b); free(la); free(lb);
    }
}

typedef struct { FastLaunch *fl; char *why; size_t cap; } FlCheck;
static void fl_check_path_cb(void *ctx, const char *p) {
    FlCheck *c = ctx;
    if (c->why[0]) return;
    for (int k = 0; k < c->fl->filec; k++) if (!strcmp(c->fl->file[k], p)) return;   /* restored below */
    if (access(p, F_OK) != 0) snprintf(c->why, c->cap, "%s is missing", p);
}
static int fl_proc_runnable(const FlProc *p, char *why, size_t cap) {
    if (access(p->exe, X_OK) != 0) { snprintf(why, cap, "%s is missing", p->exe); return 0; }
    if (p->cwd[0] && access(p->cwd, X_OK) != 0) { snprintf(why, cap, "%s is missing", p->cwd); return 0; }
    return 1;
}

/* The last value a file gives for key: RetroArch "key = value" lines, or
   knulli.conf "global.retroarch.key=value" lines when prefix is given. */
static int fl_file_value(const char *path, const char *prefix, const char *key, char *out, size_t cap) {
    FILE *f = fopen(path, "r");
    if (!f) return 0;
    char line[2048], k[160], v[1024];
    size_t pl = strlen(prefix);
    int found = 0;
    while (fgets(line, sizeof line, f)) {
        const char *p = line;
        while (*p == ' ' || *p == '\t') p++;
        if (pl && strncmp(p, prefix, pl)) continue;
        if (fl_kv(p + pl, k, sizeof k, v, sizeof v) && !strcmp(k, key)) { snprintf(out, cap, "%s", v); found = 1; }
    }
    fclose(f);
    return found;
}

/* In-game preferences changed since the recording (a new save slot, say) go
   into the restored RetroArch config as Knulli's launcher would write them --
   but only where the recording shows Knulli passes that setting straight
   through. One Knulli sets itself sends the launch back through Knulli. */
static int fl_apply_user_settings(const char *key, const FastLaunch *fl, char *why, size_t cap) {
    enum { FL_MAX_PREFS = 64 };
    static char names[FL_MAX_PREFS][160], values[FL_MAX_PREFS][1024];
    const char *cfg = NULL;
    for (int i = 1; i + 1 < fl->game.argc; i++) if (!strcmp(fl->game.argv[i], "--config")) cfg = fl->game.argv[i + 1];
    if (!cfg || !fl_user_setting) return 1;
    char recorded[600], line[2048], k[160], v[1024], list[300] = "";
    fl_path(recorded, sizeof recorded, key, ".in0");
    FILE *f = fopen(fl_knulli_conf, "r");
    if (!f) return 1;
    int n = 0, patches = 0, patch[FL_MAX_PREFS] = {0}, written[FL_MAX_PREFS] = {0};
    while (fgets(line, sizeof line, f)) {
        if (!fl_is_user_line(line) || !fl_kv(line + 17, k, sizeof k, v, sizeof v)) continue;
        int j = 0;
        while (j < n && strcmp(names[j], k)) j++;
        if (j == n) { if (n == FL_MAX_PREFS) continue; n++; }
        snprintf(names[j], sizeof names[j], "%s", k);
        snprintf(values[j], sizeof values[j], "%s", v);
    }
    fclose(f);
    for (int j = 0; j < n; j++) {
        char snap[1024], rec[1024];
        int have_rec = fl_file_value(recorded, "global.retroarch.", names[j], rec, sizeof rec);
        if (have_rec && !strcmp(rec, values[j])) continue;                       /* unchanged since the recording */
        if (!fl_file_value(cfg, "", names[j], snap, sizeof snap)) continue;      /* Knulli never writes it */
        if (!strcmp(snap, values[j])) continue;                                  /* already what RetroArch gets */
        if (!have_rec || strcmp(rec, snap)) {                                    /* Knulli sets this one itself */
            snprintf(why, cap, "RetroArch setting %s changed", names[j]);
            return 0;
        }
        patch[j] = 1;
        patches++;
        size_t used = strlen(list);
        if (used + 2 < sizeof list) snprintf(list + used, sizeof list - used, "%s%s", used ? " " : "", names[j]);
    }
    if (!patches) return 1;
    char tmp[620];
    snprintf(tmp, sizeof tmp, "%s.snapfe-tmp", cfg);
    FILE *in = fopen(cfg, "r"), *out = in ? fopen(tmp, "w") : NULL;
    if (!in || !out) {
        if (in) fclose(in);
        snprintf(why, cap, "could not update %s", cfg);
        return 0;
    }
    while (fgets(line, sizeof line, in)) {
        int j = -1;
        if (fl_kv(line, k, sizeof k, v, sizeof v))
            for (int m = 0; m < n; m++) if (patch[m] && !strcmp(names[m], k)) j = m;
        if (j < 0) { fputs(line, out); continue; }
        if (!written[j]) fprintf(out, "%s = \"%s\"\n", names[j], values[j]);
        written[j] = 1;
    }
    fclose(in);
    if (fclose(out) != 0 || rename(tmp, cfg) != 0) {
        unlink(tmp);
        snprintf(why, cap, "could not update %s", cfg);
        return 0;
    }
    fl_log("applied RetroArch settings for %s: %s", key, list);
    return 1;
}

/* Load and check this system's recording, restore its configs and put the new
   ROM in. Returns 1 with fl ready to spawn; 0 means launch through Knulli. */
static int fastlaunch_prepare(const char *sys, const char *core, const char *rom, FastLaunch *fl) {
    char key[160], why[300] = "", core_now[64], path[600], suffix[16];
    fl_key(sys, core, key, sizeof key);
    memset(fl, 0, sizeof *fl);
    if (!fast_launch_enabled) return 0;
    fl_snapshot_inputs();   /* also what a recording of this launch will be checked against */
    if (fl_bypass) { fl_bypass = 0; fl_log("retrying %s through Knulli", key); return 0; }
    if (!fl_read_entry(key, fl)) { fl_log("no recording for %s yet", key); return 0; }
    int rom_index = fl_rom_index(fl);
    fl_core_stamp(fl_core_so(&fl->game), core_now, sizeof core_now);
    if (strcmp(fl_now_sig, fl->sig)) {
        snprintf(why, sizeof why, "controllers or input devices changed");
        fl_log("input devices recorded: %s", fl->sig);
        fl_log("input devices now: %s", fl_now_sig);
    }
    else if (strcmp(fl_now_inputs, fl->inputs) && strcmp(fl_now_inputs, fl->inputs2)) {
        snprintf(why, sizeof why, "knulli.conf or EmulationStation settings changed");
        fl_log_input_changes(key);
    }
    else if (strcmp(core_now, fl->core)) snprintf(why, sizeof why, "the core changed");
    else if (fl_has_game_overrides(sys)) snprintf(why, sizeof why, "%s has per-game settings", sys);
    else if (rom_index < 0) snprintf(why, sizeof why, "recording is incomplete");
    else if (!rom || access(rom, R_OK) != 0) snprintf(why, sizeof why, "ROM not readable");
    else if (fl_rom_has_keys(rom)) snprintf(why, sizeof why, "this game has its own key map");
    else if (fl_proc_runnable(&fl->game, why, sizeof why))
        for (int h = 0; h < fl->helperc && fl_proc_runnable(&fl->helper[h], why, sizeof why); h++) {}
    if (!why[0]) {
        FlCheck check = { fl, why, sizeof why };
        fl_arg_paths(&fl->game, rom_index, fl_check_path_cb, &check);
        for (int h = 0; h < fl->helperc; h++) fl_arg_paths(&fl->helper[h], -1, fl_check_path_cb, &check);
    }
    for (int i = 0; !why[0] && i < fl->filec; i++) {
        snprintf(suffix, sizeof suffix, ".f%d", i);
        fl_path(path, sizeof path, key, suffix);
        fl_mkdirs(fl->file[i]);
        if (!fl_copy(path, fl->file[i])) snprintf(why, sizeof why, "could not restore %s", fl->file[i]);
    }
    if (!why[0]) fl_apply_user_settings(key, fl, why, sizeof why);
    if (!why[0]) {
        char *copy = strdup(rom);
        if (!copy) snprintf(why, sizeof why, "out of memory");
        else { free(fl->game.argv[rom_index]); fl->game.argv[rom_index] = copy; }
    }
    if (why[0]) {
        fl_log("normal launch for %s: %s", key, why);
        fastlaunch_free(fl);
        return 0;
    }
    fl_log("fast launch for %s (%d helpers)", key, fl->helperc);
    return 1;
}

static pid_t fl_exec(const FlProc *p) {
    pid_t pid = fork();
    if (pid == 0) {
        setsid();
        /* Same as the launcher path: their chatter never reaches the SD card. */
        int null_out = open("/dev/null", O_WRONLY);
        if (null_out >= 0) {
            (void)dup2(null_out, STDOUT_FILENO);
            if (null_out != STDOUT_FILENO) close(null_out);
        }
        if (p->cwd[0]) (void)!chdir(p->cwd);
        execve(p->exe, p->argv, p->envp);
        _exit(127);
    }
    return pid;
}

/* Helpers started for the running fast launch, and the runtime files Knulli
   would have removed with them. */
static pid_t fl_helper_pids[FL_MAX_HELPERS];
static int fl_helper_count = 0;
static char fl_runtime_files[FL_MAX_FILES][512];
static int fl_runtime_count = 0;

static void fastlaunch_stop_helpers(void) {
    for (int i = 0; i < fl_helper_count; i++)
        if (fl_helper_pids[i] > 0) { kill(-fl_helper_pids[i], SIGTERM); kill(fl_helper_pids[i], SIGTERM); }
    for (int tries = 0; tries < 25; tries++) {
        int left = 0;
        for (int i = 0; i < fl_helper_count; i++) {
            if (fl_helper_pids[i] <= 0) continue;
            pid_t r = waitpid(fl_helper_pids[i], NULL, WNOHANG);
            if (r == fl_helper_pids[i] || (r < 0 && errno == ECHILD)) fl_helper_pids[i] = 0;
            else left++;
        }
        if (!left) break;
        usleep(20000);
    }
    for (int i = 0; i < fl_helper_count; i++)
        if (fl_helper_pids[i] > 0) {
            kill(-fl_helper_pids[i], SIGKILL);
            kill(fl_helper_pids[i], SIGKILL);
            waitpid(fl_helper_pids[i], NULL, 0);
            fl_helper_pids[i] = 0;
        }
    for (int i = 0; i < fl_runtime_count; i++) unlink(fl_runtime_files[i]);
    fl_helper_count = fl_runtime_count = 0;
}

static void fl_runtime_add(const char *p) {
    if (!strncmp(p, "/userdata/", 10) || fl_runtime_count >= FL_MAX_FILES) return;
    for (int k = 0; k < fl_runtime_count; k++) if (!strcmp(fl_runtime_files[k], p)) return;
    snprintf(fl_runtime_files[fl_runtime_count++], sizeof fl_runtime_files[0], "%s", p);
}
static void fl_runtime_cb(void *ctx, const char *p) {
    FastLaunch *fl = ctx;
    for (int k = 0; k < fl->filec; k++) if (!strcmp(fl->file[k], p)) fl_runtime_add(p);
}

/* Helpers first, as Knulli does, then RetroArch. */
static pid_t fastlaunch_spawn(FastLaunch *fl) {
    fastlaunch_stop_helpers();
    size_t dl = strlen(fl_evmapy_dir);
    for (int h = 0; h < fl->helperc; h++) {
        pid_t pid = fl_exec(&fl->helper[h]);
        if (pid > 0) fl_helper_pids[fl_helper_count++] = pid;
        fl_arg_paths(&fl->helper[h], -1, fl_runtime_cb, fl);
    }
    for (int k = 0; fl->helperc && k < fl->filec; k++)   /* evmapy's key maps go with it */
        if (!strncmp(fl->file[k], fl_evmapy_dir, dl) && fl->file[k][dl] == '/') fl_runtime_add(fl->file[k]);
    pid_t pid = fl_exec(&fl->game);
    if (pid < 0) fastlaunch_stop_helpers();
    return pid;
}

/* The running game was started fast; its helpers stop with it, and if it
   fails straight away the recording is forgotten and the main loop starts it
   again through Knulli. */
static int emu_fast = 0;
static Uint32 emu_fast_started = 0;
static int fl_retry = 0;
static char fl_retry_key[160], fl_retry_path[640], fl_retry_title[256], fl_retry_platform[64];

static void fastlaunch_started(const char *sys, const char *core, const char *path,
                               const char *title, const char *platform) {
    emu_fast = 1;
    emu_fast_started = SDL_GetTicks();
    fl_pending = 0;
    fl_captured_key[0] = 0;
    fl_key(sys, core, fl_retry_key, sizeof fl_retry_key);
    snprintf(fl_retry_path, sizeof fl_retry_path, "%s", path ? path : "");
    snprintf(fl_retry_title, sizeof fl_retry_title, "%s", title ? title : "");
    snprintf(fl_retry_platform, sizeof fl_retry_platform, "%s", platform ? platform : "");
}
static void fastlaunch_exited(int status_known, int status) {
    if (!emu_fast) return;
    emu_fast = 0;
    fastlaunch_stop_helpers();
    Uint32 ran = SDL_GetTicks() - emu_fast_started;
    int clean = status_known && WIFEXITED(status) && WEXITSTATUS(status) == 0;
    if (ran < 5000 && !clean) {
        fastlaunch_forget(fl_retry_key);
        fl_retry = 1;
        fl_log("%s ended after %u ms (status %d): forgotten, retrying through Knulli",
               fl_retry_key, ran, status_known ? status : -1);
    }
}
#endif
