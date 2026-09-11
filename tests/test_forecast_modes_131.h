#ifndef SNAP_TEST_FORECAST_MODES_131_H
#define SNAP_TEST_FORECAST_MODES_131_H
/* The weather app's forecast strip reads the same fetch three ways -- the next
   hours, the next days, or those days folded into week averages -- switched
   with L1/R1. The point of the by-week view is comparison, so it only appears
   once there is a whole week to average. */

static void forecast_modes_fill(int hours, int days) {
    g_weather_hourly_count = 0; g_weather_daily_count = 0;
    for (int i = 0; i < hours && i < WEATHER_HOURLY_COUNT; i++) {
        snprintf(g_weather_hourly[i], 64, "%d PM %dF Clear %d%%", 1 + i % 12, 70 + i, i);
        g_weather_hourly_count++;
    }
    static const char *names[7] = {"Mon","Tue","Wed","Thu","Fri","Sat","Sun"};
    g_weather_daily_date_count = 0;
    for (int i = 0; i < days && i < WEATHER_DAILY_COUNT; i++) {
        /* Week one runs 80/60 with 10% rain, week two 60/40 with 50%, so an
           average that is really averaging comes out clearly different. */
        int hi = i < 7 ? 80 : 60, lo = i < 7 ? 60 : 40, pop = i < 7 ? 10 : 50;
        snprintf(g_weather_daily[i], 64, "%s %dF/%dF Cloudy %d%%", names[i % 7], hi, lo, pop);
        /* Sep 10 onwards, so a week label has real dates to span and the
           second week runs into October. */
        int day = 10 + i, mon = 9;
        if (day > 30) { day -= 30; mon = 10; }
        snprintf(g_weather_daily_date[i], 16, "2026-%02d-%02d", mon, day);
        g_weather_daily_date_count++;
        g_weather_daily_count++;
    }
}

