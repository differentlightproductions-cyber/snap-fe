/* Exercise the production prediction code with independent device clocks and
   deterministic transport delay, jitter, loss and reordering. No device traffic. */
static MgxPacket pong_test_snapshot(Uint32 sent,int sample){
    MgxPacket p={0};p.seq=htonl((uint32_t)sample+2);
    mgx_pkt_set(&p,2,(int)((120+(int32_t)(sent-10000)*4.6f/16.6667f)*10));
    mgx_pkt_set(&p,3,2500);mgx_pkt_set(&p,4,460);mgx_pkt_set(&p,5,0);
    mgx_pkt_set(&p,6,2500);mgx_pkt_set(&p,7,2500);mgx_pkt_set(&p,8,1);mgx_pkt_set(&p,14,1);
    /* Host clock is nine seconds ahead. The echoed request spent48ms in
       transit followed by16ms waiting for the next host frame. */
    mgx_pkt_set_u32(&p,15,sent+9000);mgx_pkt_set_u32(&p,17,sent-64);mgx_pkt_set_u32(&p,19,sent+9000-16);
    return p;
}

static void test_pong_latency(void){
    mgx_pong_reset();mgx_pong_mode=2;mgx_pong_lobby=0;
    MgxPacket first=pong_test_snapshot(9952,-1);mgx_pong_take_state_at(&first,10000);
    assert(mgx_pong_sync.clock_valid&&fabsf(mgx_pong_sync.clock_offset-9000)<.01f);
    assert(fabsf(pg.bx-120)<.12f&&pg.started);
    float last_draw=pg.bx+mgx_pong_sync.ball_dx,release_y=0,max_jump=0;
    int delivered=-1,packets=0;
    const int jitter[]={0,16,48,0,32,16,64,0};
    for(int frame=1;frame<=75;frame++){
        Uint32 now=10000+frame*16;int mv=frame<=5?1:0;
        mgx_pong_visual_decay(16);mgx_pong_client_advance(mv,16,now);
        if(frame==5)release_y=pg.cpu;
        /* Newer packets can overtake older ones; every eleventh is lost. */
        int newest=delivered;
        for(int i=0;i<=75;i++)if(i%11!=7&&10000+i*16+48+jitter[i%8]<=now&&i>newest)newest=i;
        if(newest>delivered){
            MgxPacket p=pong_test_snapshot(10000+newest*16,newest);
            mgx_pong_take_state_at(&p,now);delivered=newest;packets++;
        }
        float drawn=pg.bx+mgx_pong_sync.ball_dx,jump=drawn-last_draw;
        if(fabsf(jump)>max_jump)max_jump=fabsf(jump);
        assert(jump>=3.8f&&jump<=5.1f); /* No rewind, duplicate-frame pause or jump. */
        assert(fabsf(pg.bx-(120+frame*16*4.6f/16.6667f))<.25f);
        if(frame>5)assert(fabsf(pg.cpu-release_y)<.001f); /* Release never snaps back. */
        last_draw=drawn;
    }
    assert(packets>25&&packets<65&&max_jump<5.1f);

    /* Extrapolation is bounded during an outage; our own paddle stays usable. */
    float x=pg.bx,y=pg.cpu;
    mgx_pong_client_advance(-1,16,mgx_pong_sync.remote_at+300);
    assert(pg.bx==x&&pg.cpu<y);

    /* Host uses the guest's current position plus measured transit time,
       rather than replaying an old direction from a mismatched starting point. */
    mgx_pong_reset();pg.started=1;pg.cpu=250;
    MgxPacket input={0};mgx_pkt_set(&input,0,3000);mgx_pkt_set(&input,1,1);mgx_pkt_set(&input,2,1);
    mgx_pkt_set(&input,3,1);mgx_pkt_set(&input,10,1);
    mgx_pkt_set_u32(&input,4,18968);mgx_pkt_set_u32(&input,6,9904);mgx_pkt_set_u32(&input,8,18952);
    mgx_pong_take_input(&input,10016);assert(fabsf(pg.cpu-(300+48*6.4f/16.6667f))<.01f);
    mgx_pkt_set(&input,1,-1);mgx_pkt_set(&input,3,2);
    mgx_pong_take_input(&input,10020);assert(mgx_pong_remote_mv==1); /* Previous match packets cannot steer. */

    /* Different uptimes and a wrapping millisecond clock keep sub-frame
       precision; float-only clock offsets lose128ms on long-running devices. */
    memset(&mgx_pong_sync,0,sizeof mgx_pong_sync);
    Uint32 base=0xfffff000u,offset=0x70000000u;
    mgx_pkt_set_u32(&input,4,base+9968+offset);mgx_pkt_set_u32(&input,6,base+9904);mgx_pkt_set_u32(&input,8,base+9952+offset);
    assert(fabsf(mgx_pong_packet_age(&input,4,6,8,base+10016)-48)<.01f);

    /* A stale pre-impact snapshot must reproduce our local deflection, not
       turn the ball back into the paddle while waiting for the host echo. */
    mgx_pong_reset();mgx_pong_mode=2;mgx_pong_lobby=0;
    float contact=pg.right-pg.pw-pg.br;
    first=pong_test_snapshot(9952,-1);mgx_pkt_set(&first,2,(int)((contact-12-48*4.6f/16.6667f)*10));
    mgx_pong_take_state_at(&first,10000);
    for(int frame=1;frame<=5;frame++){
        mgx_pong_visual_decay(16);mgx_pong_client_advance(0,16,10000+frame*16);
        if(frame>=3){
            Uint32 sent=10000+(frame-3)*16;
            MgxPacket stale=pong_test_snapshot(sent,frame);
            mgx_pkt_set(&stale,2,(int)((contact-12+(frame-3)*16*4.6f/16.6667f)*10));
            mgx_pong_take_state_at(&stale,10000+frame*16);
            assert(pg.bvx<0&&pg.bx<contact&&pg.sy==0&&pg.sc==0);
        }
    }

    /* A missed render frame still catches the paddle, with the same result as
       four ordinary frames. This checks production collision substeps. */
    pong_reset();pg.started=1;pg.bx=pg.right-pg.pw-pg.br-20;pg.by=pg.cpu;pg.bvx=9.5f;pg.bvy=0;
    float bx=pg.bx,by=pg.by;mgx_pong_ball_advance(64);assert(pg.bvx<0);
    float caught_x=pg.bx,caught_y=pg.by;
    pg.bx=bx;pg.by=by;pg.bvx=9.5f;pg.bvy=0;
    for(int i=0;i<4;i++)mgx_pong_ball_advance(16);
    assert(fabsf(pg.bx-caught_x)<.001f&&fabsf(pg.by-caught_y)<.001f&&pg.bvx<0);

    /* Production receiver rejects old/out-of-order packets on a loopback-only
       socket and drains a burst to its newest state in one render frame. */
    mgx_net_close();mgx_net.fd=mgx_net_socket(0);assert(mgx_net.fd>=0);
    int sender=mgx_net_socket(0);assert(sender>=0);
    struct sockaddr_in receiver={0},source={0};socklen_t len=sizeof receiver;
    assert(getsockname(mgx_net.fd,(struct sockaddr*)&receiver,&len)==0);
    len=sizeof source;assert(getsockname(sender,(struct sockaddr*)&source,&len)==0);
    receiver.sin_addr.s_addr=source.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    mgx_net.peer=source;mgx_net.connected=1;mgx_net.role=MGX_NET_JOIN;mgx_net.session=123;mgx_net.last_rx=SDL_GetTicks();
    MgxPacket packet={0},received={0};packet.magic=htonl(MGX_NET_MAGIC);packet.version=MGX_NET_VERSION;
    packet.game=MG_PONG;packet.type=MGX_PKT_STATE;packet.session=htonl(123);
    int seqs[]={10,9,11};
    for(int i=0;i<3;i++){packet.seq=htonl(seqs[i]);assert(sendto(sender,&packet,sizeof packet,0,(struct sockaddr*)&receiver,sizeof receiver)==sizeof packet);}
    assert(mgx_net_poll(MG_PONG,&received)==MGX_PKT_STATE&&ntohl(received.seq)==11);
    packet.seq=htonl(8);assert(sendto(sender,&packet,sizeof packet,0,(struct sockaddr*)&receiver,sizeof receiver)==sizeof packet);
    assert(mgx_net_poll(MG_PONG,&received)==0);
    packet.seq=htonl(12);packet.version=MGX_NET_VERSION-1;
    assert(sendto(sender,&packet,sizeof packet,0,(struct sockaddr*)&receiver,sizeof receiver)==sizeof packet);
    assert(mgx_net_poll(MG_PONG,&received)==0);close(sender);mgx_net_close();mgx_pong_reset();
    puts("PASS: Pong48-112ms delay/jitter + loss/reordering; continuous ball; immediate local paddle/release; host transit compensation; late-frame collision; stale protocol/sequence rejection");
}
