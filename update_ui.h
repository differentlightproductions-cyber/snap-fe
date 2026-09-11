#ifndef SNAPFE_UPDATE_UI_H
#define SNAPFE_UPDATE_UI_H
/* Settings > Device > Check for Updates, and Roll Back Previous Update.

   snapfe_update.py does the work -- GitHub's latest.json, the download and its
   SHA-256, staging, the backup, the install -- and reports through a small
   status file; this is its screen. After an install or a rollback Snap FE
   saves and exits, and custom.sh starts whichever version is now in place. A
   freshly updated Snap FE confirms it started (upd_confirm_start); if it never
   gets that far, custom.sh puts the previous version back by itself. */

static char upd_status_dir[512] = "/tmp/snapfe-update";
static char upd_app_override[512] = "";   /* tests; otherwise sn_data_root() */

typedef struct {
    int active;
    pid_t pid;
    char mode[16];                 /* check / install / rollback */
    char state[24], message[256], latest[32];
    long long done, total, size;
    char notes[8192];
    int scroll, notes_lines;
    Uint32 polled, restart_at;
    int boot_confirmed, restart_requested;
} UpdUi;
static UpdUi upd;

static const char *upd_app(void) { return upd_app_override[0] ? upd_app_override : sn_data_root(); }
static int upd_state_is(const char *s) { return !strcmp(upd.state, s); }
static int upd_working(void) {
    return upd_state_is("starting") || upd_state_is("checking") || upd_state_is("downloading") ||
           upd_state_is("verifying") || upd_state_is("extracting") || upd_state_is("installing") ||
           upd_state_is("rolling-back");
}
/* Past the point where stopping could leave a half-changed installation. */
static int upd_committed(void) {
    return upd_state_is("verifying") || upd_state_is("extracting") || upd_state_is("installing") ||
           upd_state_is("rolling-back") || upd_state_is("installed") || upd_state_is("rolled-back");
}

static void upd_current_version(char *out, size_t cap) {
    const char *v = SNAPFE_VERSION;
    while (*v && !isdigit((unsigned char)*v)) v++;
    snprintf(out, cap, "%s", *v ? v : "0");
}

static void upd_set(const char *state, const char *message) {
    snprintf(upd.state, sizeof upd.state, "%s", state);
    snprintf(upd.message, sizeof upd.message, "%s", message);
}

/* The previous version the last update left behind, if there is one. */
static int upd_backup_available(char *version, size_t cap) {
    char path[700], text[512];
    snprintf(path, sizeof path, "%s/update/previous/backup.json", upd_app());
    FILE *f = fopen(path, "r");
    if (!f) return 0;
    size_t n = fread(text, 1, sizeof text - 1, f);
    fclose(f);
    text[n] = 0;
    if (version && cap) {
        version[0] = 0;
        const char *key = strstr(text, "\"version\"");
        const char *open = key ? strchr(key + 9, '"') : NULL;
        const char *close = open ? strchr(open + 1, '"') : NULL;
        if (close && (size_t)(close - open - 1) < cap) {
            memcpy(version, open + 1, (size_t)(close - open - 1));
            version[close - open - 1] = 0;
        }
    }
    return 1;
}

static void upd_read_status(void) {
    char path[600], line[600];
    snprintf(path, sizeof path, "%s/status", upd_status_dir);
    FILE *f = fopen(path, "r");
    if (!f) return;
    while (fgets(line, sizeof line, f)) {
        line[strcspn(line, "\r\n")] = 0;
        char *eq = strchr(line, '=');
        if (!eq) continue;
        *eq = 0;
        const char *v = eq + 1;
        if (!strcmp(line, "state")) snprintf(upd.state, sizeof upd.state, "%s", v);
        else if (!strcmp(line, "message")) snprintf(upd.message, sizeof upd.message, "%s", v);
        else if (!strcmp(line, "latest")) snprintf(upd.latest, sizeof upd.latest, "%s", v);
        else if (!strcmp(line, "done")) upd.done = atoll(v);
        else if (!strcmp(line, "total")) upd.total = atoll(v);
        else if (!strcmp(line, "size")) upd.size = atoll(v);
    }
    fclose(f);
    if (upd_state_is("available") && !upd.notes[0]) {
        snprintf(path, sizeof path, "%s/notes.txt", upd_status_dir);
        f = fopen(path, "r");
        if (f) {
            size_t n = fread(upd.notes, 1, sizeof upd.notes - 1, f);
            upd.notes[n] = 0;
            fclose(f);
        }
    }
}

