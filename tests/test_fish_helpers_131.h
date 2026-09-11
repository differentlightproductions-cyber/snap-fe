#ifndef SNAP_TEST_FISH_HELPERS_131_H
#define SNAP_TEST_FISH_HELPERS_131_H
/* Crazy Fish rebuild, part one: helpers are creatures that swim to their work
   instead of a passive tick reaching across the tank from a badge in the
   corner, hunger is a readable staged clock, and you choose which two helpers
   come with you. */

static void fish_helpers_arena(int species_a,int species_b){
    mgx_fish_reset();
    mgx_fish.helpers=MGX_HELP_COUNT;      /* everything discovered */
    mgx_fish.started=1;mgx_fish.intro=0;mgx_fish.picking=0;mgx_fish.shop_open=0;
    mgx_fish.helper_pick[0]=species_a;mgx_fish.helper_pick[1]=species_b;
    mgx_fish_helper_spawn();
    memset(mgx_fish.drops,0,sizeof mgx_fish.drops);
    memset(mgx_fish.monster,0,sizeof mgx_fish.monster);
}
/* Run the simulation for `secs`, in frames, the way the game does. */
static void fish_helpers_run(float secs){
    for(float t=0;t<secs;t+=.05f){mgx_fish.last=SDL_GetTicks()-50;mgx_fish_step(0,0);}
}

