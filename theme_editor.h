#ifndef SNAPFE_THEME_EDITOR_H
#define SNAPFE_THEME_EDITOR_H

/*
 * Snap FE built-in and named custom theme colour editor.
 *
 * This is intentionally header-only: main.c owns Theme and includes this file
 * once, immediately after the built-in themes and THEME_RAINBOW are declared.
 * SDL2/SDL_ttf and the standard C headers used below are already included by
 * main.c. Built-in names/indices remain owned by main.c and are never changed
 * on disk. Named custom copies are appended after the built-ins.
 *
 * Controls while open:
 *   Up/Down      colour role
 *   L1/R1        R, G or B channel
 *   Left/Right   subtract/add the current RGB step
 *   L2/R2        previous/next curated palette colour
 *   X            RGB step: 1, 5 or 16
 *   Select       restore this theme's source built-in colours (preview only)
 *   Y            name a built-in copy / rename a custom theme
 *   A or B       review closing the editor
 *
 * Custom themes can also be deleted from the Settings theme row with Y.
 * Built-ins never expose that action.
 *
 * The close prompt deliberately matches the rest of Snap FE:
 *   A/X Save, Y Discard, B Cancel.
 */

#define THEME_EDITOR_MAX_THEMES 32
#define THEME_EDITOR_FILE_NAME "theme-colors.cfg"
#define THEME_EDITOR_FILE_VERSION 2
#define THEME_EDITOR_NAME_MAX 47

enum ThemeEditorRole {
    THEME_EDITOR_ROLE_BACKGROUND = 0,
    THEME_EDITOR_ROLE_TEXT,
    THEME_EDITOR_ROLE_DIM,
    THEME_EDITOR_ROLE_ACCENT1,
    THEME_EDITOR_ROLE_ACCENT2,
    THEME_EDITOR_ROLE_ACCENT3,
    THEME_EDITOR_ROLE_SELECTION,
    /* Exposed only for a theme with code-drawn colour effects.  It is an
       identity (white) tint by default, so built-in rendering is unchanged. */
    THEME_EDITOR_ROLE_EFFECT_TINT,
    THEME_EDITOR_ROLE_COUNT
};

enum ThemeEditorResult {
    THEME_EDITOR_RESULT_NONE       = 0,
    THEME_EDITOR_RESULT_CONSUMED   = 1 << 0,
    THEME_EDITOR_RESULT_CHANGED    = 1 << 1,
    THEME_EDITOR_RESULT_SAVED      = 1 << 2,
    THEME_EDITOR_RESULT_CLOSED     = 1 << 3,
    THEME_EDITOR_RESULT_NAME       = 1 << 4,
    THEME_EDITOR_RESULT_DELETED    = 1 << 5
};

typedef struct ThemeEditorState {
    Theme *themes;
    Theme defaults[THEME_EDITOR_MAX_THEMES];
    SDL_Color effect_tint[THEME_EDITOR_MAX_THEMES];
    SDL_Color default_effect_tint[THEME_EDITOR_MAX_THEMES];
    unsigned char has_special_effect[THEME_EDITOR_MAX_THEMES];
    int source_theme[THEME_EDITOR_MAX_THEMES];
    int legacy_map[THEME_EDITOR_MAX_THEMES];
    char names[THEME_EDITOR_MAX_THEMES][THEME_EDITOR_NAME_MAX + 1];
    int builtin_count;
    int capacity;
    int theme_count;
    int *theme_count_out;
    int initialized;

    int active;
    int close_prompt;
    int delete_prompt;
    int edit_theme;
    int selected_role;
    int selected_channel;
    int rgb_step_index;
    Theme backup;
    SDL_Color backup_effect_tint;
    int dirty;
    int name_request; /* 1 = rename current custom, 2 = save a new copy */
    int last_deleted_index;

    char path[768];
    char status[128];
} ThemeEditorState;

static ThemeEditorState g_theme_editor;

static const char *theme_editor_role_names[THEME_EDITOR_ROLE_COUNT] = {
    "Background", "Primary Text", "Muted Text", "Accent 1",
    "Accent 2", "Accent 3", "Selection", "Special Effect Tint"
};

static const int theme_editor_rgb_steps[] = { 1, 5, 16 };

/* A deliberately small, useful set. RGB editing remains available for exact
   colours; these are fast jumps, not a restriction on what can be entered. */
static const SDL_Color theme_editor_palette[] = {
    {   0,   0,   0, 255 }, {  18,  18,  22, 255 },
    {  48,  48,  56, 255 }, { 112, 112, 124, 255 },
    { 198, 200, 208, 255 }, { 245, 245, 245, 255 },
    { 244, 234, 214, 255 }, { 120,  60,  36, 255 },
    { 232,  64,  64, 255 }, { 255, 132,  40, 255 },
    { 244, 199,  54, 255 }, { 152, 206, 132, 255 },
    {  48, 168,  92, 255 }, {  42, 184, 176, 255 },
    {  60, 200, 232, 255 }, {  70, 120, 220, 255 },
    { 112,  72, 180, 255 }, { 180,  72, 210, 255 },
    { 230,  70, 150, 255 }, { 244, 188, 207, 255 }
};

#define THEME_EDITOR_PALETTE_COUNT \
    ((int)(sizeof theme_editor_palette / sizeof theme_editor_palette[0]))

static int theme_editor_color_equal(SDL_Color a, SDL_Color b) {
    return a.r == b.r && a.g == b.g && a.b == b.b;
}

static int theme_editor_clamp_byte(int v) {
    return v < 0 ? 0 : v > 255 ? 255 : v;
}

static void theme_editor_copy_colors(Theme *dst, const Theme *src) {
    if (!dst || !src) return;
    dst->bg = src->bg;
    dst->text = src->text;
    dst->dim = src->dim;
    dst->accent1 = src->accent1;
    dst->accent2 = src->accent2;
    dst->accent3 = src->accent3;
    dst->select_bg = src->select_bg;
}

static int theme_editor_name_equal(const char *a, const char *b) {
    if (!a || !b) return a == b;
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return 0;
        a++; b++;
    }
    return *a == *b;
}

static int theme_editor_clean_name(const char *in, char *out, size_t outsz) {
    if (!out || !outsz) return 0;
    size_t n = 0;
    int pending_space = 0;
    while (in && *in && isspace((unsigned char)*in)) in++;
    for (; in && *in && n + 1 < outsz; in++) {
        unsigned char c = (unsigned char)*in;
        if (c < 32 || c == 127) continue;
        if (isspace(c)) { pending_space = n > 0; continue; }
        if (pending_space && n + 1 < outsz) out[n++] = ' ';
        pending_space = 0;
        out[n++] = (char)c;
    }
    out[n] = '\0';
    return n > 0;
}