static void upd_start(const char *mode) {
    char script[700], current[32], path[600];
    snprintf(script, sizeof script, "%s/snapfe_update.py", upd_app());
    upd_current_version(current, sizeof current);
    mkdir(upd_status_dir, 0755);
    snprintf(path, sizeof path, "%s/status", upd_status_dir);
    unlink(path);
    snprintf(path, sizeof path, "%s/notes.txt", upd_status_dir);
    unlink(path);
    snprintf(upd.mode, sizeof upd.mode, "%s", mode);
    upd.done = upd.total = upd.size = 0;
    upd.notes[0] = 0;
    upd.scroll = upd.notes_lines = 0;
    upd.restart_at = 0;
    upd.latest[0] = 0;
    upd_set("starting", !strcmp(mode, "rollback") ? "Restoring the previous version..." : "Checking for updates...");
    if (access(script, R_OK) != 0) {
        upd_set("error", "The updater is missing from this installation. Reinstall Snap FE from the release ZIP.");
        return;
    }
    pid_t pid = fork();
    if (pid == 0) {
        setsid();
        snprintf(path, sizeof path, "%s/updater.log", upd_status_dir);
        int log_fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        int null_fd = open("/dev/null", O_RDONLY);
        if (null_fd >= 0) dup2(null_fd, STDIN_FILENO);
        if (log_fd >= 0) { dup2(log_fd, STDOUT_FILENO); dup2(log_fd, STDERR_FILENO); }
        execlp("python3", "python3", script, mode, "--current", current, "--status-dir", upd_status_dir, (char *)NULL);
        _exit(127);
    }
    upd.pid = pid > 0 ? pid : 0;
    upd.polled = 0;
    if (pid < 0) upd_set("error", "The updater could not be started.");
}

static void upd_close(void) {
    if (upd.pid > 0) {
        kill(-upd.pid, SIGTERM);
        kill(upd.pid, SIGTERM);
        waitpid(upd.pid, NULL, 0);
        upd.pid = 0;
    }
    upd.active = 0;
}

static void upd_open_check(void) {
    upd.active = 1;
    upd_start("check");
}

static void upd_open_rollback(void) {
    char version[32] = "", text[256];
    upd.active = 1;
    upd.pid = 0;
    snprintf(upd.mode, sizeof upd.mode, "rollback");
    upd.notes[0] = 0;
    upd.notes_lines = 0;
    if (!upd_backup_available(version, sizeof version)) {
        upd_set("error", "There is no previous version to go back to.");
        return;
    }
    snprintf(upd.latest, sizeof upd.latest, "%s", version);
    snprintf(text, sizeof text, "Go back to Snap FE %s? Your settings, games and saves stay as they are.",
             version[0] ? version : "the previous version");
    upd_set("confirm-rollback", text);
}

/* Once a frame while the screen is up: follow the updater, and restart once an
   install or a rollback is in place. */
static void upd_tick(void) {
    if (!upd.active) return;
    Uint32 now = SDL_GetTicks();
    if (upd.pid > 0 && now - upd.polled >= 200) {
        upd.polled = now;
        upd_read_status();
        int status = 0;
        pid_t r = waitpid(upd.pid, &status, WNOHANG);
        if (r == upd.pid || (r < 0 && errno == ECHILD)) {
            upd.pid = 0;
            upd_read_status();
            if (upd_working())
                upd_set("error", upd_committed() ? "The updater stopped part-way. Use Roll Back Previous Update if Snap FE misbehaves."
                                                 : "The updater stopped unexpectedly. Nothing was changed.");
        }
    }
    if (!upd.restart_at && upd.pid == 0 && (upd_state_is("installed") || upd_state_is("rolled-back")))
        upd.restart_at = now + 2000;
    if (upd.restart_at && (Sint32)(now - upd.restart_at) >= 0) upd.restart_requested = 1;
}

