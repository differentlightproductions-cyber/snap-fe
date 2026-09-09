#ifndef SNAPFE_BACKGROUND_VARIANTS_H
#define SNAPFE_BACKGROUND_VARIANTS_H

/* Folder names describe the panel, not the logical design size or image name.
   The midpoint chooses the nearer supported landscape ratio for other panels. */
static const char *background_aspect_folder(void) {
    return WIN_W > 0 && WIN_H > 0 && (double)WIN_W / WIN_H >= 17.0 / 12.0 ? "3x2" : "4x3";
}

static int background_is_directory(const char *path) {
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

static int background_directory(int platform, char *out, size_t size) {
    if (!size) return 0;
    out[0] = '\0';
    if (platform < 0 || platform >= PLATFORM_COUNT) return 0;
    char narrow[900], wide[900];
    snprintf(narrow, sizeof narrow, "%s/assets/backgrounds/4x3/%s", sn_data_root(), platform_dirs[platform]);
    snprintf(wide, sizeof wide, "%s/assets/backgrounds/3x2/%s", sn_data_root(), platform_dirs[platform]);
    int written;
    /* Once a system adopts the new layout, a missing matching variant is an
       empty choice. Never mix old copies or substitute the other panel's art. */
    if (background_is_directory(narrow) || background_is_directory(wide))
        written = snprintf(out, size, "%s", !strcmp(background_aspect_folder(), "4x3") ? narrow : wide);
    else
        written = snprintf(out, size, "%s/assets/backgrounds/%s", sn_data_root(), platform_dirs[platform]);
    if (written < 0 || (size_t)written >= size) { out[0] = '\0'; return 0; }
    return 1;
}

static int background_filename_valid(const char *filename) {
    if (!filename || !filename[0] || filename[0] == '.' || strlen(filename) >= 256) return 0;
    for (const unsigned char *p = (const unsigned char *)filename; *p; p++)
        if (*p < 32 || *p == '/' || *p == '\\' || *p == ':') return 0;
    return has_ext(filename, ".png") || has_ext(filename, ".jpg") ||
           has_ext(filename, ".jpeg") || has_ext(filename, ".svg");
}

static int background_file_path(int platform, const char *filename, char *out, size_t size) {
    if (!size) return 0;
    out[0] = '\0';
    char directory[900];
    if (!background_filename_valid(filename) || !background_directory(platform, directory, sizeof directory)) return 0;
    int written = snprintf(out, size, "%s/%s", directory, filename);
    if (written < 0 || (size_t)written >= size) { out[0] = '\0'; return 0; }
    return 1;
}

static int background_download_directory(int platform, char *out, size_t size) {
    if (!size) return 0;
    out[0] = '\0';
    if (platform < 0 || platform >= PLATFORM_COUNT) return 0;
    int written = snprintf(out, size, "%s/assets/backgrounds/%s/%s", sn_data_root(),
                           background_aspect_folder(), platform_dirs[platform]);
    if (written < 0 || (size_t)written >= size) { out[0] = '\0'; return 0; }
    return 1;
}

static int background_cache_aspect = -1;
static void background_sync_screen(void) {
    int aspect = !strcmp(background_aspect_folder(), "3x2");
    if (background_cache_aspect == aspect) return;
    background_cache_aspect = aspect;
    for (int p = 0; p < PLATFORM_COUNT; p++) invalidate_carousel_bg(p);
}

static int background_entry_filter(const struct dirent *entry) {
    return background_filename_valid(entry->d_name);
}

static int background_entry_compare(const struct dirent **a, const struct dirent **b) {
    int folded = strcasecmp((*a)->d_name, (*b)->d_name);
    return folded ? folded : strcmp((*a)->d_name, (*b)->d_name);
}

int list_backgrounds_for_platform(int platform, char filenames[][256], int max_count) {
    background_sync_screen();
    if (!filenames || max_count <= 0) return 0;
    char directory[900];
    if (!background_directory(platform, directory, sizeof directory)) return 0;
    struct dirent **entries = NULL;
    int total = scandir(directory, &entries, background_entry_filter, background_entry_compare);
    if (total < 0) return 0;
    int count = 0;
    for (int i = 0; i < total; i++) {
        if (count < max_count && (!count || strcasecmp(filenames[count-1], entries[i]->d_name))) {
            char path[1200]; struct stat st;
            snprintf(path, sizeof path, "%s/%s", directory, entries[i]->d_name);
            if (lstat(path, &st) == 0 && S_ISREG(st.st_mode)) {
                snprintf(filenames[count], 256, "%s", entries[i]->d_name);
                count++;
            }
        }
        free(entries[i]);
    }
    free(entries);
    return count;
}
#endif
