#ifndef SNAP_TEST_FAST_LAUNCH_131_H
#define SNAP_TEST_FAST_LAUNCH_131_H
/* Fast Game Launch records what Knulli's launcher started -- RetroArch and the
   evmapy helper beside it -- and replays it only when nothing that feeds the
   launcher has changed. The recording is taken from real processes here,
   exactly as on the device, just not a real RetroArch or evmapy. */

static void fl_test_write(const char *path, const char *text) {
    fl_mkdirs(path);
    FILE *f = fopen(path, "w");
    assert(f);
    fputs(text, f);
    assert(fclose(f) == 0);
}
static int fl_test_same(const char *path, const char *text) {
    char buf[2048];
    return fl_read_file(path, buf, sizeof buf) >= 0 && !strcmp(buf, text);
}

/* Stand-in for ra_user_setting_allowed, which only the device build has. */
static int fl_test_user_setting(const char *key) {
    return !strcmp(key, "state_slot") || !strcmp(key, "video_smooth");
}

static void test_fast_launch_131(void) {
    char dir[] = "/tmp/snapfe-fast-XXXXXX";
    assert(mkdtemp(dir));
    char saved_cache[512], saved_conf[512], saved_input[512], saved_settings[512], saved_evmapy[512], saved_scratch[512];
    snprintf(saved_evmapy, sizeof saved_evmapy, "%s", fl_evmapy_dir);
    snprintf(saved_scratch, sizeof saved_scratch, "%s", fl_scratch_dir);
    char saved_sys_scripts[512], saved_user_scripts[512];
    snprintf(saved_sys_scripts, sizeof saved_sys_scripts, "%s", fl_system_scripts);
    snprintf(saved_user_scripts, sizeof saved_user_scripts, "%s", fl_user_scripts);
    char saved_sys_scripts2[512];
    snprintf(saved_sys_scripts2, sizeof saved_sys_scripts2, "%s", fl_system_scripts2);
    snprintf(saved_cache, sizeof saved_cache, "%s", fl_cache_dir);
    snprintf(saved_conf, sizeof saved_conf, "%s", fl_knulli_conf);
    snprintf(saved_input, sizeof saved_input, "%s", fl_es_input);
    snprintf(saved_settings, sizeof saved_settings, "%s", fl_es_settings);
    int saved_enabled = fast_launch_enabled;
    fast_launch_enabled = 1;
    fl_require_retroarch = 0;

    char cfg[600], opts[600], core_so[600], rom1[600], rom2[600], evmapy[600], keymap[600], path[700];
    snprintf(fl_cache_dir, sizeof fl_cache_dir, "%s/cache", dir);
    snprintf(fl_knulli_conf, sizeof fl_knulli_conf, "%s/knulli.conf", dir);
    snprintf(fl_es_input, sizeof fl_es_input, "%s/es_input.cfg", dir);
    snprintf(fl_es_settings, sizeof fl_es_settings, "%s/es_settings.cfg", dir);
    snprintf(fl_evmapy_dir, sizeof fl_evmapy_dir, "%s/run/evmapy", dir);
    snprintf(fl_scratch_dir, sizeof fl_scratch_dir, "%s/scratch", dir);
    char saved_game_log[512];
    snprintf(saved_game_log, sizeof saved_game_log, "%s", fl_game_log);
    snprintf(fl_game_log, sizeof fl_game_log, "%s/game-fast.log", dir);
    snprintf(fl_system_scripts, sizeof fl_system_scripts, "%s/scripts/system", dir);
    snprintf(fl_user_scripts, sizeof fl_user_scripts, "%s/scripts/user", dir);
    snprintf(fl_system_scripts2, sizeof fl_system_scripts2, "%s/scripts/batocera-none", dir);
    /* Knulli's game scripts: one per folder, one a level down, and a note that
       isn't executable and must be left alone. Each logs how it was called. */
    char hooks[600], hook[700], hook_text[800];
    snprintf(hooks, sizeof hooks, "%s/hooks.log", dir);
    snprintf(hook, sizeof hook, "%s/10-loading-screen", fl_system_scripts);
    snprintf(hook_text, sizeof hook_text, "#!/bin/sh\necho \"system $*\" >> '%s'\n", hooks);
    fl_test_write(hook, hook_text);
    assert(chmod(hook, 0755) == 0);
    snprintf(hook, sizeof hook, "%s/device/20-deep", fl_user_scripts);
    snprintf(hook_text, sizeof hook_text, "#!/bin/sh\necho \"user $*\" >> '%s'\n", hooks);
    fl_test_write(hook, hook_text);
    assert(chmod(hook, 0755) == 0);
    snprintf(hook, sizeof hook, "%s/readme.txt", fl_user_scripts);
    fl_test_write(hook, "not a script\n");
    snprintf(cfg, sizeof cfg, "%s/configs/retroarchcustom.cfg", dir);
    snprintf(opts, sizeof opts, "%s/configs/core-options.cfg", dir);
    snprintf(core_so, sizeof core_so, "%s/cores/fake_libretro.so", dir);
    snprintf(rom1, sizeof rom1, "%s/roms/gba/First Game.gba", dir);
    snprintf(rom2, sizeof rom2, "%s/roms/gba/Second Game.gba", dir);
    snprintf(evmapy, sizeof evmapy, "%s/bin/evmapy", dir);
    snprintf(keymap, sizeof keymap, "%s/run/evmapy/event3.json", dir);
    fl_test_write(fl_knulli_conf, "global.retroarch.audio_mute_enable=false\ngba.ratio=4/3\n");
    fl_test_write(fl_es_input, "<inputList/>\n");
    fl_test_write(fl_es_settings, "<config/>\n");
    /* Knulli's per-launch bezel: an overlay config and its image in the runtime
       folder, named from RetroArch's config rather than on its command line. */
    char overlay_cfg[600], bezel[600], overlay_text[800];
    snprintf(overlay_cfg, sizeof overlay_cfg, "%s/run/bezel/overlay.cfg", dir);
    snprintf(bezel, sizeof bezel, "%s/run/bezel/gba-4_3_adapted.png", dir);
    snprintf(overlay_text, sizeof overlay_text, "overlays = 1\noverlay0_overlay = \"%s\"\n", bezel);
    fl_test_write(overlay_cfg, overlay_text);
    fl_test_write(bezel, "PNG");
    char cfg_text[900];
    snprintf(cfg_text, sizeof cfg_text, "video_driver = \"gl\"\ncore_options_path = \"%s\"\ninput_overlay = \"%s\"\n",
             opts, overlay_cfg);
    fl_test_write(cfg, cfg_text);
    fl_test_write(opts, "mgba_skip_bios = \"ON\"\n");
    fl_test_write(keymap, "{\"actions\": []}\n");
    fl_test_write(core_so, "core");
    fl_test_write(rom1, "rom one");
    fl_test_write(rom2, "rom two");
    fl_mkdirs(evmapy);
    assert(fl_copy("/bin/sh", evmapy) && chmod(evmapy, 0755) == 0);   /* a helper whose name is evmapy */

    assert(fl_helper_allowed("retroarch") && fl_helper_allowed("emulatorlaunche") && !fl_helper_allowed("python3"));
    assert(fl_helper_replayable("evmapy") && !fl_helper_replayable("hotkeygen") && !fl_helper_allowed("sleep"));
    char key[160];
    fl_key("gba", "", key, sizeof key);
    assert(!strcmp(key, "gba-default"));

    /* Stand-ins for what Knulli starts: RetroArch with the ROM on its command
       line and evmapy with its key map, one session, both waiting on a pipe. */
    int hold[2];
    assert(pipe(hold) == 0);
    char script[2000];
    snprintf(script, sizeof script, "'%s' -c 'read y <&3; :' evmapy & read x <&3; :", evmapy);   /* no arguments, like the real one */
    pid_t ra = fork();
    assert(ra >= 0);
    if (ra == 0) {
        setsid();
        dup2(hold[0], 3);
        close(hold[1]);
        setenv("SNAPFE_FAST_TEST", "kept", 1);
        if (setpriority(PRIO_PROCESS, 0, 3) != 0) _exit(126);   /* Knulli starts RetroArch at a set priority */
        execl("/bin/sh", "sh", "-c", script, "retroarch", "-L", core_so, "--config", cfg, rom1, (char *)NULL);
        _exit(127);
    }
    close(hold[0]);
    char members[400] = "", why[300];
    for (int i = 0; i < 300 && !strstr(members, "evmapy"); i++) {
        fl_session_clean(ra, why, sizeof why, members, sizeof members);
        if (!strstr(members, "evmapy")) usleep(10000);
    }
    assert(strstr(members, "evmapy"));

    /* Recorded: RetroArch, the helper, and the configs both read. */
    assert(fastlaunch_capture(ra, ra, "gba", "", rom1));
    FastLaunch fl;
    assert(fl_read_entry(key, &fl));
    assert(fl.helperc == 1 && fl.filec == 5);   /* config, core options, bezel overlay + image, key map */
    assert(fl.game.has_nice && fl.game.nice == 3);
    fastlaunch_free(&fl);

    /* Another system's launch rewrote the config and Knulli removed the key map;
       the replay puts both back and runs the new ROM with the recorded environment. */
    fl_test_write(cfg, "video_driver = \"other system\"\n");
    assert(unlink(keymap) == 0);
    assert(unlink(bezel) == 0 && unlink(overlay_cfg) == 0);   /* a reboot empties /tmp */
    assert(fastlaunch_prepare("gba", "", rom2, &fl));
    assert(fl_test_same(cfg, cfg_text) && access(keymap, R_OK) == 0);
    assert(fl_test_same(overlay_cfg, overlay_text) && access(bezel, R_OK) == 0);
    int has_rom2 = 0, has_rom1 = 0, has_env = 0;
    for (int i = 0; i < fl.game.argc; i++) { has_rom2 |= !strcmp(fl.game.argv[i], rom2); has_rom1 |= !strcmp(fl.game.argv[i], rom1); }
    for (int i = 0; i < fl.game.envc; i++) has_env |= !strcmp(fl.game.envp[i], "SNAPFE_FAST_TEST=kept");
    assert(has_rom2 && !has_rom1 && has_env && fl.helperc == 1);
    assert(fl.game.argv[fl.game.argc] == NULL && fl.game.envp[fl.game.envc] == NULL);

    /* Replayed for real: the helper starts, and stops with the game, taking the
       runtime key map with it as Knulli would. (Without the pipe they exit at once.) */
    pid_t game = fastlaunch_spawn(&fl);
    assert(game > 0 && fl_helper_count == 1 && fl_runtime_count == 1);
    fastlaunch_free(&fl);
    fastlaunch_started("gba", "", rom2, "Second Game", "gba");
    waitpid(game, NULL, 0);
    fastlaunch_exited(1, 0);
    assert(fl_helper_count == 0 && access(keymap, F_OK) != 0 && !fl_retry);

    /* Knulli's gameStart scripts ran before the game, system then user, and its
       gameStop scripts after it, user then system -- with the launcher's arguments. */
    char expected_hooks[3000], seen_hooks[3000];
    snprintf(expected_hooks, sizeof expected_hooks,
             "system gameStart gba libretro fake %s\nuser gameStart gba libretro fake %s\n"
             "user gameStop gba libretro fake %s\nsystem gameStop gba libretro fake %s\n",
             rom2, rom2, rom2, rom2);
    assert(fl_read_file(hooks, seen_hooks, sizeof seen_hooks) >= 0 && !strcmp(seen_hooks, expected_hooks));
    assert(unlink(hooks) == 0);

    /* Anything that feeds Knulli's launcher changing sends it back through Knulli. */
    fl_test_write(fl_knulli_conf, "global.retroarch.audio_mute_enable=false\ngba.ratio=16/9\n");
    assert(!fastlaunch_prepare("gba", "", rom2, &fl));
    fl_test_write(fl_knulli_conf, "global.retroarch.audio_mute_enable=false\ngba.ratio=4/3\n");
    assert(fastlaunch_prepare("gba", "", rom2, &fl));
    fastlaunch_free(&fl);
    assert(!fastlaunch_prepare("gba", "mgba", rom2, &fl));   /* another core has its own recording */

    /* Per-game settings or key maps, a missing ROM, the switch, and a retry never go fast. */
    fl_test_write(fl_knulli_conf, "global.retroarch.audio_mute_enable=false\ngba.ratio=4/3\ngba[\"Second Game.gba\"].ratio=1/1\n");
    assert(!fl_has_game_overrides("snes") && fl_has_game_overrides("gba"));
    fl_test_write(fl_knulli_conf, "global.retroarch.audio_mute_enable=false\ngba.ratio=4/3\n");
    snprintf(path, sizeof path, "%s.keys", rom2);
    fl_test_write(path, "{}\n");
    assert(!fastlaunch_prepare("gba", "", rom2, &fl));
    assert(unlink(path) == 0);
    snprintf(path, sizeof path, "%s/roms/gba/Missing.gba", dir);
    assert(!fastlaunch_prepare("gba", "", path, &fl));
    fast_launch_enabled = 0;
    assert(!fastlaunch_prepare("gba", "", rom2, &fl));
    fast_launch_enabled = 1;
    fl_bypass = 1;
    assert(!fastlaunch_prepare("gba", "", rom2, &fl) && !fl_bypass);

    /* A fast launch that fails at once forgets the recording and asks for a retry;
       one that exits cleanly (the player quit) keeps it. */
    fastlaunch_started("gba", "", rom2, "Second Game", "gba");
    fastlaunch_exited(1, 0);
    assert(!fl_retry && fastlaunch_prepare("gba", "", rom2, &fl));
    fastlaunch_free(&fl);
    fastlaunch_started("gba", "", rom2, "Second Game", "gba");
    fastlaunch_exited(1, 1 << 8);
    assert(fl_retry && !strcmp(fl_retry_path, rom2));
    fl_retry = 0;
    fl_path(path, sizeof path, key, ".launch");
    assert(access(path, F_OK) != 0);

    /* Inputs are read as the launch starts, not while the game runs (when evmapy
       has its own virtual device attached): a change mid-game doesn't spoil it. */
    fastlaunch_clear_all();
    assert(!fastlaunch_prepare("gba", "", rom1, &fl));
    fastlaunch_arm("gba", "", rom1, ra);
    fl_test_write(fl_es_settings, "<config mid-game/>\n");
    fastlaunch_on_ready(ra);
    fl_test_write(fl_es_settings, "<config/>\n");
    assert(fastlaunch_prepare("gba", "", rom2, &fl));
    fastlaunch_free(&fl);

    /* Knulli's launcher writing to knulli.conf during a launch leaves the state
       the next launch starts from: both before and after are accepted, a real
       change is not, and the recording kept a copy to say what changed. */
    const char *conf = "global.retroarch.audio_mute_enable=false\ngba.ratio=4/3\n";
    fastlaunch_clear_all();
    assert(!fastlaunch_prepare("gba", "", rom1, &fl));
    fastlaunch_arm("gba", "", rom1, ra);
    fl_test_write(fl_knulli_conf, "global.retroarch.audio_mute_enable=false\ngba.ratio=4/3\nglobal.knulli.wrote=1\n");
    fastlaunch_on_ready(ra);
    assert(fastlaunch_prepare("gba", "", rom2, &fl));
    fastlaunch_free(&fl);
    fl_test_write(fl_knulli_conf, conf);
    assert(fastlaunch_prepare("gba", "", rom2, &fl));
    fastlaunch_free(&fl);
    fl_path(path, sizeof path, key, ".in0");
    assert(fl_test_same(path, conf));
    fl_test_write(fl_knulli_conf, "global.retroarch.audio_mute_enable=false\ngba.ratio=16/9\n");
    assert(!fastlaunch_prepare("gba", "", rom2, &fl));
    fl_test_write(fl_knulli_conf, conf);
    char masked[128];
    fl_mask("wifi.key=hunter2", masked, sizeof masked);
    assert(!strcmp(masked, "wifi.key=<hidden>"));
    fl_mask("wifi.password=hunter2", masked, sizeof masked);
    assert(!strcmp(masked, "wifi.password=<hidden>"));
    fl_mask("global.retroarch.input_enable_hotkey_btn=12", masked, sizeof masked);
    assert(!strcmp(masked, "global.retroarch.input_enable_hotkey_btn=12"));

    /* evmapy is never replayed without the key maps it reads. */
    assert(unlink(keymap) == 0);
    assert(!fastlaunch_capture(ra, ra, "gba", "", rom1));
    fl_test_write(keymap, "{\"actions\": []}\n");

    /* A config that names the game is never recorded. */
    char named[900];
    snprintf(named, sizeof named, "%sinput_overlay = \"/bezels/First Game.png\"\n", cfg_text);
    fl_test_write(cfg, named);
    assert(!fastlaunch_capture(ra, ra, "gba", "", rom1));
    fl_test_write(cfg, cfg_text);
    assert(fastlaunch_capture(ra, ra, "gba", "", rom1));

    /* A snapshot gone missing falls back instead of replaying half a session. */
    fl_path(path, sizeof path, key, ".f0");
    assert(unlink(path) == 0);
    assert(!fastlaunch_prepare("gba", "", rom2, &fl));

    /* Turning the setting off clears every recording. */
    assert(fastlaunch_capture(ra, ra, "gba", "", rom1));
    fastlaunch_clear_all();
    fl_path(path, sizeof path, key, ".launch");
    assert(access(path, F_OK) != 0);

    /* In-game RetroArch preferences SNAP keeps in knulli.conf: a new save slot
       still launches fast, and RetroArch gets the new slot -- but only a
       setting Knulli is seen to pass straight through (smooth here it sets
       itself), and the set of preferences itself still has to match. */
    int (*saved_user_setting)(const char *) = fl_user_setting;
    fl_user_setting = fl_test_user_setting;
    fl_test_write(fl_knulli_conf, "gba.ratio=4/3\nglobal.retroarch.state_slot=\"1\"\nglobal.retroarch.video_smooth=\"false\"\n");
    char cfg_prefs[1200];
    snprintf(cfg_prefs, sizeof cfg_prefs, "%sstate_slot = \"1\"\nvideo_smooth = \"true\"\n", cfg_text);
    fl_test_write(cfg, cfg_prefs);
    fastlaunch_clear_all();
    assert(fastlaunch_capture(ra, ra, "gba", "", rom1));
    fl_test_write(fl_knulli_conf, "gba.ratio=4/3\nglobal.retroarch.state_slot=\"7\"\nglobal.retroarch.video_smooth=\"false\"\n");
    fl_test_write(cfg, "state_slot = \"3\"\n");   /* RetroArch rewrote its config on exit */
    assert(fastlaunch_prepare("gba", "", rom2, &fl));
    fastlaunch_free(&fl);
    char applied[4096];
    assert(fl_read_file(cfg, applied, sizeof applied) >= 0);
    assert(strstr(applied, "state_slot = \"7\"") && !strstr(applied, "state_slot = \"1\"") &&
           strstr(applied, "video_smooth = \"true\"") && strstr(applied, "core_options_path"));
    /* The same settings in another order, or with a key repeated where the last
       one is the same, are the same settings. */
    fl_test_write(fl_knulli_conf, "global.retroarch.video_smooth=\"false\"\ngba.ratio=1/1\n"
                                  "global.retroarch.state_slot=\"7\"\ngba.ratio=4/3\n");
    assert(fastlaunch_prepare("gba", "", rom2, &fl));
    fastlaunch_free(&fl);
    fl_test_write(fl_knulli_conf, "gba.ratio=4/3\ngba.ratio=1/1\nglobal.retroarch.state_slot=\"7\"\nglobal.retroarch.video_smooth=\"false\"\n");
    assert(!fastlaunch_prepare("gba", "", rom2, &fl));   /* here the last ratio is a different one */
    /* Volume and brightness presses land in knulli.conf too; they don't change
       the game session, so they never cost a slow launch. */
    fl_test_write(fl_knulli_conf, "gba.ratio=4/3\naudio.volume=30\ndisplay.brightness=80\n"
                                  "global.retroarch.state_slot=\"7\"\nglobal.retroarch.video_smooth=\"false\"\n");
    assert(fastlaunch_prepare("gba", "", rom2, &fl));
    fastlaunch_free(&fl);
    fl_test_write(fl_knulli_conf, "gba.ratio=4/3\nglobal.retroarch.state_slot=\"7\"\nglobal.retroarch.video_smooth=\"maybe\"\n");
    assert(!fastlaunch_prepare("gba", "", rom2, &fl));
    fl_test_write(fl_knulli_conf, "gba.ratio=4/3\nglobal.retroarch.state_slot=\"7\"\n");
    assert(!fastlaunch_prepare("gba", "", rom2, &fl));
    fl_user_setting = saved_user_setting;

    close(hold[1]);
    kill(-ra, SIGKILL);
    waitpid(ra, NULL, 0);
    snprintf(fl_cache_dir, sizeof fl_cache_dir, "%s", saved_cache);
    snprintf(fl_knulli_conf, sizeof fl_knulli_conf, "%s", saved_conf);
    snprintf(fl_es_input, sizeof fl_es_input, "%s", saved_input);
    snprintf(fl_es_settings, sizeof fl_es_settings, "%s", saved_settings);
    snprintf(fl_evmapy_dir, sizeof fl_evmapy_dir, "%s", saved_evmapy);
    snprintf(fl_scratch_dir, sizeof fl_scratch_dir, "%s", saved_scratch);
    snprintf(fl_game_log, sizeof fl_game_log, "%s", saved_game_log);
    snprintf(fl_system_scripts, sizeof fl_system_scripts, "%s", saved_sys_scripts);
    snprintf(fl_user_scripts, sizeof fl_user_scripts, "%s", saved_user_scripts);
    snprintf(fl_system_scripts2, sizeof fl_system_scripts2, "%s", saved_sys_scripts2);
    fast_launch_enabled = saved_enabled;
    fl_require_retroarch = 1;
    char rm[700];
    snprintf(rm, sizeof rm, "rm -rf '%s'", dir);
    assert(system(rm) == 0);
    puts("PASS: fast launch records Knulli's RetroArch session and its evmapy helper, replays both with a new ROM and their own configs, stops the helper with the game, and falls back on any changed input, per-game setting or key map, missing piece, early failure or when switched off");
}
#endif
