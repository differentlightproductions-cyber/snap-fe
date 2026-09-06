#ifndef SNAP_LOCATION_CATALOG_H
#define SNAP_LOCATION_CATALOG_H

/* Offline choices come from the device's installed timezone database. City
   aliases cover familiar places that share another city's IANA timezone. */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#define WLOC_CAPACITY 1024
#define WLOC_NORMAL_SIZE 128
#ifndef WLOC_ZONEINFO_ROOT
#define WLOC_ZONEINFO_ROOT "/usr/share/zoneinfo"
#endif
typedef struct { char name[64], zone[64], region[64]; } WLocation;
static WLocation wloc_items[WLOC_CAPACITY];
static int wloc_count, wloc_loaded;

static int wloc_valid_zone(const char *zone) {
    if (!zone || !zone[0] || strlen(zone) >= sizeof(wloc_items[0].zone) ||
        zone[0] == '/' || zone[strlen(zone)-1] == '/' || strstr(zone,"..") || strstr(zone,"//")) return 0;
    for (const unsigned char *p=(const unsigned char *)zone; *p; ++p)
        if (!((*p>='a'&&*p<='z') || (*p>='A'&&*p<='Z') || (*p>='0'&&*p<='9') ||
              *p=='/' || *p=='_' || *p=='-' || *p=='+')) return 0;
    return 1;
}

static void wloc_clean(char *dst, size_t size, const char *src) {
    size_t n=0;
    if (!src) src="";
    while (*src && isspace((unsigned char)*src)) ++src;
    while (*src && n+1<size) {
        unsigned char c=(unsigned char)*src++;
        if (c>=32 && c!=127) dst[n++]=(char)c;
    }
    while (n && isspace((unsigned char)dst[n-1])) --n;
    dst[n]=0;
}

/* Names are compared without punctuation, accents, or separators, so typing
   "newyork", "New_York", and "New York" finds the same choice. */
static void wloc_normalize(const char *src, char dst[WLOC_NORMAL_SIZE]) {
    size_t n=0;
    const unsigned char *p=(const unsigned char *)(src?src:"");
    while (*p && n+3<WLOC_NORMAL_SIZE) {
        unsigned cp=*p++;
        if (cp>=0xc2 && cp<=0xdf && (p[0]&0xc0)==0x80) { cp=((cp&31)<<6)|(p[0]&63); ++p; }
        else if (cp>=0xe0 && cp<=0xef && p[0] && (p[0]&0xc0)==0x80 && p[1] && (p[1]&0xc0)==0x80) {
            cp=((cp&15)<<12)|((p[0]&63)<<6)|(p[1]&63); p+=2;
        }
        else if (cp>=128) continue;
        if (cp>='A'&&cp<='Z') cp+=32;
        if ((cp>='a'&&cp<='z') || (cp>='0'&&cp<='9')) { dst[n++]=(char)cp; continue; }
        switch (cp) {
            case 0xc0:case 0xc1:case 0xc2:case 0xc3:case 0xc4:case 0xc5:
            case 0xe0:case 0xe1:case 0xe2:case 0xe3:case 0xe4:case 0xe5:
            case 0x100:case 0x101:case 0x102:case 0x103:case 0x104:case 0x105: dst[n++]='a';break;
            case 0xc7:case 0xe7:case 0x106:case 0x107:case 0x10c:case 0x10d: dst[n++]='c';break;
            case 0x10e:case 0x10f:case 0x110:case 0x111: dst[n++]='d';break;
            case 0xc8:case 0xc9:case 0xca:case 0xcb:case 0xe8:case 0xe9:case 0xea:case 0xeb:
            case 0x112:case 0x113:case 0x116:case 0x117:case 0x118:case 0x119:case 0x11a:case 0x11b: dst[n++]='e';break;
            case 0x11e:case 0x11f: dst[n++]='g';break;
            case 0xcc:case 0xcd:case 0xce:case 0xcf:case 0xec:case 0xed:case 0xee:case 0xef:
            case 0x12a:case 0x12b:case 0x130:case 0x131: dst[n++]='i';break;
            case 0x139:case 0x13a:case 0x13d:case 0x13e:case 0x141:case 0x142: dst[n++]='l';break;
            case 0xd1:case 0xf1:case 0x143:case 0x144:case 0x147:case 0x148: dst[n++]='n';break;
            case 0xd2:case 0xd3:case 0xd4:case 0xd5:case 0xd6:case 0xd8:
            case 0xf2:case 0xf3:case 0xf4:case 0xf5:case 0xf6:case 0xf8:
            case 0x14c:case 0x14d:case 0x150:case 0x151: dst[n++]='o';break;
            case 0x154:case 0x155:case 0x158:case 0x159: dst[n++]='r';break;
            case 0x15a:case 0x15b:case 0x15e:case 0x15f:case 0x160:case 0x161: dst[n++]='s';break;
            case 0x164:case 0x165: dst[n++]='t';break;
            case 0xd9:case 0xda:case 0xdb:case 0xdc:case 0xf9:case 0xfa:case 0xfb:case 0xfc:
            case 0x16a:case 0x16b:case 0x16e:case 0x16f:case 0x170:case 0x171: dst[n++]='u';break;
            case 0xdd:case 0xfd:case 0xff:case 0x178: dst[n++]='y';break;
            case 0x179:case 0x17a:case 0x17b:case 0x17c:case 0x17d:case 0x17e: dst[n++]='z';break;
            case 0xc6:case 0xe6: dst[n++]='a';dst[n++]='e';break;
            case 0x152:case 0x153: dst[n++]='o';dst[n++]='e';break;
            case 0xdf: dst[n++]='s';dst[n++]='s';break;
        }
    }
    dst[n]=0;
}

