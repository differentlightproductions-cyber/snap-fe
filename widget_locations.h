#ifndef SNAP_WIDGET_LOCATIONS_H
#define SNAP_WIDGET_LOCATIONS_H
#include "location_catalog.h"
// ONE saved-location list, shared by both widget families.
//
// There used to be two independent lists -- group 0 for the Clock/Date cards
// and group 1 for Weather -- which meant a city saved in Weather Settings was
// invisible to the clock, and the clock's list stayed at just "Local". The card
// still offered "Up/Down Zones", so it looked broken when it was in fact
// cycling a list of one. Saving a place now adds it once and both widgets can
// reach it.
//
// The two rows of the array survive because group 1 additionally carries each
// place's weather cache (reading, timestamps, unit). Identity -- name, zone,
// query -- is kept identical between the rows, index for index, so
// widget_places[g][i] means the same place whichever g you ask for. What
// differs per group is only the cursor, widget_place_sel[g]: the clock and the
// weather card can sit on different places in the same list.
//
// Raised from three saved places to five in 1.3.1. The on-disk format is a
// plain list of rows tagged by group, so an older widget-places.cfg still loads
// -- both of its lists are merged into the shared one.
#define WIDGET_PLACE_MAX 5
typedef struct {char name[64],zone[64],query[160],weather[80];time_t at,mtime;Uint32 kicked;int unit,failed;} WidgetPlace;
static WidgetPlace widget_places[2][WIDGET_PLACE_MAX];
static int widget_place_count[2]={1,1},widget_place_sel[2]={0,0},widget_places_loaded;
static int widget_place_group,widget_place_searching,widget_place_result_count,widget_place_result_sel;
static int widget_place_results[24];
static char widget_place_query[96],widget_place_status[120],widget_local_city[64];
static AppState widget_places_return_state=STATE_HOME;
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

/* --- the shared list ---------------------------------------------------- */

/* Same place, wherever it came from. Zone is what actually distinguishes two
   cities of the same name, so it is part of the identity. */
static int widget_place_same(const WidgetPlace *a,const char *name,const char *zone){
    return !strcasecmp(a->name,name)&&!strcmp(a->zone,zone);
}
static int widget_place_find(const char *name,const char *zone){
    for(int i=0;i<widget_place_count[0];i++)
        if(widget_place_same(&widget_places[0][i],name,zone))return i;
    return -1;
}

/* Append to the shared list. Returns the new index, or -1 when it is full.
   Both rows are written together so index i names the same place in each; row 1
   additionally gets a cleared weather cache, since nothing has been fetched for
   this place yet. */
static int widget_place_append(const char *name,const char *zone,const char *query){
    int n=widget_place_count[0];
    if(n>=WIDGET_PLACE_MAX)return -1;
    for(int g=0;g<2;g++){
        WidgetPlace *p=&widget_places[g][n];
        memset(p,0,sizeof *p);
        snprintf(p->name,64,"%.63s",name);
        snprintf(p->zone,64,"%.63s",zone?zone:"");
        snprintf(p->query,sizeof p->query,"%s",query?query:"");
        p->unit=weather_unit;
    }
    widget_place_count[0]=widget_place_count[1]=n+1;
    return n;
}

/* Drop one entry from both rows at once, keeping the weather cache lined up
   with the place it belongs to, and pull both cursors back into range. */
