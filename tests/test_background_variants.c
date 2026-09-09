/* Runs against isolated temporary artwork/settings and a software renderer.
   No frontend startup, user artwork, network requests, or hardware access. */
#define main snapfe_application_main
#define SNAPFE_TESTING 1
#include "../main.c"
#undef main
#include <assert.h>

static char fixture_root[512];

static void fixture_path(char *out, size_t size, const char *relative) {
    int n = snprintf(out, size, "%s/%s", fixture_root, relative);
    assert(n >= 0 && (size_t)n < size);
}

static void fixture_mkdir(const char *relative) {
    char path[1024]; fixture_path(path, sizeof path, relative);
    for (char *p = path + strlen(fixture_root) + 1; *p; p++) {
        if (*p != '/') continue;
        *p = 0; assert(mkdir(path, 0700) == 0 || errno == EEXIST); *p = '/';
    }
    assert(mkdir(path, 0700) == 0 || errno == EEXIST);
}

static void fixture_text(const char *relative, const char *text) {
    char path[1024]; fixture_path(path, sizeof path, relative);
    FILE *f = fopen(path, "w"); assert(f);
    assert(fputs(text, f) >= 0); assert(fclose(f) == 0);
}

static void fixture_png(const char *relative, Uint8 red, Uint8 green, Uint8 blue) {
    char path[1024]; fixture_path(path, sizeof path, relative);
    SDL_Surface *s = SDL_CreateRGBSurfaceWithFormat(0, 12, 8, 32, SDL_PIXELFORMAT_RGBA32);
    assert(s);
    assert(SDL_FillRect(s, NULL, SDL_MapRGB(s->format, red, green, blue)) == 0);
    assert(IMG_SavePNG(s, path) == 0); SDL_FreeSurface(s);
}

static int fixture_exists(const char *relative) {
    char path[1024]; fixture_path(path, sizeof path, relative);
    return access(path, F_OK) == 0;
}

static int fixture_contains(const char *relative, const char *text) {
    char path[1024], contents[32768]; fixture_path(path, sizeof path, relative);
    FILE *f = fopen(path, "r"); assert(f);
    size_t n = fread(contents, 1, sizeof contents - 1, f);
    assert(!ferror(f)); contents[n] = 0; fclose(f);
    return strstr(contents, text) != NULL;
}

static int fixture_platform(const char *system) {
    for (int p = 0; p < PLATFORM_COUNT; p++) if (!strcmp(platform_dirs[p], system)) return p;
    assert(!"Fixture system is absent from production platform catalog"); return -1;
}

static void assert_directory(int p, const char *relative) {
    char path[1024] = "", expected[1024];
    background_directory(p, path, sizeof path);
    fixture_path(expected, sizeof expected, relative);
    assert(!strcmp(path, expected));
}

static void assert_texture(SDL_Renderer *ren, SDL_Texture *texture,
                           Uint8 red, Uint8 green, Uint8 blue) {
    assert(texture);
    assert(SDL_SetRenderDrawColor(ren, 0, 0, 0, 255) == 0);
    assert(SDL_RenderClear(ren) == 0);
    assert(SDL_RenderCopy(ren, texture, NULL, &(SDL_Rect){0, 0, 16, 16}) == 0);
    Uint8 pixel[4] = {0};
    assert(SDL_RenderReadPixels(ren, &(SDL_Rect){8, 8, 1, 1}, SDL_PIXELFORMAT_RGBA32, pixel, 4) == 0);
    assert(pixel[0] == red && pixel[1] == green && pixel[2] == blue);
}

