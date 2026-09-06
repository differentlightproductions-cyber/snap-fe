#ifndef SNAP_TEST_WEATHER_REFRESH_H
#define SNAP_TEST_WEATHER_REFRESH_H
#ifdef SNAP_WEATHER_REFRESH_TEST_MAIN
#ifndef SNAPFE_TESTING
#define SNAPFE_TESTING 1
#endif
#define main snapfe_application_main
#include "../main.c"
#undef main
#include <assert.h>
#endif

/* PATH substitution confines every weather request to this deterministic
   fixture. No provider calls or writes to a running frontend's cache occur. */
static void weather_test_mode(const char *root,const char *mode) {
    char path[900];snprintf(path,sizeof path,"%s/mode",root);FILE *f=fopen(path,"w");assert(f);fprintf(f,"%s\n",mode);fclose(f);
}
static void weather_test_curl(const char *root) {
    char path[900];snprintf(path,sizeof path,"%s/bin",root);assert(!mkdir(path,0755));
    snprintf(path,sizeof path,"%s/bin/curl",root);FILE *f=fopen(path,"w");assert(f);
    fputs("#!/bin/sh\n"
          "case \"$*\" in *Las*Vegas*) city=vegas ;; *Pyongyang*) city=pyongyang ;; *) city=local ;; esac\n"
          "printf '%s\\n' \"$*\" > \"$SNAP_TEST_WEATHER_ROOT/request-$city\"\n"
          "printf '%s\\n' \"$city\" >> \"$SNAP_TEST_WEATHER_ROOT/requests\"\n"
          "if [ \"$city\" = vegas ]; then sleep 0.20; fi\n"
          "mode=$(cat \"$SNAP_TEST_WEATHER_ROOT/mode\" 2>/dev/null)\n"
          "if [ \"$mode\" = fail ]; then printf 'server unavailable\\n'; exit 22; fi\n"
          "if [ \"$mode\" = invalid ]; then printf 'Unknown location\\n'; exit 0; fi\n"
          "case \"$*\" in *'&m'*) unit=C ;; *) unit=F ;; esac\n"
          "case \"$city:$unit\" in vegas:F) temperature=95 ;; vegas:C) temperature=35 ;; pyongyang:F) temperature=77 ;; pyongyang:C) temperature=25 ;; local:F) temperature=68 ;; *) temperature=20 ;; esac\n"
          "printf '+%s\\302\\260%s Clear|06:16:18|19:01:57\\n' \"$temperature\" \"$unit\"\n",f);
    assert(!fclose(f));assert(!chmod(path,0755));
}
static void weather_test_config(const char *root,int custom) {
    char path[900];snprintf(path,sizeof path,"%s/widget-places.cfg",root);FILE *f=fopen(path,"w");assert(f);
    fprintf(f,"S\t0\t2\n0\tLocal\t\t\n1\tLocal\t\t\n");
    if(custom)fprintf(f,"1\tUser Coast\tAmerica/Los_Angeles\tMy custom bay\n");
    else fprintf(f,"1\tPyongyang\tAsia/Pyongyang\tPyongyang, Korea (North)\n");
    fprintf(f,"1\tLas Vegas\tAmerica/Los_Angeles\tLas Vegas, Nevada, United State\n");fclose(f);
}
static void weather_test_wait_reading(const char *wanted,const char *forbidden) {
    Uint32 started=SDL_GetTicks();
    do {
        weather_poll();weather_read();
        if(forbidden)assert(!strstr(g_weather_str,forbidden));
        if(strstr(g_weather_str,wanted))return;
        weather_kick(0);SDL_Delay(5);
    }while(SDL_GetTicks()-started<4000);
    fprintf(stderr,"Expected weather '%s'; got '%s'\n",wanted,g_weather_str);assert(0);
}
static void weather_test_wait_failure(void) {
    Uint32 started=SDL_GetTicks();
    do {weather_poll();weather_read();if(g_weather_failed)return;SDL_Delay(5);}while(SDL_GetTicks()-started<4000);
    assert(g_weather_failed);
}
static int weather_test_requests(const char *root) {
    char path[900];snprintf(path,sizeof path,"%s/requests",root);FILE *f=fopen(path,"r");if(!f)return 0;
    int count=0,c;while((c=fgetc(f))!=EOF)if(c=='\n')count++;fclose(f);return count;
}