static void widget_place_erase(int index){
    if(index<0||index>=widget_place_count[0])return;
    for(int g=0;g<2;g++){
        for(int j=index;j<widget_place_count[g]-1;j++)widget_places[g][j]=widget_places[g][j+1];
        memset(&widget_places[g][widget_place_count[g]-1],0,sizeof widget_places[g][0]);
    }
    widget_place_count[0]=widget_place_count[1]=widget_place_count[0]-1;
    if(widget_place_count[0]<1)widget_place_count[0]=widget_place_count[1]=1;
    for(int g=0;g<2;g++){
        if(widget_place_sel[g]>index)widget_place_sel[g]--;
        if(widget_place_sel[g]>=widget_place_count[g])widget_place_sel[g]=widget_place_count[g]-1;
        if(widget_place_sel[g]<0)widget_place_sel[g]=0;
    }
}
static void widget_places_save(void){
    char path[768],tmp[780];snprintf(path,sizeof path,"%s/widget-places.cfg",sn_data_root());snprintf(tmp,sizeof tmp,"%s.tmp",path);
    FILE *f=fopen(tmp,"w");if(!f){snprintf(widget_place_status,sizeof widget_place_status,"Could not save locations");return;}
    fprintf(f,"S\t%d\t%d\n",widget_place_sel[0],widget_place_sel[1]);
    // One list, written once. Rows keep the group tag so an older build still
    // reads the file -- it just sees every place as a clock place.
    for(int i=0;i<widget_place_count[0];i++){WidgetPlace *p=&widget_places[0][i];fprintf(f,"0\t%s\t%s\t%s\n",p->name,p->zone,p->query);}
    int ok=!ferror(f);if(fclose(f)!=0)ok=0;if(ok&&rename(tmp,path)==0)return;
    snprintf(widget_place_status,sizeof widget_place_status,"Could not save locations");
}
/* Adopt the canonical place name the provider resolved a lookup to.
   A place typed by hand before there was a picker -- "Prescott Arizona", no
   comma -- keeps that raw text as its name and its query forever, and it is
   what the app then shows. The geocoder answers with the proper
   "Prescott, Arizona", so once a reading comes back for this place, take that
   spelling. Only when it plainly describes the same place: same first word, and
   the stored form is the poorer one. */
static int widget_place_adopt_canonical(const char *canonical){
    if(!widget_places_loaded||!canonical||!canonical[0])return 0;
    if(!strchr(canonical,','))return 0;                 /* nothing better to offer */
    WidgetPlace *p=&widget_places[1][widget_place_sel[1]];
    if(!strcmp(p->name,"Local"))return 0;               /* automatic: never renamed */
    if(strchr(p->name,',')&&strchr(p->query,','))return 0;   /* already canonical */
    /* Compare up to the first separator: "Prescott" vs "Prescott". */
    size_t n=strcspn(canonical,",");
    if(!n||strncasecmp(p->name,canonical,n))return 0;
    if(p->name[n]&&p->name[n]!=','&&p->name[n]!=' ')return 0;
    for(int g=0;g<2;g++){
        snprintf(widget_places[g][widget_place_sel[1]].name,64,"%.63s",canonical);
        snprintf(widget_places[g][widget_place_sel[1]].query,sizeof widget_places[g][0].query,"%s",canonical);
    }
    snprintf(weather_loc,sizeof weather_loc,"%s",canonical);
    widget_places_save();
    return 1;
}

