#ifndef SNAP_FISH_RARES_H
#define SNAP_FISH_RARES_H
/* --- Rare reef fish -------------------------------------------------------
   Bought once with shells, kept forever, and shown in two places: swimming in
   the aquarium mini-game's tank, and on the aquarium sleep screen. That second
   one is the point of them -- a rare fish you only see while playing is a
   number in a menu, whereas one that turns up on the idle screen is something
   you own.

   This lives in its own header because the sleep screen is drawn far earlier in
   main.c than the mini-games are included, and both ends need the same table.
   The game writes which ones are owned into fish-helpers.cfg; the sleep screen
   reads that file back, so the dependency only ever runs one way. */

enum {MGX_RARE_KOI,MGX_RARE_LANTERN,MGX_RARE_GHOST,MGX_RARE_CORAL,
      MGX_RARE_PRISM,MGX_RARE_COUNT};

typedef struct {
    const char *name;
    int shells;                 /* what it costs, once */
    SDL_Color body;             /* ignored for the prism fish, which cycles */
    const char *note;
} MgxRareFish;

static const MgxRareFish mgx_rare_fish[MGX_RARE_COUNT]={
    {"KOI",        8,{243,138,74,255}, "Orange and white, and never in a hurry."},
    {"LANTERNFISH",14,{126,214,255,255},"Carries its own light through the dark."},
    {"GHOST EEL",  22,{206,222,238,255},"Barely there, and hard to look away from."},
    {"CORAL ANGEL",30,{255,122,178,255},"Reef colours, and fins like a fan."},
    {"PRISM FISH", 45,{255,255,255,255},"Every colour at once. Nobody knows how."},
};

/* The prism fish is the RGB one: it walks the hue wheel about once every six
   seconds. Everything else returns its fixed colour, so callers can ask for a
   colour without caring which fish it is. */
static SDL_Color mgx_rare_color(int rare,Uint32 now){
    if(rare<0||rare>=MGX_RARE_COUNT)return (SDL_Color){255,255,255,255};
    if(rare!=MGX_RARE_PRISM)return mgx_rare_fish[rare].body;
    float h=(float)(now%6000u)/6000.0f*6.0f;          /* hue, in sixths */
    int sector=(int)h;float f=h-(float)sector;
    Uint8 hi=255,lo=54,mid_up=(Uint8)(lo+(hi-lo)*f),mid_dn=(Uint8)(hi-(hi-lo)*f);
    switch(sector){
        case 0:  return (SDL_Color){hi,mid_up,lo,255};
        case 1:  return (SDL_Color){mid_dn,hi,lo,255};
        case 2:  return (SDL_Color){lo,hi,mid_up,255};
        case 3:  return (SDL_Color){lo,mid_dn,hi,255};
        case 4:  return (SDL_Color){mid_up,lo,hi,255};
        default: return (SDL_Color){hi,lo,mid_dn,255};
    }
}
static int mgx_rare_owned(int mask,int rare){
    return rare>=0&&rare<MGX_RARE_COUNT&&(mask&(1<<rare))!=0;
}
static int mgx_rare_owned_count(int mask){
    int n=0;for(int i=0;i<MGX_RARE_COUNT;i++)n+=mgx_rare_owned(mask,i)!=0;return n;
}
/* The cheapest one not yet owned -- what the shop offers next. -1 once the
   whole reef is collected. */
static int mgx_rare_next(int mask){
    for(int i=0;i<MGX_RARE_COUNT;i++)if(!mgx_rare_owned(mask,i))return i;
    return -1;
}
#endif /* SNAP_FISH_RARES_H */
