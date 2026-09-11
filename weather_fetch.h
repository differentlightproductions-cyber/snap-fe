#ifndef SNAP_WEATHER_FETCH_H
#define SNAP_WEATHER_FETCH_H

#define WEATHER_REPLY_MAX 12288

typedef struct {char query[160],path[768];int unit,auto_ip,success;} WeatherRequest;
static WeatherRequest weather_request;
static SDL_Thread *weather_thread;
static SDL_atomic_t weather_done;
static char weather_force_query[160];
static int weather_force_unit,weather_force_pending;
#ifdef SNAPFE_TESTING
static int weather_test_fetch_enabled;
#endif
/* Is there a network at all?
   A fetch with no connection is a curl that sits there until it times out, and
   because a failure shortens the refresh period to 30 seconds, an offline
   handheld would retry forever -- waking the CPU and the radio twice a minute
   for someone who just wants to play Game Boy games on a bus. Every background
   fetch asks this first. The answer is cached: it is a few bytes from sysfs,
   but there is no reason to read it more than twice a minute. */
#define WEATHER_NET_RECHECK_MS 30000u
static Uint32 weather_net_checked;
static int weather_net_up = 1;
static int weather_network_up(void){
#ifdef SNAPOS_TARGET_KNULLI
    Uint32 now=SDL_GetTicks();
    if(weather_net_checked&&now-weather_net_checked<WEATHER_NET_RECHECK_MS)return weather_net_up;
    weather_net_checked=now?now:1;
    weather_net_up=0;
    static const char *ifaces[]={"wlan0","eth0","wlan1",NULL};
    for(int i=0;ifaces[i]&&!weather_net_up;i++){
        char path[128];snprintf(path,sizeof path,"/sys/class/net/%s/operstate",ifaces[i]);
        FILE *f=fopen(path,"r");if(!f)continue;
        char state[16]="";
        if(fgets(state,sizeof state,f)&&!strncmp(state,"up",2))weather_net_up=1;
        fclose(f);
    }
    return weather_net_up;
#else
    return 1;   /* desktop dev builds always try */
#endif
}

/* The cache file only changes when a fetch lands, so stat()-ing it on every
   frame is dozens of syscalls a second to answer a question whose answer
   changes at most twice an hour. weather_poll() still runs every call -- it is
   what joins a finished fetch -- and clears this throttle so a new reading
   still appears immediately. */
#define WEATHER_READ_POLL_MS 500u
static Uint32 weather_read_checked;


static void weather_detail_clear(void){
    g_weather_place[0]=g_weather_condition[0]=g_weather_feels[0]=0;
    g_weather_high[0]=g_weather_low[0]=g_weather_precip[0]=0;
    g_weather_wind[0]=g_weather_humidity[0]=g_weather_updated[0]=0;
    g_weather_hourly_count=g_weather_daily_count=0;
    g_weather_daily_date_count=0;
    for(int i=0;i<WEATHER_HOURLY_COUNT;i++)g_weather_hourly[i][0]=0;
    for(int i=0;i<WEATHER_DAILY_COUNT;i++){g_weather_daily[i][0]=0;g_weather_daily_date[i][0]=0;}
    g_weather_alert_level=0;g_weather_alert[0]=0;
}

static void weather_strip(char *s){
    if(!s)return;
    char *p=s;while(isspace((unsigned char)*p))p++;
    if(p!=s)memmove(s,p,strlen(p)+1);
    size_t n=strlen(s);
    while(n&&isspace((unsigned char)s[n-1]))s[--n]=0;
}

static int weather_summary_valid(const char *text,char out[80]){
    char tmp[256];out[0]=0;
    if(!text||strlen(text)>=sizeof tmp)return 0;
    snprintf(tmp,sizeof tmp,"%s",text);weather_strip(tmp);
    if(!tmp[0]||strlen(tmp)>=80||strpbrk(tmp,"<>{}\r\n"))return 0;
    char *unit;double temperature=strtod(tmp,&unit);
    if(unit==tmp||!isfinite(temperature)||temperature<-150||temperature>180)return 0;
    if((unsigned char)unit[0]==0xc2&&(unsigned char)unit[1]==0xb0)unit+=2;
    if(*unit!='F'&&*unit!='C')return 0;
    unit++;if(!isspace((unsigned char)*unit))return 0;
    while(isspace((unsigned char)*unit))unit++;
    if(!*unit)return 0;
    for(const unsigned char *p=(const unsigned char*)unit;*p;p++)if(*p<32||*p==127)return 0;
    weather_trim_plus(tmp);snprintf(out,80,"%s",tmp);
    return 1;
}

