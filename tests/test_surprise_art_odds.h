#ifndef SNAP_TEST_SURPRISE_ART_ODDS_H
#define SNAP_TEST_SURPRISE_ART_ODDS_H
#ifdef SNAP_SURPRISE_ART_TEST_MAIN
#define main snapfe_application_main
#include "../main.c"
#undef main
#include <assert.h>
#endif
static void surprise_test_png(const char *path) {
    SDL_Surface *s=SDL_CreateRGBSurfaceWithFormat(0,4,4,32,SDL_PIXELFORMAT_RGBA32);assert(s);
    SDL_FillRect(s,NULL,SDL_MapRGBA(s->format,35,90,160,255));assert(IMG_SavePNG(s,path)==0);SDL_FreeSurface(s);
}
static void test_surprise_art_metadata(void) {
    char temporary[]="/tmp/snapfe-surprise-check-XXXXXX";assert(mkdtemp(temporary));
    const char *old=getenv("SNAPFE_DATA_ROOT");char *saved=old?strdup(old):NULL;
    int old_nr=g_roms_nroots,old_type=display_art_idx;char old_roots[sizeof g_roms_roots];memcpy(old_roots,g_roms_roots,sizeof old_roots);
    assert(!setenv("SNAPFE_DATA_ROOT",temporary,1));g_roms_nroots=1;snprintf(g_roms_roots[0],512,"%s/roms",temporary);
    char path[900];const char *directories[]={"boxart","boxart/nes","boxart/nes/box2d","boxart/nes/logo","roms","roms/nes","roms/nes/images","custom"};
    for (size_t i=0;i<sizeof directories/sizeof directories[0];i++) {snprintf(path,sizeof path,"%s/%s",temporary,directories[i]);assert(!mkdir(path,0755));}
    snprintf(path,sizeof path,"%s/boxart/nes/box2d/WGame0000.png",temporary);surprise_test_png(path);
    snprintf(path,sizeof path,"%s/roms/nes/images/WGame0001-image.png",temporary);surprise_test_png(path);
    snprintf(path,sizeof path,"%s/custom/special.png",temporary);surprise_test_png(path);
    snprintf(path,sizeof path,"%s/boxart/nes/logo/WGame0003.png",temporary);surprise_test_png(path);
    // A directory and an extension the loader does not probe must not count.
    snprintf(path,sizeof path,"%s/boxart/nes/box2d/WGame0004.png",temporary);assert(!mkdir(path,0755));
    snprintf(path,sizeof path,"%s/boxart/nes/box2d/WGame0005.PNG",temporary);surprise_test_png(path);
    snprintf(path,sizeof path,"%s/roms/nes/gamelist.xml",temporary);FILE *f=fopen(path,"w");assert(f);
    fprintf(f,"<gameList><game><path>./wgame0002.nes</path><image>%s/custom/special.png</image></game></gameList>",temporary);fclose(f);
    if(gl_mtx)SDL_LockMutex(gl_mtx);gl_cache_plat[0]='\1';gl_cache_plat[1]=0;if(gl_mtx)SDL_UnlockMutex(gl_mtx);
    int platform=-1;for(int p=0;p<PLATFORM_COUNT;p++)if(!strcmp(platform_dirs[p],"nes"))platform=p;assert(platform>=0);
    surprise_path_cache_n=0;
    for (int i=0;i<MAX_GAMES;i++) {snprintf(path,sizeof path,"%s/roms/nes/WGame%04d.nes",temporary,i);surprise_cache_add(path,platform);}
    display_art_idx=0;Uint64 started=SDL_GetPerformanceCounter();surprise_art_pools_build();double cold=1000.0*(SDL_GetPerformanceCounter()-started)/SDL_GetPerformanceFrequency();
    assert(surprise_pool_n[1]==3&&surprise_pool_n[0]==MAX_GAMES-3);
    assert(surprise_has_art[0]&&surprise_has_art[1]&&surprise_has_art[2]);assert(!surprise_has_art[3]&&!surprise_has_art[4]&&!surprise_has_art[5]);
    surprise_cache_generation=library_index_generation;started=SDL_GetPerformanceCounter();
    for(int i=0;i<1000;i++)surprise_cache_prepare();double warm=1000.0*(SDL_GetPerformanceCounter()-started)/SDL_GetPerformanceFrequency();
    assert(cold<2000&&warm<200);display_art_idx=4;surprise_cache_prepare();assert(surprise_pool_n[1]==1&&surprise_has_art[3]);
    // A populated art directory exercises the index as well as its ROM side.
    char source[900];snprintf(source,sizeof source,"%s/boxart/nes/box2d/WGame0000.png",temporary);
    for(int i=1;i<MAX_GAMES;i++){snprintf(path,sizeof path,"%s/boxart/nes/box2d/WGame%04d.png",temporary,i);assert(!link(source,path)||errno==EEXIST);}
    display_art_idx=0;started=SDL_GetPerformanceCounter();surprise_cache_prepare();double dense=1000.0*(SDL_GetPerformanceCounter()-started)/SDL_GetPerformanceFrequency();
    assert(surprise_pool_n[1]==MAX_GAMES-1&&surprise_pool_n[0]==1&&dense<2000);
    printf("PASS: Surprise metadata matching, gamelist case matching, selected art type, and %d-game preparation: %.3fms sparse / %.3fms full art directory / %.3fms for 1000 cached checks\n",MAX_GAMES,cold,dense,warm);
    surprise_path_cache_n=0;surprise_cache_generation=0;surprise_cache_art_type=-1;surprise_pool_n[0]=surprise_pool_n[1]=0;
    display_art_idx=old_type;g_roms_nroots=old_nr;memcpy(g_roms_roots,old_roots,sizeof old_roots);
    if(saved){setenv("SNAPFE_DATA_ROOT",saved,1);free(saved);}else unsetenv("SNAPFE_DATA_ROOT");
    if(gl_mtx)SDL_LockMutex(gl_mtx);gl_cache_plat[0]='\1';gl_cache_plat[1]=0;if(gl_mtx)SDL_UnlockMutex(gl_mtx);
}
static void test_surprise_art_odds(void) {
    const int art[]={10,11},plain[]={20,21,22,23,24,25,26,27};
    int totals[28]={0},art_total=0,plain_total=0;
    // An intentionally uneven pool size catches per-game weighting, which
    // would give a different outcome from an explicit 80/20 pool choice.
    for (unsigned seed=1;seed<=12;seed++) {
        srand(seed*997u);int per_seed_art=0;
        for (int i=0;i<10000;i++) {
            int chosen=surprise_weighted_index(art,2,plain,8);
            assert((chosen>=10&&chosen<=11)||(chosen>=20&&chosen<=27));totals[chosen]++;
            if (chosen<20) {art_total++;per_seed_art++;} else plain_total++;
        }
        assert(per_seed_art>7700&&per_seed_art<8300);
    }
    assert(art_total>94800&&art_total<97200 && plain_total>22800&&plain_total<25200);
    for (int i=10;i<=11;i++) assert(totals[i]>46500&&totals[i]<49500);
    for (int i=20;i<=27;i++) assert(totals[i]>2700&&totals[i]<3300);
    assert(surprise_weighted_index(NULL,0,NULL,0)==-1);
    for (int i=0;i<1000;i++) {
        int a=surprise_weighted_index(art,2,NULL,0),b=surprise_weighted_index(NULL,0,plain,8);
        assert(a==10||a==11);assert(b>=20&&b<=27);
        assert(surprise_weighted_index(art,1,NULL,0)==10);
        assert(surprise_weighted_index(NULL,0,plain,1)==20);
    }
    printf("PASS: Surprise Me artwork odds %.2f%% / %.2f%% across 120000 seeded picks; uniform pools and all-art/no-art/empty fallbacks\n",
           art_total/1200.0,plain_total/1200.0);
    test_surprise_art_metadata();
}
#ifdef SNAP_SURPRISE_ART_TEST_MAIN
int main(void) {assert(SDL_Init(0)==0);test_surprise_art_odds();SDL_Quit();return 0;}
#endif
#endif