static void test_family_listing(void) {
    int n64 = fixture_platform("n64"), gba = fixture_platform("gba"), snes = fixture_platform("snes");
    fixture_mkdir("assets/backgrounds/4x3/n64");
    fixture_mkdir("assets/backgrounds/3x2/n64");
    fixture_mkdir("assets/backgrounds/n64");
    fixture_png("assets/backgrounds/4x3/n64/shared.png", 190, 20, 10);
    fixture_png("assets/backgrounds/3x2/n64/shared.png", 10, 30, 190);
    fixture_png("assets/backgrounds/3x2/n64/wide-only.png", 10, 30, 190);
    fixture_png("assets/backgrounds/n64/shared.png", 10, 190, 30);
    fixture_png("assets/backgrounds/n64/legacy-only.png", 10, 190, 30);
    char files[16][256] = {{0}};
    WIN_W = 640; WIN_H = 480;
    assert(!strcmp(background_aspect_folder(), "4x3"));
    assert_directory(n64, "assets/backgrounds/4x3/n64");
    assert(list_backgrounds_for_platform(n64, files, 16) == 1);
    assert(!strcmp(files[0], "shared.png"));
    WIN_W = 720;
    assert(!strcmp(background_aspect_folder(), "3x2"));
    assert_directory(n64, "assets/backgrounds/3x2/n64");
    assert(list_backgrounds_for_platform(n64, files, 16) == 2);
    assert(!strcmp(files[0], "shared.png") && !strcmp(files[1], "wide-only.png"));
    WIN_W = 1440; WIN_H = 960;
    assert(!strcmp(background_aspect_folder(), "3x2"));
    WIN_W = 1280;
    assert(!strcmp(background_aspect_folder(), "4x3"));

    /* A deliberately empty or not-yet-filled family must not reveal the other
       family or old copies in the legacy directory. */
    fixture_mkdir("assets/backgrounds/3x2/gba");
    fixture_mkdir("assets/backgrounds/gba");
    fixture_png("assets/backgrounds/3x2/gba/wide.png", 0, 0, 190);
    fixture_png("assets/backgrounds/gba/legacy.png", 0, 190, 0);
    WIN_W = 640; WIN_H = 480;
    assert_directory(gba, "assets/backgrounds/4x3/gba");
    assert(list_backgrounds_for_platform(gba, files, 16) == 0);
    fixture_mkdir("assets/backgrounds/4x3/gba");
    assert(list_backgrounds_for_platform(gba, files, 16) == 0);

    /* Creating ratio folders for one system leaves legacy-only systems usable. */
    fixture_mkdir("assets/backgrounds/snes");
    fixture_png("assets/backgrounds/snes/legacy.png", 0, 190, 0);
    assert_directory(snes, "assets/backgrounds/snes");
    assert(list_backgrounds_for_platform(snes, files, 16) == 1);
    WIN_W = 720;
    assert_directory(snes, "assets/backgrounds/snes");
    assert(list_backgrounds_for_platform(snes, files, 16) == 1);
}

static void test_filter_sort_and_paths(void) {
    int p = fixture_platform("nes");
    fixture_mkdir("assets/backgrounds/4x3/nes");
    fixture_text("assets/backgrounds/4x3/nes/zebra.jpeg", "fixture");
    fixture_text("assets/backgrounds/4x3/nes/alpha.PNG", "fixture");
    fixture_text("assets/backgrounds/4x3/nes/ALPHA.png", "duplicate spelling");
    fixture_text("assets/backgrounds/4x3/nes/middle.jpg", "fixture");
    fixture_text("assets/backgrounds/4x3/nes/vector.svg", "<svg xmlns='http://www.w3.org/2000/svg'/>");
    fixture_text("assets/backgrounds/4x3/nes/.hidden.png", "hidden");
    fixture_text("assets/backgrounds/4x3/nes/zebra.jpeg:Zone.Identifier", "metadata");
    fixture_text("assets/backgrounds/4x3/nes/._zebra.jpeg", "Apple metadata");
    fixture_text("assets/backgrounds/4x3/nes/middle.jpg.license.txt", "Title: Fixture\n");
    fixture_mkdir("assets/backgrounds/4x3/nes/folder.png");
    char linked[1024], target[1024];
    fixture_path(linked, sizeof linked, "assets/backgrounds/4x3/nes/opposite-family.png");
    fixture_path(target, sizeof target, "assets/backgrounds/3x2/n64/shared.png");
    assert(symlink(target, linked) == 0);
    WIN_W = 640; WIN_H = 480;
    char files[16][256] = {{0}}, repeated[16][256] = {{0}};
    assert(list_backgrounds_for_platform(p, files, 16) == 4);
    assert(!strcasecmp(files[0], "alpha.png"));
    assert(!strcmp(files[1], "middle.jpg") && !strcmp(files[2], "vector.svg") && !strcmp(files[3], "zebra.jpeg"));
    assert(list_backgrounds_for_platform(p, repeated, 16) == 4);
    assert(!memcmp(files, repeated, sizeof files));
    assert(list_backgrounds_for_platform(p, repeated, 2) == 2);
    assert(!strcmp(files[0], repeated[0]) && !strcmp(files[1], repeated[1]));
    assert(fixture_exists("assets/backgrounds/4x3/nes/zebra.jpeg:Zone.Identifier"));
    assert(fixture_exists("assets/backgrounds/4x3/nes/.hidden.png"));
    assert(fixture_exists("assets/backgrounds/4x3/nes/._zebra.jpeg"));
    assert(fixture_exists("assets/backgrounds/4x3/nes/opposite-family.png"));

    char path[1024], expected[1024];
    assert(background_file_path(p, "middle.jpg", path, sizeof path));
    fixture_path(expected, sizeof expected, "assets/backgrounds/4x3/nes/middle.jpg");
    assert(!strcmp(path, expected));
    const char *invalid[] = {"", ".", "..", "../3x2/nes/wide.png", "/tmp/wide.png", "sub/image.png", "sub\\image.png", "image.png:Zone.Identifier"};
    for (size_t i = 0; i < sizeof invalid / sizeof invalid[0]; i++)
        assert(!background_file_path(p, invalid[i], path, sizeof path));
    assert(!background_file_path(-1, "image.png", path, sizeof path));
    assert(!background_file_path(PLATFORM_COUNT, "image.png", path, sizeof path));
    assert(!background_file_path(p, "middle.jpg", path, 4));
    WIN_W = 720;
    background_download_directory(p, path, sizeof path);
    fixture_path(expected, sizeof expected, "assets/backgrounds/3x2/nes");
    assert(!strcmp(path, expected));
}