static void widget_weather_store(void){WidgetPlace *p=&widget_places[1][widget_place_sel[1]];snprintf(p->weather,80,"%s",g_weather_str);p->at=g_weather_at;p->mtime=g_weather_mtime;p->kicked=g_weather_kicked;p->unit=weather_unit;p->failed=g_weather_failed;}
static void widget_weather_restore(void){
    WidgetPlace *p=&widget_places[1][widget_place_sel[1]];snprintf(weather_loc,sizeof weather_loc,"%s",!strcmp(p->name,"Local")?"":p->query[0]?p->query:p->name);
    if(p->unit!=weather_unit){p->weather[0]=0;p->at=p->mtime=0;p->kicked=0;p->failed=0;p->unit=weather_unit;}
    snprintf(g_weather_str,80,"%s",p->weather);g_weather_at=p->at;g_weather_kicked=p->kicked;g_weather_failed=p->failed;
    weather_detail_clear();
    /* Re-read this location's full cache. The old handoff restored only the
       headline and left feels/high/low/rain/hourly blank or from another city. */
    g_weather_mtime=0;
}
static void widget_places_init(void){
    if(widget_places_loaded)return;
    widget_places_loaded=1;widget_catalog_seed();memset(widget_places,0,sizeof widget_places);
    widget_place_count[0]=widget_place_count[1]=0;widget_place_sel[0]=widget_place_sel[1]=0;
    char path[768];snprintf(path,sizeof path,"%s/widget-local-city.txt",sn_data_root());FILE *f=fopen(path,"r");
    if(f){char city[96]="";fgets(city,sizeof city,f);fclose(f);city[strcspn(city,"\r\n")]=0;if(widget_city_valid(city))snprintf(widget_local_city,64,"%.63s",city);}
    if(!widget_local_city[0]){f=fopen("/tmp/snapos-radio-loc.txt","r");if(f){char city[96]="";for(int i=0;i<3;i++)if(!fgets(city,sizeof city,f)){city[0]=0;break;}fclose(f);city[strcspn(city,"\r\n")]=0;if(widget_city_valid(city))snprintf(widget_local_city,64,"%.63s",city);}}
    snprintf(path,sizeof path,"%s/widget-places.cfg",sn_data_root());f=fopen(path,"r");
    int migrated=0;
    if(f){
        /* Rows are still tagged by group on disk. A file written before the
           lists were merged has two of them; read both and fold them into the
           one shared list, first-seen wins, so nobody loses a saved city on
           upgrade. `keep[g]` remembers which name each group had selected so
           both cursors can be restored against the merged order. */
        int selected[2]={0},seen[2]={0};
        char keep[2][64]={{0},{0}};
        int per_group[2]={0};
        char line[512];
        while(fgets(line,sizeof line,f)){
            if(line[0]=='S'&&line[1]=='\t'){sscanf(line,"S\t%d\t%d",&selected[0],&selected[1]);seen[0]=seen[1]=1;continue;}
            line[strcspn(line,"\r\n")]=0;
            char *name=strchr(line,'\t');if(!name)continue;*name++=0;
            char *zone=strchr(name,'\t');if(!zone)continue;*zone++=0;
            char *query=strchr(zone,'\t');if(query)*query++=0;
            int g=atoi(line);
            if((strcmp(line,"0")&&strcmp(line,"1"))||!widget_city_valid(name)||(zone[0]&&!wloc_valid_zone(zone)))continue;
            /* Remember this group's selected name before any merging moves it. */
            if(seen[g]&&per_group[g]==selected[g])snprintf(keep[g],64,"%.63s",name);
            per_group[g]++;
            if(widget_place_find(name,zone)>=0)continue;          /* already merged in */
            const char *q=query&&query[0]?query:(strcmp(name,"Local")?name:"");
            int at=widget_place_append(name,zone,q);
            if(at<0)continue;                                     /* list is full */
            /* Older saves carry a free-form weather query; replace it with the
               catalog's canonical "City, Region" so geocoding keeps working. */
            if(strcmp(name,"Local"))for(int c=0;c<wloc_count;c++){
                if(strcasecmp(name,wloc_items[c].name)||strcmp(zone,wloc_items[c].zone))continue;
                char canonical[160];widget_weather_query(canonical,sizeof canonical,&wloc_items[c]);
                if(strcmp(widget_places[0][at].query,canonical)){
                    for(int g2=0;g2<2;g2++)snprintf(widget_places[g2][at].query,sizeof widget_places[g2][at].query,"%s",canonical);
                    migrated=1;
                }
                break;
            }
        }
        fclose(f);
        if(widget_place_count[0]>0){
            for(int g=0;g<2;g++){
                int at=keep[g][0]?widget_place_find(keep[g],""):-1;
                if(at<0&&keep[g][0])
                    for(int i=0;i<widget_place_count[0];i++)
                        if(!strcasecmp(widget_places[0][i].name,keep[g])){at=i;break;}
                if(at<0)at=(selected[g]>=0&&selected[g]<widget_place_count[g])?selected[g]:0;
                widget_place_sel[g]=at;
            }
            widget_weather_restore();
            /* The merge changed the file's shape; write it back in the new one
               so the next boot does not have to redo it. */
            migrated=1;
        }
    }
    /* Nothing on disk yet: seed one entry so the list is never empty. A
       weather_loc carried over from before there were saved places becomes that
       entry rather than being silently dropped. Seeded AFTER the file is read,
       so it can never displace the saved order. */
    if(widget_place_count[0]<1){
        if(weather_loc[0])widget_place_append(weather_loc,"",weather_loc);
        else              widget_place_append("Local","","");
        widget_place_sel[0]=widget_place_sel[1]=0;
    }
    if(migrated)widget_places_save();
}
static int widget_place_kind(int k){return k==HOME_WIDGET_CLOCK||k==HOME_WIDGET_DATE||k==HOME_WIDGET_WEATHER||k==HOME_WIDGET_DATEWX;}
static int widget_place_family(int k){return k==HOME_WIDGET_WEATHER||k==HOME_WIDGET_DATEWX;}
static void widget_place_select(int group,int index){widget_places_init();if(index<0||index>=widget_place_count[group])return;if(group)widget_weather_store();widget_place_sel[group]=index;if(group)widget_weather_restore();}
static void widget_place_cycle(int group,int dir){widget_places_init();widget_place_select(group,(widget_place_sel[group]+dir+widget_place_count[group])%widget_place_count[group]);}
static const char *widget_place_label(int group,int index){
    static char labels[2][WIDGET_PLACE_MAX][112];widget_places_init();WidgetPlace *p=&widget_places[group][index];if(strcmp(p->name,"Local"))return p->name;
    if(widget_local_city[0])snprintf(labels[group][index],112,"Local (%s)",widget_local_city);
    else{const char *zone=getenv("TZ"),*city=zone?strrchr(zone,'/'):NULL;
        if(city&&city[1]){char pretty[64];snprintf(pretty,64,"%.63s",city+1);for(char *s=pretty;*s;s++)if(*s=='_')*s=' ';snprintf(labels[group][index],112,"Local (%s time)",pretty);}
        else snprintf(labels[group][index],112,"Local (finding city...)");}
    return labels[group][index];
}
static const char *widget_place_name(int group){widget_places_init();return widget_place_label(group,widget_place_sel[group]);}
/* The full display form of a saved place. widget_place_label() returns the bare
   city because home-screen widget tiles are too narrow for anything longer, but
   Weather Settings and the Weather app have the room and were showing a bare
   "Prescott" for somewhere actually saved -- and queried -- as "Prescott,
   Arizona". Prefer the geocoder's comma form wherever it fits. */
