/* Loopback-only invitation tests never announce onto the user's LAN. */
static void test_friends_129(SDL_Renderer *ren){
    sf_init();assert(sf_valid_id(sf.id));if(sf.fd>=0)close(sf.fd);sf.fd=mgx_net_socket(0);assert(sf.fd>=0);
    int peerfd=mgx_net_socket(0);assert(peerfd>=0);struct sockaddr_in local;socklen_t size=sizeof local;
    assert(getsockname(sf.fd,(struct sockaddr*)&local,&size)==0);local.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    SfPacket p={0};p.magic=htonl(SF_MAGIC);p.version=1;strcpy(p.id,"bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb");strcpy(p.to,sf.id);strcpy(p.name,"Test friend");
    #define INJECT(t,token_value) do{p.type=(t);p.token=htonl(token_value);assert(sendto(peerfd,&p,sizeof p,0,(struct sockaddr*)&local,sizeof local)==sizeof p);sf.beacon=SDL_GetTicks();sf_tick();}while(0)
    INJECT(SF_CONNECT,1);assert(sf.prompt&&sf.n==1&&!sf.peers[0].saved);sf_answer(1);assert(sf.peers[0].met&&!strcmp(sf.connected,p.id));
    /* A connected friend must not put a ROM-folder scan in front of a game that can't be linked. */
    link_game_count=7;assert(!sf_offer_regular("/userdata/roms/snes/Test.sfc","Test","snes"));assert(link_game_count==7);link_game_count=0;
    INJECT(SF_FRIEND,2);assert(sf.prompt&&!sf.peers[0].saved);sf_answer(1);assert(sf.peers[0].saved);
    strcpy(p.name,"Renamed friend");INJECT(SF_BEACON,3);assert(sf.n==1&&!strcmp(sf.peers[0].name,"Renamed friend"));
    p.game=htons(MG_PONG);INJECT(SF_INVITE,4);assert(sf.prompt);sf_answer(0);assert(sf.start==-1);
    INJECT(SF_INVITE,5);assert(sf.prompt);sf_prompt_render(ren);capture(ren,"continued-invite");sf_answer(1);assert(sf.start==MG_PONG&&!sf.host);sf.start=-1;
    INJECT(SF_INVITE,5);assert(!sf.prompt&&sf.start==-1); /* duplicate acknowledged, never launched twice */
    p.game=htons(999);INJECT(SF_INVITE,6);assert(sf.prompt);sf_answer(1);assert(sf.start==-1&&sf.response.type==SF_DENY);
    strcpy(p.id,"cccccccccccccccccccccccccccccccc");INJECT(SF_FRIEND,7);assert(!sf.prompt&&!sf.peers[1].saved);
    sf.mini=0;sf.sel=0;sf_render(ren);capture(ren,"continued-friends");sf.mini=1;sf_render(ren);capture(ren,"continued-link-minigames");
    sf_save();sf.peers[0].saved=0;int oldfd=sf.fd;memset(&sf,0,sizeof sf);close(oldfd);sf.fd=-1;sf.start=-1;sf_init();assert(sf.n==1&&sf.peers[0].saved&&sf.peers[0].met);
    close(peerfd);if(sf.fd>=0)close(sf.fd);sf.fd=-1;sf.connected[0]=0;sf.prompt=0;
    #undef INJECT
    puts("PASS: stable friend ID, explicit connect/add/deny, persistence, rename deduplication, invitation replay, missing-game rejection, unknown-peer protection");
}
