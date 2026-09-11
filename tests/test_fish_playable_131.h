#ifndef SNAP_TEST_FISH_PLAYABLE_131_H
#define SNAP_TEST_FISH_PLAYABLE_131_H
/* Crazy Fish has been retuned three times by adjusting numbers and then asking
   whether it felt right. This plays it instead: a simulated player runs real
   levels through the real step and key functions, and the rules are judged by
   what happens to it.

   Three players:
     attentive - feeds a fish as soon as it is hungry, collects coins, buys a
                 couple of extra guppies, buys egg pieces when it can afford to
     casual    - only notices a fish once it is very hungry, reacts slowly,
                 never buys extra fish
     idle      - does nothing at all

   The rules are right when the attentive player clears levels without losing
   fish, the casual one still clears the first levels, and the idle one loses
   its fish -- slowly enough that looking away is not fatal, but it does happen. */

typedef struct {
    int stages, losses, feeds, first_loss_idx;
    float clear_at[4], first_loss_at;
    int min_coins;
} FishPlay;

enum { FISHPLAY_IDLE = 0, FISHPLAY_CASUAL = 1, FISHPLAY_ATTENTIVE = 2 };

static void fishplay_setup(void) {
    srand(7);
    mgx_fish_reset();
    mgx_fish.helpers = 0; mgx_fish.stage = 1; mgx_fish.shells = 0; mgx_fish.rares = 0;
    mgx_fish.coins = 70; mgx_fish_set_goal();
    mgx_fish.started = 1; mgx_fish.intro = 0; mgx_fish.picking = 0; mgx_fish.between = 0;
    mgx_fish.clock = 0; mgx_fish.next_spawn = 90;
}

static FishPlay fishplay_run(int skill, float limit, int want_stages) {
    FishPlay r; memset(&r, 0, sizeof r);
    r.first_loss_at = -1; r.min_coins = 1 << 30;
    fishplay_setup();
    int stage0 = mgx_fish.stage, alive = mgx_fish_alive();
    float act_cool = 0;
    for (float t = 0; t < limit && !mgx_fish.over; t += 0.05f) {
        /* The screens between levels. */
        if (mgx_fish.picking) { mgx_fish_key(SDLK_RETURN); mgx_fish_key(SDLK_F1); }
        if (mgx_fish.between == 2) mgx_fish_key(SDLK_RETURN);
        if (mgx_fish.intro > 0) mgx_fish_key(SDLK_RETURN);

        int dx = 0, dy = 0, press = 0;
        if (skill != FISHPLAY_IDLE && !mgx_fish.between && !mgx_fish.hatch_pending) {
            float tx = mgx_fish.x, ty = mgx_fish.y;
            int target = 0;
            if (mgx_fish_monsters_alive()) {
                press = 1;                       /* A fires at the nearest one */
            } else {
                /* The hungriest fish past this player's threshold, unless food
                   is already on its way down to it. */
                int want = skill == FISHPLAY_ATTENTIVE ? 1 : 2, pick = -1; float worst = -1;
                if (!mgx_fish.bonus) for (int i = 0; i < MGX_FISH_MAX; i++) {
                    MgxFishPet *f = &mgx_fish.fish[i];
                    if (!f->active || mgx_fish_hunger_stage(f) < want) continue;
                    int coming = 0;
                    for (int d = 0; d < MGX_FISH_DROPS; d++)
                        if (mgx_fish.drops[d].active && mgx_fish.drops[d].kind == MGX_RES_FOOD &&
                            mgx_len2(mgx_fish.drops[d].x - f->x, mgx_fish.drops[d].y - f->y) < 90 * 90) coming = 1;
                    if (coming) continue;
                    if (f->hunger > worst) { worst = f->hunger; pick = i; }
                }
                if (pick >= 0) {
                    tx = mgx_fish.fish[pick].x; ty = mgx_fish.fish[pick].y; target = 1;
                    if (mgx_len2(tx - mgx_fish.x, ty - mgx_fish.y) < 14 * 14) press = 1;
                } else {
                    float best = 1e12f;
                    for (int d = 0; d < MGX_FISH_DROPS; d++) {
                        MgxFishDrop *dr = &mgx_fish.drops[d];
                        if (!dr->active || dr->kind == MGX_RES_FOOD || dr->kind == MGX_RES_WASTE) continue;
                        float dd = mgx_len2(dr->x - mgx_fish.x, dr->y - mgx_fish.y);
                        if (dd < best) { best = dd; tx = dr->x; ty = dr->y; target = 1; }
                    }
                }
            }
            if (target) {
                dx = tx > mgx_fish.x + 4 ? 1 : tx < mgx_fish.x - 4 ? -1 : 0;
                dy = ty > mgx_fish.y + 4 ? 1 : ty < mgx_fish.y - 4 ? -1 : 0;
            }
        }
        mgx_fish.last = SDL_GetTicks() - 50;
        mgx_fish_step(dx, dy);
        act_cool -= 0.05f;
        if (press && act_cool <= 0) {
            int coins = mgx_fish.coins, firing = mgx_fish_monsters_alive() > 0;
            /* The ray's cooldown is kept in real milliseconds, and this loop runs
               far faster than real time -- so the simulation clears it and paces
               the shots itself, at the cadence a held A actually gets. */
            if (firing) mgx_fish.last_shot = 0;
            mgx_fish_key(SDLK_RETURN);
            if (mgx_fish.coins < coins) r.feeds++;
            act_cool = firing ? 0.4f : skill == FISHPLAY_ATTENTIVE ? 0.35f : 1.5f;
        }

        /* Shopping, keeping enough back to feed everyone twice. */
        if (skill != FISHPLAY_IDLE && !mgx_fish.bonus && !mgx_fish.between && mgx_fish.intro <= 0) {
            int reserve = mgx_fish_food_cost() * 2 * mgx_fish_alive();
            if (mgx_fish.coins >= mgx_fish_egg_cost() + reserve) mgx_fish_buy_egg();
            else if (skill == FISHPLAY_ATTENTIVE && mgx_fish_alive() < 4 &&
                     mgx_fish.coins >= mgx_fish_cost[MGX_PET_GUPPY] + reserve) {
                mgx_fish.shop_type = MGX_PET_GUPPY; mgx_fish_key(SDLK_s);
            }
        }

        int now_alive = mgx_fish_alive();
        if (now_alive < alive) {
            r.losses += alive - now_alive;
            if (r.first_loss_at < 0) r.first_loss_at = t;
        }
        alive = now_alive;
        if (mgx_fish.coins < r.min_coins) r.min_coins = mgx_fish.coins;
        if (mgx_fish.stage > stage0 + r.stages) {
            if (r.stages < 4) r.clear_at[r.stages] = t;
            r.stages++;
            if (r.stages >= want_stages) break;
        }
    }
    return r;
}

