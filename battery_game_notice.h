#ifndef SNAPFE_BATTERY_GAME_NOTICE_H
#define SNAPFE_BATTERY_GAME_NOTICE_H

/* In-game recommendations are passive. Choices remain queued for the normal
 * frontend, whose A/B buttons cannot accidentally act on a running game. */
#define BATTERY_GAME_NOTICE_W 480
#define BATTERY_GAME_NOTICE_H 116
#define BATTERY_GAME_NOTICE_MS 6000u
static int battery_game_notice_active = 0;
static int battery_game_notice_last_percent = -1;
static Uint32 battery_game_notice_until = 0;

static void battery_game_notice_hide(void) {
    battery_game_notice_active = 0;
    battery_game_notice_last_percent = -1;
}

/* 0 hides, 1 retains the committed buffer, 2 redraws. The policy persists
 * whether each threshold has been announced; this state only times the OSD. */
static int battery_game_notice_update(Uint32 now, int pending, int due, int percent) {
    if (!pending || percent < 0 || percent > 100) {
        battery_game_notice_hide();
        return 0;
    }
    if (due) {
        battery_game_notice_active = 1;
        battery_game_notice_until = now + BATTERY_GAME_NOTICE_MS;
        battery_game_notice_last_percent = percent;
        return 2;
    }
    if (!battery_game_notice_active || SDL_TICKS_PASSED(now, battery_game_notice_until)) {
        battery_game_notice_hide();
        return 0;
    }
    if (percent != battery_game_notice_last_percent) {
        battery_game_notice_last_percent = percent;
        return 2;
    }
    return 1;
}

static int battery_game_notice_line(SDL_Surface *panel, TTF_Font *font,
                                    const char *text, int y, int maxh, SDL_Color color) {
    if (!font) return 0;
    SDL_Surface *line = TTF_RenderUTF8_Blended(font, text, color);
    if (!line) return 0;
    float scale = fminf(1.0f, fminf((panel->w - 32.0f) / line->w, (float)maxh / line->h));
    int w = (int)(line->w * scale), h = (int)(line->h * scale);
    SDL_Rect dest = {(panel->w-w)/2,y+(maxh-h)/2,w,h};
    int ok = SDL_BlitScaled(line,NULL,panel,&dest)==0;
    SDL_FreeSurface(line);
    return ok;
}

static SDL_Surface *battery_game_notice_surface(int percent) {
    SDL_Surface *panel = SDL_CreateRGBSurfaceWithFormat(0,BATTERY_GAME_NOTICE_W,
        BATTERY_GAME_NOTICE_H,32,SDL_PIXELFORMAT_ARGB8888);
    if (!panel) return NULL;
    SDL_FillRect(panel,NULL,SDL_MapRGBA(panel->format,22,24,32,248));
    Uint32 edge = SDL_MapRGBA(panel->format,231,173,70,255);
    SDL_FillRect(panel,&(SDL_Rect){0,0,panel->w,2},edge);
    SDL_FillRect(panel,&(SDL_Rect){0,panel->h-2,panel->w,2},edge);
    SDL_FillRect(panel,&(SDL_Rect){0,0,2,panel->h},edge);
    SDL_FillRect(panel,&(SDL_Rect){panel->w-2,0,2,panel->h},edge);
    char value[40];snprintf(value,sizeof value,"Battery is at %d%%.",percent);
    TTF_Font *font = font_label ? font_label : font_fixed;
    int ok = battery_game_notice_line(panel,font,value,10,26,(SDL_Color){255,225,152,255}) &&
        battery_game_notice_line(panel,font,"Power Save recommended.",43,24,(SDL_Color){245,246,250,255}) &&
        battery_game_notice_line(panel,font,"Choose when you return to SNAP.",78,22,(SDL_Color){209,213,224,255});
    if (!ok) {SDL_FreeSurface(panel);return NULL;}
    return panel;
}
#endif
