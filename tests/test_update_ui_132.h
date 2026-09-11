#ifndef SNAP_TEST_UPDATE_UI_132_H
#define SNAP_TEST_UPDATE_UI_132_H
/* Settings > Check for Updates: the screen follows snapfe_update.py's status
   file, offers the rollback only when there is something to go back to, never
   lets B interrupt an install, restarts once a new version is in place, and
   keeps an update only after the new version has run for a few seconds.
   (tests/test_snapfe_update.py covers the updater itself.) */

static void upd_test_write(const char *path, const char *text) {
    fl_mkdirs(path);
    FILE *f = fopen(path, "w");
    assert(f);
    fputs(text, f);
    assert(fclose(f) == 0);
}
static int upd_test_has_row(int want) {
    int rt[MAX_DEVICE_ROWS], ex[MAX_DEVICE_ROWS];
    int n = build_device_rows(rt, ex);
    for (int i = 0; i < n; i++) if (rt[i] == want) return 1;
    return 0;
}

static void test_update_ui_132(SDL_Renderer *ren) {
    char dir[] = "/tmp/snapfe-upd-XXXXXX";
    assert(mkdtemp(dir));
    char saved_status[512], saved_app[512], path[800], version[32];
    snprintf(saved_status, sizeof saved_status, "%s", upd_status_dir);
    snprintf(saved_app, sizeof saved_app, "%s", upd_app_override);
    snprintf(upd_status_dir, sizeof upd_status_dir, "%s/status", dir);
    snprintf(upd_app_override, sizeof upd_app_override, "%s/app", dir);
    mkdir(upd_status_dir, 0755);
    memset(&upd, 0, sizeof upd);

    /* The running version comes from the binary itself. */
    upd_current_version(version, sizeof version);
    assert(isdigit((unsigned char)version[0]) && strstr(SNAPFE_VERSION, version));

    /* No backup: Check for Updates is on the page, the rollback is not. */
    int keep_open = dev_grp_system_open;
    dev_grp_system_open = 1;
    assert(!upd_backup_available(NULL, 0));
    assert(upd_test_has_row(ROW_DEV_UPDATE) && !upd_test_has_row(ROW_DEV_ROLLBACK));
    upd_open_rollback();
    assert(upd.active && upd_state_is("error") && strstr(upd.message, "no previous version"));
    upd_key(SDLK_RETURN);
    assert(!upd.active);

    /* With one, the rollback row appears and asks before doing anything. */
    snprintf(path, sizeof path, "%s/update/previous/backup.json", upd_app_override);
    upd_test_write(path, "{\"version\": \"1.3.1\", \"to\": \"1.3.2\", \"created\": 1}\n");
    assert(upd_backup_available(version, sizeof version) && !strcmp(version, "1.3.1"));
    assert(upd_test_has_row(ROW_DEV_ROLLBACK));
    upd_open_rollback();
    assert(upd_state_is("confirm-rollback") && strstr(upd.message, "1.3.1"));
    upd_render(ren);
    upd_key(SDLK_ESCAPE);
    assert(!upd.active);
    dev_grp_system_open = keep_open;

    /* An update on offer: version, size and scrollable notes from the updater. */
    upd.active = 1;
    upd.pid = 0;
    snprintf(upd.mode, sizeof upd.mode, "check");
    upd.notes[0] = 0;
    snprintf(path, sizeof path, "%s/status", upd_status_dir);
    upd_test_write(path, "state=available\nlatest=9.9.9\nsize=5242880\n"
                         "message=Snap FE 9.9.9 is available (you have 1.3.2).\n");
    snprintf(path, sizeof path, "%s/notes.txt", upd_status_dir);
    upd_test_write(path, "NEW\n- One\n- Two\n");
    upd_read_status();
    assert(upd_state_is("available") && !strcmp(upd.latest, "9.9.9") && upd.size == 5242880);
    assert(strstr(upd.notes, "- Two"));
    upd_render(ren);
    assert(upd.notes_lines == 3);
    upd_key(SDLK_DOWN);
    assert(upd.scroll == 1);
    upd_key(SDLK_UP);
    assert(upd.scroll == 0);

    /* Downloading can be cancelled; installing cannot. */
    snprintf(path, sizeof path, "%s/status", upd_status_dir);
    upd_test_write(path, "state=downloading\ndone=1048576\ntotal=4194304\nmessage=Downloading Snap FE 9.9.9...\n");
    upd_read_status();
    assert(upd_working() && !upd_committed() && upd.total == 4194304);
    upd_render(ren);
    upd_test_write(path, "state=installing\ndone=3\ntotal=4\nmessage=Installing...\n");
    upd_read_status();
    assert(upd_committed());
    upd_key(SDLK_ESCAPE);
    assert(upd.active);
    upd_render(ren);

    /* Installed: Snap FE restarts two seconds later. */
    upd_test_write(path, "state=installed\nlatest=9.9.9\nmessage=Snap FE 9.9.9 is installed. Restarting...\n");
    upd_read_status();
    upd.restart_at = 0;
    upd_tick();
    assert(upd.restart_at && !upd.restart_requested);
    upd.restart_at = SDL_GetTicks() - 1;
    upd_tick();
    assert(upd.restart_requested);
    upd.restart_requested = 0;
    upd.active = 0;

    /* The new version keeps the update only once it has been up a few seconds. */
    char pending[800], tries[800];
    snprintf(pending, sizeof pending, "%s/update/pending", upd_app_override);
    snprintf(tries, sizeof tries, "%s/update/tries", upd_app_override);
    upd_test_write(pending, "9.9.9\n");
    upd_test_write(tries, "1\n");
    upd.boot_confirmed = 0;
    upd_confirm_start(4000);
    assert(access(pending, F_OK) == 0);
    upd_confirm_start(9000);
    assert(access(pending, F_OK) != 0 && access(tries, F_OK) != 0);
    assert(upd.active && upd_state_is("notice") && strstr(upd.message, "9.9.9 is installed and running"));
    upd_key(SDLK_RETURN);
    assert(!upd.active);

    /* ...and says so when custom.sh had to put the previous version back. */
    snprintf(path, sizeof path, "%s/update/auto-rollback", upd_app_override);
    upd_test_write(path, "9.9.9\n");
    upd.boot_confirmed = 0;
    upd_confirm_start(9000);
    assert(upd.active && upd_state_is("notice") && strstr(upd.message, "9.9.9"));
    assert(access(path, F_OK) != 0);
    upd_render(ren);
    upd_key(SDLK_RETURN);
    assert(!upd.active);

    memset(&upd, 0, sizeof upd);
    snprintf(upd_status_dir, sizeof upd_status_dir, "%s", saved_status);
    snprintf(upd_app_override, sizeof upd_app_override, "%s", saved_app);
    char rm[700];
    snprintf(rm, sizeof rm, "rm -rf '%s'", dir);
    assert(system(rm) == 0);
    puts("PASS: Check for Updates follows the updater, offers a rollback only when one exists, never interrupts an install, restarts into the new version and keeps it only after a good start");
}
#endif