static void fishplay_report(const char *who, const FishPlay *r) {
    printf("FISHPLAY %-9s stages=%d clear=[%.0f %.0f %.0f] losses=%d first_loss=%.0fs feeds=%d min_coins=%d\n",
           who, r->stages, r->clear_at[0], r->clear_at[1], r->clear_at[2],
           r->losses, r->first_loss_at, r->feeds, r->min_coins);
}

static void test_fish_playable_131(void) {
    FishPlay att = fishplay_run(FISHPLAY_ATTENTIVE, 900, 3);
    FishPlay cas = fishplay_run(FISHPLAY_CASUAL, 900, 2);
    FishPlay idle = fishplay_run(FISHPLAY_IDLE, 240, 1);
    fishplay_report("attentive", &att);
    fishplay_report("casual", &cas);
    fishplay_report("idle", &idle);
    fflush(stdout);   /* the numbers must survive a failed assert below */

    /* Paying attention wins: three levels, and not one fish lost doing it. */
    assert(att.stages >= 3 && att.losses == 0);
    /* The first level takes a couple of minutes -- not seconds, not forever. */
    assert(att.clear_at[0] >= 60.0f && att.clear_at[0] <= 240.0f);
    /* A slower, later-reacting player still gets through the first two. */
    assert(cas.stages >= 2 && cas.losses == 0);
    /* Nobody who is feeding their fish is ever left unable to buy food: the
       old economy took even perfect play to $0. */
    assert(att.min_coins >= mgx_fish_food_cost() && cas.min_coins >= mgx_fish_food_cost());
    /* Ignored fish still die -- but not inside the first half-minute. */
    assert(idle.losses > 0 && idle.first_loss_at >= 30.0f && idle.first_loss_at <= 60.0f);
    puts("PASS: Crazy Fish is winnable -- a simulated attentive player clears three levels and a casual one two, neither losing a fish, while an ignored tank still starves");
}
#endif