static int wloc_add(const char *name, const char *zone, const char *region) {
    if (!name || !wloc_valid_zone(zone)) return -1;
    WLocation candidate={0};
    wloc_clean(candidate.name,sizeof candidate.name,name);
    wloc_clean(candidate.region,sizeof candidate.region,region);
    snprintf(candidate.zone,sizeof candidate.zone,"%s",zone);
    char normalized[WLOC_NORMAL_SIZE];wloc_normalize(candidate.name,normalized);
    if (!normalized[0]) return -1;
    for (int i=0;i<wloc_count;i++) {
        char other[WLOC_NORMAL_SIZE];wloc_normalize(wloc_items[i].name,other);
        if (!strcmp(other,normalized) && !strcmp(wloc_items[i].zone,zone)) {
            if (!wloc_items[i].region[0] && candidate.region[0]) snprintf(wloc_items[i].region,64,"%s",candidate.region);
            return i;
        }
    }
    if (wloc_count>=WLOC_CAPACITY) return -1;
    wloc_items[wloc_count]=candidate;return wloc_count++;
}

static const WLocation wloc_common[] = {
    {"Tokyo","Asia/Tokyo","Japan"}, {"Phoenix","America/Phoenix","Arizona, United States"},
    {"New York","America/New_York","New York, United States"}, {"Miami","America/New_York","Florida, United States"},
    {"Seattle","America/Los_Angeles","Washington, United States"}, {"San Francisco","America/Los_Angeles","California, United States"},
    {"Los Angeles","America/Los_Angeles","California, United States"}, {"San Diego","America/Los_Angeles","California, United States"},
    {"San Jose","America/Los_Angeles","California, United States"}, {"San Jose","America/Costa_Rica","Costa Rica"},
    {"Portland","America/Los_Angeles","Oregon, United States"}, {"Las Vegas","America/Los_Angeles","Nevada, United States"},
    {"Denver","America/Denver","Colorado, United States"}, {"Salt Lake City","America/Denver","Utah, United States"},
    {"Tucson","America/Phoenix","Arizona, United States"}, {"Mesa","America/Phoenix","Arizona, United States"},
    {"Chicago","America/Chicago","Illinois, United States"}, {"Dallas","America/Chicago","Texas, United States"},
    {"Houston","America/Chicago","Texas, United States"}, {"Austin","America/Chicago","Texas, United States"},
    {"San Antonio","America/Chicago","Texas, United States"}, {"Minneapolis","America/Chicago","Minnesota, United States"},
    {"New Orleans","America/Chicago","Louisiana, United States"}, {"Nashville","America/Chicago","Tennessee, United States"},
    {"Boston","America/New_York","Massachusetts, United States"}, {"Philadelphia","America/New_York","Pennsylvania, United States"},
    {"Washington DC","America/New_York","United States"}, {"Atlanta","America/New_York","Georgia, United States"},
    {"Orlando","America/New_York","Florida, United States"}, {"Tampa","America/New_York","Florida, United States"},
    {"Honolulu","Pacific/Honolulu","Hawaii, United States"}, {"Anchorage","America/Anchorage","Alaska, United States"},
    {"Toronto","America/Toronto","Canada"}, {"Montreal","America/Toronto","Quebec, Canada"},
    {"Vancouver","America/Vancouver","Canada"}, {"Calgary","America/Edmonton","Alberta, Canada"},
    {"Mexico City","America/Mexico_City","Mexico"}, {"Sao Paulo","America/Sao_Paulo","Brazil"},
    {"Rio de Janeiro","America/Sao_Paulo","Brazil"}, {"Buenos Aires","America/Argentina/Buenos_Aires","Argentina"},
    {"Lima","America/Lima","Peru"}, {"Bogota","America/Bogota","Colombia"},
    {"Santiago","America/Santiago","Chile"}, {"London","Europe/London","United Kingdom"},
    {"Manchester","Europe/London","United Kingdom"}, {"Edinburgh","Europe/London","United Kingdom"},
    {"Dublin","Europe/Dublin","Ireland"}, {"Paris","Europe/Paris","France"},
    {"Berlin","Europe/Berlin","Germany"}, {"Munich","Europe/Berlin","Germany"},
    {"Madrid","Europe/Madrid","Spain"}, {"Barcelona","Europe/Madrid","Spain"},
    {"Rome","Europe/Rome","Italy"}, {"Milan","Europe/Rome","Italy"},
    {"Amsterdam","Europe/Amsterdam","Netherlands"}, {"Lisbon","Europe/Lisbon","Portugal"},
    {"Zurich","Europe/Zurich","Switzerland"}, {"Vienna","Europe/Vienna","Austria"},
    {"Prague","Europe/Prague","Czechia"}, {"Warsaw","Europe/Warsaw","Poland"},
    {"Stockholm","Europe/Stockholm","Sweden"}, {"Oslo","Europe/Oslo","Norway"},
    {"Athens","Europe/Athens","Greece"}, {"Istanbul","Europe/Istanbul","Turkey"},
    {"Kyiv","Europe/Kyiv","Ukraine"}, {"Moscow","Europe/Moscow","Russia"},
    {"Cairo","Africa/Cairo","Egypt"}, {"Cape Town","Africa/Johannesburg","South Africa"},
    {"Johannesburg","Africa/Johannesburg","South Africa"}, {"Nairobi","Africa/Nairobi","Kenya"},
    {"Dubai","Asia/Dubai","United Arab Emirates"}, {"Mumbai","Asia/Kolkata","India"},
    {"Delhi","Asia/Kolkata","India"}, {"New Delhi","Asia/Kolkata","India"},
    {"Bengaluru","Asia/Kolkata","India"}, {"Kolkata","Asia/Kolkata","India"},
    {"Bangkok","Asia/Bangkok","Thailand"}, {"Singapore","Asia/Singapore","Singapore"},
    {"Beijing","Asia/Shanghai","China"}, {"Shanghai","Asia/Shanghai","China"},
    {"Hong Kong","Asia/Hong_Kong","Hong Kong"}, {"Taipei","Asia/Taipei","Taiwan"},
    {"Seoul","Asia/Seoul","South Korea"}, {"Osaka","Asia/Tokyo","Japan"},
    {"Kyoto","Asia/Tokyo","Japan"}, {"Manila","Asia/Manila","Philippines"},
    {"Jakarta","Asia/Jakarta","Indonesia"}, {"Sydney","Australia/Sydney","New South Wales, Australia"},
    {"Melbourne","Australia/Melbourne","Victoria, Australia"}, {"Brisbane","Australia/Brisbane","Queensland, Australia"},
    {"Perth","Australia/Perth","Western Australia"}, {"Adelaide","Australia/Adelaide","South Australia"},
    {"Auckland","Pacific/Auckland","New Zealand"}, {"Wellington","Pacific/Auckland","New Zealand"},
    {"UTC","UTC","Coordinated Universal Time"}
};

