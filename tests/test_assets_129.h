static int check_list_icons(const char *path){
    DIR *dir=opendir(path);assert(dir);struct dirent *entry;int count=0;
    while((entry=readdir(dir))){if(entry->d_name[0]=='.')continue;char file[1024];snprintf(file,sizeof file,"%s/%s",path,entry->d_name);struct stat st;assert(!stat(file,&st));
        if(S_ISDIR(st.st_mode)){count+=check_list_icons(file);continue;}
        const char *ext=strrchr(file,'.');if(!ext||(strcasecmp(ext,".png")&&strcasecmp(ext,".jpg")&&strcasecmp(ext,".jpeg")&&strcasecmp(ext,".svg")))continue;
        SDL_Surface *s=IMG_Load(file);if(!s)fprintf(stderr,"Icon failed: %s: %s\n",file,IMG_GetError());assert(s&&s->w>0&&s->h>0);SDL_FreeSurface(s);count++;
    }closedir(dir);return count;
}
static void wav_u32(FILE *f,unsigned v){for(int i=0;i<4;i++)fputc((v>>(8*i))&255,f);}
static void wav_u16(FILE *f,unsigned v){fputc(v&255,f);fputc((v>>8)&255,f);}
static void test_assets_129(void){
    int count=check_list_icons("assets/icons/list");assert(count>=52);
    snprintf(mg_music_job.path,sizeof mg_music_job.path,"%s/music-test.wav",sn_data_root());FILE *f=fopen(mg_music_job.path,"wb");assert(f);
    fwrite("RIFF",1,4,f);wav_u32(f,36+400);fwrite("WAVEfmt ",1,8,f);wav_u32(f,16);wav_u16(f,1);wav_u16(f,1);wav_u32(f,22050);wav_u32(f,44100);wav_u16(f,2);wav_u16(f,16);fwrite("data",1,4,f);wav_u32(f,400);for(int i=0;i<200;i++)wav_u16(f,(i%20)*500);fclose(f);
    mg_music_load(NULL);assert(SDL_AtomicGet(&mg_music_job.done)&&mg_music_job.buf&&mg_music_job.len>0);free(mg_music_job.buf);mg_music_job.buf=NULL;
    char a[160],b[160],c[160];strcpy(weather_loc,"Tokyo");weather_unit=0;widget_weather_path(a,sizeof a);widget_place_sel[1]=2;widget_weather_path(b,sizeof b);assert(!strcmp(a,b));weather_unit=1;widget_weather_path(c,sizeof c);assert(strcmp(a,c));strcpy(weather_loc,"Miami");widget_weather_path(c,sizeof c);assert(strcmp(b,c));widget_place_sel[1]=0;
    printf("PASS: %d list icons decoded; WAV conversion; city/unit cache identity\n",count);
}