static void theme_editor_update_count(void) {
    ThemeEditorState *ed = &g_theme_editor;
    if (ed->theme_count_out) *ed->theme_count_out = ed->theme_count;
}

static Uint8 *theme_editor_channel(SDL_Color *c, int channel) {
    if (channel == 0) return &c->r;
    if (channel == 1) return &c->g;
    return &c->b;
}

static SDL_Color *theme_editor_theme_role(Theme *t, int role) {
    if (!t) return NULL;
    switch (role) {
        case THEME_EDITOR_ROLE_BACKGROUND: return &t->bg;
        case THEME_EDITOR_ROLE_TEXT:       return &t->text;
        case THEME_EDITOR_ROLE_DIM:        return &t->dim;
        case THEME_EDITOR_ROLE_ACCENT1:    return &t->accent1;
        case THEME_EDITOR_ROLE_ACCENT2:    return &t->accent2;
        case THEME_EDITOR_ROLE_ACCENT3:    return &t->accent3;
        case THEME_EDITOR_ROLE_SELECTION:  return &t->select_bg;
        default:                           return NULL;
    }
}

static SDL_Color *theme_editor_current_role(int theme_index, int role) {
    ThemeEditorState *ed = &g_theme_editor;
    if (!ed->initialized || theme_index < 0 || theme_index >= ed->theme_count) return NULL;
    if (role == THEME_EDITOR_ROLE_EFFECT_TINT) return &ed->effect_tint[theme_index];
    return theme_editor_theme_role(&ed->themes[theme_index], role);
}

static SDL_Color *theme_editor_default_role(int theme_index, int role) {
    ThemeEditorState *ed = &g_theme_editor;
    if (!ed->initialized || theme_index < 0 || theme_index >= ed->theme_count) return NULL;
    if (role == THEME_EDITOR_ROLE_EFFECT_TINT) return &ed->default_effect_tint[theme_index];
    return theme_editor_theme_role(&ed->defaults[theme_index], role);
}

static int theme_editor_role_count_for(int theme_index) {
    ThemeEditorState *ed = &g_theme_editor;
    if (ed->initialized && theme_index >= 0 && theme_index < ed->theme_count &&
        ed->has_special_effect[theme_index]) return THEME_EDITOR_ROLE_COUNT;
    return THEME_EDITOR_ROLE_EFFECT_TINT; /* seven ordinary Theme fields */
}

static int theme_editor_theme_is_default(int theme_index) {
    ThemeEditorState *ed = &g_theme_editor;
    if (!ed->initialized || theme_index < 0 || theme_index >= ed->theme_count) return 1;
    for (int role = 0; role < THEME_EDITOR_ROLE_EFFECT_TINT; role++) {
        SDL_Color *a = theme_editor_current_role(theme_index, role);
        SDL_Color *b = theme_editor_default_role(theme_index, role);
        if (!a || !b || !theme_editor_color_equal(*a, *b)) return 0;
    }
    return theme_editor_color_equal(ed->effect_tint[theme_index],
                                    ed->default_effect_tint[theme_index]);
}

static int theme_editor_is_custom(int theme_index) {
    ThemeEditorState *ed = &g_theme_editor;
    return ed->initialized && theme_index >= ed->builtin_count &&
           theme_index < ed->theme_count;
}

static int theme_editor_base_index(int theme_index) {
    ThemeEditorState *ed = &g_theme_editor;
    if (!ed->initialized || theme_index < 0 || theme_index >= ed->theme_count)
        return theme_index;
    return ed->source_theme[theme_index];
}

static int theme_editor_resolve_loaded_index(int theme_index) {
    ThemeEditorState *ed = &g_theme_editor;
    if (!ed->initialized) return theme_index;
    if (theme_index >= 0 && theme_index < ed->builtin_count &&
        ed->legacy_map[theme_index] >= ed->builtin_count)
        return ed->legacy_map[theme_index];
    return (theme_index >= 0 && theme_index < ed->theme_count) ? theme_index : 0;
}

static void theme_editor_refresh_dirty(void) {
    ThemeEditorState *ed = &g_theme_editor;
    if (!ed->active) { ed->dirty = 0; return; }
    Theme *t = &ed->themes[ed->edit_theme];
    ed->dirty = !theme_editor_color_equal(t->bg,        ed->backup.bg) ||
                !theme_editor_color_equal(t->text,      ed->backup.text) ||
                !theme_editor_color_equal(t->dim,       ed->backup.dim) ||
                !theme_editor_color_equal(t->accent1,   ed->backup.accent1) ||
                !theme_editor_color_equal(t->accent2,   ed->backup.accent2) ||
                !theme_editor_color_equal(t->accent3,   ed->backup.accent3) ||
                !theme_editor_color_equal(t->select_bg, ed->backup.select_bg) ||
                !theme_editor_color_equal(ed->effect_tint[ed->edit_theme],
                                          ed->backup_effect_tint);
}

static int theme_editor_next_int(char **cursor, int *out) {
    char *p = *cursor;
    while (*p && isspace((unsigned char)*p)) p++;
    if (!*p || *p == '#') return 0;
    errno = 0;
    char *end = NULL;
    long v = strtol(p, &end, 10);
    if (errno || end == p || v < -2147483647L || v > 2147483647L) return 0;
    *out = (int)v;
    *cursor = end;
    return 1;
}

static int theme_editor_next_word(char **cursor, char *out, size_t outsz) {
    char *p = *cursor;
    size_t n = 0;
    while (*p && isspace((unsigned char)*p)) p++;
    if (!*p || *p == '#') return 0;
    while (*p && !isspace((unsigned char)*p) && *p != '#') {
        if (n + 1 < outsz) out[n++] = *p;
        p++;
    }
    if (outsz) out[n] = '\0';
    *cursor = p;
    return n > 0;
}