static void test_texture_selection_and_ratio_change(SDL_Renderer *ren) {
    int p = fixture_platform("n64"), empty = fixture_platform("gba");
    platform_game_count_cache[p] = platform_game_count_cache[empty] = 0;
    WIN_W = 640; WIN_H = 480;
    snprintf(platform_bg_choice[p], sizeof platform_bg_choice[p], "shared.png");
    load_platform_assets(ren, p);
    assert_texture(ren, platform_bg_tex, 190, 20, 10);

    /* Same selected basename resolves to this screen's actual pixels. */
    WIN_W = 720;
    load_platform_assets(ren, p);
    assert_texture(ren, platform_bg_tex, 10, 30, 190);
    WIN_W = 640;
    ensure_carousel_bg_loaded(ren, p);
    assert_texture(ren, platform_bg_cache[p], 190, 20, 10);
    assert(platform_assets_loaded_for != p);
    load_platform_assets(ren, p);
    assert_texture(ren, platform_bg_tex, 190, 20, 10);

    /* A stale saved selection and legacy duplicate must not bypass listing. */
    invalidate_carousel_bg(p);
    snprintf(platform_bg_choice[p], sizeof platform_bg_choice[p], "wide-only.png");
    ensure_carousel_bg_loaded(ren, p);
    assert_texture(ren, platform_bg_cache[p], 190, 20, 10);
    invalidate_carousel_bg(p);
    snprintf(platform_bg_choice[p], sizeof platform_bg_choice[p], "../3x2/n64/wide-only.png");
    ensure_carousel_bg_loaded(ren, p);
    assert_texture(ren, platform_bg_cache[p], 190, 20, 10);
    snprintf(platform_bg_choice[empty], sizeof platform_bg_choice[empty], "legacy.png");
    load_platform_assets(ren, empty);
    assert(!platform_bg_cache[empty] && !platform_bg_tex);

    WIN_W = 720;
    ensure_carousel_bg_loaded(ren, p);
    assert_texture(ren, platform_bg_cache[p], 10, 30, 190);
    WIN_W = 640;
    char files[16][256];
    assert(list_backgrounds_for_platform(p, files, 16) == 1);
    assert(!platform_bg_cache[p] && !platform_bg_cache_attempted[p]);
    load_platform_assets(ren, p);
    assert_texture(ren, platform_bg_tex, 190, 20, 10);
}

