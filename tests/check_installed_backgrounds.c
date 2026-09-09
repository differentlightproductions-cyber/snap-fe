/* Read-only device artwork check; no window, input, settings or network. */
#define main snapfe_application_main
#define SNAPFE_TESTING 1
#include "../main.c"
#undef main
#include <assert.h>
int main(int argc,char **argv) {
    assert(argc==3);WIN_W=atoi(argv[1]);WIN_H=480;
    assert(WIN_W==640||WIN_W==720);
    assert(SDL_Init(SDL_INIT_TIMER)==0);IMG_Init(IMG_INIT_PNG|IMG_INIT_JPG);
    int total=0,systems=0;char names[MAX_BG_FILES][256],path[1200],segment[32];
    snprintf(segment,sizeof segment,"/backgrounds/%s/",background_aspect_folder());
    for(int p=0;p<PLATFORM_COUNT;p++){
        int n=list_backgrounds_for_platform(p,names,MAX_BG_FILES);if(n)systems++;
        for(int i=0;i<n;i++){
            assert(background_file_path(p,names[i],path,sizeof path));assert(strstr(path,segment));
            SDL_Surface *art=IMG_Load(path);if(!art){fprintf(stderr,"Cannot decode %s: %s\n",path,IMG_GetError());return 1;}
            assert(art->w>0&&art->h>0);SDL_FreeSurface(art);total++;
        }
    }
    assert(total==atoi(argv[2]));
    printf("PASS: %s screen lists and decodes %d backgrounds across %d systems; no opposite-ratio or legacy entries\n",background_aspect_folder(),total,systems);
    IMG_Quit();SDL_Quit();return 0;
}