static void wloc_init(void) {
    if (wloc_loaded) return;
    wloc_loaded=1;
    for (size_t i=0;i<sizeof wloc_common/sizeof wloc_common[0];i++)
        wloc_add(wloc_common[i].name,wloc_common[i].zone,wloc_common[i].region);
    struct {char code[3],name[64];} countries[300];int country_count=0;
    char line[1024];FILE *f=fopen(WLOC_ZONEINFO_ROOT "/iso3166.tab","r");
    if (f) {
        while (country_count<300 && fgets(line,sizeof line,f)) {
            if (line[0]=='#' || strlen(line)<3 || line[2]!='\t') continue;
            countries[country_count].code[0]=line[0];countries[country_count].code[1]=line[1];countries[country_count].code[2]=0;
            wloc_clean(countries[country_count].name,64,line+3);country_count++;
        }
        fclose(f);
    }
    f=fopen(WLOC_ZONEINFO_ROOT "/zone.tab","r");
    if (!f) f=fopen(WLOC_ZONEINFO_ROOT "/zone1970.tab","r");
    if (!f) return;
    while (fgets(line,sizeof line,f)) {
        if (line[0]=='#' || !line[0]) continue;
        char *tab1=strchr(line,'\t');if (!tab1) continue;
        char *tab2=strchr(tab1+1,'\t');if (!tab2) continue;
        *tab1=0;char *zone=tab2+1;zone[strcspn(zone,"\t\r\n")]=0;
        if (!wloc_valid_zone(zone)) continue;
        const char *last=strrchr(zone,'/');last=last?last+1:zone;
        char name[64],region[64];snprintf(name,sizeof name,"%s",last);
        for (char *p=name;*p;p++) if (*p=='_') *p=' ';
        snprintf(region,sizeof region,"%s",line);
        for (int i=0;i<country_count;i++) if (!strncmp(line,countries[i].code,2) && (!line[2] || line[2]==',')) {
            snprintf(region,sizeof region,"%s",countries[i].name);break;
        }
        wloc_add(name,zone,region);
    }
    fclose(f);
}

