#ifndef SNAP_TEST_LOCATION_CATALOG_H
#define SNAP_TEST_LOCATION_CATALOG_H
#ifdef SNAP_LOCATION_CATALOG_TEST_FALLBACK
#define WLOC_ZONEINFO_ROOT "/nonexistent-snapfe-test-zoneinfo"
#endif
#include "../location_catalog.h"
#include <assert.h>

static void test_location_catalog(void) {
    int results[24];wloc_init();assert(wloc_count>=90 && wloc_count<=WLOC_CAPACITY);
    struct {const char *query,*name,*zone;} cases[]={
        {"Tokoyo","Tokyo","Asia/Tokyo"},
        {"Phonix","Phoenix","America/Phoenix"},
        {"New Yrok","New York","America/New_York"},
        {"new_york","New York","America/New_York"},
        {"MIAMI","Miami","America/New_York"},
        {"Seaattle","Seattle","America/Los_Angeles"},
        {"S\xc3\xa3o Paulo","Sao Paulo","America/Sao_Paulo"},
        {"m\xc3\xbcnchen","Munich","Europe/Berlin"}
    };
    /* Preserve local-language aliases where the timezone city is an exonym. */
    int munich=wloc_add("M\xc3\xbcnchen","Europe/Berlin","Germany");assert(munich>=0);
    for (size_t i=0;i<sizeof cases/sizeof cases[0];i++) {
        int n=wloc_search(cases[i].query,results,24);assert(n>0);
        assert(!strcmp(wloc_items[results[0]].zone,cases[i].zone));
        if (i+1<sizeof cases/sizeof cases[0]) assert(!strcmp(wloc_items[results[0]].name,cases[i].name));
    }
    int count=wloc_search("San Jose",results,24);assert(count>=2);
    assert(!strcmp(wloc_items[results[0]].name,"San Jose") && !strcmp(wloc_items[results[1]].name,"San Jose"));
    assert(strcmp(wloc_items[results[0]].zone,wloc_items[results[1]].zone));
    count=wloc_search("san",results,24);assert(count>=4);
    int saved=wloc_count;assert(wloc_add(" New_York ","America/New_York","")>=0&&wloc_count==saved);
    const char *bad[]={"/etc/passwd","../UTC","America/../UTC","America//New_York","America/New_York/","UTC\n","America/New.York",""};
    for (size_t i=0;i<sizeof bad/sizeof bad[0];i++) {
        assert(!wloc_valid_zone(bad[i]));assert(wloc_add("Unsafe",bad[i],"")==-1);
    }
    assert(wloc_valid_zone("America/Argentina/Buenos_Aires") && wloc_valid_zone("Etc/GMT+7") && wloc_valid_zone("UTC"));
    for (int i=0;i<wloc_count;i++) assert(wloc_valid_zone(wloc_items[i].zone));
    assert(wloc_search("zzzzzzzzzzzzzzzzzzzz",results,24)==0);
    assert(wloc_search("Tokyo",results,0)==0 && wloc_search("Tokyo",NULL,24)==0);
    int bounded[3]={-7,-7,-7};assert(wloc_search("san",bounded+1,1)==1 && bounded[0]==-7 && bounded[2]==-7);
    char long_query[512];memset(long_query,'q',sizeof long_query-1);long_query[sizeof long_query-1]=0;
    assert(wloc_search(long_query,results,24)==0);
    char normalized[WLOC_NORMAL_SIZE];wloc_normalize("\xe2\x82",normalized);assert(!normalized[0]);
    printf("PASS: %d offline city choices; ranked typos, ambiguous cities, accents, bounds, and safe timezone identifiers\n",wloc_count);
}
#ifdef SNAP_LOCATION_CATALOG_TEST_MAIN
int main(void) {test_location_catalog();return 0;}
#endif
#endif
