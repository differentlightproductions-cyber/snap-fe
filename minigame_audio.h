#ifndef SNAP_MINIGAME_AUDIO_H
#define SNAP_MINIGAME_AUDIO_H
/* WAV decode/conversion is off the UI thread. Only the main thread swaps the
 * mixer buffer, under SDL's audio lock; the callback never touches the disk. */
static Sint16 *mg_music_buf=NULL;static int mg_music_len=0,mg_music_pos=0;
static struct {SDL_Thread *thread;SDL_atomic_t done;char path[768];Sint16 *buf;int len,game,level;} mg_music_job;
static int mg_music_current=-1,mg_music_level=-1;
static const char *mg_music_dirs[]={"snake","pong","flapping-bird","breakout","connect-4","block-roll","duck-dash","pulse-runner","pocket-crossing","art-shuffle","button-blitz","tank-duel","crazy-fish"};
static int mg_music_load(void *unused){
    (void)unused;struct stat st;SDL_AudioSpec spec;Uint8 *data=NULL;Uint32 len=0;SDL_AudioCVT cvt;
    if(stat(mg_music_job.path,&st)||st.st_size>32*1024*1024||st.st_size<44)goto done;
    if(!SDL_LoadWAV(mg_music_job.path,&spec,&data,&len))goto done;
    if(SDL_BuildAudioCVT(&cvt,spec.format,spec.channels,spec.freq,AUDIO_S16SYS,1,SAMPLE_RATE)<0)goto done;
    if(cvt.len_mult<1||len>64*1024*1024u/(unsigned)cvt.len_mult)goto done;
    cvt.len=(int)len;cvt.buf=malloc(len*cvt.len_mult);if(!cvt.buf)goto done;
    memcpy(cvt.buf,data,len);
    if(SDL_ConvertAudio(&cvt)==0){mg_music_job.buf=(Sint16*)cvt.buf;mg_music_job.len=cvt.len_cvt/(int)sizeof(Sint16);}else free(cvt.buf);
done:
    if(data)SDL_FreeWAV(data);SDL_AtomicSet(&mg_music_job.done,1);return 0;
}
static void mg_music_swap(Sint16 *buf,int len){
    if(audio_dev)SDL_LockAudioDevice(audio_dev);Sint16 *old=mg_music_buf;mg_music_buf=buf;mg_music_len=len;mg_music_pos=0;if(audio_dev)SDL_UnlockAudioDevice(audio_dev);free(old);
}
static void mg_music_tick(int game,int level){
    if(mg_music_job.thread){if(!SDL_AtomicGet(&mg_music_job.done))return;SDL_WaitThread(mg_music_job.thread,NULL);mg_music_job.thread=NULL;
        if(game==mg_music_job.game&&level==mg_music_job.level)mg_music_swap(mg_music_job.buf,mg_music_job.len);else free(mg_music_job.buf);mg_music_job.buf=NULL;}
    if(game==mg_music_current&&level==mg_music_level)return;
    mg_music_current=game;mg_music_level=level;mg_music_swap(NULL,0);
    if(game<0||game>=13)return;
    snprintf(mg_music_job.path,sizeof mg_music_job.path,"%s/assets/minigames/%s/level-%02d/music.wav",sn_data_root(),mg_music_dirs[game],level);
    mg_music_job.game=game;mg_music_job.level=level;mg_music_job.len=0;SDL_AtomicSet(&mg_music_job.done,0);mg_music_job.thread=SDL_CreateThread(mg_music_load,"minigame-music",NULL);
}
static void mg_music_stop(void){if(mg_music_job.thread){SDL_WaitThread(mg_music_job.thread,NULL);mg_music_job.thread=NULL;free(mg_music_job.buf);mg_music_job.buf=NULL;}mg_music_swap(NULL,0);}
#endif