static void weather_copy_clean(char *dst,size_t size,const char *src){
    char tmp[512];
    if(!dst||!size)return;
    snprintf(tmp,sizeof tmp,"%s",src?src:"");
    weather_strip(tmp);
    if(strpbrk(tmp,"<>{}\r\n"))tmp[0]=0;
    snprintf(dst,size,"%s",tmp);
}

static void weather_split_forecast(char rows[][64],int max_rows,int *count,const char *src){
    *count=0;
    if(!src||!*src)return;
    // Big enough for a full WEATHER_HOURLY_COUNT run of 63-character rows.
    char tmp[2048];snprintf(tmp,sizeof tmp,"%s",src);
    char *save=NULL;
    for(char *p=strtok_r(tmp,";",&save);p&&*count<max_rows;p=strtok_r(NULL,";",&save)){
        weather_strip(p);
        if(!p[0]||strpbrk(p,"<>{}\r\n"))continue;
        snprintf(rows[*count],64,"%s",p);
        (*count)++;
    }
}

static void weather_split_dates(const char *src){
    g_weather_daily_date_count=0;
    if(!src||!*src)return;
    char tmp[512];snprintf(tmp,sizeof tmp,"%s",src);
    char *save=NULL;
    for(char *p=strtok_r(tmp,";",&save);p&&g_weather_daily_date_count<WEATHER_DAILY_COUNT;p=strtok_r(NULL,";",&save)){
        weather_strip(p);
        if(!p[0]||strpbrk(p,"<>{}\r\n"))g_weather_daily_date[g_weather_daily_date_count][0]=0;
        else snprintf(g_weather_daily_date[g_weather_daily_date_count],16,"%s",p);
        g_weather_daily_date_count++;
    }
}

static int weather_parse_legacy(const char *reply,char out[80],int *rise,int *set,int commit){
    char text[256];out[0]=0;*rise=*set=-1;
    if(!reply||strlen(reply)>=sizeof text)return 0;
    snprintf(text,sizeof text,"%s",reply);
    size_t n=strlen(text);while(n&&isspace((unsigned char)text[n-1]))text[--n]=0;
    char *sunrise=strchr(text,'|'),*sunset=NULL;
    if(sunrise){*sunrise++=0;sunset=strchr(sunrise,'|');if(sunset)*sunset++=0;}
    if(!weather_summary_valid(text,out))return 0;
    if(sunrise&&sunset){*rise=weather_clock_minute(sunrise);*set=weather_clock_minute(sunset);}
    if(commit){
        weather_detail_clear();
        const char *sp=strchr(out,' ');
        weather_copy_clean(g_weather_condition,sizeof g_weather_condition,(sp&&sp[1])?sp+1:"Current conditions");
        weather_copy_clean(g_weather_place,sizeof g_weather_place,widget_place_name(1));
        weather_copy_clean(g_weather_updated,sizeof g_weather_updated,"cached");
    }
    return 1;
}