static void test_fish_helpers_131(SDL_Renderer *ren){
    /* --- A helper must TRAVEL to a coin, not teleport its effect there. --- */
    fish_helpers_arena(MGX_HELP_DART,-1);
    mgx_fish.helper[0].x=60;mgx_fish.helper[0].y=90;
    mgx_fish_drop_add((float)(WIN_W-60),(float)(WIN_H-120),9,MGX_RES_GOLD,0);
    int drop=fish_find_drop(MGX_RES_GOLD);assert(drop>=0);
    mgx_fish.drops[drop].vy=0;                       /* hold it still to test travel */
    int coins_before=mgx_fish.coins;
    float start_dist=sqrtf(mgx_len2(mgx_fish.helper[0].x-mgx_fish.drops[drop].x,
                                    mgx_fish.helper[0].y-mgx_fish.drops[drop].y));
    /* One frame: it has moved, and has NOT banked a coin on the far side. */
    mgx_fish.last=SDL_GetTicks()-50;mgx_fish_step(0,0);
    assert(mgx_fish.coins==coins_before);
    float after=sqrtf(mgx_len2(mgx_fish.helper[0].x-mgx_fish.drops[drop].x,
                               mgx_fish.helper[0].y-mgx_fish.drops[drop].y));
    assert(after<start_dist);                        /* it is on its way */
    /* Given time to swim the tank, it arrives and banks it. */
    fish_helpers_run(12.0f);
    assert(mgx_fish.coins>coins_before);

    /* --- Glimmer upgrades what it reaches, so it is worth more than Dart. --- */
    fish_helpers_arena(MGX_HELP_GLIMMER,-1);
    mgx_fish_drop_add(mgx_fish.helper[0].x,mgx_fish.helper[0].y,3,MGX_RES_SILVER,0);
    int silver=fish_find_drop(MGX_RES_SILVER);assert(silver>=0);
    mgx_fish.drops[silver].vy=0;
    int before_gold=mgx_fish.coins;
    fish_helpers_run(2.0f);
    assert(mgx_fish.coins-before_gold>=mgx_fish_resource_value(MGX_PET_TETRA));

    /* --- Spike has to reach a monster before it can hurt it. --- */
    fish_helpers_arena(MGX_HELP_SPIKE,-1);
    mgx_fish_spawn_monster();
    assert(mgx_fish_monsters_alive()==1);
    mgx_fish.monster[0].x=(float)(WIN_W-50);mgx_fish.monster[0].y=200;
    mgx_fish.monster[0].hp=mgx_fish.monster[0].max_hp=40;
    mgx_fish.helper[0].x=50;mgx_fish.helper[0].y=200;
    int mhp=mgx_fish.monster[0].hp;
    mgx_fish.last=SDL_GetTicks()-50;mgx_fish_step(0,0);
    assert(mgx_fish.monster[0].hp==mhp);             /* far away: no free hit */
    fish_helpers_run(8.0f);
    assert(mgx_fish.monster[0].hp<mhp);              /* arrived, and bit it */

    /* --- Shelly stays on the floor and clears what has settled. --- */
    fish_helpers_arena(MGX_HELP_SHELLY,-1);
    float floor_y=mgx_fish.helper[0].y;
    mgx_fish_drop_add(120,(float)(WIN_H-32),5,MGX_RES_GOLD,0);
    int landed=fish_find_drop(MGX_RES_GOLD);assert(landed>=0);
    mgx_fish.drops[landed].bottom_age=0.1f;          /* already on the bottom */
    fish_helpers_run(10.0f);
    assert(fabsf(mgx_fish.helper[0].y-floor_y)<2.0f);   /* never left the floor */

    /* --- Nibbles produces food, but on its own slow schedule. --- */
    fish_helpers_arena(MGX_HELP_NIBBLES,-1);
    assert(fish_find_drop(MGX_RES_FOOD)<0);
    fish_helpers_run(9.0f);
    assert(fish_find_drop(MGX_RES_FOOD)>=0);

    /* --- Only the two chosen helpers are in the tank. --- */
    fish_helpers_arena(MGX_HELP_DART,MGX_HELP_SPIKE);
    assert(mgx_fish.helper[0].active&&mgx_fish.helper[1].active);
    assert(mgx_fish.helper[0].species==MGX_HELP_DART);
    assert(mgx_fish.helper[1].species==MGX_HELP_SPIKE);
    fish_helpers_arena(MGX_HELP_BUBBLE,-1);
    assert(mgx_fish.helper[0].active&&!mgx_fish.helper[1].active);

    /* --- The picker: two slots, toggling, and no undiscovered helpers. --- */
    mgx_fish_reset();mgx_fish.helpers=3;
    for(int s=0;s<MGX_HELP_SLOTS;s++)mgx_fish.helper_pick[s]=-1;
    mgx_fish_pick_toggle(0);assert(mgx_fish_pick_count()==1);
    mgx_fish_pick_toggle(1);assert(mgx_fish_pick_count()==2);
    mgx_fish_pick_toggle(0);assert(mgx_fish_pick_count()==1);   /* toggles off */
    mgx_fish_pick_toggle(0);
    /* A third choice replaces rather than being silently refused. */
    mgx_fish_pick_toggle(2);assert(mgx_fish_pick_count()==MGX_HELP_SLOTS);
    assert(mgx_fish_pick_slot_of(2)>=0);
    /* Undiscovered helpers cannot be chosen at all. */
    int held=mgx_fish_pick_count();
    mgx_fish_pick_toggle(MGX_HELP_COUNT-1);
    assert(mgx_fish_pick_count()==held&&mgx_fish_pick_slot_of(MGX_HELP_COUNT-1)<0);

    /* Two or more discovered -> the picker appears; fewer -> it is skipped,
       because a menu with no decision in it is just a keypress. */
    mgx_fish_reset();mgx_fish.helpers=2;mgx_fish.started=1;
    mgx_fish_begin_level();assert(mgx_fish.picking);
    mgx_fish_reset();mgx_fish.helpers=1;mgx_fish.started=1;
    mgx_fish_begin_level();assert(!mgx_fish.picking&&mgx_fish.helper[0].active);

    /* --- Hunger, in the seconds the design is actually written in. ------- */
    {
        /* A newly hatched guppy: a meal keeps it content twelve seconds, and it
           does not start starving until thirty. */
        MgxFishPet baby={0};baby.type=MGX_PET_GUPPY;baby.growth=0;
        baby.hunger_span=mgx_food_hold[0];
        assert(fabsf(mgx_fish_full_span(&baby)-12.0f)<0.01f);
        assert(fabsf(mgx_fish_hunger_span(&baby)-30.0f)<0.01f);
        /* Fully grown it is patient: content for twenty seconds, not starving
           until forty-five. */
        MgxFishPet grown=baby;grown.growth=MGX_GROWTH_MAX;
        assert(fabsf(mgx_fish_full_span(&grown)-20.0f)<0.01f);
        assert(fabsf(mgx_fish_hunger_span(&grown)-45.0f)<0.01f);
        /* Growing up only ever buys time -- never takes it away at some step. */
        for(int g=1;g<=MGX_GROWTH_MAX;g++){
            MgxFishPet before=baby,after=baby;before.growth=g-1;after.growth=g;
            assert(mgx_fish_full_span(&after)>mgx_fish_full_span(&before));
            assert(mgx_fish_hunger_span(&after)>mgx_fish_hunger_span(&before));
        }
        /* The four stages walk in order across the whole clock. */
        baby.hunger=0.0f;                             assert(mgx_fish_hunger_stage(&baby)==0);
        baby.hunger=11.9f;                             assert(mgx_fish_hunger_stage(&baby)==0);
        baby.hunger=12.1f;                             assert(mgx_fish_hunger_stage(&baby)==1);
        baby.hunger=mgx_fish_urgent_at(&baby)-0.1f;   assert(mgx_fish_hunger_stage(&baby)==2);
        baby.hunger=mgx_fish_urgent_at(&baby)+0.1f;   assert(mgx_fish_hunger_stage(&baby)==3);
        baby.hunger=29.9f;                            assert(mgx_fish_hunger_stage(&baby)==3);
        /* Better food widens every stage, not only the last one. */
        MgxFishPet best=baby;best.hunger_span=mgx_food_hold[MGX_FOOD_TIERS-1];
        assert(mgx_fish_full_span(&best)>mgx_fish_full_span(&baby));
        assert(mgx_fish_hunger_span(&best)>mgx_fish_hunger_span(&baby));
        best.hunger=baby.hunger;
        assert(mgx_fish_hunger_stage(&best)<mgx_fish_hunger_stage(&baby));
        /* A shark holds out longer than a guppy at the same age. */
        MgxFishPet shark=baby;shark.type=MGX_PET_SHARK;
        assert(mgx_fish_hunger_span(&shark)>mgx_fish_hunger_span(&baby));
    }

    /* The complaint this retune answers: a new fish must survive being ignored
       for ten seconds. It used to be losing health by then. */
    fish_helpers_arena(-1,-1);
    for(int i=1;i<MGX_FISH_MAX;i++)mgx_fish.fish[i].active=0;
    mgx_fish.fish[0].active=1;mgx_fish.fish[0].growth=0;mgx_fish.fish[0].hunger=0;
    mgx_fish.fish[0].hp=mgx_fish.fish[0].max_hp=mgx_fish_max_hp(mgx_fish.fish[0].type);
    fish_helpers_run(10.0f);
    assert(mgx_fish.fish[0].active&&mgx_fish.fish[0].hp==mgx_fish.fish[0].max_hp);

    /* A fish left unfed for its whole window loses health, then dies. */
    fish_helpers_arena(-1,-1);
    for(int i=1;i<MGX_FISH_MAX;i++)mgx_fish.fish[i].active=0;
    mgx_fish.fish[0].active=1;mgx_fish.fish[0].hp=1;mgx_fish.fish[0].hunger=0;
    fish_helpers_run(mgx_fish_hunger_span(&mgx_fish.fish[0])+1.0f);
    assert(!mgx_fish.fish[0].active);

    /* Both new screens draw. */
    mgx_fish_reset();mgx_fish.helpers=MGX_HELP_COUNT-1;mgx_fish.started=1;mgx_fish.intro=0;
    mgx_fish.picking=1;mgx_fish.pick_cursor=1;mgx_fish.helper_pick[0]=0;
    mgx_fish_render(ren);capture(ren,"fish-helper-picker");
    mgx_fish.picking=0;mgx_fish.shop_open=1;mgx_fish.shop_row=MGX_SHOP_FOOD;
    mgx_fish_render(ren);capture(ren,"fish-shop");
    mgx_fish.shop_open=0;

    puts("PASS: Crazy Fish helpers swim to their work, hunger is a staged clock, and you pick two helpers per level");
}
#endif
