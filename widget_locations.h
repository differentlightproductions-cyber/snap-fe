#ifndef SNAP_WIDGET_LOCATIONS_H
#define SNAP_WIDGET_LOCATIONS_H
#include "location_catalog.h"
typedef struct {char name[64],zone[64],query[160],weather[80];time_t at,mtime;Uint32 kicked;int unit,failed;} WidgetPlace;
static WidgetPlace widget_places[2][3];
static int widget_place_count[2]={1,1},widget_place_sel[2]={0,0},widget_places_loaded;
static int widget_place_group,widget_place_searching,widget_place_result_count,widget_place_result_sel;
static int widget_place_results[24];
static char widget_place_query[96],widget_place_status[120],widget_local_city[64];
static Uint32 widget_local_attempt;
static SDL_Thread *widget_local_thread;
static SDL_atomic_t widget_local_done;
static char widget_local_result[64];
static void widget_catalog_seed(void);

static void widget_weather_path_for(char *out,size_t size,const char *query,int unit){
    unsigned long long hash=14695981039346656037ULL;
    for(const unsigned char *p=(const unsigned char*)query;*p;p++){hash^=*p;hash*=1099511628211ULL;}
#ifdef SNAPFE_TESTING
    snprintf(out,size,"%s/weather.%016llx.%d",sn_data_root(),hash,unit);
#else
    snprintf(out,size,"%s.%016llx.%d",WEATHER_TMP,hash,unit);
#endif
}
static void widget_weather_path(char *out,size_t size){widget_weather_path_for(out,size,weather_loc,weather_unit);}
static void widget_weather_query(char *out,size_t size,const WLocation *location){
    snprintf(out,size,"%s%s%s",location->name,location->region[0]?", ":"",location->region);
}
static int widget_city_valid(const char *s){
    if(!s[0]||strlen(s)>60||strchr(s,'<')||strchr(s,'{'))return 0;
    for(const unsigned char *p=(const unsigned char*)s;*p;p++)if(*p<32||*p==127)return 0;
    return strcmp(s,"Undefined")&&strcmp(s,"null");
}
static void widget_local_read_reply(FILE *stream,int has_status,char out[64]){
    char city[160]="",status[32]="";out[0]=0;
    if(has_status){if(!fgets(status,sizeof status,stream))return;status[strcspn(status,"\r\n")]=0;if(strcmp(status,"success"))return;}
    if(fgets(city,sizeof city,stream)){city[strcspn(city,"\r\n")]=0;if(widget_city_valid(city))snprintf(out,64,"%.63s",city);}
}
static int widget_local_fetch(void *unused){
    (void)unused;widget_local_result[0]=0;
    /* Fixed HTTPS endpoint: user-entered text never becomes a shell command. */
    FILE *p=popen("curl -fsS --connect-timeout 2 --max-time 4 https://ipapi.co/city/ 2>/dev/null","r");
    if(p){widget_local_read_reply(p,0,widget_local_result);if(pclose(p)!=0)widget_local_result[0]=0;}
    /* The radio's existing location provider is a fallback if the first is busy. */
    if(!widget_local_result[0]){p=popen("curl -fsS --connect-timeout 2 --max-time 4 'http://ip-api.com/line/?fields=status,city' 2>/dev/null","r");if(p){widget_local_read_reply(p,1,widget_local_result);if(pclose(p)!=0)widget_local_result[0]=0;}}
    SDL_AtomicSet(&widget_local_done,1);return 0;
}
static void widget_local_tick(int allow_fetch){
    if(widget_local_thread&&SDL_AtomicGet(&widget_local_done)){
        SDL_WaitThread(widget_local_thread,NULL);widget_local_thread=NULL;
        if(widget_local_result[0]){snprintf(widget_local_city,64,"%s",widget_local_result);
            char path[768];snprintf(path,sizeof path,"%s/widget-local-city.txt",sn_data_root());FILE *f=fopen(path,"w");if(f){fprintf(f,"%s\n",widget_local_city);fclose(f);}}
    }
#ifdef SNAPOS_TARGET_KNULLI
    Uint32 now=SDL_GetTicks();
    if(allow_fetch&&!widget_local_thread&&(!widget_local_attempt||now-widget_local_attempt>(widget_local_city[0]?3600000u:60000u))){
        widget_local_attempt=now?now:1;SDL_AtomicSet(&widget_local_done,0);widget_local_thread=SDL_CreateThread(widget_local_fetch,"local-city",NULL);}
#else
    (void)allow_fetch;
#endif
}
static void widget_places_stop(void){weather_stop();if(widget_local_thread){SDL_WaitThread(widget_local_thread,NULL);widget_local_thread=NULL;}}
static void widget_places_save(void){
    char path[768],tmp[780];snprintf(path,sizeof path,"%s/widget-places.cfg",sn_data_root());snprintf(tmp,sizeof tmp,"%s.tmp",path);
    FILE *f=fopen(tmp,"w");if(!f){snprintf(widget_place_status,sizeof widget_place_status,"Could not save locations");return;}
    fprintf(f,"S\t%d\t%d\n",widget_place_sel[0],widget_place_sel[1]);
    for(int g=0;g<2;g++)for(int i=0;i<widget_place_count[g];i++){WidgetPlace *p=&widget_places[g][i];fprintf(f,"%d\t%s\t%s\t%s\n",g,p->name,p->zone,p->query);}
    int ok=!ferror(f);if(fclose(f)!=0)ok=0;if(ok&&rename(tmp,path)==0)return;
    snprintf(widget_place_status,sizeof widget_place_status,"Could not save locations");
}
static void widget_weather_store(void){WidgetPlace *p=&widget_places[1][widget_place_sel[1]];snprintf(p->weather,80,"%s",g_weather_str);p->at=g_weather_at;p->mtime=g_weather_mtime;p->kicked=g_weather_kicked;p->unit=weather_unit;p->failed=g_weather_failed;}
static void widget_weather_restore(void){
    WidgetPlace *p=&widget_places[1][widget_place_sel[1]];snprintf(weather_loc,sizeof weather_loc,"%s",!strcmp(p->name,"Local")?"":p->query[0]?p->query:p->name);
    if(p->unit!=weather_unit){p->weather[0]=0;p->at=p->mtime=0;p->kicked=0;p->failed=0;p->unit=weather_unit;}
    snprintf(g_weather_str,80,"%s",p->weather);g_weather_at=p->at;g_weather_mtime=p->mtime;g_weather_kicked=p->kicked;g_weather_failed=p->failed;
}
static void widget_places_init(void){
    if(widget_places_loaded)return;
    widget_places_loaded=1;widget_catalog_seed();memset(widget_places,0,sizeof widget_places);
    widget_place_count[0]=widget_place_count[1]=1;widget_place_sel[0]=widget_place_sel[1]=0;
    strcpy(widget_places[0][0].name,"Local");snprintf(widget_places[1][0].name,64,"%.63s",weather_loc[0]?weather_loc:"Local");snprintf(widget_places[1][0].query,sizeof widget_places[1][0].query,"%s",weather_loc);
    char path[768];snprintf(path,sizeof path,"%s/widget-local-city.txt",sn_data_root());FILE *f=fopen(path,"r");
    if(f){char city[96]="";fgets(city,sizeof city,f);fclose(f);city[strcspn(city,"\r\n")]=0;if(widget_city_valid(city))snprintf(widget_local_city,64,"%.63s",city);}
    if(!widget_local_city[0]){f=fopen("/tmp/snapos-radio-loc.txt","r");if(f){char city[96]="";for(int i=0;i<3;i++)if(!fgets(city,sizeof city,f)){city[0]=0;break;}fclose(f);city[strcspn(city,"\r\n")]=0;if(widget_city_valid(city))snprintf(widget_local_city,64,"%.63s",city);}}
    snprintf(path,sizeof path,"%s/widget-places.cfg",sn_data_root());f=fopen(path,"r");
    int migrated=0;
    if(f){int counts[2]={0},selected[2]={0};char line[512];while(fgets(line,sizeof line,f)){
        if(line[0]=='S'&&line[1]=='\t'){sscanf(line,"S\t%d\t%d",&selected[0],&selected[1]);continue;}
        line[strcspn(line,"\r\n")]=0;char *name=strchr(line,'\t');if(!name)continue;*name++=0;char *zone=strchr(name,'\t');if(!zone)continue;*zone++=0;char *query=strchr(zone,'\t');if(query)*query++=0;
        int g=atoi(line);if((strcmp(line,"0")&&strcmp(line,"1"))||counts[g]>=3||!widget_city_valid(name)||(zone[0]&&!wloc_valid_zone(zone)))continue;
        WidgetPlace *p=&widget_places[g][counts[g]++];snprintf(p->name,64,"%.63s",name);snprintf(p->zone,64,"%.63s",zone);snprintf(p->query,sizeof p->query,"%s",query&&query[0]?query:strcmp(name,"Local")?name:"");p->unit=weather_unit;
        if(g&&strcmp(p->name,"Local"))for(int c=0;c<wloc_count;c++){
            if(strcasecmp(p->name,wloc_items[c].name)||strcmp(p->zone,wloc_items[c].zone))continue;
            char canonical[160];widget_weather_query(canonical,sizeof canonical,&wloc_items[c]);
            if(strcmp(p->query,canonical)){snprintf(p->query,sizeof p->query,"%s",canonical);migrated=1;}break;
        }
    }fclose(f);for(int g=0;g<2;g++){if(counts[g])widget_place_count[g]=counts[g];if(selected[g]>=0&&selected[g]<widget_place_count[g])widget_place_sel[g]=selected[g];}if(counts[1])widget_weather_restore();}
    if(migrated)widget_places_save();
}
static int widget_place_kind(int k){return k==HOME_WIDGET_CLOCK||k==HOME_WIDGET_DATE||k==HOME_WIDGET_WEATHER||k==HOME_WIDGET_DATEWX;}
static int widget_place_family(int k){return k==HOME_WIDGET_WEATHER||k==HOME_WIDGET_DATEWX;}
static void widget_place_select(int group,int index){widget_places_init();if(index<0||index>=widget_place_count[group])return;if(group)widget_weather_store();widget_place_sel[group]=index;if(group)widget_weather_restore();}
static void widget_place_cycle(int group,int dir){widget_places_init();widget_place_select(group,(widget_place_sel[group]+dir+widget_place_count[group])%widget_place_count[group]);}
static const char *widget_place_label(int group,int index){
    static char labels[2][3][112];widget_places_init();WidgetPlace *p=&widget_places[group][index];if(strcmp(p->name,"Local"))return p->name;
    if(widget_local_city[0])snprintf(labels[group][index],112,"Local (%s)",widget_local_city);
    else{const char *zone=getenv("TZ"),*city=zone?strrchr(zone,'/'):NULL;
        if(city&&city[1]){char pretty[64];snprintf(pretty,64,"%.63s",city+1);for(char *s=pretty;*s;s++)if(*s=='_')*s=' ';snprintf(labels[group][index],112,"Local (%s time)",pretty);}
        else snprintf(labels[group][index],112,"Local (finding city...)");}
    return labels[group][index];
}
static const char *widget_place_name(int group){widget_places_init();return widget_place_label(group,widget_place_sel[group]);}
static struct tm widget_place_time(int group,time_t t){
    widget_places_init();const char *zone=widget_places[group][widget_place_sel[group]].zone;
    struct tm out;localtime_r(&t,&out);if(!zone[0]||!wloc_valid_zone(zone))return out;
    char path[160];snprintf(path,sizeof path,"/usr/share/zoneinfo/%s",zone);if(access(path,R_OK))return out;
    const char *old=getenv("TZ");char *saved=old?strdup(old):NULL;setenv("TZ",zone,1);tzset();localtime_r(&t,&out);
    if(saved){setenv("TZ",saved,1);free(saved);}else unsetenv("TZ");tzset();return out;
}
static void widget_places_open(int group){widget_places_init();widget_place_group=group;widget_place_searching=0;widget_place_result_sel=0;widget_place_status[0]=0;widget_local_tick(1);}
static void widget_place_search(const char *query){
    widget_places_init();snprintf(widget_place_query,sizeof widget_place_query,"%.95s",query);widget_place_result_sel=0;
    widget_place_result_count=wloc_search(query,widget_place_results,24);widget_place_searching=1;
    snprintf(widget_place_status,sizeof widget_place_status,"%s",widget_place_result_count?"Choose a matching city, then press A":"No matching locations. X searches again.");
}
static int widget_place_add_location(const WLocation *location){
    widget_places_init();int g=widget_place_group;
    if(!location||!wloc_valid_zone(location->zone)){snprintf(widget_place_status,sizeof widget_place_status,"This location is unavailable on this device");return 0;}
    for(int i=0;i<widget_place_count[g];i++)if(!strcasecmp(widget_places[g][i].name,location->name)&&!strcmp(widget_places[g][i].zone,location->zone)){widget_place_select(g,i);snprintf(widget_place_status,sizeof widget_place_status,"Already saved - selected for your widget");widget_place_searching=0;return 1;}
    if(widget_place_count[g]>=3){snprintf(widget_place_status,sizeof widget_place_status,"Three locations saved. Remove one to add another.");return 0;}
    if(g)widget_weather_store();WidgetPlace *p=&widget_places[g][widget_place_count[g]++];memset(p,0,sizeof *p);
    snprintf(p->name,64,"%s",location->name);snprintf(p->zone,64,"%s",location->zone);p->unit=weather_unit;
    widget_weather_query(p->query,sizeof p->query,location);
    widget_place_sel[g]=widget_place_count[g]-1;if(g)widget_weather_restore();snprintf(widget_place_status,sizeof widget_place_status,"Added %s - now shown in your widget",p->name);widget_place_searching=0;widget_places_save();return 1;
}
/* Exact-name helper retained for imports. Interactive entry always shows choices. */
static int widget_place_add(const char *text){widget_places_init();int ids[24],n=wloc_search(text,ids,24);for(int i=0;i<n;i++)if(!strcasecmp(wloc_items[ids[i]].name,text))return widget_place_add_location(&wloc_items[ids[i]]);return 0;}
static void widget_place_remove(void){
    int g=widget_place_group,i=widget_place_sel[g];if(widget_place_count[g]<=1){snprintf(widget_place_status,sizeof widget_place_status,"Keep at least one location");return;}
    if(g)widget_weather_store();for(int j=i;j<widget_place_count[g]-1;j++)widget_places[g][j]=widget_places[g][j+1];widget_place_count[g]--;widget_place_sel[g]=0;if(g)widget_weather_restore();snprintf(widget_place_status,sizeof widget_place_status,"Location removed");widget_places_save();
}
#endif