static void test_picker_mutations(void) {
    int p = fixture_platform("n64");
    fixture_text("assets/backgrounds/4x3/n64/shared.png.license.txt", "Title: Standard Screen\nCreator: Fixture\nResolution: 640x480\n");
    fixture_text("assets/backgrounds/3x2/n64/shared.png.license.txt", "Title: Wide Screen\nCreator: Other Fixture\n");
    fixture_text("assets/backgrounds/n64/shared.png.license.txt", "Title: Legacy Screen\n");
    WIN_W = 640; WIN_H = 480; bg_picker_platform = p;
    refresh_background_picker();
    assert(background_file_count == 1 && !strcmp(background_titles[0], "Standard Screen"));
    assert(strstr(background_details[0], "640x480") && strstr(background_details[0], "Fixture"));
    snprintf(platform_bg_choice[p], sizeof platform_bg_choice[p], "shared.png");
    background_picker_selected = 1;
    snprintf(kb_buffer, sizeof kb_buffer, "Renamed Scene");
    rename_selected_background();
    assert(!strcmp(platform_bg_choice[p], "Renamed Scene.png"));
    assert(fixture_exists("assets/backgrounds/4x3/n64/Renamed Scene.png"));
    assert(!fixture_exists("assets/backgrounds/4x3/n64/shared.png"));
    assert(fixture_contains("assets/backgrounds/4x3/n64/Renamed Scene.png.license.txt", "Title: Renamed Scene\n"));
    assert(fixture_contains("assets/backgrounds/4x3/n64/Renamed Scene.png.license.txt", "Creator: Fixture\n"));
    assert(fixture_exists("assets/backgrounds/3x2/n64/shared.png"));
    assert(fixture_contains("assets/backgrounds/3x2/n64/shared.png.license.txt", "Title: Wide Screen\n"));
    assert(fixture_exists("assets/backgrounds/n64/shared.png"));
    assert(fixture_contains("settings.cfg", "bg_choice_n64=Renamed Scene.png\n"));
    platform_bg_choice[p][0] = 0; load_settings();
    assert(!strcmp(platform_bg_choice[p], "Renamed Scene.png"));
    assert(background_picker_selected == 1 && !strcmp(background_titles[0], "Renamed Scene"));
    delete_selected_background();
    assert(!platform_bg_choice[p][0] && !background_file_count);
    assert(!fixture_exists("assets/backgrounds/4x3/n64/Renamed Scene.png"));
    assert(!fixture_exists("assets/backgrounds/4x3/n64/Renamed Scene.png.license.txt"));
    assert(!fixture_contains("settings.cfg", "bg_choice_n64="));
    assert(fixture_exists("assets/backgrounds/3x2/n64/shared.png"));
    assert(fixture_exists("assets/backgrounds/n64/shared.png"));
    WIN_W = 720; refresh_background_picker();
    assert(background_file_count == 2 && !strcmp(background_titles[0], "Wide Screen"));
}

static void fixture_remove_tree(const char *path) {
    assert(!strncmp(path, fixture_root, strlen(fixture_root)));
    DIR *dir = opendir(path); assert(dir);
    struct dirent *entry;
    while ((entry = readdir(dir))) {
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) continue;
        char child[2048]; struct stat info;
        int n = snprintf(child, sizeof child, "%s/%s", path, entry->d_name);
        assert(n > 0 && n < (int)sizeof child && lstat(child, &info) == 0);
        if (S_ISDIR(info.st_mode)) fixture_remove_tree(child);
        else assert(unlink(child) == 0);
    }
    closedir(dir); assert(rmdir(path) == 0);
}

int main(void) {
    snprintf(fixture_root, sizeof fixture_root, "/tmp/snapfe-background-variants-XXXXXX");
    assert(mkdtemp(fixture_root)); assert(setenv("SNAPFE_DATA_ROOT", fixture_root, 1) == 0);
    assert(setenv("SDL_VIDEODRIVER", "dummy", 1) == 0);
    assert(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == 0);
    assert((IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) != 0);
    SDL_Surface *screen = SDL_CreateRGBSurfaceWithFormat(0, 32, 32, 32, SDL_PIXELFORMAT_RGBA32);
    assert(screen); SDL_Renderer *ren = SDL_CreateSoftwareRenderer(screen); assert(ren);
    test_family_listing();
    test_filter_sort_and_paths();
    test_texture_selection_and_ratio_change(ren);
    test_picker_mutations();
    for (int p = 0; p < PLATFORM_COUNT; p++) invalidate_carousel_bg(p);
    SDL_DestroyRenderer(ren); SDL_FreeSurface(screen); IMG_Quit(); SDL_Quit();
    fixture_remove_tree(fixture_root);
    puts("PASS: ratio-isolated backgrounds, deterministic dedup/filter, selected pixels and cache changes, metadata/rename/delete/settings, download destination");
    return 0;
}