static void test_weather_refresh(void) {
    char reply[80];int rise,set;
    assert(weather_parse_reply("+88\xc2\xb0" "F Clear |06:16:18|19:01:57",reply,&rise,&set));
    assert(strstr(reply,"88")&&strstr(reply,"Clear")&&rise==376&&set==1141);
    assert(weather_parse_reply("-8\xc2\xb0" "C Snow|07:30|17:10",reply,&rise,&set));
    assert(strstr(reply,"-8")&&rise==450&&set==1030);
    const char *bad[]={"","<html>500 server error</html>","Unknown location","HTTP/1.1 500 Internal Server Error","+88\xc2\xb0" "F "};
    for(size_t i=0;i<sizeof bad/sizeof bad[0];i++)assert(!weather_parse_reply(bad[i],reply,&rise,&set));
    weather_stop();int saved_unit=weather_unit;
    char temporary[]="/tmp/snapfe-weather-check-XXXXXX";assert(mkdtemp(temporary));
    const char *v=getenv("SNAPFE_DATA_ROOT");char *saved_root=v?strdup(v):NULL;
    v=getenv("PATH");char *saved_path=v?strdup(v):NULL;
    v=getenv("SNAP_TEST_WEATHER_ROOT");char *saved_mock=v?strdup(v):NULL;
    assert(!setenv("SNAPFE_DATA_ROOT",temporary,1));assert(!setenv("SNAP_TEST_WEATHER_ROOT",temporary,1));
    weather_test_curl(temporary);weather_test_mode(temporary,"success");
    char path[4096];snprintf(path,sizeof path,"%s/bin:%s",temporary,saved_path?saved_path:"");assert(!setenv("PATH",path,1));
    weather_test_config(temporary,0);widget_places_loaded=0;widget_places_init();
    assert(widget_place_count[1]==3&&!strcmp(widget_places[1][2].name,"Las Vegas"));
    assert(!strcmp(widget_places[1][2].query,"Las Vegas, Nevada, United States"));
    assert(!strcmp(widget_places[1][1].query,"Pyongyang, Korea (North)"));assert(!widget_places[1][0].query[0]);
    weather_test_config(temporary,1);widget_places_loaded=0;widget_places_init();
    assert(!strcmp(widget_places[1][1].query,"My custom bay")&&!widget_places[1][0].query[0]);
    weather_test_config(temporary,0);widget_places_loaded=0;widget_places_init();
    weather_test_fetch_enabled=1;weather_unit=0;widget_place_select(1,2);
    Uint64 began=SDL_GetPerformanceCounter();weather_kick(1);
    assert(1000.0*(SDL_GetPerformanceCounter()-began)/SDL_GetPerformanceFrequency()<100);
    widget_place_select(1,1);weather_kick(1);
    weather_test_wait_reading("77","95");
    widget_place_select(1,2);weather_read();assert(strstr(g_weather_str,"95"));
    // Manual refresh must survive another city's in-flight fetch even when
    // the selected city's recent cache would suppress an automatic request.
    int before_queue=weather_test_requests(temporary);weather_kick(1);
    SDL_Thread *in_flight=weather_thread;assert(in_flight&&weather_refreshing());
    widget_place_select(1,1);assert(strstr(g_weather_str,"77")&&!weather_refreshing());
    assert(g_weather_kicked&&SDL_GetTicks()-g_weather_kicked<weather_refresh_period());
    for(int i=0;i<10;i++)weather_kick(1);
    assert(weather_thread==in_flight&&weather_force_pending&&weather_refreshing());
    Uint32 queue_started=SDL_GetTicks();
    do {
        weather_poll();weather_read();weather_kick(0);assert(strstr(g_weather_str,"77")&&!strstr(g_weather_str,"95"));
        if(!weather_thread&&!weather_force_pending&&weather_test_requests(temporary)>=before_queue+2)break;
        SDL_Delay(5);
    }while(SDL_GetTicks()-queue_started<4000);
    assert(!weather_thread&&!weather_force_pending&&!weather_refreshing()&&weather_test_requests(temporary)==before_queue+2);
    widget_place_select(1,2);weather_read();assert(strstr(g_weather_str,"95"));
    // An in-flight Fahrenheit request may finish after the Celsius selection.
    weather_kick(1);weather_unit=1;widget_weather_restore();weather_kick(1);
    weather_test_wait_reading("35","95");
    char cache[900];widget_weather_path(cache,sizeof cache);assert(!strncmp(cache,temporary,strlen(temporary)));
    time_t good_at=g_weather_at;weather_test_mode(temporary,"fail");weather_kick(1);weather_test_wait_failure();
    assert(strstr(g_weather_str,"35")&&g_weather_at==good_at&&weather_refresh_period()==30000u);
    weather_test_mode(temporary,"invalid");g_weather_failed=0;weather_kick(1);weather_test_wait_failure();assert(strstr(g_weather_str,"35"));
    weather_test_mode(temporary,"success");
    int requests=weather_test_requests(temporary);g_weather_kicked=SDL_GetTicks();weather_kick(0);SDL_Delay(30);weather_poll();assert(weather_test_requests(temporary)==requests);
    g_weather_kicked=SDL_GetTicks()-30001u;weather_kick(0);
    Uint32 retry_started=SDL_GetTicks();while(g_weather_failed&&SDL_GetTicks()-retry_started<4000){weather_poll();weather_read();SDL_Delay(5);}
    assert(!g_weather_failed&&weather_test_requests(temporary)>requests&&strstr(g_weather_str,"35"));
    assert(weather_refresh_period()==1800000u);
    g_weather_str[0]=0;assert(weather_refresh_period()==30000u);
    weather_kick(1);widget_place_select(1,1);weather_kick(1);assert(weather_force_pending);
    weather_stop();assert(!weather_thread&&!weather_force_pending&&!weather_refreshing());weather_test_fetch_enabled=0;
    weather_unit=saved_unit;
    if(saved_root){setenv("SNAPFE_DATA_ROOT",saved_root,1);free(saved_root);}else unsetenv("SNAPFE_DATA_ROOT");
    if(saved_path){setenv("PATH",saved_path,1);free(saved_path);}else unsetenv("PATH");
    if(saved_mock){setenv("SNAP_TEST_WEATHER_ROOT",saved_mock,1);free(saved_mock);}else unsetenv("SNAP_TEST_WEATHER_ROOT");
    widget_places_loaded=0;g_weather_str[0]=0;g_weather_at=g_weather_mtime=0;g_weather_kicked=0;g_weather_failed=0;
    puts("PASS: canonical weather query migration, preserved custom/Local places, strict replies, asynchronous three-slot/unit isolation, queued manual refresh of a cached city, retained good data on failure, 30-second retry / 30-minute refresh, and pending-request cleanup");
}
#ifdef SNAP_WEATHER_REFRESH_TEST_MAIN
int main(void) {assert(!SDL_Init(SDL_INIT_TIMER));test_weather_refresh();SDL_Quit();return 0;}
#endif
#endif
