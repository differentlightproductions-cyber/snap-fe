#ifndef SNAP_WIDGET_PLACES_UI_H
#define SNAP_WIDGET_PLACES_UI_H
static void widget_places_keyboard(AppState *state){
    snprintf(kb_buffer,sizeof kb_buffer,"%s",widget_place_searching?widget_place_query:"");kb_len=(int)strlen(kb_buffer);kb_cursor=kb_len;kb_row=kb_col=0;
    kb_return_state=STATE_WIDGET_PLACES;kb_purpose=KB_PURPOSE_WIDGET_PLACE;*state=STATE_KEYBOARD;
}
static void widget_places_key(SDL_Keycode key,AppState *state){
    if(key==SDLK_ESCAPE){if(widget_place_searching){widget_place_searching=0;widget_place_status[0]=0;}else{widget_places_save();*state=STATE_HOME;}return;}
    if(widget_place_searching){
        int n=widget_place_result_count;
        if(n&&(key==SDLK_UP||key==SDLK_DOWN))widget_place_result_sel=(widget_place_result_sel+(key==SDLK_DOWN?1:n-1))%n;
        if(key==SDLK_RETURN&&n)widget_place_add_location(&wloc_items[widget_place_results[widget_place_result_sel]]);
        if(key==SDLK_s)widget_places_keyboard(state);
        return;
    }
    if(key==SDLK_UP||key==SDLK_DOWN)widget_place_cycle(widget_place_group,key==SDLK_DOWN?1:-1);
    if(key==SDLK_f)widget_place_remove();
    if(key==SDLK_RETURN||key==SDLK_s){
        if(widget_place_count[widget_place_group]<3)widget_places_keyboard(state);
        else snprintf(widget_place_status,sizeof widget_place_status,"Three locations saved. Y removes the selected one.");
    }
}
static void widget_places_text(SDL_Renderer *ren,TTF_Font *font,const char *s,SDL_Color c,int x,int y,int width,int height){
    SDL_Texture *t=render_text(ren,font,s,c);if(!t)return;int w,h;SDL_QueryTexture(t,NULL,NULL,&w,&h);float scale=fminf(1,fminf((float)width/fmaxf(w,1),(float)height/fmaxf(h,1)));
    SDL_RenderCopy(ren,t,NULL,&(SDL_Rect){x,y,(int)(w*scale),(int)(h*scale)});
}
static void widget_places_render(SDL_Renderer *ren){
    Theme *th=&themes[theme_idx];widget_places_init();
    widget_places_text(ren,font_small,widget_place_searching?"CHOOSE LOCATION":widget_place_group?"WEATHER LOCATIONS":"TIME LOCATIONS",th->accent2,30,54,WIN_W-60,34);
    if(widget_place_searching){
        char query[130];snprintf(query,sizeof query,"Matches for: %s",widget_place_query[0]?widget_place_query:"all locations");
        widget_places_text(ren,font_label,query,g_ui_dim,30,94,WIN_W-60,24);
        int first=widget_place_result_sel/5*5;
        for(int j=0;j<5&&first+j<widget_place_result_count;j++){
            int i=first+j;WLocation *p=&wloc_items[widget_place_results[i]];int selected=i==widget_place_result_sel;SDL_Rect row={30,126+j*49,WIN_W-60,44};
            mgx_panel(ren,row,selected?th->select_bg:th->bg,selected?th->accent2:th->dim,255);
            if(selected){SDL_SetRenderDrawColor(ren,th->accent2.r,th->accent2.g,th->accent2.b,255);SDL_RenderFillRect(ren,&(SDL_Rect){31,row.y+2,5,row.h-4});}
            char label[140];snprintf(label,sizeof label,"%s%s%s",p->name,p->region[0]?", ":"",p->region);
            widget_places_text(ren,font_label,label,selected?th->accent2:g_ui_text,44,row.y+2,WIN_W-90,23);
            widget_places_text(ren,font_fixed?font_fixed:font_label,p->zone,g_ui_dim,44,row.y+26,WIN_W-90,15);
        }
        widget_places_text(ren,font_label,widget_place_status,g_ui_dim,30,386,WIN_W-60,25);
        widget_places_text(ren,font_label,"Up/Down Choose   A Add   X Search   B Back",g_ui_text,30,WIN_H-40,WIN_W-60,26);
    }else{
        for(int i=0;i<widget_place_count[widget_place_group];i++){
            int selected=i==widget_place_sel[widget_place_group];SDL_Rect row={30,112+i*60,WIN_W-60,50};
            mgx_panel(ren,row,selected?th->select_bg:th->bg,selected?th->accent2:th->dim,255);
            if(selected){SDL_SetRenderDrawColor(ren,th->accent2.r,th->accent2.g,th->accent2.b,255);SDL_RenderFillRect(ren,&(SDL_Rect){31,row.y+2,5,row.h-4});}
            widget_places_text(ren,font_label,widget_place_label(widget_place_group,i),selected?th->accent2:g_ui_text,44,row.y+12,WIN_W-88,29);
        }
        char count[96];snprintf(count,sizeof count,"%d of 3 locations saved",widget_place_count[widget_place_group]);
        widget_places_text(ren,font_label,count,g_ui_dim,30,307,WIN_W-60,26);
        widget_places_text(ren,font_label,widget_place_status,g_ui_dim,30,346,WIN_W-60,27);
        widget_places_text(ren,font_label,"A Add location   Y Remove   Up/Down Select",g_ui_text,30,WIN_H-75,WIN_W-60,25);
        widget_places_text(ren,font_label,"B Back to widget",g_ui_dim,30,WIN_H-39,WIN_W-60,25);
    }
}
#endif