static const char *widget_place_label_full(int group,int index){
    static char full[2][WIDGET_PLACE_MAX][176];
    widget_places_init();
    if(group<0||group>1||index<0||index>=WIDGET_PLACE_MAX)return widget_place_label(group,index);
    WidgetPlace *p=&widget_places[group][index];
    if(!strcmp(p->name,"Local"))return widget_place_label(group,index);
    if(p->query[0]&&strchr(p->query,',')){
        snprintf(full[group][index],sizeof full[group][index],"%s",p->query);
        return full[group][index];
    }
    return p->name;
}
static const char *widget_place_name_full(int group){widget_places_init();return widget_place_label_full(group,widget_place_sel[group]);}
/* Local time at one saved place, by index rather than by whatever the group is
   currently sitting on -- the Locations screen shows the clock for every row. */
static struct tm widget_place_time_at(int group,int index,time_t t){
    widget_places_init();
    if(index<0||index>=widget_place_count[group])index=0;
    const char *zone=widget_places[group][index].zone;
    struct tm out;localtime_r(&t,&out);if(!zone[0]||!wloc_valid_zone(zone))return out;
    char path[160];snprintf(path,sizeof path,"/usr/share/zoneinfo/%s",zone);if(access(path,R_OK))return out;
    const char *old=getenv("TZ");char *saved=old?strdup(old):NULL;setenv("TZ",zone,1);tzset();localtime_r(&t,&out);
    if(saved){setenv("TZ",saved,1);free(saved);}else unsetenv("TZ");tzset();return out;
}
static struct tm widget_place_time(int group,time_t t){
    widget_places_init();const char *zone=widget_places[group][widget_place_sel[group]].zone;
    struct tm out;localtime_r(&t,&out);if(!zone[0]||!wloc_valid_zone(zone))return out;
    char path[160];snprintf(path,sizeof path,"/usr/share/zoneinfo/%s",zone);if(access(path,R_OK))return out;
    const char *old=getenv("TZ");char *saved=old?strdup(old):NULL;setenv("TZ",zone,1);tzset();localtime_r(&t,&out);
    if(saved){setenv("TZ",saved,1);free(saved);}else unsetenv("TZ");tzset();return out;
}
static void widget_places_open_for(int group,AppState return_state){widget_places_init();widget_place_group=group;widget_places_return_state=return_state;widget_place_searching=0;widget_place_result_sel=0;widget_place_status[0]=0;widget_local_tick(1);}
static void widget_places_open(int group){widget_places_open_for(group,STATE_HOME);}
static void widget_place_search(const char *query){
    widget_places_init();snprintf(widget_place_query,sizeof widget_place_query,"%.95s",query);widget_place_result_sel=0;
    widget_place_result_count=wloc_search(query,widget_place_results,24);widget_place_searching=1;
    snprintf(widget_place_status,sizeof widget_place_status,"%s",widget_place_result_count?"Choose a matching city, then press A":"No matching locations. X searches again.");
}
static int widget_place_add_location(const WLocation *location){
    widget_places_init();int g=widget_place_group;
    if(!location||!wloc_valid_zone(location->zone)){snprintf(widget_place_status,sizeof widget_place_status,"This location is unavailable on this device");return 0;}
    /* One shared list, so a place saved here is immediately reachable from the
       clock card as well as the weather one. Only the group doing the adding
       moves its cursor onto it; the other keeps showing what it was showing. */
    int at=widget_place_find(location->name,location->zone);
    if(at>=0){
        widget_place_select(g,at);
        snprintf(widget_place_status,sizeof widget_place_status,"Already saved - selected for your widget");
        widget_place_searching=0;return 1;
    }
    if(widget_place_count[0]>=WIDGET_PLACE_MAX){snprintf(widget_place_status,sizeof widget_place_status,"%d locations saved. Remove one to add another.",WIDGET_PLACE_MAX);return 0;}
    char query[160];widget_weather_query(query,sizeof query,location);
    widget_weather_store();                          /* park the current reading */
    at=widget_place_append(location->name,location->zone,query);
    if(at<0)return 0;
    widget_place_sel[g]=at;
    widget_weather_restore();
    snprintf(widget_place_status,sizeof widget_place_status,"Added %s - now shown in your widget",location->name);
    widget_place_searching=0;widget_places_save();return 1;
}
/* Exact-name helper retained for imports. Interactive entry always shows choices. */
static int widget_place_add(const char *text){widget_places_init();int ids[24],n=wloc_search(text,ids,24);for(int i=0;i<n;i++)if(!strcasecmp(wloc_items[ids[i]].name,text))return widget_place_add_location(&wloc_items[ids[i]]);return 0;}
static void widget_place_remove(void){
    int g=widget_place_group,i=widget_place_sel[g];
    if(widget_place_count[0]<=1){snprintf(widget_place_status,sizeof widget_place_status,"Keep at least one location");return;}
    /* Removing takes it out of the shared list, so it leaves both cards at
       once, and widget_place_erase drags both cursors back into range. */
    widget_weather_store();
    widget_place_erase(i);
    widget_weather_restore();
    snprintf(widget_place_status,sizeof widget_place_status,"Location removed");widget_places_save();
}
#endif
