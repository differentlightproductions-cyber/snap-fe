#ifndef SNAP_WEATHER_FETCH_H
#define SNAP_WEATHER_FETCH_H

typedef struct {char query[160],path[768];int unit,success;} WeatherRequest;
static WeatherRequest weather_request;
static SDL_Thread *weather_thread;
static SDL_atomic_t weather_done;
static char weather_force_query[160];
static int weather_force_unit,weather_force_pending;
#ifdef SNAPFE_TESTING
static int weather_test_fetch_enabled;
#endif

static int weather_parse_reply(const char *reply,char out[80],int *rise,int *set){
    char text[256];out[0]=0;*rise=*set=-1;
    if(!reply||strlen(reply)>=sizeof text)return 0;
    snprintf(text,sizeof text,"%s",reply);
    size_t n=strlen(text);while(n&&isspace((unsigned char)text[n-1]))text[--n]=0;
    char *sunrise=strchr(text,'|'),*sunset=NULL;
    if(sunrise){*sunrise++=0;sunset=strchr(sunrise,'|');if(sunset)*sunset++=0;}
    n=strlen(text);while(n&&isspace((unsigned char)text[n-1]))text[--n]=0;
    if(!n||n>=80||strpbrk(text,"<>{}\r\n"))return 0;
    char *unit;double temperature=strtod(text,&unit);
    if(unit==text||!isfinite(temperature)||temperature < -150 || temperature > 180)return 0;
    if((unsigned char)unit[0]==0xc2&&(unsigned char)unit[1]==0xb0)unit+=2;
    if(*unit!='F'&&*unit!='C')return 0;
    unit++;if(!isspace((unsigned char)*unit))return 0;
    while(isspace((unsigned char)*unit))unit++;
    if(!*unit)return 0;
    for(const unsigned char *p=(const unsigned char*)unit;*p;p++)if(*p<32||*p==127)return 0;
    weather_trim_plus(text);snprintf(out,80,"%s",text);
    if(sunrise&&sunset){*rise=weather_clock_minute(sunrise);*set=weather_clock_minute(sunset);}
    return 1;
}

static int weather_fetch_worker(void *data){
    WeatherRequest *request=data;
    char encoded[512];size_t used=0;
    for(const unsigned char *p=(const unsigned char*)request->query;*p&&used+4<sizeof encoded;p++){
        if(isalnum(*p)||*p==','||*p=='-'||*p=='.')encoded[used++]=(char)*p;
        else if(*p==' ')encoded[used++]='+';
        else{snprintf(encoded+used,sizeof encoded-used,"%%%02X",*p);used+=3;}
    }
    encoded[used]=0;
    char command[800];
    snprintf(command,sizeof command,"curl -fsS --connect-timeout 3 --max-time 8 'https://wttr.in/%s?format=%%t+%%C%%7C%%S%%7C%%s&%s' 2>/dev/null",encoded,request->unit?"m":"u");
    FILE *pipe=popen(command,"r");char reply[256]="",chunk[256];size_t length=0;int overflow=0,status=-1;
    if(pipe){size_t got;while((got=fread(chunk,1,sizeof chunk,pipe))>0){
        if(length+got>=sizeof reply)overflow=1;
        if(!overflow){memcpy(reply+length,chunk,got);length+=got;reply[length]=0;}
    }status=pclose(pipe);}
    char reading[80];int rise,set;
    request->success=0;
    if(status==0&&!overflow&&weather_parse_reply(reply,reading,&rise,&set)){
        char temporary[800];snprintf(temporary,sizeof temporary,"%s.tmp",request->path);
        FILE *file=fopen(temporary,"w");
        if(file){int ok=fputs(reply,file)>=0;if(fclose(file)!=0)ok=0;
            if(ok&&rename(temporary,request->path)==0)request->success=1;
            else unlink(temporary);
        }
    }
    SDL_AtomicSet(&weather_done,1);return 0;
}

static void weather_poll(void){
    if(!weather_thread||!SDL_AtomicGet(&weather_done))return;
    SDL_WaitThread(weather_thread,NULL);weather_thread=NULL;
    if(weather_unit==weather_request.unit&&!strcmp(weather_loc,weather_request.query)){
        g_weather_failed=!weather_request.success;
        if(weather_request.success)g_weather_mtime=0;
    }
    if(widget_places_loaded)for(int i=0;i<widget_place_count[1];i++){
        WidgetPlace *place=&widget_places[1][i];
        const char *query=!strcmp(place->name,"Local")?"":place->query[0]?place->query:place->name;
        if(place->unit!=weather_request.unit||strcmp(query,weather_request.query))continue;
        place->failed=!weather_request.success;if(weather_request.success)place->mtime=0;
    }
}

static Uint32 weather_refresh_period(void){
    return g_weather_failed||!g_weather_str[0]?WEATHER_RETRY_MS:WEATHER_PERIOD_MS;
}
static void weather_kick(int force){
    weather_poll();
#ifdef SNAPFE_TESTING
    if(!weather_test_fetch_enabled)return;
#endif
    if(weather_thread){
        // Preserve a manual refresh for a different city/unit until this fetch finishes.
        if(force&&(weather_request.unit!=weather_unit||strcmp(weather_request.query,weather_loc))){
            snprintf(weather_force_query,sizeof weather_force_query,"%s",weather_loc);
            weather_force_unit=weather_unit;weather_force_pending=1;
        }
        return; // Repeated refresh presses cannot launch duplicate requests.
    }
    if(weather_force_pending&&weather_force_unit==weather_unit&&!strcmp(weather_force_query,weather_loc)){
        force=1;weather_force_pending=0;
    }
    Uint32 now=SDL_GetTicks();
    if(!force&&g_weather_kicked&&now-g_weather_kicked<weather_refresh_period())return;
    g_weather_kicked=now?now:1;
    snprintf(weather_request.query,sizeof weather_request.query,"%s",weather_loc);
    weather_request.unit=weather_unit;weather_request.success=0;
    widget_weather_path(weather_request.path,sizeof weather_request.path);
    SDL_AtomicSet(&weather_done,0);
    weather_thread=SDL_CreateThread(weather_fetch_worker,"weather",&weather_request);
    if(!weather_thread)g_weather_failed=1;
}

static void weather_read(void){
    weather_poll();
    char path[768];widget_weather_path(path,sizeof path);
    struct stat info;if(stat(path,&info)!=0||info.st_mtime==g_weather_mtime)return;
    FILE *file=fopen(path,"r");if(!file)return;
    char reply[256]="";size_t length=fread(reply,1,sizeof reply-1,file);reply[length]=0;
    int complete=feof(file);fclose(file);g_weather_mtime=info.st_mtime;
    char reading[80];int rise,set;
    if(complete&&weather_parse_reply(reply,reading,&rise,&set)){
        snprintf(g_weather_str,sizeof g_weather_str,"%s",reading);g_weather_at=info.st_mtime;
        if(rise>=0&&set>=0&&widget_place_sel[1]==0){g_sunrise_minute=rise;g_sunset_minute=set;g_sun_times_at=info.st_mtime;}
    }
}
static const char *weather_empty_message(void){
    return g_weather_failed?"Unavailable - retrying...":"Fetching weather...";
}
static int weather_refreshing(void){
    return (weather_thread&&weather_request.unit==weather_unit&&!strcmp(weather_request.query,weather_loc))||
           (weather_force_pending&&weather_force_unit==weather_unit&&!strcmp(weather_force_query,weather_loc));
}
static void weather_stop(void){
    if(weather_thread){SDL_WaitThread(weather_thread,NULL);weather_thread=NULL;}
    weather_force_pending=0;
}
#endif