/* Optimal-string-alignment distance handles a mistyped letter, an omitted
   letter, or adjacent swapped letters. Fixed rows bound memory and input. */
static int wloc_distance(const char *a,const char *b,int limit) {
    int an=(int)strlen(a),bn=(int)strlen(b);
    if (an>=WLOC_NORMAL_SIZE || bn>=WLOC_NORMAL_SIZE || abs(an-bn)>limit) return limit+1;
    int previous2[WLOC_NORMAL_SIZE],previous[WLOC_NORMAL_SIZE],current[WLOC_NORMAL_SIZE];
    for (int j=0;j<=bn;j++) previous[j]=previous2[j]=j;
    for (int i=1;i<=an;i++) {
        current[0]=i;
        for (int j=1;j<=bn;j++) {
            int v=previous[j]+1;if (current[j-1]+1<v) v=current[j-1]+1;
            int substitution=previous[j-1]+(a[i-1]!=b[j-1]);if (substitution<v) v=substitution;
            if (i>1&&j>1&&a[i-1]==b[j-2]&&a[i-2]==b[j-1]&&previous2[j-2]+1<v) v=previous2[j-2]+1;
            current[j]=v;
        }
        memcpy(previous2,previous,(size_t)(bn+1)*sizeof(int));
        memcpy(previous,current,(size_t)(bn+1)*sizeof(int));
    }
    return previous[bn];
}

static int wloc_search(const char *query,int *out_indices,int max_results) {
    wloc_init();
    if (!out_indices || max_results<=0) return 0;
    if (max_results>WLOC_CAPACITY) max_results=WLOC_CAPACITY;
    char q[WLOC_NORMAL_SIZE];wloc_normalize(query,q);
    int scores[WLOC_CAPACITY],found=0,qlen=(int)strlen(q);
    for (int i=0;i<wloc_count;i++) {
        char name[WLOC_NORMAL_SIZE],region[WLOC_NORMAL_SIZE],zone[WLOC_NORMAL_SIZE];
        wloc_normalize(wloc_items[i].name,name);wloc_normalize(wloc_items[i].region,region);wloc_normalize(wloc_items[i].zone,zone);
        int score=100000;
        if (!qlen) score=0;
        else if (!strcmp(q,name)) score=0;
        else if (!strncmp(name,q,(size_t)qlen)) score=100+(int)strlen(name)-qlen;
        else if (strstr(name,q)) score=200+(int)strlen(name)-qlen;
        else if (!strcmp(q,zone)) score=250;
        else {
            int limit=qlen>=8?3:qlen>=5?2:qlen>=3?1:0;
            int distance=wloc_distance(q,name,limit);
            if (limit && distance<=limit) score=300+distance*10+abs((int)strlen(name)-qlen);
            else if (qlen>=3 && (strstr(region,q)||strstr(zone,q))) score=500;
            else {char full[WLOC_NORMAL_SIZE];snprintf(full,sizeof full,"%s%s",name,region);if (qlen>=3&&strstr(full,q)) score=550;}
        }
        if (score==100000) continue;
        int pos=found;
        while (pos>0 && (score<scores[pos-1] || (score==scores[pos-1] &&
               (strcasecmp(wloc_items[i].name,wloc_items[out_indices[pos-1]].name)<0 ||
                (!strcasecmp(wloc_items[i].name,wloc_items[out_indices[pos-1]].name) &&
                 strcasecmp(wloc_items[i].region,wloc_items[out_indices[pos-1]].region)<0))))) --pos;
        if (pos>=max_results) continue;
        int end=found<max_results?found:max_results-1;
        for (int j=end;j>pos;j--) {scores[j]=scores[j-1];out_indices[j]=out_indices[j-1];}
        scores[pos]=score;out_indices[pos]=i;if (found<max_results) ++found;
    }
    return found;
}
#endif