static int weather_parse_modern(const char *reply,char out[80],int *rise,int *set,int commit){
    if(!reply||strncmp(reply,"SNAPWEATHER 1",13)!=0)return 0;
    if(strlen(reply)>=WEATHER_REPLY_MAX)return 0;
    char text[WEATHER_REPLY_MAX];snprintf(text,sizeof text,"%s",reply);
    char summary[80]="",current[32]="",condition[80]="";
    char *save=NULL;int parsed=0;
    if(commit)weather_detail_clear();
    for(char *line=strtok_r(text,"\n",&save);line;line=strtok_r(NULL,"\n",&save)){
        line[strcspn(line,"\r")]=0;weather_strip(line);
        if(!line[0]||!strcmp(line,"SNAPWEATHER 1"))continue;
        char *eq=strchr(line,'=');
        if(!eq)continue;
        *eq++=0;weather_strip(line);weather_strip(eq);
        if(!strcmp(line,"summary"))weather_copy_clean(summary,sizeof summary,eq);
        else if(!strcmp(line,"current"))weather_copy_clean(current,sizeof current,eq);
        else if(!strcmp(line,"condition"))weather_copy_clean(condition,sizeof condition,eq);
        if(commit){
            if(!strcmp(line,"place"))weather_copy_clean(g_weather_place,sizeof g_weather_place,eq);
            else if(!strcmp(line,"condition"))weather_copy_clean(g_weather_condition,sizeof g_weather_condition,eq);
            else if(!strcmp(line,"feels"))weather_copy_clean(g_weather_feels,sizeof g_weather_feels,eq);
            else if(!strcmp(line,"high"))weather_copy_clean(g_weather_high,sizeof g_weather_high,eq);
            else if(!strcmp(line,"low"))weather_copy_clean(g_weather_low,sizeof g_weather_low,eq);
            else if(!strcmp(line,"precip"))weather_copy_clean(g_weather_precip,sizeof g_weather_precip,eq);
            else if(!strcmp(line,"wind"))weather_copy_clean(g_weather_wind,sizeof g_weather_wind,eq);
            else if(!strcmp(line,"humidity"))weather_copy_clean(g_weather_humidity,sizeof g_weather_humidity,eq);
            else if(!strcmp(line,"updated"))weather_copy_clean(g_weather_updated,sizeof g_weather_updated,eq);
            else if(!strcmp(line,"alert"))weather_copy_clean(g_weather_alert,sizeof g_weather_alert,eq);
            else if(!strcmp(line,"alert_level")){int level=atoi(eq);if(level<0)level=0;if(level>2)level=2;g_weather_alert_level=level;}
            else if(!strcmp(line,"hourly"))weather_split_forecast(g_weather_hourly,WEATHER_HOURLY_COUNT,&g_weather_hourly_count,eq);
            else if(!strcmp(line,"daily"))weather_split_forecast(g_weather_daily,WEATHER_DAILY_COUNT,&g_weather_daily_count,eq);
            else if(!strcmp(line,"daily_dates"))weather_split_dates(eq);
            else if(!strcmp(line,"sunrise"))*rise=weather_clock_minute(eq);
            else if(!strcmp(line,"sunset"))*set=weather_clock_minute(eq);
        }else{
            if(!strcmp(line,"sunrise"))*rise=weather_clock_minute(eq);
            else if(!strcmp(line,"sunset"))*set=weather_clock_minute(eq);
        }
        parsed=1;
    }
    if(!summary[0]&&current[0]&&condition[0])snprintf(summary,sizeof summary,"%s %s",current,condition);
    if(!weather_summary_valid(summary,out))return 0;
    if(commit&&!g_weather_condition[0])weather_copy_clean(g_weather_condition,sizeof g_weather_condition,condition[0]?condition:"Current conditions");
    return parsed;
}

static int weather_parse_reply_ex(const char *reply,char out[80],int *rise,int *set,int commit){
    *rise=*set=-1;
    if(reply&&strncmp(reply,"SNAPWEATHER 1",13)==0)return weather_parse_modern(reply,out,rise,set,commit);
    return weather_parse_legacy(reply,out,rise,set,commit);
}

static int weather_parse_reply(const char *reply,char out[80],int *rise,int *set){
    return weather_parse_reply_ex(reply,out,rise,set,1);
}

static void weather_shell_quote(char *out,size_t size,const char *src){
    size_t n=0;if(!size)return;
    out[n++]='\'';
    for(const char *p=src?src:"";*p&&n+5<size;p++){
        if(*p=='\''){memcpy(out+n,"'\\''",4);n+=4;}
        else out[n++]=*p;
    }
    if(n+1<size)out[n++]='\'';
    out[n<size?n:size-1]=0;
}

static int weather_read_command(const char *command,char *reply,size_t cap,int *overflow){
    FILE *pipe=popen(command,"r");char chunk[512];size_t length=0;int status=-1;*overflow=0;reply[0]=0;
    if(pipe){size_t got;while((got=fread(chunk,1,sizeof chunk,pipe))>0){
        if(length+got>=cap)*overflow=1;
        if(!*overflow){memcpy(reply+length,chunk,got);length+=got;reply[length]=0;}
    }status=pclose(pipe);}
    return status;
}