static void test_forecast_modes_131(SDL_Renderer *ren) {
    int saved_mode = g_weather_fc_mode, saved_sel = weather_hourly_sel;
    int saved_h = g_weather_hourly_count, saved_d = g_weather_daily_count;
    AppState state = STATE_WEATHER;

    /* The provider now sends two weeks, which is what makes "by week" a
       comparison rather than a restatement of one week. */
    assert(WEATHER_DAILY_COUNT >= 14);

    forecast_modes_fill(18, 14);
    g_weather_fc_mode = WEATHER_FC_HOURLY; weather_hourly_sel = 0;
    assert(weather_fc_count(WEATHER_FC_HOURLY) == 18);
    assert(weather_fc_count(WEATHER_FC_DAILY) == 14);
    assert(weather_fc_count(WEATHER_FC_WEEKLY) == 2);

    /* R1 walks forward through the three modes and wraps. */
    weather_app_key(SDLK_e, &state); assert(g_weather_fc_mode == WEATHER_FC_DAILY);
    weather_app_key(SDLK_e, &state); assert(g_weather_fc_mode == WEATHER_FC_WEEKLY);
    weather_app_key(SDLK_e, &state); assert(g_weather_fc_mode == WEATHER_FC_HOURLY);
    weather_app_key(SDLK_q, &state); assert(g_weather_fc_mode == WEATHER_FC_WEEKLY);
    assert(state == STATE_WEATHER);          /* switching never leaves the app */

    /* Each mode names itself, so the strip's heading is never a lie. */
    assert(!strcmp(weather_fc_title(WEATHER_FC_HOURLY), "HOURLY"));
    assert(!strcmp(weather_fc_title(WEATHER_FC_DAILY), "DAILY"));
    assert(!strcmp(weather_fc_title(WEATHER_FC_WEEKLY), "BY WEEK"));

    /* A cursor from the eighteen-hour list cannot survive into a two-week one. */
    g_weather_fc_mode = WEATHER_FC_HOURLY; weather_hourly_sel = 15;
    weather_app_key(SDLK_e, &state);          /* -> daily, 14 rows */
    assert(weather_hourly_sel == 13);
    weather_app_key(SDLK_e, &state);          /* -> weekly, 2 rows */
    assert(weather_hourly_sel == 1);

    /* Rows come back in the shape the strip already knows how to split. */
    {
        char row[64], label[20], temp[24], rain[20];
        weather_fc_row(WEATHER_FC_DAILY, 0, row, sizeof row);
        weather_hourly_parts(row, label, sizeof label, temp, sizeof temp, rain, sizeof rain);
        assert(!strcmp(label, "Mon") && !strcmp(temp, "80/60") && !strcmp(rain, "10%"));

        /* Week one is the average of week one's days, not of everything. */
        /* A week is named by the days it actually covers, as one token so
           the strip can still split the row on spaces. */
        weather_fc_row(WEATHER_FC_WEEKLY, 0, row, sizeof row);
        assert(!strcmp(row, "Sep10-16 80/60 10%"));
        weather_fc_row(WEATHER_FC_WEEKLY, 1, row, sizeof row);
        assert(!strcmp(row, "Sep17-23 60/40 50%"));
        weather_hourly_parts(row, label, sizeof label, temp, sizeof temp, rain, sizeof rain);
        assert(!strcmp(label, "Sep17-23") && !strcmp(temp, "60/40"));
        /* A week that crosses a month says both months. */
        snprintf(g_weather_daily_date[7], 16, "2026-09-28");
        snprintf(g_weather_daily_date[13], 16, "2026-10-04");
        weather_fc_row(WEATHER_FC_WEEKLY, 1, row, sizeof row);
        assert(!strncmp(row, "Sep28-Oct4 ", 11));
        /* No dates from the provider falls back to a plain week number
           rather than printing a blank label. */
        g_weather_daily_date_count = 0;
        weather_fc_row(WEATHER_FC_WEEKLY, 1, row, sizeof row);
        assert(!strncmp(row, "Wk2 ", 4));
        forecast_modes_fill(18, 14);
        /* Out of range writes nothing rather than stale text. */
        row[0] = 'x'; weather_fc_row(WEATHER_FC_WEEKLY, 2, row, sizeof row);
        assert(!row[0]);
    }

    /* With only hourly back, L1/R1 has nowhere to go and stays put instead of
       parking on an empty strip. */
    forecast_modes_fill(18, 0);
    g_weather_fc_mode = WEATHER_FC_HOURLY; weather_hourly_sel = 0;
    weather_app_key(SDLK_e, &state); assert(g_weather_fc_mode == WEATHER_FC_HOURLY);

    /* Six days is not a week: daily is offered, by-week is not. */
    forecast_modes_fill(18, 6);
    assert(weather_fc_count(WEATHER_FC_WEEKLY) == 0);
    g_weather_fc_mode = WEATHER_FC_HOURLY;
    weather_app_key(SDLK_e, &state); assert(g_weather_fc_mode == WEATHER_FC_DAILY);
    weather_app_key(SDLK_e, &state); assert(g_weather_fc_mode == WEATHER_FC_HOURLY);

    /* Left/Right browse whichever list is showing, and the full screen picks up
       the same mode -- including L1/R1 while it is open. */
    forecast_modes_fill(18, 14);
    g_weather_fc_mode = WEATHER_FC_WEEKLY; weather_hourly_sel = 0;
    weather_app_sel = WEATHER_FOCUS_HOURLY;
    weather_app_key(SDLK_RIGHT, &state); assert(weather_hourly_sel == 1);
    weather_app_key(SDLK_RIGHT, &state); assert(weather_hourly_sel == 0);   /* wrapped at 2 */
    weather_app_key(SDLK_RETURN, &state); assert(state == STATE_WEATHER_HOURLY);
    weather_hourly_key(SDLK_q, &state); assert(g_weather_fc_mode == WEATHER_FC_DAILY);
    weather_hourly_key(SDLK_DOWN, &state); assert(weather_hourly_sel == 1);
    weather_hourly_key(SDLK_ESCAPE, &state); assert(state == STATE_WEATHER);

    /* All three draw, compact and full-screen. */
    for (int m = 0; m < WEATHER_FC_COUNT; m++) {
        g_weather_fc_mode = m; weather_hourly_sel = 0;
        weather_app_render(ren);
        capture(ren, m == WEATHER_FC_HOURLY ? "weather-forecast-hourly"
                   : m == WEATHER_FC_DAILY  ? "weather-forecast-daily"
                                            : "weather-forecast-weekly");
        weather_hourly_render(ren);
        capture(ren, m == WEATHER_FC_HOURLY ? "weather-forecast-full-hourly"
                   : m == WEATHER_FC_DAILY  ? "weather-forecast-full-daily"
                                            : "weather-forecast-full-weekly");
    }

    g_weather_fc_mode = saved_mode; weather_hourly_sel = saved_sel;
    g_weather_hourly_count = saved_h; g_weather_daily_count = saved_d;
    puts("PASS: L1/R1 reads the forecast as hours, days or week averages, and the cursor follows the shorter list");
}
#endif