static int theme_editor_hex_nibble(int c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static int theme_editor_decode_name(const char *hex, char *out, size_t outsz) {
    size_t hn = hex ? strlen(hex) : 0;
    if (!out || outsz < 2 || !hn || (hn & 1)) return 0;
    size_t n = 0;
    for (size_t i = 0; i + 1 < hn && n + 1 < outsz; i += 2) {
        int hi = theme_editor_hex_nibble((unsigned char)hex[i]);
        int lo = theme_editor_hex_nibble((unsigned char)hex[i + 1]);
        if (hi < 0 || lo < 0 || (hi == 0 && lo == 0)) return 0;
        out[n++] = (char)((hi << 4) | lo);
    }
    out[n] = '\0';
    return theme_editor_clean_name(out, out, outsz);
}

static void theme_editor_encode_name(const char *name, char *out, size_t outsz) {
    static const char hex[] = "0123456789ABCDEF";
    size_t n = 0;
    for (const unsigned char *p = (const unsigned char *)name;
         p && *p && n + 2 < outsz; p++) {
        out[n++] = hex[*p >> 4];
        out[n++] = hex[*p & 15];
    }
    if (outsz) out[n] = '\0';
}

static int theme_editor_name_exists(const char *name, int ignore_index) {
    ThemeEditorState *ed = &g_theme_editor;
    for (int i = 0; i < ed->theme_count; i++) {
        if (i == ignore_index || !ed->themes[i].name) continue;
        if (theme_editor_name_equal(name, ed->themes[i].name)) return 1;
    }
    return 0;
}

static void theme_editor_unique_name(const char *wanted, char *out, size_t outsz) {
    char base[THEME_EDITOR_NAME_MAX + 1];
    if (!theme_editor_clean_name(wanted, base, sizeof base))
        snprintf(base, sizeof base, "Custom Theme");
    snprintf(out, outsz, "%s", base);
    for (int n = 2; theme_editor_name_exists(out, -1); n++) {
        char suffix[16];
        snprintf(suffix, sizeof suffix, " %d", n);
        int keep = THEME_EDITOR_NAME_MAX - (int)strlen(suffix);
        if (keep < 1) keep = 1;
        snprintf(out, outsz, "%.*s%s", keep, base, suffix);
    }
}

static int theme_editor_read_rgb(char **cursor,
                                 int rgb[THEME_EDITOR_ROLE_COUNT][3]) {
    int good = 1;
    for (int role = 0; good && role < THEME_EDITOR_ROLE_COUNT; role++) {
        for (int ch = 0; ch < 3; ch++) {
            good = theme_editor_next_int(cursor, &rgb[role][ch]);
            if (good && (rgb[role][ch] < 0 || rgb[role][ch] > 255)) good = 0;
        }
    }
    return good;
}

static void theme_editor_apply_rgb(int idx,
                                   int rgb[THEME_EDITOR_ROLE_COUNT][3]) {
    for (int role = 0; role < THEME_EDITOR_ROLE_COUNT; role++) {
        SDL_Color *c = theme_editor_current_role(idx, role);
        if (!c) continue;
        c->r = (Uint8)rgb[role][0];
        c->g = (Uint8)rgb[role][1];
        c->b = (Uint8)rgb[role][2];
        c->a = 255;
    }
}

static int theme_editor_append_loaded(int source, const char *name,
                                      int rgb[THEME_EDITOR_ROLE_COUNT][3]) {
    ThemeEditorState *ed = &g_theme_editor;
    if (source < 0 || source >= ed->builtin_count ||
        ed->theme_count >= ed->capacity) return -1;
    int idx = ed->theme_count++;
    ed->themes[idx] = ed->defaults[source];
    ed->defaults[idx] = ed->defaults[source];
    theme_editor_unique_name(name, ed->names[idx], sizeof ed->names[idx]);
    ed->themes[idx].name = ed->names[idx];
    ed->defaults[idx].name = ed->names[idx];
    ed->source_theme[idx] = source;
    ed->has_special_effect[idx] = ed->has_special_effect[source];
    ed->effect_tint[idx] = ed->default_effect_tint[source];
    ed->default_effect_tint[idx] = ed->default_effect_tint[source];
    theme_editor_apply_rgb(idx, rgb);
    theme_editor_update_count();
    return idx;
}

static int theme_editor_load_file(void) {
    ThemeEditorState *ed = &g_theme_editor;
    FILE *f = fopen(ed->path, "r");
    if (!f) return errno == ENOENT ? 1 : 0;

    char line[1024];
    while (fgets(line, sizeof line, f)) {
        char *p = line;
        while (*p && isspace((unsigned char)*p)) p++;
        if (strncmp(p, "custom", 6) == 0 && isspace((unsigned char)p[6])) {
            p += 6;
            int stored_idx = -1, source = -1;
            char encoded[THEME_EDITOR_NAME_MAX * 2 + 4];
            char name[THEME_EDITOR_NAME_MAX + 1];
            int rgb[THEME_EDITOR_ROLE_COUNT][3];
            int good = theme_editor_next_int(&p, &stored_idx) &&
                       theme_editor_next_int(&p, &source) &&
                       theme_editor_next_word(&p, encoded, sizeof encoded) &&
                       theme_editor_decode_name(encoded, name, sizeof name) &&
                       theme_editor_read_rgb(&p, rgb);
            /* Saved files are contiguous, which keeps theme_idx stable. A
               damaged/missing line is skipped instead of exposing a blank slot. */
            if (good && stored_idx == ed->theme_count)
                theme_editor_append_loaded(source, name, rgb);
        } else if (strncmp(p, "theme", 5) == 0 && isspace((unsigned char)p[5])) {
            /* Version-1 compatibility: old files overrode a built-in index.
               Import each override as a named custom copy and leave the
               built-in untouched. legacy_map keeps a selected old index aimed
               at the imported copy for this boot and after the next save. */
            p += 5;
            int old_idx = -1;
            int rgb[THEME_EDITOR_ROLE_COUNT][3];
            int good = theme_editor_next_int(&p, &old_idx) &&
                       theme_editor_read_rgb(&p, rgb);
            if (good && old_idx >= 0 && old_idx < ed->builtin_count &&
                ed->theme_count < ed->capacity) {
                char wanted[THEME_EDITOR_NAME_MAX + 1];
                snprintf(wanted, sizeof wanted, "%.35s Custom",
                         ed->themes[old_idx].name ? ed->themes[old_idx].name : "Theme");
                ed->legacy_map[old_idx] =
                    theme_editor_append_loaded(old_idx, wanted, rgb);
            }
        }
    }
    int ok = !ferror(f);
    fclose(f);
    return ok;
}

static int theme_editor_save_file(void) {
    ThemeEditorState *ed = &g_theme_editor;
    char tmp[800];
    snprintf(tmp, sizeof tmp, "%.767s.tmp", ed->path);
    FILE *f = fopen(tmp, "w");
    if (!f) {
        snprintf(ed->status, sizeof ed->status, "Could not save colour overrides");
        return 0;
    }

    fprintf(f, "SNAPFE_THEME_COLORS %d\n", THEME_EDITOR_FILE_VERSION);
    fprintf(f, "# custom index, source built-in, hex name, then 8 RGB colour roles\n");
    for (int i = ed->builtin_count; i < ed->theme_count; i++) {
        char encoded[THEME_EDITOR_NAME_MAX * 2 + 4];
        theme_editor_encode_name(ed->themes[i].name, encoded, sizeof encoded);
        fprintf(f, "custom %d %d %s", i, ed->source_theme[i], encoded);
        for (int role = 0; role < THEME_EDITOR_ROLE_COUNT; role++) {
            SDL_Color *c = theme_editor_current_role(i, role);
            fprintf(f, " %u %u %u", (unsigned)c->r, (unsigned)c->g, (unsigned)c->b);
        }
        fprintf(f, "\n");
    }

    int ok = !ferror(f);
    if (fflush(f) != 0) ok = 0;
    if (fclose(f) != 0) ok = 0;
    if (!ok || rename(tmp, ed->path) != 0) {
        remove(tmp);
        snprintf(ed->status, sizeof ed->status, "Could not save colour overrides");
        return 0;
    }
    ed->status[0] = '\0';
    return 1;
}

/* Move one populated custom slot while repairing the Theme::name pointers.
   Custom themes live in one compact array and one v2 config file; there are
   no per-theme files to orphan when an entry is removed. */
static void theme_editor_move_slot(int dst, int src) {
    ThemeEditorState *ed = &g_theme_editor;
    if (dst < 0 || src < 0 || dst >= ed->capacity || src >= ed->capacity) return;
    ed->themes[dst] = ed->themes[src];
    ed->defaults[dst] = ed->defaults[src];
    ed->effect_tint[dst] = ed->effect_tint[src];
    ed->default_effect_tint[dst] = ed->default_effect_tint[src];
    ed->has_special_effect[dst] = ed->has_special_effect[src];
    ed->source_theme[dst] = ed->source_theme[src];
    snprintf(ed->names[dst], sizeof ed->names[dst], "%s", ed->names[src]);
    ed->themes[dst].name = ed->names[dst];
    ed->defaults[dst].name = ed->names[dst];
}

static void theme_editor_clear_slot(int idx) {
    ThemeEditorState *ed = &g_theme_editor;
    if (idx < 0 || idx >= ed->capacity) return;
    memset(&ed->themes[idx], 0, sizeof ed->themes[idx]);
    memset(&ed->defaults[idx], 0, sizeof ed->defaults[idx]);
    ed->names[idx][0] = '\0';
    ed->effect_tint[idx] = (SDL_Color){ 255, 255, 255, 255 };
    ed->default_effect_tint[idx] = ed->effect_tint[idx];
    ed->has_special_effect[idx] = 0;
    ed->source_theme[idx] = idx;
}

/* Delete and compact transactionally. If persistence fails, restore both the
   in-memory entry and every shifted slot so the caller can safely stay in the
   editor and try again. */
static int theme_editor_delete_custom_now(int idx) {
    ThemeEditorState *ed = &g_theme_editor;
    if (!theme_editor_is_custom(idx)) return 0;

    Theme removed_theme = ed->themes[idx];
    Theme removed_default = ed->defaults[idx];
    SDL_Color removed_tint = ed->effect_tint[idx];
    SDL_Color removed_default_tint = ed->default_effect_tint[idx];
    unsigned char removed_special = ed->has_special_effect[idx];
    int removed_source = ed->source_theme[idx];
    int old_legacy[THEME_EDITOR_MAX_THEMES];
    char removed_name[THEME_EDITOR_NAME_MAX + 1];
    memcpy(old_legacy, ed->legacy_map, sizeof old_legacy);
    snprintf(removed_name, sizeof removed_name, "%s", ed->names[idx]);

    int old_count = ed->theme_count;
    for (int i = idx; i + 1 < old_count; i++)
        theme_editor_move_slot(i, i + 1);
    ed->theme_count = old_count - 1;
    theme_editor_clear_slot(ed->theme_count);

    /* A v1 import can leave a boot-only built-in -> custom redirect. Keep it
       valid until the next boot even after compacting the custom array. */
    for (int i = 0; i < ed->builtin_count; i++) {
        if (ed->legacy_map[i] == idx) ed->legacy_map[i] = -1;
        else if (ed->legacy_map[i] > idx) ed->legacy_map[i]--;
    }
    theme_editor_update_count();

    if (!theme_editor_save_file()) {
        for (int i = ed->theme_count; i > idx; i--)
            theme_editor_move_slot(i, i - 1);
        ed->theme_count = old_count;
        ed->themes[idx] = removed_theme;
        ed->defaults[idx] = removed_default;
        snprintf(ed->names[idx], sizeof ed->names[idx], "%s", removed_name);
        ed->themes[idx].name = ed->names[idx];
        ed->defaults[idx].name = ed->names[idx];
        ed->effect_tint[idx] = removed_tint;
        ed->default_effect_tint[idx] = removed_default_tint;
        ed->has_special_effect[idx] = removed_special;
        ed->source_theme[idx] = removed_source;
        memcpy(ed->legacy_map, old_legacy, sizeof old_legacy);
        theme_editor_update_count();
        return 0;
    }

    ed->last_deleted_index = idx;
    return 1;
}

/* Any stored selection above a removed compact slot moves down one. A stored
   reference to the deleted theme returns to SNAP FE's first built-in theme. */
static int theme_editor_adjust_index_after_delete(int value, int deleted_index) {
    ThemeEditorState *ed = &g_theme_editor;
    if (value == deleted_index) return 0;
    if (value > deleted_index) value--;
    return (value >= 0 && value < ed->theme_count) ? value : 0;
}

static int theme_editor_last_deleted_index(void) {
    return g_theme_editor.last_deleted_index;
}

static void theme_editor_init_with_capacity(const char *data_root, Theme *themes_in,
                                            int builtin_count, int capacity,
                                            int *theme_count_out) {
    ThemeEditorState *ed = &g_theme_editor;
    memset(ed, 0, sizeof *ed);
    if (!themes_in || builtin_count <= 0) return;
    if (capacity < builtin_count) capacity = builtin_count;
    if (capacity > THEME_EDITOR_MAX_THEMES) capacity = THEME_EDITOR_MAX_THEMES;
    if (builtin_count > capacity) builtin_count = capacity;
    ed->themes = themes_in;
    ed->builtin_count = builtin_count;
    ed->capacity = capacity;
    ed->theme_count = builtin_count;
    ed->theme_count_out = theme_count_out;
    ed->last_deleted_index = -1;
    snprintf(ed->path, sizeof ed->path, "%s/%s", data_root ? data_root : ".",
             THEME_EDITOR_FILE_NAME);

    for (int i = 0; i < THEME_EDITOR_MAX_THEMES; i++) {
        ed->legacy_map[i] = -1;
        ed->source_theme[i] = i;
        ed->effect_tint[i] = (SDL_Color){ 255, 255, 255, 255 };
        ed->default_effect_tint[i] = ed->effect_tint[i];
    }
    for (int i = 0; i < builtin_count; i++) {
        ed->defaults[i] = themes_in[i];
        /* Rainbow Road is currently the sole theme whose renderer supplies
           colours outside Theme. Name matching avoids coupling the module to
           one array index and survives future theme reordering. */
        ed->has_special_effect[i] = themes_in[i].name &&
                                    strcmp(themes_in[i].name, "Rainbow Road") == 0;
    }
    ed->initialized = 1;
    theme_editor_update_count();
    if (!theme_editor_load_file())
        snprintf(ed->status, sizeof ed->status, "Colour overrides could not be loaded");
}

/* Kept for isolated/older callers that only provide a fixed built-in array. */
static void theme_editor_init(const char *data_root, Theme *themes_in, int theme_count) {
    theme_editor_init_with_capacity(data_root, themes_in, theme_count, theme_count, NULL);
}

static int theme_editor_active(void) {
    return g_theme_editor.active;
}

static void theme_editor_open(int theme_index) {
    ThemeEditorState *ed = &g_theme_editor;
    if (!ed->initialized || theme_index < 0 || theme_index >= ed->theme_count) return;
    ed->active = 1;
    ed->close_prompt = 0;
    ed->delete_prompt = 0;
    ed->edit_theme = theme_index;
    ed->selected_role = 0;
    ed->selected_channel = 0;
    ed->rgb_step_index = 1; /* 5 */
    ed->backup = ed->themes[theme_index];
    ed->backup_effect_tint = ed->effect_tint[theme_index];
    ed->dirty = 0;
    ed->name_request = 0;
    ed->last_deleted_index = -1;
    ed->status[0] = '\0';
}

/* Settings calls this only for the currently selected custom theme. It opens
   a dedicated confirmation overlay without exposing a delete path for any
   built-in slot. */
static int theme_editor_request_delete(int theme_index) {
    if (!theme_editor_is_custom(theme_index)) return 0;
    theme_editor_open(theme_index);
    g_theme_editor.delete_prompt = 1;
    return 1;
}

static void theme_editor_restore_backup_and_close(void) {
    ThemeEditorState *ed = &g_theme_editor;
    if (!ed->active) return;
    ed->themes[ed->edit_theme] = ed->backup;
    ed->effect_tint[ed->edit_theme] = ed->backup_effect_tint;
    ed->active = 0;
    ed->close_prompt = 0;
    ed->delete_prompt = 0;
    ed->dirty = 0;
    ed->name_request = 0;
}

static void theme_editor_restore_current_default(void) {
    ThemeEditorState *ed = &g_theme_editor;
    if (!ed->active) return;
    int i = ed->edit_theme;
    theme_editor_copy_colors(&ed->themes[i], &ed->defaults[i]);
    ed->effect_tint[i] = ed->default_effect_tint[i];
    theme_editor_refresh_dirty();
}

/* Factory-reset integration: restore memory and remove the independent file. */
static void theme_editor_forget_all(void) {
    ThemeEditorState *ed = &g_theme_editor;
    if (!ed->initialized) return;
    for (int i = 0; i < ed->builtin_count; i++) {
        ed->themes[i] = ed->defaults[i];
        ed->effect_tint[i] = ed->default_effect_tint[i];
    }
    for (int i = ed->builtin_count; i < ed->theme_count; i++) {
        memset(&ed->themes[i], 0, sizeof ed->themes[i]);
        ed->names[i][0] = '\0';
    }
    ed->theme_count = ed->builtin_count;
    theme_editor_update_count();
    ed->active = 0;
    ed->close_prompt = 0;
    ed->delete_prompt = 0;
    ed->dirty = 0;
    ed->name_request = 0;
    ed->last_deleted_index = -1;
    remove(ed->path);
    { char tmp[800]; snprintf(tmp, sizeof tmp, "%.767s.tmp", ed->path); remove(tmp); }
}

static int theme_editor_nearest_palette(SDL_Color c) {
    int best = 0;
    unsigned long best_d = ~0UL;
    for (int i = 0; i < THEME_EDITOR_PALETTE_COUNT; i++) {
        long dr = (long)c.r - theme_editor_palette[i].r;
        long dg = (long)c.g - theme_editor_palette[i].g;
        long db = (long)c.b - theme_editor_palette[i].b;
        unsigned long d = (unsigned long)(dr * dr + dg * dg + db * db);
        if (d < best_d) { best = i; best_d = d; }
    }
    return best;
}

static int theme_editor_begin_name(int save_as) {
    ThemeEditorState *ed = &g_theme_editor;
    if (!ed->active) return 0;
    if (save_as && ed->theme_count >= ed->capacity) {
        snprintf(ed->status, sizeof ed->status,
                 "Custom theme limit reached (%d)", ed->capacity - ed->builtin_count);
        ed->close_prompt = 0;
        return 0;
    }
    ed->name_request = save_as ? 2 : 1;
    ed->close_prompt = 0;
    ed->status[0] = '\0';
    return 1;
}

static const char *theme_editor_name_suggestion(void) {
    ThemeEditorState *ed = &g_theme_editor;
    static char suggestion[THEME_EDITOR_NAME_MAX + 1];
    const char *current = ed->active && ed->themes[ed->edit_theme].name
                        ? ed->themes[ed->edit_theme].name : "Custom Theme";
    if (ed->name_request == 1) {
        snprintf(suggestion, sizeof suggestion, "%s", current);
    } else {
        char wanted[THEME_EDITOR_NAME_MAX + 1];
        snprintf(wanted, sizeof wanted, "%.38s Copy", current);
        theme_editor_unique_name(wanted, suggestion, sizeof suggestion);
    }
    return suggestion;
}

static void theme_editor_cancel_name(void) {
    g_theme_editor.name_request = 0;
}

/* Called after the shared on-screen keyboard accepts a name. Returns the
   selected custom slot, or -1 so Settings can return to the editor with a
   useful error. */
static int theme_editor_commit_name(const char *entered) {
    ThemeEditorState *ed = &g_theme_editor;
    if (!ed->active || !ed->name_request) return -1;
    int request = ed->name_request;
    ed->name_request = 0;
    char clean[THEME_EDITOR_NAME_MAX + 1];
    if (!theme_editor_clean_name(entered, clean, sizeof clean)) {
        snprintf(ed->status, sizeof ed->status, "Enter a theme name");
        return -1;
    }

    if (request == 1) { /* rename and save the current custom theme */
        int idx = ed->edit_theme;
        if (!theme_editor_is_custom(idx)) {
            snprintf(ed->status, sizeof ed->status, "Built-in names are protected; use Save As");
            return -1;
        }
        if (theme_editor_name_exists(clean, idx)) {
            snprintf(ed->status, sizeof ed->status, "A theme already has that name");
            return -1;
        }
        char old[THEME_EDITOR_NAME_MAX + 1];
        snprintf(old, sizeof old, "%s", ed->names[idx]);
        snprintf(ed->names[idx], sizeof ed->names[idx], "%s", clean);
        if (!theme_editor_save_file()) {
            snprintf(ed->names[idx], sizeof ed->names[idx], "%s", old);
            return -1;
        }
        ed->active = 0;
        ed->close_prompt = 0;
        ed->dirty = 0;
        return idx;
    }

    if (ed->theme_count >= ed->capacity) {
        snprintf(ed->status, sizeof ed->status, "Custom theme limit reached (%d)",
                 ed->capacity - ed->builtin_count);
        return -1;
    }
    if (theme_editor_name_exists(clean, -1)) {
        snprintf(ed->status, sizeof ed->status, "A theme already has that name");
        return -1;
    }

    int edit = ed->edit_theme;
    int source = ed->source_theme[edit];
    Theme edited = ed->themes[edit];
    SDL_Color edited_tint = ed->effect_tint[edit];
    int idx = ed->theme_count;
    ed->themes[idx] = edited;
    snprintf(ed->names[idx], sizeof ed->names[idx], "%s", clean);
    ed->themes[idx].name = ed->names[idx];
    ed->defaults[idx] = ed->defaults[source];
    ed->defaults[idx].name = ed->names[idx];
    ed->source_theme[idx] = source;
    ed->has_special_effect[idx] = ed->has_special_effect[source];
    ed->effect_tint[idx] = edited_tint;
    ed->default_effect_tint[idx] = ed->default_effect_tint[source];
    ed->theme_count++;
    theme_editor_update_count();

    /* Save As must never commit the preview over its source theme. */
    ed->themes[edit] = ed->backup;
    ed->effect_tint[edit] = ed->backup_effect_tint;
    if (!theme_editor_save_file()) {
        ed->themes[edit] = edited;
        ed->effect_tint[edit] = edited_tint;
        ed->theme_count--;
        memset(&ed->themes[idx], 0, sizeof ed->themes[idx]);
        ed->names[idx][0] = '\0';
        theme_editor_update_count();
        return -1;
    }

    ed->active = 0;
    ed->close_prompt = 0;
    ed->dirty = 0;
    return idx;
}

static int theme_editor_handle_key_for_active(SDL_Keycode key, int *active_theme) {
    ThemeEditorState *ed = &g_theme_editor;
    if (!ed->active) return THEME_EDITOR_RESULT_NONE;
    int out = THEME_EDITOR_RESULT_CONSUMED;

    if (ed->delete_prompt) {
        if (key == SDLK_RETURN) {                /* A: DELETE */
            int idx = ed->edit_theme;
            int previous_active = active_theme ? *active_theme : -1;
            /* The visible selection must stop pointing at the slot before it
               is compacted. This also makes a mid-write failure safe. */
            if (active_theme && *active_theme == idx) *active_theme = 0;
            if (theme_editor_delete_custom_now(idx)) {
                if (active_theme && previous_active != idx)
                    *active_theme = theme_editor_adjust_index_after_delete(previous_active, idx);
                ed->active = 0;
                ed->close_prompt = 0;
                ed->delete_prompt = 0;
                ed->dirty = 0;
                ed->name_request = 0;
                return out | THEME_EDITOR_RESULT_CHANGED |
                       THEME_EDITOR_RESULT_DELETED | THEME_EDITOR_RESULT_CLOSED;
            }
            if (active_theme) *active_theme = previous_active;
            ed->delete_prompt = 0; /* remain in the editor with the save error */
        } else if (key == SDLK_ESCAPE) {         /* B: CANCEL */
            ed->active = 0;
            ed->delete_prompt = 0;
            ed->close_prompt = 0;
            ed->dirty = 0;
            ed->name_request = 0;
            return out | THEME_EDITOR_RESULT_CLOSED;
        }
        return out;
    }

    if (ed->close_prompt) {
        if (key == SDLK_RETURN && theme_editor_is_custom(ed->edit_theme)) {
            /* A updates a named custom theme in place. Built-ins always take
               the Save As path below, so their colours/names remain pristine. */
            if (theme_editor_save_file()) {
                ed->active = 0;
                ed->close_prompt = 0;
                ed->dirty = 0;
                return out | THEME_EDITOR_RESULT_SAVED | THEME_EDITOR_RESULT_CLOSED;
            }
            ed->close_prompt = 0; /* remain in editor and display the error */
        } else if (key == SDLK_RETURN || key == SDLK_s) {
            if (theme_editor_begin_name(1))
                return out | THEME_EDITOR_RESULT_NAME;
        } else if (key == SDLK_f) {              /* Y: discard */
            theme_editor_restore_backup_and_close();
            return out | THEME_EDITOR_RESULT_CHANGED | THEME_EDITOR_RESULT_CLOSED;
        } else if (key == SDLK_ESCAPE) {         /* B: cancel close */
            ed->close_prompt = 0;
        }
        return out;
    }

    int role_count = theme_editor_role_count_for(ed->edit_theme);
    if (key == SDLK_UP) {
        ed->selected_role = (ed->selected_role - 1 + role_count) % role_count;
    } else if (key == SDLK_DOWN) {
        ed->selected_role = (ed->selected_role + 1) % role_count;
    } else if (key == SDLK_q) {                 /* L1 */
        ed->selected_channel = (ed->selected_channel + 2) % 3;
    } else if (key == SDLK_e) {                 /* R1 */
        ed->selected_channel = (ed->selected_channel + 1) % 3;
    } else if (key == SDLK_s) {                 /* X */
        ed->rgb_step_index = (ed->rgb_step_index + 1) %
                             (int)(sizeof theme_editor_rgb_steps / sizeof theme_editor_rgb_steps[0]);
    } else if (key == SDLK_LEFT || key == SDLK_RIGHT) {
        SDL_Color *c = theme_editor_current_role(ed->edit_theme, ed->selected_role);
        Uint8 *v = theme_editor_channel(c, ed->selected_channel);
        int delta = theme_editor_rgb_steps[ed->rgb_step_index] * (key == SDLK_RIGHT ? 1 : -1);
        *v = (Uint8)theme_editor_clamp_byte((int)*v + delta);
        c->a = 255;
        theme_editor_refresh_dirty();
        out |= THEME_EDITOR_RESULT_CHANGED;
    } else if (key == SDLK_PAGEUP || key == SDLK_PAGEDOWN) { /* L2 / R2 */
        SDL_Color *c = theme_editor_current_role(ed->edit_theme, ed->selected_role);
        int at = theme_editor_nearest_palette(*c);
        int dir = key == SDLK_PAGEDOWN ? 1 : -1;
        *c = theme_editor_palette[(at + dir + THEME_EDITOR_PALETTE_COUNT) % THEME_EDITOR_PALETTE_COUNT];
        theme_editor_refresh_dirty();
        out |= THEME_EDITOR_RESULT_CHANGED;
    } else if (key == SDLK_SLASH) {             /* Select */
        theme_editor_restore_current_default();
        out |= THEME_EDITOR_RESULT_CHANGED;
    } else if (key == SDLK_f) {                 /* Y: name / rename */
        int save_as = !theme_editor_is_custom(ed->edit_theme);
        if (theme_editor_begin_name(save_as))
            return out | THEME_EDITOR_RESULT_NAME;
    } else if (key == SDLK_RETURN) {            /* A: review save */
        if (ed->dirty) ed->close_prompt = 1;
        else {
            ed->active = 0;
            return out | THEME_EDITOR_RESULT_CLOSED;
        }
    } else if (key == SDLK_ESCAPE) {            /* B: close or review */
        if (ed->dirty) ed->close_prompt = 1;
        else {
            ed->active = 0;
            return out | THEME_EDITOR_RESULT_CLOSED;
        }
    }
    return out;
}

/* Compatibility wrapper for focused tests/older callers. The full frontend
   uses the pointer-taking form so deleting the active theme can switch to the
   default before compacting the array. */
static int theme_editor_handle_key(SDL_Keycode key) {
    return theme_editor_handle_key_for_active(key, NULL);
}

/* Map a code-drawn special-effect colour through the edited role. At built-in
   defaults this is an exact identity, so Rainbow Road retains its present
   appearance. Editing an accent adds that role's delta to the generated hue;
   Special Effect Tint then supplies an optional overall RGB tint. */
static SDL_Color theme_editor_map_effect_color(int theme_index, int source_role,
                                               SDL_Color generated) {
    ThemeEditorState *ed = &g_theme_editor;
    if (!ed->initialized || theme_index < 0 || theme_index >= ed->theme_count)
        return generated;

    SDL_Color *cur = theme_editor_current_role(theme_index, source_role);
    SDL_Color *def = theme_editor_default_role(theme_index, source_role);
    SDL_Color tint = ed->effect_tint[theme_index];
    int r = generated.r, g = generated.g, b = generated.b;
    if (cur && def && source_role != THEME_EDITOR_ROLE_EFFECT_TINT) {
        r += (int)cur->r - (int)def->r;
        g += (int)cur->g - (int)def->g;
        b += (int)cur->b - (int)def->b;
    }
    r = theme_editor_clamp_byte(r) * tint.r / 255;
    g = theme_editor_clamp_byte(g) * tint.g / 255;
    b = theme_editor_clamp_byte(b) * tint.b / 255;
    return (SDL_Color){ (Uint8)r, (Uint8)g, (Uint8)b, generated.a };
}

static SDL_Color theme_editor_safe_text(SDL_Color bg) {
    int lum = (bg.r * 30 + bg.g * 59 + bg.b * 11) / 100;
    return lum < 142 ? (SDL_Color){ 248, 248, 250, 255 }
                     : (SDL_Color){  18,  18,  22, 255 };
}

static void theme_editor_fill(SDL_Renderer *ren, SDL_Rect r, SDL_Color c, Uint8 alpha) {
    SDL_SetRenderDrawColor(ren, c.r, c.g, c.b, alpha);
    SDL_RenderFillRect(ren, &r);
}

static void theme_editor_text(SDL_Renderer *ren, TTF_Font *font, const char *text,
                              SDL_Color color, int x, int y, int max_width) {
    if (!ren || !font || !text || !text[0]) return;
    SDL_Surface *s = TTF_RenderUTF8_Blended(font, text, color);
    if (!s) return;
    SDL_Texture *t = SDL_CreateTextureFromSurface(ren, s);
    int w = s->w, h = s->h;
    SDL_FreeSurface(s);
    if (!t) return;
    SDL_Rect d = { x, y, w, h };
    if (max_width > 0 && d.w > max_width) d.w = max_width;
    SDL_RenderCopy(ren, t, NULL, &d);
    SDL_DestroyTexture(t);
}

static void theme_editor_draw(SDL_Renderer *ren, TTF_Font *title_font,
                              TTF_Font *body_font, int screen_w, int screen_h) {
    ThemeEditorState *ed = &g_theme_editor;
    if (!ed->active || !ren || !body_font) return;
    if (!title_font) title_font = body_font;
    Theme *theme = &ed->themes[ed->edit_theme];

    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
    theme_editor_fill(ren, (SDL_Rect){ 0, 0, screen_w, screen_h },
                      (SDL_Color){ 0, 0, 0, 255 }, 178);

    int px = screen_w > 560 ? 28 : 12;
    int py = 18;
    int pw = screen_w - px * 2;
    int ph = screen_h - py * 2;
    SDL_Rect panel = { px, py, pw, ph };
    theme_editor_fill(ren, panel, theme->bg, 255);
    SDL_SetRenderDrawColor(ren, theme->accent2.r, theme->accent2.g, theme->accent2.b, 255);
    for (int n = 0; n < 2; n++) {
        SDL_Rect border = { panel.x + n, panel.y + n, panel.w - n * 2, panel.h - n * 2 };
        SDL_RenderDrawRect(ren, &border);
    }

    SDL_Color panel_text = theme_editor_safe_text(theme->bg);
    char title[160];
    snprintf(title, sizeof title, "%s COLORS%s  [%s]",
             theme->name ? theme->name : "THEME",
             ed->dirty ? "  *" : "",
             theme_editor_is_custom(ed->edit_theme) ? "CUSTOM" : "BUILT-IN");
    theme_editor_text(ren, title_font, title, panel_text, px + 18, py + 12, pw - 36);

    int preview_y = py + 48;
    int preview_h = 50;
    SDL_Rect preview = { px + 16, preview_y, pw - 32, preview_h };
    theme_editor_fill(ren, preview, theme->select_bg, 235);
    SDL_Color preview_text = theme_editor_safe_text(theme->select_bg);
    theme_editor_text(ren, body_font, "Aa  Primary", theme->text,
                      preview.x + 10, preview.y + 5, preview.w / 3 - 12);
    theme_editor_text(ren, body_font, "Muted", theme->dim,
                      preview.x + 10, preview.y + 25, preview.w / 3 - 12);
    SDL_Color accents[3] = { theme->accent1, theme->accent2, theme->accent3 };
    int chip_w = (preview.w / 2 - 20) / 3;
    int chips_x = preview.x + preview.w - chip_w * 3 - 10;
    for (int i = 0; i < 3; i++) {
        SDL_Rect chip = { chips_x + i * chip_w, preview.y + 9, chip_w - 5, preview.h - 18 };
        theme_editor_fill(ren, chip, accents[i], 255);
        SDL_SetRenderDrawColor(ren, preview_text.r, preview_text.g, preview_text.b, 130);
        SDL_RenderDrawRect(ren, &chip);
    }

    int role_count = theme_editor_role_count_for(ed->edit_theme);
    int rows_y = preview_y + preview_h + 8;
    int footer_h = 94;
    int row_h = (ph - (rows_y - py) - footer_h) / role_count;
    if (row_h > 30) row_h = 30;
    if (row_h < 22) row_h = 22;
    static const char *channel_names[3] = { "R", "G", "B" };

    for (int role = 0; role < role_count; role++) {
        int y = rows_y + role * row_h;
        int selected = role == ed->selected_role;
        SDL_Color *c = theme_editor_current_role(ed->edit_theme, role);
        if (selected) {
            SDL_Color hi = theme->select_bg;
            theme_editor_fill(ren, (SDL_Rect){ px + 12, y, pw - 24, row_h - 2 }, hi, 245);
        }
        SDL_Color row_bg = selected ? theme->select_bg : theme->bg;
        SDL_Color row_text = theme_editor_safe_text(row_bg);
        theme_editor_text(ren, body_font, theme_editor_role_names[role], row_text,
                          px + 22, y + 2, pw / 2 - 60);

        int swatch_x = px + pw - 262;
        SDL_Rect swatch = { swatch_x, y + 3, 38, row_h - 8 };
        theme_editor_fill(ren, swatch, *c, 255);
        SDL_SetRenderDrawColor(ren, row_text.r, row_text.g, row_text.b, 180);
        SDL_RenderDrawRect(ren, &swatch);

        char hex[16];
        snprintf(hex, sizeof hex, "#%02X%02X%02X", c->r, c->g, c->b);
        theme_editor_text(ren, body_font, hex, row_text, swatch_x + 48, y + 2, 104);

        int channel_x = px + pw - 94;
        for (int ch = 0; ch < 3; ch++) {
            SDL_Rect cr = { channel_x + ch * 25, y + 3, 20, row_h - 8 };
            if (selected && ch == ed->selected_channel) {
                theme_editor_fill(ren, cr, theme->accent2, 255);
                theme_editor_text(ren, body_font, channel_names[ch],
                                  theme_editor_safe_text(theme->accent2),
                                  cr.x + 5, y + 2, 14);
            } else {
                SDL_SetRenderDrawColor(ren, row_text.r, row_text.g, row_text.b, 110);
                SDL_RenderDrawRect(ren, &cr);
                theme_editor_text(ren, body_font, channel_names[ch], row_text,
                                  cr.x + 5, y + 2, 14);
            }
        }
    }

    int footer_y = py + ph - footer_h + 4;
    char controls1[160];
    snprintf(controls1, sizeof controls1,
             "D-pad Role/RGB   L1/R1 Channel   L2/R2 Palette   X Step %d",
             theme_editor_rgb_steps[ed->rgb_step_index]);
    theme_editor_text(ren, body_font, controls1, panel_text,
                      px + 18, footer_y, pw - 36);
    theme_editor_text(ren, body_font,
                      theme_editor_is_custom(ed->edit_theme)
                        ? "Y Rename   Select Restore Source   A Done   B Back"
                        : "Y Name Copy   Select Restore Built-in   A Done   B Back",
                      panel_text, px + 18, footer_y + 22, pw - 36);
    if (ed->status[0])
        theme_editor_text(ren, body_font, ed->status,
                          (SDL_Color){ 255, 110, 90, 255 },
                          px + 18, footer_y + 44, pw - 36);

    if (ed->close_prompt) {
        theme_editor_fill(ren, (SDL_Rect){ 0, 0, screen_w, screen_h },
                          (SDL_Color){ 0, 0, 0, 255 }, 165);
        int cw = screen_w - 100;
        if (cw > 520) cw = 520;
        int ch = 126;
        SDL_Rect confirm = { (screen_w - cw) / 2, (screen_h - ch) / 2, cw, ch };
        SDL_Color cbg = { 24, 24, 30, 255 };
        theme_editor_fill(ren, confirm, cbg, 255);
        SDL_SetRenderDrawColor(ren, 210, 210, 220, 255);
        SDL_RenderDrawRect(ren, &confirm);
        theme_editor_text(ren, title_font,
                          theme_editor_is_custom(ed->edit_theme)
                            ? "Save custom theme changes?"
                            : "Save as a new custom theme?",
                          (SDL_Color){ 245, 245, 248, 255 },
                          confirm.x + 20, confirm.y + 18, confirm.w - 40);
        theme_editor_text(ren, body_font,
                          theme_editor_is_custom(ed->edit_theme)
                            ? "A Save   X Save As   Y Discard   B Cancel"
                            : "A / X Name & Save   Y Discard   B Cancel",
                          (SDL_Color){ 198, 200, 208, 255 },
                          confirm.x + 20, confirm.y + 72, confirm.w - 40);
    }

    if (ed->delete_prompt) {
        theme_editor_fill(ren, (SDL_Rect){ 0, 0, screen_w, screen_h },
                          (SDL_Color){ 0, 0, 0, 255 }, 190);
        int cw = screen_w - 100;
        if (cw > 520) cw = 520;
        int ch = 156;
        SDL_Rect confirm = { (screen_w - cw) / 2, (screen_h - ch) / 2, cw, ch };
        SDL_Color cbg = { 24, 24, 30, 255 };
        theme_editor_fill(ren, confirm, cbg, 255);
        SDL_SetRenderDrawColor(ren, 230, 94, 94, 255);
        for (int n = 0; n < 2; n++) {
            SDL_Rect border = { confirm.x + n, confirm.y + n,
                                confirm.w - n * 2, confirm.h - n * 2 };
            SDL_RenderDrawRect(ren, &border);
        }
        theme_editor_text(ren, title_font, "Delete this custom theme?",
                          (SDL_Color){ 248, 248, 250, 255 },
                          confirm.x + 20, confirm.y + 18, confirm.w - 40);
        theme_editor_text(ren, body_font, "This cannot be undone.",
                          (SDL_Color){ 198, 200, 208, 255 },
                          confirm.x + 20, confirm.y + 59, confirm.w - 40);
        theme_editor_text(ren, body_font, "A  DELETE        B  CANCEL",
                          (SDL_Color){ 255, 126, 110, 255 },
                          confirm.x + 20, confirm.y + 105, confirm.w - 40);
    }
}

#endif /* SNAPFE_THEME_EDITOR_H */