static int weather_fetch_worker(void *data){
    WeatherRequest *request=data;
    char reply[WEATHER_REPLY_MAX]="";int overflow=0,status=-1;
    char script[900];snprintf(script,sizeof script,"%s/weather_service.py",sn_data_root());
    if(access(script,R_OK)==0){
        char qs[240],ss[1200],command[1800];
        weather_shell_quote(qs,sizeof qs,request->query);
        weather_shell_quote(ss,sizeof ss,script);
        snprintf(command,sizeof command,"python3 %s --query %s --unit %c --auto-ip %d 2>/dev/null",
                 ss,qs,request->unit?'c':'f',request->auto_ip?1:0);
        status=weather_read_command(command,reply,sizeof reply,&overflow);
    }
    char reading[80];int rise,set;
    if(status!=0||overflow||!weather_parse_reply_ex(reply,reading,&rise,&set,0)){
        char encoded[512];size_t used=0;
        for(const unsigned char *p=(const unsigned char*)request->query;*p&&used+4<sizeof encoded;p++){
            if(isalnum(*p)||*p==','||*p=='-'||*p=='.')encoded[used++]=(char)*p;
            else if(*p==' ')encoded[used++]='+';
            else{snprintf(encoded+used,sizeof encoded-used,"%%%02X",*p);used+=3;}
        }
        encoded[used]=0;
        char command[800];
        snprintf(command,sizeof command,"curl -fsS --connect-timeout 3 --max-time 8 'https://wttr.in/%s?format=%%t+%%C%%7C%%S%%7C%%s&%s' 2>/dev/null",encoded,request->unit?"m":"u");
        status=weather_read_command(command,reply,sizeof reply,&overflow);
    }
    request->success=0;
    if(status==0&&!overflow&&weather_parse_reply_ex(reply,reading,&rise,&set,0)){
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
    weather_read_checked=0;   /* a reading just landed: read it on this frame */
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
        if(force){
            snprintf(weather_force_query,sizeof weather_force_query,"%s",weather_loc);
            weather_force_unit=weather_unit;weather_force_pending=1;
        }
        return;
    }
    if(weather_force_pending&&weather_force_unit==weather_unit&&!strcmp(weather_force_query,weather_loc)){
        force=1;weather_force_pending=0;
    }
    /* Power Save exists to stop the device doing work nobody asked for.
       A scheduled forecast refresh is exactly that -- it wakes the radio on
       a timer. Pressing X in the Weather app still refreshes, because that
       is somebody asking. */
    if(!force&&power_save_mode)return;
    Uint32 now=SDL_GetTicks();
    if(!force&&g_weather_kicked&&now-g_weather_kicked<weather_refresh_period())return;
    /* Background maintenance never runs without a network. A user-driven
       refresh still tries, so pressing X reports the failure instead of
       silently doing nothing. */
    if(!force&&!weather_network_up())return;
    g_weather_kicked=now?now:1;
    snprintf(weather_request.query,sizeof weather_request.query,"%s",weather_loc);
    weather_request.unit=weather_unit;weather_request.auto_ip=weather_auto_ip;weather_request.success=0;
    widget_weather_path(weather_request.path,sizeof weather_request.path);
    SDL_AtomicSet(&weather_done,0);
    weather_thread=SDL_CreateThread(weather_fetch_worker,"weather",&weather_request);
    if(!weather_thread)g_weather_failed=1;
}

static void weather_read(void){
    weather_poll();
    Uint32 read_now=SDL_GetTicks();
    // g_weather_mtime == 0 means the current cache file has not been parsed yet
    // -- a location or unit change -- so that read is never throttled.
    if(g_weather_mtime&&weather_read_checked&&read_now-weather_read_checked<WEATHER_READ_POLL_MS)return;
    weather_read_checked=read_now?read_now:1;
    char path[768];widget_weather_path(path,sizeof path);
    struct stat info;if(stat(path,&info)!=0||info.st_mtime==g_weather_mtime)return;
    FILE *file=fopen(path,"r");if(!file)return;
    char reply[WEATHER_REPLY_MAX]="";size_t length=fread(reply,1,sizeof reply-1,file);reply[length]=0;
    int complete=feof(file);fclose(file);g_weather_mtime=info.st_mtime;
    char reading[80];int rise,set;
    if(complete&&weather_parse_reply(reply,reading,&rise,&set)){
        snprintf(g_weather_str,sizeof g_weather_str,"%s",reading);g_weather_at=info.st_mtime;
        /* The provider resolved this lookup to a properly punctuated name; if
           the saved place is a rawer version of it, adopt the good spelling. */
        widget_place_adopt_canonical(g_weather_place);
        if(rise>=0&&set>=0&&widget_place_sel[1]==0){g_sunrise_minute=rise;g_sunset_minute=set;g_sun_times_at=info.st_mtime;}
    }
}
static const char *weather_empty_message(void){
    if(!weather_auto_ip&&!weather_loc[0])return "Choose a location";
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