static void upd_key(SDL_Keycode k) {
    if (k == SDLK_UP) { if (upd.scroll > 0) upd.scroll--; return; }
    if (k == SDLK_DOWN) { if (upd.scroll + 1 < upd.notes_lines) upd.scroll++; return; }
    int enter = k == SDLK_RETURN, back = k == SDLK_ESCAPE;
    if (!enter && !back) return;
    if (upd_committed()) return;                                   /* can't be stopped now */
    if (upd_state_is("available")) { if (enter) upd_start("install"); else upd_close(); return; }
    if (upd_state_is("confirm-rollback")) { if (enter) upd_start("rollback"); else upd_close(); return; }
    if (upd_state_is("error")) {
        if (enter && strcmp(upd.message, "There is no previous version to go back to."))
            upd_start(upd.mode[0] ? upd.mode : "check");
        else upd_close();
        return;
    }
    if (!upd_working()) { upd_close(); return; }                   /* up to date, notices */
    if (back) upd_close();                                         /* checking or downloading */
}

/* After an update: a few seconds of running prove the new version works, so
   the update is kept. Also reports an automatic rollback that custom.sh made
   because a new version never got this far. */
static void upd_confirm_start(Uint32 running_ms) {
    if (upd.boot_confirmed || running_ms < 8000) return;
    upd.boot_confirmed = 1;
    char path[700], failed[64] = "", installed[64] = "", text[256];
    snprintf(path, sizeof path, "%s/update/pending", upd_app());
    FILE *f = fopen(path, "r");
    if (f) {
        if (!fgets(installed, sizeof installed, f)) installed[0] = 0;
        fclose(f);
        installed[strcspn(installed, "\r\n")] = 0;
        unlink(path);
        snprintf(path, sizeof path, "%s/update/tries", upd_app());
        unlink(path);
        /* The update took: say so once, so an update is plain to see. */
        snprintf(text, sizeof text, "Snap FE %s is installed and running.", installed[0] ? installed : "update");
        upd.active = 1;
        upd.pid = 0;
        upd.mode[0] = 0;
        upd.notes[0] = 0;
        upd.notes_lines = 0;
        upd_set("notice", text);
    }
    snprintf(path, sizeof path, "%s/update/auto-rollback", upd_app());
    f = fopen(path, "r");
    if (!f) return;
    if (!fgets(failed, sizeof failed, f)) failed[0] = 0;
    fclose(f);
    unlink(path);
    failed[strcspn(failed, "\r\n")] = 0;
    snprintf(text, sizeof text, "Snap FE %s did not start properly, so the previous version was put back.",
             failed[0] ? failed : "(the update)");
    upd.active = 1;
    upd.pid = 0;
    upd.mode[0] = 0;
    upd.notes[0] = 0;
    upd.notes_lines = 0;
    upd_set("notice", text);
}

static int upd_line(SDL_Renderer *ren, TTF_Font *font, const char *text, SDL_Color color, int x, int y, int maxw) {
    if (!text || !text[0]) return 0;
    SDL_Texture *t = render_text_fit(ren, font, text, color, maxw);
    if (!t) return 0;
    int w, h;
    SDL_QueryTexture(t, NULL, NULL, &w, &h);
    SDL_RenderCopy(ren, t, NULL, &(SDL_Rect){x, y, w, h});
    return h;
}
static int upd_wrapped(SDL_Renderer *ren, TTF_Font *font, const char *text, SDL_Color color, int x, int y, int maxw) {
    char lines[MAX_LINES][128];
    int n = wrap_text(font, text, maxw, lines), used = 0;
    for (int i = 0; i < n; i++) used += upd_line(ren, font, lines[i], color, x, y + used, maxw) + 2;
    return used;
}

static void upd_render(SDL_Renderer *ren) {
    if (!upd.active) return;
    TTF_Font *head = font_label ? font_label : font_fixed;
    TTF_Font *body = font_small ? font_small : head;
    SDL_Color gold = {255, 225, 152, 255}, white = {245, 246, 250, 255}, red = {255, 160, 150, 255},
              dim = {170, 176, 190, 255};
    SDL_BlendMode old;
    SDL_GetRenderDrawBlendMode(ren, &old);
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(ren, 0, 0, 0, 190);
    SDL_RenderFillRect(ren, NULL);
    int w = WIN_W - 40 > 600 ? 600 : WIN_W - 40, h = WIN_H - 60;
    SDL_Rect box = {(WIN_W - w) / 2, (WIN_H - h) / 2, w, h};
    SDL_SetRenderDrawColor(ren, 22, 24, 32, 248);
    SDL_RenderFillRect(ren, &box);
    SDL_SetRenderDrawColor(ren, 231, 173, 70, 255);
    for (int b = 0; b < 2; b++) SDL_RenderDrawRect(ren, &(SDL_Rect){box.x + b, box.y + b, box.w - 2 * b, box.h - 2 * b});

    int x = box.x + 20, y = box.y + 16, maxw = box.w - 40, bottom = box.y + box.h - 44;
    int rollback = !strcmp(upd.mode, "rollback");
    y += upd_line(ren, head, rollback ? "Roll Back Previous Update" : upd_state_is("notice") ? "Snap FE Update" : "Check for Updates",
                  gold, x, y, maxw) + 12;
    y += upd_wrapped(ren, body, upd.message, upd_state_is("error") ? red : white, x, y, maxw) + 10;

    /* Progress: a bar, and megabytes while downloading. */
    if (upd_working() && upd.total > 0 && !upd_state_is("checking")) {
        float frac = (float)upd.done / (float)upd.total;
        if (frac < 0) frac = 0;
        if (frac > 1) frac = 1;
        SDL_Rect bar = {x, y, maxw, 14};
        SDL_SetRenderDrawColor(ren, 60, 64, 78, 255);
        SDL_RenderFillRect(ren, &bar);
        SDL_SetRenderDrawColor(ren, 231, 173, 70, 255);
        SDL_RenderFillRect(ren, &(SDL_Rect){bar.x, bar.y, (int)(bar.w * frac), bar.h});
        y += bar.h + 8;
        char amount[96];
        if (upd_state_is("downloading"))
            snprintf(amount, sizeof amount, "%.1f of %.1f MB", upd.done / 1048576.0, upd.total / 1048576.0);
        else
            snprintf(amount, sizeof amount, "%d%%", (int)(frac * 100.0f + 0.5f));
        y += upd_line(ren, body, amount, dim, x, y, maxw) + 8;
    }

    /* Release notes, scrollable. */
    if (upd_state_is("available") && upd.notes[0]) {
        if (upd.size > 0) {
            char size[64];
            snprintf(size, sizeof size, "Download: %.0f MB", upd.size / 1048576.0);
            y += upd_line(ren, body, size, dim, x, y, maxw) + 6;
        }
        int skip = TTF_FontLineSkip(body);
        if (skip < 12) skip = 12;
        char copy[sizeof upd.notes];
        snprintf(copy, sizeof copy, "%s", upd.notes);
        int line = 0;
        char *save = NULL;
        for (char *l = strtok_r(copy, "\n", &save); l; l = strtok_r(NULL, "\n", &save), line++) {
            l[strcspn(l, "\r")] = 0;
            if (line < upd.scroll || y + skip > bottom) continue;
            upd_line(ren, body, l[0] ? l : " ", white, x, y, maxw);
            y += skip;
        }
        upd.notes_lines = line;
    }

    const char *hint =
        upd_state_is("available")        ? "A Download and install     B Not now     Up/Down Notes" :
        upd_state_is("confirm-rollback") ? "A Roll back     B Cancel" :
        upd_state_is("error")            ? (strcmp(upd.message, "There is no previous version to go back to.") ? "A Try again     B Close" : "B Close") :
        upd_committed() && upd_working() ? "Please wait -- don't turn off the handheld" :
        upd_working()                    ? "B Cancel" :
        (upd_state_is("installed") || upd_state_is("rolled-back")) ? "" : "A / B Close";
    upd_line(ren, body, hint, gold, x, box.y + box.h - 32, maxw);
    SDL_SetRenderDrawBlendMode(ren, old);
}
#endif
